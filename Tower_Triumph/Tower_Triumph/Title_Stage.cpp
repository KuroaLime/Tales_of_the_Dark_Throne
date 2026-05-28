#include "Title_Stage.h"
#include "resource.h"

Title_Stage::Title_Stage() {
	

}
void Title_Stage::Init(HWND hWnd, HINSTANCE const g_hInst) {
	GetClientRect(hWnd, &rects);

	hBitBackground = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_SMAP_BACKGROUND));
	GetObject(hBitBackground, sizeof(BITMAP), &BackgroundInfo);

	Loc.first.x = rects.right / 2;
	Loc.first.y = rects.bottom / 2;
	Loc.second.sX = BackgroundInfo.bmWidth/ 2;
	Loc.second.sY = BackgroundInfo.bmHeight / 2;

	Buttons.resize(buttonCount);
	int i = 0;
	for(auto& a : Buttons)
	{
		a.SetLocation(200, 200 + i * 100);
		a.SetButtonID(i);
		i++;
	}

	Buttons[0].SetTargetPointID(MAP_STAGE); //START
	Buttons[1].SetTargetPointID(MAP_STAGE); //SETTING
	Buttons[2].SetTargetPointID(GAME_END); //END

	SelectionButton = 0;
}
void Title_Stage::Update(HWND hWnd) {
	for (auto& a: Buttons)
		a.Update();
}
void Title_Stage::Draw(HDC hMemDC, HDC hMemDC1) {

	HBITMAP hOld = (HBITMAP)SelectObject(hMemDC1, hBitBackground);
	TransparentBlt(hMemDC, Loc.first.x - Loc.second.sX, Loc.first.y - Loc.second.sY, BackgroundInfo.bmWidth, BackgroundInfo.bmHeight, hMemDC1, 0, 0, BackgroundInfo.bmWidth, BackgroundInfo.bmHeight, RGB(255, 255, 255));
	SelectObject(hMemDC1, hOld);

	for (auto& a : Buttons)
		a.Draw(hMemDC, hMemDC1);
}

void Title_Stage::Comp_NextStage(int ID) {

}
int Title_Stage::Input(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static int CLK_X, CLK_Y;
	switch (uMsg) {
	case WM_LBUTTONDOWN:
		CLK_X = LOWORD(lParam);
		CLK_Y = HIWORD(lParam);
		for (int i = 0; i < buttonCount; i++)
		{
			Buttons[i].bClick();
		}
		break;
	case WM_LBUTTONUP:
		for (int i = 0; i < buttonCount; i++)
		{
			if (Buttons[i].GetbClick()) {

				Buttons[i].SetbClick(false);
				return Buttons[i].GetTargetPointID();
			}
		}
		break;
	case WM_RBUTTONDOWN:
		break;
	case WM_RBUTTONUP:
		break;
	case WM_MOUSEMOVE:
		CLK_X = LOWORD(lParam);
		CLK_Y = HIWORD(lParam);
		for (int i = 0; i < buttonCount; i++)
		{
			if (Buttons[i].bCollision(CLK_X, CLK_Y)) {
				Buttons[SelectionButton].SetIsPointed(false);
				SelectionButton = i;
				Buttons[SelectionButton].SetIsPointed(true);
			}
			else if(Buttons[i].GetIsPointed()) {
				Buttons[SelectionButton].SetIsPointed(false);
			}
		}
		break;
	case WM_KEYDOWN:
		if(wParam == VK_ESCAPE)
		{
			SendMessage(hWnd, WM_CLOSE, 0, 0);
		}
		else if (wParam == VK_UP) {
			Buttons[SelectionButton].SetIsPointed(false);
			SelectionButton = (SelectionButton - 1);
			if (SelectionButton < 0)
				SelectionButton = buttonCount - 1;
			Buttons[SelectionButton].SetIsPointed(true);
		}
		else if (wParam == VK_DOWN) {
			Buttons[SelectionButton].SetIsPointed(false);
			SelectionButton = (SelectionButton + 1) % buttonCount;
			Buttons[SelectionButton].SetIsPointed(true);
		}
		break;
	case WM_CHAR:
		break;
	}
	return -2;
}