#include "Map_Stage.h"
#include "resource.h"

Map_Stage::Map_Stage() {
	

}
void Map_Stage::Init(HWND hWnd, HINSTANCE const g_hInst) {

	GetClientRect(hWnd, &rects);
	
	hBitBackground = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_SMAP_BACKGROUND));
	GetObject(hBitBackground, sizeof(BITMAP), &BackgroundInfo);
	
	Camera = new ACamera((float)(((BackgroundInfo.bmWidth) / 2) / 64 * TILE_SIZE - 1280 / 2 + TILE_SIZE / 2),
		(float)(((BackgroundInfo.bmHeight) / 2) / 64 * TILE_SIZE - 720 / 2 + TILE_SIZE / 2),
		((BackgroundInfo.bmWidth) / 2) / 64, ((BackgroundInfo.bmHeight) / 2)/64);

	Loc.first.x = rects.right / 2;
	Loc.first.y = rects.bottom / 2;
	Loc.second.sX = BackgroundInfo.bmWidth / 2;
	Loc.second.sY = BackgroundInfo.bmHeight / 2;

	//타일 크기
	TILE_SIZE = 64;
	//타일 개수
	W = BackgroundInfo.bmWidth / TILE_SIZE;
	H = BackgroundInfo.bmHeight / TILE_SIZE;
	Baord.resize(H, std::vector<Tile>(W));

	Buttons.resize(buttonCount);
	int i = 0;
	for (auto& a : Buttons)
	{
		a.SetLocation(BackgroundInfo.bmWidth / 2 + ButtonLocs[i].x, BackgroundInfo.bmHeight / 2 + ButtonLocs[i].y);
		a.SetButtonID(i);
		i++;
	}

	Buttons[0].SetTargetPointID(MAP_STAGE); //상점
	Buttons[1].SetTargetPointID(MAP_STAGE); //스테이지1
	Buttons[2].SetTargetPointID(MAP_STAGE); //스테이지2
	Buttons[3].SetTargetPointID(MAP_STAGE); //스테이지3
	Buttons[4].SetTargetPointID(MAP_STAGE); //스테이지4

	SelectionButton = 0;
}
void Map_Stage::Update(HWND hWnd) {
	for (auto& a : Buttons)
		a.Update();
	Camera->Update(W, H, TILE_SIZE, 1280, 720);
}

//TEST용 보드 그리기 코드
void DrawGrid(HDC hDC, int width, int height, int tileSIZE, int offsetX, int offsetY) {
	HPEN hGridPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
	HPEN hOldPen = (HPEN)SelectObject(hDC, hGridPen);

	for (int y = 0; y <= height; ++y) {
		MoveToEx(hDC, offsetX, offsetY + (y * tileSIZE), NULL);
		LineTo(hDC, offsetX + (width * tileSIZE) + 1, offsetY + (y * tileSIZE));
	}

	for (int x = 0; x <= width; ++x) {
		MoveToEx(hDC, offsetX + (x * tileSIZE), offsetY, NULL);
		LineTo(hDC, offsetX + (x * tileSIZE), offsetY + (height * tileSIZE) + 1);
	}

	SelectObject(hDC, hOldPen);
	DeleteObject(hGridPen);
}

void Map_Stage::Draw(HDC hMemDC, HDC hMemDC1) {

	HBITMAP hOld = (HBITMAP)SelectObject(hMemDC1, hBitBackground);
	std::pair<float,float> CameraOffset = Camera->Getg_Cam();
	TransparentBlt(hMemDC, (int)-CameraOffset.first, (int)-CameraOffset.second, BackgroundInfo.bmWidth, BackgroundInfo.bmHeight, hMemDC1, 0, 0, BackgroundInfo.bmWidth, BackgroundInfo.bmHeight, RGB(255, 255, 255));
	SelectObject(hMemDC1, hOld);

	DrawGrid(hMemDC, W, H, TILE_SIZE, (int)-CameraOffset.first, (int)-CameraOffset.second);
}

void Map_Stage::Comp_NextStage(int ID) {

}
int Map_Stage::Input(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static int CLK_X=0, CLK_Y=0;
	std::pair<float, float> g_Cam= Camera->Getg_Cam();
	static int gridX= Camera->GetPointGrid().first ,gridY= Camera->GetPointGrid().second;;
	switch (uMsg) {
	case WM_LBUTTONDOWN:
		

		break;
	case WM_LBUTTONUP:
		CLK_X = LOWORD(lParam);
		CLK_Y = HIWORD(lParam);
		g_Cam = Camera->Getg_Cam();
		gridX = (g_Cam.first + CLK_X) / TILE_SIZE;
		gridY = (g_Cam.second + CLK_Y) / TILE_SIZE;

		Camera->SetPointGrid(gridX, gridY);
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
			//메뉴
		}
		else if (wParam == VK_UP) {
			if (gridY > 0)
			Camera->SetPointGrid(gridX, --gridY);
		}
		else if (wParam == VK_DOWN) {
			if (gridY < H)
			Camera->SetPointGrid(gridX, ++gridY);
		}
		else if (wParam == VK_LEFT) {
			if(gridX>0)
				Camera->SetPointGrid(--gridX, gridY);
		}
		else if (wParam == VK_RIGHT) {
			if (gridX < W)
				Camera->SetPointGrid(++gridX, gridY);
		}
		break;
	case WM_CHAR:
		break;
	}
	return -2;
}
