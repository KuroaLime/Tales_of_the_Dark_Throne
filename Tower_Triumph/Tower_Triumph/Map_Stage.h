#pragma once
#include"CStage.h"
#include"AButton.h"
#include"ACamera.h"

#define buttonCount 3


class Map_Stage : public CStage
{
private:
	
	ACamera* Camera;
	int SelectionButton;
public:
	Map_Stage();
	virtual void Init(HWND hWnd, HINSTANCE const g_hInst);
	virtual void Update(HWND hWnd);
	virtual void Draw(HDC hMemDC, HDC hMemDC1);
	virtual int Input(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	virtual void Comp_NextStage(int ID);
	virtual void BackgroundMove(int x, int y){};
};

