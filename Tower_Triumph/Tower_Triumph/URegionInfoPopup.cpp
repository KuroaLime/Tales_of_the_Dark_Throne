#include "URegionInfoPopup.h"

URegionInfoPopup::URegionInfoPopup()
{
	bVisible = false;
}

void URegionInfoPopup::Setup(HINSTANCE hInst, RECT clientRect)
{
	SetLocation(clientRect.right - 250, clientRect.bottom / 2);
	Size.sX = 400;
	Size.sY = 500;

	btnEnter = new UButton();
	btnEnter->SetLocation(Loc.x, Loc.y + 140);
	btnEnter->SetSize(100, 40);
	btnEnter->SetText(L"ENTER");
	btnEnter->SetOnClick([this]() { 
		if (OnEnterClickCallback) OnEnterClickCallback();
	});
	AddChild(btnEnter);

	btnClose = new UButton();
	btnClose->SetLocation(Loc.x, Loc.y + 200);
	btnClose->SetSize(100, 40);
	btnClose->SetText(L"CLOSE");
	btnClose->SetOnClick([this]() { 
		this->SetVisible(false); 
		if (OnCloseClickCallback) OnCloseClickCallback();
	});
	AddChild(btnClose);
}

void URegionInfoPopup::SetInfo(const std::wstring& name, const std::wstring& desc, const std::wstring& env)
{
	rName = name;
	rDesc = desc;
	rEnv = env;
	SetVisible(true);
}

void URegionInfoPopup::Draw(HDC hMemDC, HDC hMemDC1)
{
	if (!bVisible) return;

	HBRUSH bgBrush = CreateSolidBrush(RGB(40, 30, 20));
	HPEN borderPen = CreatePen(PS_SOLID, 3, RGB(200, 160, 80));
	HBRUSH oldB = (HBRUSH)SelectObject(hMemDC, bgBrush);
	HPEN oldP = (HPEN)SelectObject(hMemDC, borderPen);

	RECT r = { Loc.x - Size.sX / 2, Loc.y - Size.sY / 2, Loc.x + Size.sX / 2, Loc.y + Size.sY / 2 };
	RoundRect(hMemDC, r.left, r.top, r.right, r.bottom, 15, 15);

	SelectObject(hMemDC, oldB);
	SelectObject(hMemDC, oldP);
	DeleteObject(bgBrush);
	DeleteObject(borderPen);

	SetBkMode(hMemDC, TRANSPARENT);
	
	SetTextColor(hMemDC, RGB(255, 255, 100));
	HFONT hFont = CreateFont(28, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, L"Arial");
	HFONT oldF = (HFONT)SelectObject(hMemDC, hFont);
	RECT nameRect = { r.left + 20, r.top + 20, r.right - 20, r.top + 60 };
	DrawTextW(hMemDC, rName.c_str(), -1, &nameRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
	DeleteObject(hFont);

	SetTextColor(hMemDC, RGB(150, 200, 255));
	hFont = CreateFont(18, 0, 0, 0, FW_NORMAL, 1, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, L"Arial");
	SelectObject(hMemDC, hFont);
	RECT envRect = { r.left + 20, r.top + 70, r.right - 20, r.top + 100 };
	DrawTextW(hMemDC, (L"Env: " + rEnv).c_str(), -1, &envRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
	DeleteObject(hFont);

	SetTextColor(hMemDC, RGB(220, 220, 220));
	hFont = CreateFont(20, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, L"Arial");
	SelectObject(hMemDC, hFont);
	RECT descRect = { r.left + 20, r.top + 120, r.right - 20, r.bottom - 80 };
	DrawTextW(hMemDC, rDesc.c_str(), -1, &descRect, DT_LEFT | DT_TOP | DT_WORDBREAK);
	
	SelectObject(hMemDC, oldF);
	DeleteObject(hFont);

	UContainerWidget::Draw(hMemDC, hMemDC1);
}
