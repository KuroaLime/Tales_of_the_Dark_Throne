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
	btnSoundToggle->SetText(L"사운드: 켜짐");
	btnSoundToggle->SetOnClick([this]() {
		this->bSoundOn = !this->bSoundOn;
		this->btnSoundToggle->SetText(this->bSoundOn ? L"사운드: 켜짐" : L"사운드: 꺼짐");
	});
	AddChild(btnSoundToggle);

	// Close Button
	btnClose = new UButton();
	btnClose->SetLocation(clientRect.right / 2, clientRect.bottom / 2 + 80);
	btnClose->SetText(L"닫기");
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
		HBRUSH bgBrush = CreateSolidBrush(RGB(19, 23, 31)); 
		// Golden border
		HPEN borderPen = CreatePen(PS_SOLID, 4, RGB(176, 146, 78));

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
		DrawTextW(hMemDC, L"설정", -1, &titleRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	}

	UContainerWidget::Draw(hMemDC, hMemDC1);
}
