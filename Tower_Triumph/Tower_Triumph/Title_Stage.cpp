#include "Title_Stage.h"
#include "resource.h"

Title_Stage::Title_Stage() {
	NextStageID = -2;
	menuGroup = nullptr;
	settingPopup = nullptr;
	btnStart = nullptr;
	btnSetting = nullptr;
	btnEnd = nullptr;
	CurStageID = 0;
}

Title_Stage::~Title_Stage() {
	if (menuGroup) delete menuGroup;
	if (settingPopup) delete settingPopup;

	// btnStart, btnSetting, btnEnd 등은 menuGroup의 자식으로 등록되어 있으므로
	// menuGroup이 삭제될 때 내부에서 자동으로 delete 됩니다. (이중 해제 방지)
}
void Title_Stage::Init(HWND hWnd, HINSTANCE const g_hInst) {
	GetClientRect(hWnd, &rects);

	hBitBackground = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_SMAP_BACKGROUND));
	GetObject(hBitBackground, sizeof(BITMAP), &BackgroundInfo);

	Loc.first.x = rects.right / 2;
	Loc.first.y = rects.bottom / 2;
	Loc.second.sX = BackgroundInfo.bmWidth/ 2;
	Loc.second.sY = BackgroundInfo.bmHeight / 2;

	NextStageID = -2;

	// UI Setup
	menuGroup = new UContainerWidget();
	settingPopup = new USettingPopup();
	settingPopup->Setup(g_hInst, rects);

	int startY = rects.bottom / 2 + 50;

	btnStart = new UButton();
	btnStart->SetLocation(rects.right / 2, startY);
	btnStart->SetUIID(0);
	btnStart->SetText(L"START GAME");
	btnStart->SetOnClick([this]() { this->NextStageID = MAP_STAGE; });
	menuGroup->AddChild(btnStart);

	btnSetting = new UButton();
	btnSetting->SetLocation(rects.right / 2, startY + 80);
	btnSetting->SetUIID(1);
	btnSetting->SetText(L"SETTINGS");
	btnSetting->SetOnClick([this]() { 
		this->settingPopup->SetVisible(true);
	});
	menuGroup->AddChild(btnSetting);

	btnEnd = new UButton();
	btnEnd->SetLocation(rects.right / 2, startY + 160);
	btnEnd->SetUIID(2);
	btnEnd->SetText(L"EXIT");
	btnEnd->SetOnClick([this]() { this->NextStageID = GAME_END; });
	menuGroup->AddChild(btnEnd);

	mainHUD.AddChildWindow(menuGroup);
	mainHUD.AddChildWindow(settingPopup);
}

void Title_Stage::Update(HWND hWnd) {
	mainHUD.Update();
}

void Title_Stage::Draw(HDC hMemDC, HDC hMemDC1) {
	HBITMAP hOld = (HBITMAP)SelectObject(hMemDC1, hBitBackground);
	TransparentBlt(hMemDC, Loc.first.x - Loc.second.sX, Loc.first.y - Loc.second.sY, BackgroundInfo.bmWidth, BackgroundInfo.bmHeight, hMemDC1, 0, 0, BackgroundInfo.bmWidth, BackgroundInfo.bmHeight, RGB(255, 255, 255));
	SelectObject(hMemDC1, hOld);

	mainHUD.Draw(hMemDC, hMemDC1);
}

void Title_Stage::Comp_NextStage(int ID) {

}

int Title_Stage::Input(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static int CLK_X, CLK_Y;
	
	switch (uMsg) {
	case WM_MOUSEMOVE:
		CLK_X = LOWORD(lParam);
		CLK_Y = HIWORD(lParam);
		mainHUD.UpdateMouseHover(CLK_X, CLK_Y);
		break;

	case WM_LBUTTONDOWN:
		CLK_X = LOWORD(lParam);
		CLK_Y = HIWORD(lParam);
		if (UWidget* w = mainHUD.FindWidgetUnderMouse(CLK_X, CLK_Y)) {
			if (UButton* btn = dynamic_cast<UButton*>(w)) {
				btn->SetIsPressed(true);
			}
		}
		break;

	case WM_LBUTTONUP:
		CLK_X = LOWORD(lParam);
		CLK_Y = HIWORD(lParam);
		if (UWidget* w = mainHUD.FindWidgetUnderMouse(CLK_X, CLK_Y)) {
			if (UButton* btn = dynamic_cast<UButton*>(w)) {
				btn->Click();
			}
		}
		btnStart->SetIsPressed(false);
		btnSetting->SetIsPressed(false);
		btnEnd->SetIsPressed(false);

		// Also reset popup buttons if needed, or rely on them not being pressed.

		// Return Stage ID if modified by click
		if (NextStageID != -2) {
			int ret = NextStageID;
			NextStageID = -2; // Reset for safety
			return ret;
		}
		break;

	case WM_KEYDOWN:
		if(wParam == VK_ESCAPE) {
			SendMessage(hWnd, WM_CLOSE, 0, 0);
		}
		break;
	}
	return -2;
}