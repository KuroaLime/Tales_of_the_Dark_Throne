#include "Map_Stage.h"


Map_Stage::Map_Stage() {


}
void Map_Stage::Init(HWND hWnd) {
	//Buttons.resize(buttonCount);
	//int i = 0;
	//for (auto& a : Buttons)
	//{
	//	a.SetLocation(200, 200 + i * 100);
	//	a.SetButtonID(i);
	//	i++;
	//}

	//Buttons[0].SetTargetPointID(MAP_STAGE); //START
	//Buttons[1].SetTargetPointID(MAP_STAGE); //SETTING
	//Buttons[2].SetTargetPointID(GAME_END); //END

	SelectionButton = 0;
}
void Map_Stage::Update(HWND hWnd) {
	//for (auto& a : Buttons)
	//	a.Update();
}
void Map_Stage::Draw(HDC hMemDC, HDC hMemDC1) {
	//for (auto& a : Buttons)
	//	a.Draw(hMemDC, hMemDC1);
}

void Map_Stage::Comp_NextStage(int ID) {

}
int Map_Stage::Input(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
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

		break;
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
		{
			
		}
		else if (wParam == VK_UP) {

		}
		else if (wParam == VK_DOWN) {

		}
		break;
	case WM_CHAR:
		break;
	}
	return -2;
}