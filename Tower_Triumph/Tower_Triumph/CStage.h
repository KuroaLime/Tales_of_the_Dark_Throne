#pragma once
#include <windows.h>

#define GAME_END -1
#define TITLE_STAGE 0
#define MAP_STAGE 1


class CStage {
public:

	virtual ~CStage() {};

	virtual void Init(HWND hWnd) = 0;
	virtual void Update(HWND hWnd) = 0;
	virtual void Draw(HDC hMemDC, HDC hMemDC1) = 0;
	virtual int Input(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;

	virtual void Comp_NextStage(int ID)=0;
};