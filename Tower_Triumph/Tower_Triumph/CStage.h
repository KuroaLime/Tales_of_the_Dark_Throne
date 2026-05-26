#pragma once
#include <windows.h>

class CStage {
public:

	virtual ~CStage() {};

	virtual void Init(HWND hWnd) = 0;
	virtual void Update(HWND hWnd) = 0;
	virtual void Draw(HDC hMemDC, HDC hMemDC1) = 0;
	virtual void Input(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;
};