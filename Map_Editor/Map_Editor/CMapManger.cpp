#include "CMapManger.h"


void MapManager::Reset(TCHAR* szFilePath/*DWORD Image*/) {
	SetBackground(szFilePath);
	TILE_W = 64;
	TILE_H = 64;
	RebuildMap(BackgroundInfo.bmWidth / TILE_W + 1, BackgroundInfo.bmHeight / TILE_H + 1, TILE_W, TILE_H);
}
void MapManager::RebuildMap(int w, int h, int tileW, int tileH) {
	W = w;

	H = h;
	TILE_W = tileW;
	TILE_H = tileH;

	totalPixelW = w * TILE_W + TILE_W / 2;
	totalPixelH = h * (TILE_H / 2) + TILE_H / 2;

	m_mapData.clear();
	m_mapData.assign(h, std::vector<Tile>(w, { 0, 1 }));
}
void MapManager::SetBackground(TCHAR* szFilePath/*DWORD Image*/) {
	if (Background != NULL) {
		DeleteObject(Background);
		Background = NULL;
	}

	// 파일 경로를 이용해 외부 비트맵 파일을 직접 로드합니다.
	Background = (HBITMAP)LoadImage(NULL, szFilePath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

	if (Background != NULL) {
		GetObject(Background, sizeof(BITMAP), &BackgroundInfo);
	}
	else {
		MessageBox(NULL, L"비트맵 파일을 불러오지 못했습니다.", L"오류", MB_OK | MB_ICONERROR);
	}
	//Background = (HBITMAP)LoadBitmap(g_hInst, MAKEINTRESOURCE(Image));
	//GetObject(Background, sizeof(BITMAP), &BackgroundInfo);
}
POINT MapManager::GetScreenPos(int x, int y) {
	POINT pt;
	pt.x = x * TILE_W + m_offsetX;
	pt.y = y * TILE_H + m_offsetY;
	return pt;
}

void MapManager::Render(HDC hDC) {
	if (m_mapData.empty()) return;

	if (Background != NULL) {
		HDC hBGDC = CreateCompatibleDC(hDC);
		HBITMAP hOldBg = (HBITMAP)SelectObject(hBGDC, Background);
		StretchBlt(hDC, m_offsetX, m_offsetY, W * TILE_W, H * TILE_H, hBGDC, 0, 0, BackgroundInfo.bmWidth, BackgroundInfo.bmHeight, SRCCOPY);
		SelectObject(hBGDC, hOldBg);
		DeleteDC(hBGDC);
	}

	for (int y = 0; y < m_mapData.size(); ++y) {
		for (int x = 0; x < m_mapData[0].size(); ++x) {
			POINT pt = GetScreenPos(x, y);
			COLORREF tileColor = colorPalette[m_mapData[y][x].type];

			// 메모리 DC 생성 (알파 블렌딩용)
			HDC memDC = CreateCompatibleDC(hDC);
			HBITMAP hTileBmp = CreateCompatibleBitmap(hDC, TILE_W, TILE_H);
			HBITMAP hOldTileBmp = (HBITMAP)SelectObject(memDC, hTileBmp);

			// 정사각형 그리기
			RECT rcTile = { 0, 0, TILE_W, TILE_H };
			HBRUSH hBrush = CreateSolidBrush(tileColor);
			FillRect(memDC, &rcTile, hBrush);
			DeleteObject(hBrush);

			// 알파 블렌딩 설정
			BLENDFUNCTION bf = { AC_SRC_OVER, 0, 180, 0 };
			AlphaBlend(hDC, pt.x, pt.y, TILE_W, TILE_H, memDC, 0, 0, TILE_W, TILE_H, bf);

			// 리소스 정리
			SelectObject(memDC, hOldTileBmp);
			DeleteObject(hTileBmp);
			DeleteDC(memDC);

			// 가시성을 위한 정사각형 격자 테두리
			HPEN hGridPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
			HPEN hOldGridPen = (HPEN)SelectObject(hDC, hGridPen);
			HBRUSH hNullBrush = (HBRUSH)SelectObject(hDC, (HBRUSH)GetStockObject(NULL_BRUSH));

			Rectangle(hDC, pt.x, pt.y, pt.x + TILE_W, pt.y + TILE_H);

			SelectObject(hDC, hOldGridPen);
			SelectObject(hDC, hNullBrush);
			DeleteObject(hGridPen);
		}
	}
}
void MapManager::AddLoc(int x, int y) {
	m_offsetX += x;
	m_offsetY += y;
}
void MapManager::ScreenToMap(int mouseX, int mouseY, HWND hWnd, int g_SelectedTileType) {
	int tx = mouseX - m_offsetX;
	int ty = mouseY - m_offsetY;

	// 단순 나눗셈으로 인덱스 도출
	int mapX = tx / TILE_W;
	int mapY = ty / TILE_H;

	if (mapX >= 0 && mapX < W && mapY >= 0 && mapY < H) {
		m_mapData[mapY][mapX].type = g_SelectedTileType;
	}
}

void MapManager::Scrolling(bool bAdd) {
	if (bAdd) {
		if (TILE_W < 256) {
			TILE_W += 16;
			TILE_H += 16;
		}
	}
	else {
		if (TILE_W > 32) {
			TILE_W -= 16;
			TILE_H -= 16;
		}
	}
	totalPixelW = W * TILE_W;
	totalPixelH = H * TILE_H;
}


// MapManager 클래스의 public: 섹션 내부에 아래 두 함수를 추가해 주세요.

// [기능 1] 현재 설정된 타일 정보들을 외부 파일로 내보내기 (Save)
bool MapManager::SaveMap(TCHAR* szFilePath) {
	// 파일을 쓰기 모드로 생성합니다.
	HANDLE hFile = CreateFile(szFilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) return false;

	DWORD dwWritten;

	// 1. 맵의 가로(W), 세로(H) 칸수 정보를 먼저 파일 맨 앞에 기록합니다. (헤더 데이터)
	WriteFile(hFile, &W, sizeof(int), &dwWritten, NULL);
	WriteFile(hFile, &H, sizeof(int), &dwWritten, NULL);

	// 2. 2중 for문을 돌며 배열의 각 타일 데이터(type, height)를 순서대로 파일에 씁니다.
	for (int y = 0; y < H; ++y) {
		for (int x = 0; x < W; ++x) {
			WriteFile(hFile, &m_mapData[y][x], sizeof(Tile), &dwWritten, NULL);
		}
	}

	// 파일 핸들을 닫아 안전하게 저장 완료합니다.
	CloseHandle(hFile);
	MessageBox(NULL, L"맵 데이터를 성공적으로 내보냈습니다!", L"저장 완료", MB_OK | MB_ICONINFORMATION);
	return true;
}

// [기능 2] 외부로 내보냈던 맵 파일을 읽어와 에디터에 복원하기 (Load)
bool MapManager::LoadMap(TCHAR* szFilePath) {
	// 파일을 읽기 모드로 엽니다.
	HANDLE hFile = CreateFile(szFilePath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		MessageBox(NULL, L"맵 파일을 열 수 없습니다.", L"오류", MB_OK | MB_ICONERROR);
		return false;
	}

	DWORD dwRead;
	int loadedW = 0, loadedH = 0;

	// 1. 파일 맨 앞의 가로, 세로 크기부터 읽어옵니다.
	ReadFile(hFile, &loadedW, sizeof(int), &dwRead, NULL);
	ReadFile(hFile, &loadedH, sizeof(int), &dwRead, NULL);

	// 읽어온 크기를 바탕으로 데이터 매니저의 변수 및 벡터 배열을 재배정(Resize)합니다.
	W = loadedW;
	H = loadedH;
	totalPixelW = W * TILE_W + TILE_W / 2;
	totalPixelH = H * (TILE_H / 2) + TILE_H / 2;

	m_mapData.clear();
	m_mapData.assign(H, std::vector<Tile>(W, { 0, 1 }));

	// 2. 저장된 순서 그대로 타일 데이터를 하나씩 읽어와 배열을 채웁니다.
	for (int y = 0; y < H; ++y) {
		for (int x = 0; x < W; ++x) {
			ReadFile(hFile, &m_mapData[y][x], sizeof(Tile), &dwRead, NULL);
		}
	}

	CloseHandle(hFile);
	return true;
}