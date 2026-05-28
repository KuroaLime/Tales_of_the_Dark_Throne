#pragma once
#include"CStage.h"
#include"AButton.h"
#include"ACamera.h"

#define buttonCount 5

const Location ButtonLocs[buttonCount] = { {-200,0},{200,},{0,200},{0,-200},{0,0} };

class Map_Stage : public CStage
{
private:
	
	ACamera* Camera;

	std::vector<AButton> Buttons;

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

