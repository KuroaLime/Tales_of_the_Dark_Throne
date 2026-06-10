#include "UButton.h"

UButton::UButton()
{
	isPressed = false;
	isSelected = false;
	bEnabled = true;
	bWasPointed = false;
}

void UButton::Update()
{
	if (!bEnabled) {
		IsPointed = false;
		isPressed = false;
		return;
	}
	UWidget::Update();
	if (IsPointed && !bWasPointed) {
		if (OnHoverCallback) OnHoverCallback();
	}
	else if (!IsPointed && bWasPointed) {
		if (OnUnhoverCallback) OnUnhoverCallback();
	}
	bWasPointed = IsPointed;
}

void UButton::Draw(HDC hMemDC, HDC hMemDC1)
{
	if (!bVisible) return;

	int frame = 0;
	if (!bEnabled) frame = 0; // Or a specific disabled frame if available
	else if (isPressed) frame = (AniCount > 2) ? 2 : (AniCount > 0 ? 0 : -1);
	else if (isSelected) frame = (AniCount > 1) ? 1 : (AniCount > 0 ? 0 : -1); 
	else if (IsPointed) frame = (AniCount > 1) ? 1 : (AniCount > 0 ? 0 : -1);
	else frame = (AniCount > 0 ? 0 : -1);

	if (frame != -1 && !bBitmap.empty()) {
		HBITMAP temp = (HBITMAP)SelectObject(hMemDC1, bBitmap[frame]);
		TransparentBlt(hMemDC, Loc.x - Size.sX / 2, Loc.y - Size.sY / 2, Size.sX, Size.sY,
			hMemDC1, 0, 0, bMapinfo[frame].bmWidth, bMapinfo[frame].bmHeight, RGB(255, 255, 255));
		SelectObject(hMemDC1, temp);

		if (!bEnabled) {
			// Apply a dark overlay for disabled state
			HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 0));
			BLENDFUNCTION bf = { AC_SRC_OVER, 0, 100, 0 }; // 100/255 opacity
			// Since we don't have a direct AlphaBlend for brushes, we can use a semi-transparent black rectangle
			// But for simplicity in GDI, we might just use a dark fill or skip complex blending
			// Let's use a simpler approach: draw a dark overlay if supported, or just change fallback color
		}

		// If selected, add a border
		if (isSelected) {
			HPEN hPen = CreatePen(PS_SOLID, 3, RGB(255, 215, 0)); // Gold
			HPEN oldP = (HPEN)SelectObject(hMemDC, hPen);
			HBRUSH oldB = (HBRUSH)SelectObject(hMemDC, GetStockObject(NULL_BRUSH));
			Rectangle(hMemDC, Loc.x - Size.sX / 2, Loc.y - Size.sY / 2, Loc.x + Size.sX / 2, Loc.y + Size.sY / 2);
			SelectObject(hMemDC, oldB);
			SelectObject(hMemDC, oldP);
			DeleteObject(hPen);
		}
	}
	else {
		// Fallback drawing
		HBRUSH bBrush;
		if (!bEnabled) bBrush = CreateSolidBrush(RGB(50, 50, 50)); // Dark grey for disabled
		else if (isPressed) bBrush = CreateSolidBrush(RGB(100, 100, 100));
		else if (isSelected) bBrush = CreateSolidBrush(RGB(180, 150, 0)); // Deep Gold
		else if (IsPointed) bBrush = CreateSolidBrush(RGB(0, 255, 0));
		else bBrush = CreateSolidBrush(RGB(255, 0, 0));

		RECT rects = { Loc.x - Size.sX / 2, Loc.y - Size.sY / 2, Loc.x + Size.sX / 2, Loc.y + Size.sY / 2 };
		FillRect(hMemDC, &rects, bBrush);
		DeleteObject(bBrush);

		// Border for selected state in fallback mode
		if (isSelected) {
			HPEN hPen = CreatePen(PS_SOLID, 3, RGB(255, 255, 255));
			HPEN oldP = (HPEN)SelectObject(hMemDC, hPen);
			HBRUSH oldB = (HBRUSH)SelectObject(hMemDC, GetStockObject(NULL_BRUSH));
			Rectangle(hMemDC, rects.left, rects.top, rects.right, rects.bottom);
			SelectObject(hMemDC, oldB);
			SelectObject(hMemDC, oldP);
			DeleteObject(hPen);
		}

		if (!ButtonText.empty()) {
			int oldMode = SetBkMode(hMemDC, TRANSPARENT);
			COLORREF oldColor = SetTextColor(hMemDC, RGB(255, 255, 255)); // White text
			DrawTextW(hMemDC, ButtonText.c_str(), -1, &rects, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			SetTextColor(hMemDC, oldColor);
			SetBkMode(hMemDC, oldMode);
		}
	}
}

void UButton::Click()
{
	isPressed = false; // Reset state when clicked
	if (OnClickCallback) {
		OnClickCallback();
	}
}

void UButton::RightClick()
{
	if (OnRightClickCallback) {
		OnRightClickCallback();
	}
}
