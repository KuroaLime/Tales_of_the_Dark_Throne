#pragma once
#include"CStage.h"
#include"AButton.h"
class Title_Stage : public CStage
{
private:
	AButton StartButton;
	AButton SettingButton;
	AButton EndButton;

	int SeletionButton;
public:
	Title_Stage();
	virtual void Init(HWND hWnd);
	virtual void Update(HWND hWnd);
	virtual void Draw(HDC hMemDC, HDC hMemDC1);
	virtual void Input(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

