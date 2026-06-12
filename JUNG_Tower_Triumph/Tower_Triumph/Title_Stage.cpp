#include "Title_Stage.h"
#include "resource.h"

Title_Stage::Title_Stage() {
	NextStageID = -2;
	menuGroup = nullptr;
	settingPopup = nullptr;
	titleName = nullptr;
	btnStart = nullptr;
	btnSetting = nullptr;
	btnEnd = nullptr;
	CurStageID = TITLE_STAGE;
}

Title_Stage::~Title_Stage() {
	if (menuGroup) delete menuGroup;
	if (settingPopup) delete settingPopup;
	//if (titleName) delete titleName;

	// btnStart, btnSetting, btnEnd 등은 menuGroup의 자식으로 등록되어 있으므로
	// menuGroup이 삭제될 때 내부에서 자동으로 delete 됩니다. (이중 해제 방지)
}
void Title_Stage::Init(HWND hWnd, HINSTANCE const g_hInst) {
	GetClientRect(hWnd, &rects);

	// 배경 화면 변경: Title_Background.bmp 로드
	hBitBackground = (HBITMAP)LoadImage(NULL, L"Resource/Title/Title_Background.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	if (hBitBackground) {
		GetObject(hBitBackground, sizeof(BITMAP), &BackgroundInfo);
	}
	else {
		// 로드 실패 시 기존 리소스 폴백
		hBitBackground = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_SMAP_BACKGROUND));
		GetObject(hBitBackground, sizeof(BITMAP), &BackgroundInfo);
	}

	Loc.first.x = rects.right / 2;
	Loc.first.y = rects.bottom / 2;
	Loc.second.sX = BackgroundInfo.bmWidth/ 2;
	Loc.second.sY = BackgroundInfo.bmHeight / 2;

	NextStageID = -2;

	// UI Setup
	menuGroup = new UContainerWidget();
	settingPopup = new USettingPopup();
	settingPopup->Setup(g_hInst, rects);

	// Title Name UI 추가
	titleName = new UImage();
	titleName->SetFrameFromFile({ L"Resource/Title/Title_Name.bmp" });
	if (!titleName->GetBitmap().empty()) {
		BITMAP info = titleName->GetMapInfo()[0];
		titleName->SetSize(info.bmWidth/2, info.bmHeight/2);
	}
	titleName->SetLocation(rects.right / 2, rects.bottom / 4);

	int startY = rects.bottom / 2 + 50;

	// 버튼 설정: 텍스트 기반으로 변경
	btnStart = new UButton();
	btnStart->SetText(L"START GAME");
	btnStart->SetFont(72, FW_NORMAL, L"Gabriola"); // 우아하면서도 읽기 쉬운 Gabriola 폰트 적용
	btnStart->SetSize(400, 80);
	btnStart->SetLocation(rects.right / 2, startY);
	btnStart->SetUIID(0);
	btnStart->SetOnClick([this]() { this->NextStageID = MAP_STAGE; });
	menuGroup->AddChild(btnStart);

	btnSetting = new UButton();
	btnSetting->SetText(L"SETTINGS");
	btnSetting->SetFont(72, FW_NORMAL, L"Gabriola");
	btnSetting->SetSize(400, 80);
	btnSetting->SetLocation(rects.right / 2, startY + 110);
	btnSetting->SetUIID(1);
	btnSetting->SetOnClick([this]() { 
		this->settingPopup->SetVisible(true);
	});
	menuGroup->AddChild(btnSetting);

	btnEnd = new UButton();
	btnEnd->SetText(L"EXIT");
	btnEnd->SetFont(72, FW_NORMAL, L"Gabriola");
	btnEnd->SetSize(400, 80);
	btnEnd->SetLocation(rects.right / 2, startY + 220);
	btnEnd->SetUIID(2);
	btnEnd->SetOnClick([this]() { this->NextStageID = GAME_END; });
	menuGroup->AddChild(btnEnd);

	menuGroup->AddChild(titleName);
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
