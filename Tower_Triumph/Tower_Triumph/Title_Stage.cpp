#include "Title_Stage.h"


Title_Stage::Title_Stage() {
	

}
void Title_Stage::Init(HWND hWnd) {
	Buttons.resize(buttonCount);
	int i = 0;
	for(auto& a : Buttons)
	{
		a.SetLocation(200, 200 + i * 100);
		i++;
	}

	SelectionButton = 0;
}
void Title_Stage::Update(HWND hWnd) {
	for (auto& a: Buttons)
		a.Update();
}
void Title_Stage::Draw(HDC hMemDC, HDC hMemDC1) {
	for (auto& a : Buttons)
		a.Draw(hMemDC, hMemDC1);
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
}