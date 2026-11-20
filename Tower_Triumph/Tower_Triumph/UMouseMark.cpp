#include "UMouseMark.h"
#include <algorithm>
#include <cmath>
#include <cstring>

UMouseMark::UMouseMark() : lastUpdate(std::chrono::steady_clock::now())
{
	Size = { CursorSize, CursorSize };
	bVisible = true;
}

UMouseMark::~UMouseMark()
{
	ReleaseCursor();
}

void UMouseMark::ReleaseCursor()
{
	if (cursorBitmap) DeleteObject(cursorBitmap);
	cursorBitmap = nullptr;
	cursorBits = nullptr;
	sourcePixels.fill(0);
}

void UMouseMark::SetFrame(const HINSTANCE& instance, std::vector<WORD> frames)
{
	ReleaseCursor();
	swaySeconds = 0.0;
	lastUpdate = std::chrono::steady_clock::now();
	if (frames.empty()) return;

	// Read the original DIB bytes: LoadBitmap/device conversion can discard alpha.
	// The approved asset is a 40x40 premultiplied BGRA, uncompressed 32-bit bitmap.
	const HRSRC resource = FindResourceW(instance, MAKEINTRESOURCEW(frames.front()), RT_BITMAP);
	if (!resource) return;
	const DWORD resourceBytes = SizeofResource(instance, resource);
	const HGLOBAL loaded = LoadResource(instance, resource);
	const auto* data = loaded ? static_cast<const BYTE*>(LockResource(loaded)) : nullptr;
	if (!data || resourceBytes < sizeof(BITMAPINFOHEADER)) return;
	BITMAPINFOHEADER header{};
	std::memcpy(&header, data, sizeof(header));
	if (header.biSize < sizeof(header) || header.biSize > resourceBytes ||
		header.biWidth != CursorSize || (header.biHeight != CursorSize && header.biHeight != -CursorSize) ||
		header.biPlanes != 1 || header.biBitCount != 32 || header.biCompression != BI_RGB || header.biClrUsed != 0 ||
		resourceBytes - header.biSize < sourcePixels.size() * sizeof(std::uint32_t)) return;
	for (int y = 0; y < CursorSize; ++y)
	{
		const int sourceY = header.biHeight > 0 ? CursorSize - 1 - y : y;
		std::memcpy(sourcePixels.data() + y * CursorSize,
			data + header.biSize + sourceY * CursorSize * sizeof(std::uint32_t), CursorSize * sizeof(std::uint32_t));
	}
	bool hasAlpha = false;
	for (const auto pixel : sourcePixels)
	{
		const unsigned alpha = pixel >> 24;
		hasAlpha = hasAlpha || alpha != 0;
		if ((pixel & 255) > alpha || ((pixel >> 8) & 255) > alpha || ((pixel >> 16) & 255) > alpha)
		{
			sourcePixels.fill(0);
			return;
		}
	}
	if (!hasAlpha) return;

	BITMAPINFO info{};
	info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	info.bmiHeader.biWidth = SurfaceSize;
	info.bmiHeader.biHeight = -SurfaceSize;
	info.bmiHeader.biPlanes = 1;
	info.bmiHeader.biBitCount = 32;
	info.bmiHeader.biCompression = BI_RGB;
	void* bits = nullptr;
	cursorBitmap = CreateDIBSection(nullptr, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
	cursorBits = static_cast<std::uint32_t*>(bits);
	if (!IsDrawable()) { ReleaseCursor(); return; }
	RenderSway(0.0);
}

void UMouseMark::RenderSway(double radians)
{
	if (!IsDrawable()) return;
	GdiFlush(); // Finish any queued reads before changing the shared DIB pixels.
	const double cosine = std::cos(radians), sine = std::sin(radians);
	const auto sample = [this](int x, int y) -> std::uint32_t
	{
		return x >= 0 && x < CursorSize && y >= 0 && y < CursorSize ? sourcePixels[y * CursorSize + x] : 0;
	};
	for (int y = 0; y < SurfaceSize; ++y)
	{
		for (int x = 0; x < SurfaceSize; ++x)
		{
			// Inverse rotation about the nib; its exact pixel never moves or changes.
			const double dx = x - SwayPadding - HotspotX, dy = y - SwayPadding - HotspotY;
			const double sourceX = HotspotX + cosine * dx + sine * dy;
			const double sourceY = HotspotY - sine * dx + cosine * dy;
			const int left = static_cast<int>(std::floor(sourceX)), top = static_cast<int>(std::floor(sourceY));
			const double fx = sourceX - left, fy = sourceY - top;
			const std::uint32_t pixels[4] = { sample(left, top), sample(left + 1, top), sample(left, top + 1), sample(left + 1, top + 1) };
			const double weights[4] = { (1.0 - fx) * (1.0 - fy), fx * (1.0 - fy), (1.0 - fx) * fy, fx * fy };
			unsigned channels[4]{};
			for (int channel = 0; channel < 4; ++channel)
			{
				double value = 0.0;
				for (int i = 0; i < 4; ++i) value += ((pixels[i] >> (channel * 8)) & 255) * weights[i];
				channels[channel] = static_cast<unsigned>(std::lround(value));
			}
			// Interpolate premultiplied color and alpha together to avoid dark fringes.
			cursorBits[y * SurfaceSize + x] = (std::min)(channels[0], channels[3]) |
				((std::min)(channels[1], channels[3]) << 8) | ((std::min)(channels[2], channels[3]) << 16) | (channels[3] << 24);
		}
	}
}

void UMouseMark::Update()
{
	Loc.x = currentMousePos.x;
	Loc.y = currentMousePos.y;
	const auto now = std::chrono::steady_clock::now();
	const double elapsed = std::chrono::duration<double>(now - lastUpdate).count();
	lastUpdate = now;
	if (!bVisible || !IsDrawable()) return;
	// Do not jump through a large angle when returning from a paused/debugged frame.
	swaySeconds = std::fmod(swaySeconds + (std::max)(0.0, (std::min)(elapsed, 0.1)), SwayPeriodSeconds);
	const double radians = SwayAmplitudeRadians * std::sin(swaySeconds * 6.283185307179586 / SwayPeriodSeconds);
	RenderSway(radians);
}

void UMouseMark::Draw(HDC hMemDC, HDC hMemDC1)
{
	if (!bVisible || !IsDrawable() || !hMemDC || !hMemDC1) return;
	const HGDIOBJ previous = SelectObject(hMemDC1, cursorBitmap);
	if (!previous || previous == HGDI_ERROR) return;
	const BLENDFUNCTION blend{ AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
	AlphaBlend(hMemDC, Loc.x - HotspotX - SwayPadding, Loc.y - HotspotY - SwayPadding,
		SurfaceSize, SurfaceSize, hMemDC1, 0, 0, SurfaceSize, SurfaceSize, blend);
	SelectObject(hMemDC1, previous);
}
