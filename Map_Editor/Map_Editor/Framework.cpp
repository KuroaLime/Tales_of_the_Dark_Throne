#include <windows.h>
#include <tchar.h>
#include"resource.h"
#include<vector>
#include"CMapManger.h"
#pragma comment(lib, "msimg32.lib")


int Win_x = 1280;
int Win_y = 720;

HINSTANCE g_hInst=NULL;

HWND hPaletteDlg = NULL;
int g_SelectedTileType = 0;

#define MOVABLE_NORMAL 1
#define MOVABLE_LAVA 2
#define MOVABLE_DARK 3
#define MOVABLE_LIGHT 4

#define UNMOVABLE 5


LPCTSTR lpszClass = L"My Window Class";
LPCTSTR lpszWindowName = L"Map Editor";
LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
	HWND hWnd;
	MSG Message;
	WNDCLASSEX WndClass;
	g_hInst = hInstance;
	WndClass.cbSize = sizeof(WndClass);
	WndClass.style = CS_HREDRAW | CS_VREDRAW;
	WndClass.lpfnWndProc = (WNDPROC)WndProc;
	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hInstance = hInstance;
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	WndClass.lpszMenuName = NULL;
	WndClass.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
	WndClass.lpszClassName = lpszClass;
	WndClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	RegisterClassEx(&WndClass);
	hWnd = CreateWindow(lpszClass, lpszWindowName, WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_THICKFRAME | WS_HSCROLL | WS_VSCROLL, 100, 50, Win_x, Win_y, NULL, (HMENU)NULL, hInstance, NULL);
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
	while (GetMessage(&Message, 0, 0, 0)) {
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	}
	return Message.wParam;
}


INT_PTR CALLBACK MapSettingsDlgProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK PaletteDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hDC, hMemDC;
	static HBITMAP hBit;
	HBITMAP hOld;

	HBRUSH hBrush, oldBrush;
	HPEN hPen, oldPen;

	static int CLK_X, CLK_Y;
	static int CUR_CLK_X, CUR_CLK_Y;
	static RECT WinRect;

	static MapConfig Defualt_Map_Data;
	static MapManager g_MapManager;

	static bool bDrag;
	switch (uMsg) {
	case WM_CREATE:
		//srand((unsigned int)time(NULL));
		GetClientRect(hWnd, &WinRect);
		
		bDrag = false;

		//wParam = ID_FILE_NEWFILE;
		//g_MapManager.Reset(L"background.bmp");
		hDC = GetDC(hWnd);
		hMemDC = CreateCompatibleDC(hDC);
		hBit = CreateCompatibleBitmap(hDC, WinRect.right, WinRect.bottom);
		ReleaseDC(hWnd, hDC);

		
		SetTimer(hWnd, 1, 10, NULL);
		break;
	case WM_KEYDOWN:
		
		InvalidateRect(hWnd, NULL, FALSE);
		break;
	case WM_TIMER:
	{
		hDC = GetDC(hWnd);
		hMemDC = CreateCompatibleDC(hDC);

		HBITMAP hOldBit = (HBITMAP)SelectObject(hMemDC, hBit);

		HBRUSH hWhiteBrush = (HBRUSH)GetStockObject(WHITE_BRUSH);
		RECT rectClient;
		GetClientRect(hWnd, &rectClient);
		FillRect(hMemDC, &rectClient, hWhiteBrush);

		g_MapManager.Render(hMemDC);

		SelectObject(hMemDC, hOldBit);
		DeleteDC(hMemDC);
		ReleaseDC(hWnd, hDC);

		InvalidateRect(hWnd, NULL, FALSE);
	}
		break;
	case WM_LBUTTONDOWN: {
		int mx = LOWORD(lParam);
		int my = HIWORD(lParam);

		g_MapManager.ScreenToMap(mx, my,hWnd,g_SelectedTileType);
		break;
	}
	case WM_LBUTTONUP:
		CLK_X = LOWORD(lParam);
		CLK_Y = HIWORD(lParam);
		InvalidateRect(hWnd, NULL, FALSE);
		break;
	case WM_RBUTTONUP:
		CLK_X = LOWORD(lParam);
		CLK_Y = HIWORD(lParam);
		bDrag = false;
		InvalidateRect(hWnd, NULL, FALSE);
		break;
	case WM_RBUTTONDOWN:
		CLK_X = LOWORD(lParam);
		CLK_Y = HIWORD(lParam);

		bDrag = true;
		
		InvalidateRect(hWnd, NULL, FALSE);
		break;
	case WM_MOUSEMOVE:

		if (bDrag) {
			CUR_CLK_X = LOWORD(lParam);
			CUR_CLK_Y = HIWORD(lParam);

			g_MapManager.AddLoc(-(CLK_X - CUR_CLK_X), -(CLK_Y - CUR_CLK_Y));

			CLK_X = CUR_CLK_X;
			CLK_Y = CUR_CLK_Y;
			InvalidateRect(hWnd, NULL, FALSE);

		}
		
		break;
	case WM_MOUSEWHEEL:
	{
		short wheelDelta = (short)HIWORD(wParam);

		if (wheelDelta > 0) {
			g_MapManager.Scrolling(true);
		}
		else if (wheelDelta < 0) {
			g_MapManager.Scrolling(false);
		}

		if (LOWORD(wParam) & MK_CONTROL) {
			// Ctrl + 휠 스크롤 처리
		}
	}
	break;
	case WM_CHAR:
		if (wParam == 'q' || wParam == 'Q') {
			PostQuitMessage(0);
		}
		InvalidateRect(hWnd, NULL, FALSE);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		//메뉴와 연결
		//File
		case ID_FILE_NEWFILE:
		{
			// 파일 열기 상자의 정보를 담는 구조체
			OPENFILENAME OFN;
			// 선택된 파일의 전체 경로명이 저장될 버퍼
			TCHAR szFile[MAX_PATH] = L"";
			// 탐색기 하단 '파일 형식' 칸에 보여줄 필터링 규칙을 정의합니다. (끝에 \0 필수)
			TCHAR szFilter[] = L"비트맵 파일(*.bmp)\0*.bmp\0모든 파일(*.*)\0*.*\0";

			// 구조체 초기화 및 세팅
			memset(&OFN, 0, sizeof(OPENFILENAME));
			OFN.lStructSize = sizeof(OPENFILENAME);
			OFN.hwndOwner = hWnd;	// 대화상자의 부모 윈도우 지정
			OFN.lpstrFilter = szFilter;	// 파일 확장자 필터 적용
			OFN.lpstrFile = szFile;	// 경로를 받아올 문자열 포인터
			OFN.nMaxFile = MAX_PATH;	// 버퍼의 최대 크기
			OFN.lpstrInitialDir = L".";	// 대화상자가 처음 열릴 기본 디렉토리

			// 파일 탐색기를 열고, 사용자가 '열기(확인)'를 눌렀을 때만 작동
			if (GetOpenFileName(&OFN) != 0) {
				// 사용자가 탐색기에서 최종 선택한 외부 비트맵 파일의 경로(szFile)를 전달!
				g_MapManager.Reset(szFile);

				// 맵 크기도 배경 비트맵 해상도에 맞춰 자동으로 리빌드하고 싶다면 아래처럼 동기화 가능합니다.
				// g_MapManager.RebuildMap(..., ..., ..., ...);

				// 화면을 새로 그리도록 갱신 요청
				InvalidateRect(hWnd, NULL, FALSE);
			}
		}
			break;
		case ID_FILE_SAVE:
		{
			OPENFILENAME SFN;
			TCHAR szFile[MAX_PATH] = L"untitled.map";
			TCHAR szFilter[] = L"맵 데이터 파일(*.map)\0*.map\0모든 파일(*.*)\0*.*\0";

			memset(&SFN, 0, sizeof(OPENFILENAME));
			SFN.lStructSize = sizeof(OPENFILENAME);
			SFN.hwndOwner = hWnd;
			SFN.lpstrFilter = szFilter;
			SFN.lpstrFile = szFile;
			SFN.nMaxFile = MAX_PATH;
			SFN.lpstrInitialDir = L".";
			SFN.Flags = OFN_OVERWRITEPROMPT;

			if (GetSaveFileName(&SFN) != 0) {
				g_MapManager.SaveMap(szFile);
			}
		}
			break;
		case ID_FILE_LOAD:
		{
			OPENFILENAME OFN;
			TCHAR szFile[MAX_PATH] = L"";
			TCHAR szFilter[] = L"맵 데이터 파일(*.map)\0*.map\0모든 파일(*.*)\0*.*\0";

			memset(&OFN, 0, sizeof(OPENFILENAME));
			OFN.lStructSize = sizeof(OPENFILENAME);
			OFN.hwndOwner = hWnd;
			OFN.lpstrFilter = szFilter;
			OFN.lpstrFile = szFile;
			OFN.nMaxFile = MAX_PATH;
			OFN.lpstrInitialDir = L".";

			if (GetOpenFileName(&OFN) != 0) {
				g_MapManager.LoadMap(szFile);
			}
		}
			break;
		case ID_FILE_QUIT:
			PostQuitMessage(0);
			break;
		//Edit
		case ID_EDIT_INSERT:
			if (!IsWindow(hPaletteDlg)) {
				hPaletteDlg = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_PALETTE), hWnd, PaletteDlgProc);
				ShowWindow(hPaletteDlg, SW_SHOW);
			}
			break;
		case ID_EDIT_DELETE:
			break;
		case ID_EDIT_DEFAULTSETTING:
			if (DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_DIALOG1), hWnd, MapSettingsDlgProc, (LPARAM)&Defualt_Map_Data) == IDOK)
			{
				g_MapManager.RebuildMap(Defualt_Map_Data.X, Defualt_Map_Data.Y, Defualt_Map_Data.TILE_W, Defualt_Map_Data.TILE_H);
				InvalidateRect(hWnd, NULL, TRUE);
			}
			break;
		}
		break;
	case WM_PAINT:
		hDC = BeginPaint(hWnd, &ps);
		hMemDC = CreateCompatibleDC(hDC);

		hOld = (HBITMAP)SelectObject(hMemDC, hBit);
		BitBlt(hDC, 0, 0, WinRect.right, WinRect.bottom, hMemDC, 0, 0, SRCCOPY);

		SelectObject(hMemDC, hOld);
		DeleteDC(hMemDC);
		EndPaint(hWnd, &ps);
		return 0;
	case WM_DESTROY:
		DeleteObject(hBit);
		KillTimer(hWnd, 1);
		PostQuitMessage(0);
		break;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}



INT_PTR CALLBACK MapSettingsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_INITDIALOG:
		SetWindowLongPtr(hDlg, DWLP_USER, lParam);

		SetDlgItemInt(hDlg, IDC_DEFAULTSETTING_EDIT_X, 10, FALSE);
		SetDlgItemInt(hDlg, IDC_DEFAULTSETTING_EDIT_Y, 10, FALSE);
		SetDlgItemInt(hDlg, IDC_DEFAULTSETTING_EDIT_TILE_W, 64, FALSE);
		SetDlgItemInt(hDlg, IDC_DEFAULTSETTING_EDIT_TILE_H, 64, FALSE);
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK) {
			
			int x = GetDlgItemInt(hDlg, IDC_DEFAULTSETTING_EDIT_X, NULL, FALSE);
			int y = GetDlgItemInt(hDlg, IDC_DEFAULTSETTING_EDIT_Y, NULL, FALSE);
			int TILE_W = GetDlgItemInt(hDlg, IDC_DEFAULTSETTING_EDIT_TILE_W, NULL, FALSE);
			int TILE_H = GetDlgItemInt(hDlg, IDC_DEFAULTSETTING_EDIT_TILE_H, NULL, FALSE);

			bool bCorrectRate = true;
			if (x <= 0 || y <= 0 || x > 128 || y > 128) {
				MessageBox(hDlg, L"맵 크기는 1~128 사이여야 합니다.", L"경고", MB_ICONWARNING);
				return (INT_PTR)TRUE;
			}
			else if (TILE_W != TILE_H * 2) {
				MessageBox(hDlg, L"타일의 가로 길이는 세로 길이의 2배여야 합니다.\n자동으로 가로 길이에 맞춰 수정됩니다", L"경고", MB_ICONWARNING);
				bCorrectRate = false;
			}

			
			MapConfig* pConfig = (MapConfig*)GetWindowLongPtr(hDlg, DWLP_USER);
			pConfig->X = x;
			pConfig->Y = y;
			pConfig->TILE_W = TILE_W;
			if(bCorrectRate)
				pConfig->TILE_H = TILE_H;
			else
				pConfig->TILE_H = TILE_W /2;

			EndDialog(hDlg, IDOK);
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDCANCEL) {
			EndDialog(hDlg, IDCANCEL);
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK PaletteDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_TILE_MOVABLE_NORMAL:
			g_SelectedTileType = MOVABLE_NORMAL;
			break;
		case IDC_TILE_MOVABLE_LAVA:
			g_SelectedTileType = MOVABLE_LAVA;
			break;
		case IDC_TILE_MOVABLE_DARK:
			g_SelectedTileType = MOVABLE_DARK;
			break;
		case IDC_TILE_MOVABLE_LIGHT:
			g_SelectedTileType = MOVABLE_LIGHT;
			break;
		case IDC_TILE_UNMOVABLE:
			g_SelectedTileType = UNMOVABLE;
			break;
		case ID_OUT:
			DestroyWindow(hDlg);
			hPaletteDlg = NULL;
			return (INT_PTR)TRUE; 
			break;
		}
		return (INT_PTR)TRUE;
	case WM_CLOSE:
		DestroyWindow(hDlg);
		hPaletteDlg = NULL;
		return (INT_PTR)TRUE;
	}
	return (INT_PTR)FALSE;
}




