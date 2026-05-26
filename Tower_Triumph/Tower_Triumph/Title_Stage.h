#pragma once
#include"CStage.h"

class Title_Stage : public CStage
{

public:
	Title_Stage();
	virtual void Init(HWND hWnd);
	virtual void Update(HWND hWnd);
	virtual void Draw(HDC hMemDC, HDC hMemDC1);
	virtual void Input(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

