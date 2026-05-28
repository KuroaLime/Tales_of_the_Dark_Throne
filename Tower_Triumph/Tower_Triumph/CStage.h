#pragma once
#include <windows.h>
#include <vector>

#include "Structure.h"

#define GAME_END -1
#define TITLE_STAGE 0
#define MAP_STAGE 1


class CStage {
protected:

	std::vector<std::vector<Tile>> Baord;
	//타일 크기
	int TILE_SIZE = 64;
	//타일 개수
	int W;
	int H;
	HBITMAP hBitBackground;
	BITMAP BackgroundInfo;
	std::pair<Location,BuSIZE> Loc;

	RECT rects;
public:

	virtual ~CStage() {};

	virtual void Init(HWND hWnd,HINSTANCE const g_hInst) = 0;
	virtual void Update(HWND hWnd) = 0;
	virtual void Draw(HDC hMemDC, HDC hMemDC1) = 0;
	virtual int Input(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;

	virtual void Comp_NextStage(int ID)=0;
	virtual void BackgroundMove(int x, int y) = 0;
};