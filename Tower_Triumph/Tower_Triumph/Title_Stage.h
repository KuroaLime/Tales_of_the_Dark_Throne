#pragma once
#include"CStage.h"
#include"AButton.h"

#define buttonCount 3

class Title_Stage : public CStage
{
private:
	std::vector<AButton> Buttons;
	

	int SelectionButton;
public:
	Title_Stage();
	virtual void Init(HWND hWnd);
	virtual void Update(HWND hWnd);
	virtual void Draw(HDC hMemDC, HDC hMemDC1);
	virtual int Input(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	virtual void Comp_NextStage(int ID);
};

