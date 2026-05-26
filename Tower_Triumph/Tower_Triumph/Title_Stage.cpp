#include "Title_Stage.h"


Title_Stage::Title_Stage() {
	

}
void Title_Stage::Init(HWND hWnd) {
	StartButton.SetLocation(200, 200);
	//StartButton.SetFrame();
	SettingButton.SetLocation(200, 300);
	//SettingButton.SetFrame();
	EndButton.SetLocation(200, 400);
	//EndButton.SetFrame();

	SeletionButton = 0;
}
void Title_Stage::Update(HWND hWnd) {
	StartButton.Update();
	SettingButton.Update();
	EndButton.Update();
}
void Title_Stage::Draw(HDC hMemDC, HDC hMemDC1) {
	StartButton.Draw(hMemDC, hMemDC1);
	SettingButton.Draw(hMemDC, hMemDC1);
	EndButton.Draw(hMemDC, hMemDC1);
}
void Title_Stage::Input(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static int CLK_X, CLK_Y;
	switch (uMsg) {
	case WM_LBUTTONDOWN:
		CLK_X = LOWORD(lParam);
		CLK_Y = HIWORD(lParam);
		break;
	case WM_LBUTTONUP:
		break;
	case WM_RBUTTONDOWN:
		break;
	case WM_RBUTTONUP:
		break;
	case WM_MOUSEMOVE:
		CLK_X = LOWORD(lParam);
		CLK_Y = HIWORD(lParam);
		StartButton.bCollision(CLK_X, CLK_Y);
		SettingButton.bCollision(CLK_X, CLK_Y);
		EndButton.bCollision(CLK_X, CLK_Y);
		break;
	case WM_KEYDOWN:
		if(wParam == VK_ESCAPE)
		{
			SendMessage(hWnd, WM_CLOSE, 0, 0);
		}
		else if (wParam == VK_UP) {

		}
		else if (wParam == VK_LEFT) {

		}
		else if (wParam == VK_DOWN) {

		}
		else if (wParam == VK_RIGHT) {

		}
		break;
	case WM_CHAR:
		break;
	}
}