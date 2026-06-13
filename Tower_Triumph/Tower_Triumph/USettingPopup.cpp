#include "USettingPopup.h"

USettingPopup::USettingPopup()
{
	bSoundOn = true;
	btnClose = nullptr;
	btnSoundToggle = nullptr;
	bVisible = false; // Initially hidden
}

void USettingPopup::Init()
{
}

void USettingPopup::Setup(HINSTANCE hInst, RECT clientRect)
{
	// Set Popup Area (Center of screen)
	int width = 400;
	int height = 300;
	SetLocation(clientRect.right / 2, clientRect.bottom / 2);
	Size.sX = width;
	Size.sY = height;

	// Sound Toggle Button
	btnSoundToggle = new UButton();
	btnSoundToggle->SetLocation(clientRect.right / 2, clientRect.bottom / 2 - 20);
	btnSoundToggle->SetText(L"SOUND: ON");
	btnSoundToggle->SetOnClick([this]() {
		this->bSoundOn = !this->bSoundOn;
		this->btnSoundToggle->SetText(this->bSoundOn ? L"SOUND: ON" : L"SOUND: OFF");
	});
	AddChild(btnSoundToggle);

	// Close Button
	btnClose = new UButton();
	btnClose->SetLocation(clientRect.right / 2, clientRect.bottom / 2 + 80);
	btnClose->SetText(L"CLOSE");
	btnClose->SetOnClick([this]() {
		this->SetVisible(false);
	});
	AddChild(btnClose);
}

void USettingPopup::Draw(HDC hMemDC, HDC hMemDC1)
{
	if (!bVisible) return;

	// Draw Stylized Background (SRPG Style Parchment/Window)
	if (bBitmap.empty()) {
		// Fill with dark brown
		HBRUSH bgBrush = CreateSolidBrush(RGB(60, 45, 30)); 
		// Golden border
		HPEN borderPen = CreatePen(PS_SOLID, 4, RGB(180, 140, 70));

		HBRUSH oldBrush = (HBRUSH)SelectObject(hMemDC, bgBrush);
		HPEN oldPen = (HPEN)SelectObject(hMemDC, borderPen);

		RECT r = { Loc.x - Size.sX / 2, Loc.y - Size.sY / 2, Loc.x + Size.sX / 2, Loc.y + Size.sY / 2 };
		// Use RoundRect for a better look
		RoundRect(hMemDC, r.left, r.top, r.right, r.bottom, 10, 10);

		SelectObject(hMemDC, oldBrush);
		SelectObject(hMemDC, oldPen);
		DeleteObject(bgBrush);
		DeleteObject(borderPen);

		// Add a subtitle
		SetBkMode(hMemDC, TRANSPARENT);
		SetTextColor(hMemDC, RGB(255, 220, 100));
		RECT titleRect = { r.left, r.top + 10, r.right, r.top + 40 };
		DrawTextW(hMemDC, L"- SETTINGS -", -1, &titleRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	}

	UContainerWidget::Draw(hMemDC, hMemDC1);
}
