#include "URegionMarker.h"
#include <algorithm>
#include <cmath>
#include <cstring>

URegionMarker::URegionMarker()
    : bIsActivated(false), bIsUnlocked(false), presentationActive(true), worldBounds{},
      floatSeconds(0.0), floatPhase(0.0), transitionSeconds(0.18), visualScale(1.0f),
      transitionStart(1.0f), transitionTarget(1.0f), lastUpdate(std::chrono::steady_clock::now()) {}

URegionMarker::~URegionMarker()
{
    ReleaseLayer(building); ReleaseLayer(markerOpen); ReleaseLayer(markerClosed);
}

void URegionMarker::ReleaseLayer(SLayerBitmap& layer)
{
    if (layer.bitmap) DeleteObject(layer.bitmap);
    layer = {};
}

bool URegionMarker::LoadLayer(HINSTANCE instance, WORD resourceId, SLayerBitmap& layer)
{
    ReleaseLayer(layer);
    // LoadBitmap can discard alpha; read the embedded premultiplied DIB directly.
    const HRSRC resource = FindResourceW(instance, MAKEINTRESOURCEW(resourceId), RT_BITMAP);
    if (!resource) return false;
    const DWORD length = SizeofResource(instance, resource);
    const HGLOBAL loaded = LoadResource(instance, resource);
    const BYTE* bytes = loaded ? static_cast<const BYTE*>(LockResource(loaded)) : nullptr;
    if (!bytes || length < sizeof(BITMAPINFOHEADER)) return false;
    BITMAPINFOHEADER header{};
    std::memcpy(&header, bytes, sizeof(header));
    if (header.biSize != sizeof(header) || header.biWidth <= 0 || header.biWidth > 2048 ||
        header.biHeight == 0 || header.biHeight < -2048 || header.biHeight > 2048 ||
        header.biPlanes != 1 || header.biBitCount != 32 || header.biCompression != BI_RGB || header.biClrUsed) return false;
    const int width = header.biWidth, height = std::abs(header.biHeight);
    const size_t stride = static_cast<size_t>(width) * 4;
    if (length - header.biSize < stride * height) return false;
    const BYTE* pixels = bytes + header.biSize;
    bool hasAlpha = false;
    for (size_t i = 0; i < stride * height; i += 4)
    {
        const BYTE alpha = pixels[i + 3];
        hasAlpha = hasAlpha || alpha != 0;
        if (pixels[i] > alpha || pixels[i + 1] > alpha || pixels[i + 2] > alpha) return false;
    }
    if (!hasAlpha) return false;
    BITMAPINFO info{};
    info.bmiHeader = header;
    info.bmiHeader.biHeight = -height;
    void* destination = nullptr;
    HBITMAP bitmap = CreateDIBSection(nullptr, &info, DIB_RGB_COLORS, &destination, nullptr, 0);
    if (!bitmap || !destination) { if (bitmap) DeleteObject(bitmap); return false; }
    for (int y = 0; y < height; ++y)
    {
        const int sourceY = header.biHeight > 0 ? height - 1 - y : y;
        std::memcpy(static_cast<BYTE*>(destination) + y * stride, pixels + sourceY * stride, stride);
    }
    layer.bitmap = bitmap; layer.width = width; layer.height = height;
    return true;
}

bool URegionMarker::LoadArtwork(HINSTANCE instance, WORD buildingResource, WORD openResource, WORD closedResource, double phaseSeconds)
{
    const bool buildingReady = LoadLayer(instance, buildingResource, building);
    const bool openReady = LoadLayer(instance, openResource, markerOpen);
    const bool closedReady = LoadLayer(instance, closedResource, markerClosed);
    floatPhase = phaseSeconds; floatSeconds = 0.0;
    lastUpdate = std::chrono::steady_clock::now();
    if (!buildingReady || !openReady || !closedReady ||
        building.width != worldBounds.right - worldBounds.left || building.height != worldBounds.bottom - worldBounds.top)
    {
        ReleaseLayer(building); ReleaseLayer(markerOpen); ReleaseLayer(markerClosed);
        return false;
    }
    return true;
}

void URegionMarker::SetupLandmark(const RECT& bounds, const std::wstring&)
{
    worldBounds = bounds;
    SetLocation((bounds.left + bounds.right) / 2, (bounds.top + bounds.bottom) / 2);
    SetSize(bounds.right - bounds.left, bounds.bottom - bounds.top);
}

void URegionMarker::SetRegionInfo(const std::wstring& parentName, const std::wstring& name,
    const std::wstring& desc, const std::wstring& env)
{
    parentRegionName = parentName; regionName = name; regionDesc = desc; environment = env;
}

void URegionMarker::SetPresentationActive(bool active)
{
    presentationActive = active;
    if (!active) IsPointed = false;
}

void URegionMarker::AdvanceAnimation(double elapsed)
{
    if (!std::isfinite(elapsed) || elapsed < 0.0) return;
    if (!bVisible)
    {
        IsPointed = false;
        visualScale = transitionStart = transitionTarget = 1.0f;
        transitionSeconds = 0.18;
        return;
    }
    const float target = ((presentationActive && IsPointed) || bIsActivated) ? 1.05f : 1.0f;
    if (target != transitionTarget)
    {
        transitionStart = visualScale; transitionTarget = target; transitionSeconds = 0.0;
    }
    transitionSeconds = (std::min)(0.18, transitionSeconds + elapsed);
    const double remaining = 1.0 - transitionSeconds / 0.18;
    const float eased = static_cast<float>(1.0 - remaining * remaining * remaining);
    visualScale = transitionStart + (transitionTarget - transitionStart) * eased;
    if (presentationActive) floatSeconds = std::fmod(floatSeconds + elapsed, 3.2);
}

void URegionMarker::Update()
{
    const auto now = std::chrono::steady_clock::now();
    const double elapsed = std::chrono::duration<double>(now - lastUpdate).count();
    lastUpdate = now;
    AdvanceAnimation((std::max)(0.0, (std::min)(elapsed, 0.1)));
    if (bVisible) UContainerWidget::Update();
}

RECT URegionMarker::GetBuildingScreenRect(int offsetX, int offsetY, float zoom) const
{
    const double width = (worldBounds.right - worldBounds.left) * visualScale;
    const double height = (worldBounds.bottom - worldBounds.top) * visualScale;
    const double middle = (worldBounds.left + worldBounds.right) * 0.5;
    return { static_cast<LONG>(std::lround((middle - width * 0.5 - offsetX) * zoom)),
        static_cast<LONG>(std::lround((worldBounds.bottom - height - offsetY) * zoom)),
        static_cast<LONG>(std::lround((middle + width * 0.5 - offsetX) * zoom)),
        static_cast<LONG>(std::lround((worldBounds.bottom - offsetY) * zoom)) };
}

RECT URegionMarker::GetFloatingMarkerScreenRect(int offsetX, int offsetY, float zoom) const
{
    const double middle = (worldBounds.left + worldBounds.right) * 0.5;
    // Clear the tallest (1.05x) building state. Decoration size ignores camera zoom.
    const double top = worldBounds.top - (worldBounds.bottom - worldBounds.top) * 0.05;
    const double rise = 2.0 * (1.0 - std::cos((floatSeconds + floatPhase) * 6.283185307179586 / 3.2));
    const LONG x = static_cast<LONG>(std::lround((middle - offsetX) * zoom));
    const LONG bottom = static_cast<LONG>(std::lround((top - offsetY) * zoom - 7.0 - rise));
    return { x - 22, bottom - 44, x + 22, bottom };
}

void URegionMarker::DrawLayer(HDC target, HDC scratch, const SLayerBitmap& layer, const RECT& destination)
{
    if (!layer.bitmap || !target || !scratch || destination.right <= destination.left || destination.bottom <= destination.top) return;
    const HGDIOBJ previous = SelectObject(scratch, layer.bitmap);
    if (!previous || previous == HGDI_ERROR) return;
    const BLENDFUNCTION blend{ AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    AlphaBlend(target, destination.left, destination.top, destination.right - destination.left,
        destination.bottom - destination.top, scratch, 0, 0, layer.width, layer.height, blend);
    SelectObject(scratch, previous);
}

void URegionMarker::Draw(HDC target, HDC scratch) { DrawWithOffset(target, scratch, 0, 0); }

void URegionMarker::DrawWithOffset(HDC target, HDC scratch, int offsetX, int offsetY, float zoom)
{
    if (!bVisible || !std::isfinite(zoom) || zoom <= 0.0f) return;
    DrawLayer(target, scratch, building, GetBuildingScreenRect(offsetX, offsetY, zoom));
    DrawLayer(target, scratch, bIsUnlocked ? markerOpen : markerClosed, GetFloatingMarkerScreenRect(offsetX, offsetY, zoom));
}

bool URegionMarker::ContainsPoint(int x, int y) { return ContainsPointWithOffset(x, y, 0, 0, 1.0f); }

bool URegionMarker::ContainsPointWithOffset(int x, int y, int offsetX, int offsetY, float zoom)
{
    if (!bVisible || !std::isfinite(zoom) || zoom <= 0.0f) return false;
    const double worldX = x / static_cast<double>(zoom) + offsetX;
    const double worldY = y / static_cast<double>(zoom) + offsetY;
    // Hit area never grows/moves with hover, selection or the floating decoration.
    return worldX >= worldBounds.left && worldX < worldBounds.right && worldY >= worldBounds.top && worldY < worldBounds.bottom;
}

void URegionMarker::Activate() { bIsActivated = true; }
void URegionMarker::Deactivate() { bIsActivated = false; }
