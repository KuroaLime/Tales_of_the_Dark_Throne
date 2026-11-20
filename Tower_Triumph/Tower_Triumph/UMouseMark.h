#pragma once
#include "UImage.h"
#include <array>
#include <chrono>
#include <cstdint>

class UMouseMark : public UImage
{
private:
	static constexpr int CursorSize = 40;
	static constexpr int HotspotX = 2;
	static constexpr int HotspotY = 37;
	static constexpr int SwayPadding = 4;
	static constexpr int SurfaceSize = CursorSize + SwayPadding * 2;
	static constexpr double SwayPeriodSeconds = 4.8;
	static constexpr double SwayAmplitudeRadians = 0.05235987755982989; // Three degrees.
	std::array<std::uint32_t, CursorSize * CursorSize> sourcePixels{};
	HBITMAP cursorBitmap = nullptr;
	std::uint32_t* cursorBits = nullptr;
	POINT currentMousePos{ 0, 0 };
	double swaySeconds = 0.0;
	std::chrono::steady_clock::time_point lastUpdate;

	void ReleaseCursor();
	void RenderSway(double radians);

public:
	UMouseMark();
	~UMouseMark() override;
	UMouseMark(const UMouseMark&) = delete;
	UMouseMark& operator=(const UMouseMark&) = delete;
	void SetFrame(const HINSTANCE& instance, std::vector<WORD> frames) override;
	virtual void Update() override;
	virtual void Draw(HDC hMemDC, HDC hMemDC1) override;

	bool IsDrawable() const { return cursorBitmap != nullptr && cursorBits != nullptr; }
	bool IsVisible() const { return bVisible; }
	void SetMousePos(POINT pt) { currentMousePos = pt; Loc.x = pt.x; Loc.y = pt.y; }
};

