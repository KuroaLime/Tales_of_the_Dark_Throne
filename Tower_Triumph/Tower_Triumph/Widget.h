#pragma once
#include<windows.h>
#include<vector>
#include"Structure.h"
class UWidget
{
protected:
	int UI_ID;
	Location Loc;
	BuSIZE Size;
	RECT rects;

	bool IsPointed;

	int AniCount;
	int CurAni;
	std::vector<HBITMAP> bBitmap;
	std::vector<BITMAP> bMapinfo;

public:
	UWidget();
	virtual void Init();
	virtual void Draw(HDC hMemDC, HDC hMemDC1);
	virtual void Update();
	virtual UWidget* FindWidgetUnderMouse(int MouseX, int MouseY);
	

	//오버라이드 안 해도 되는 함수들
	virtual void SetFrame(const HINSTANCE& g_hInst, std::vector<WORD> FrameStr);
	
	virtual void SetIsPointed(bool New);
	virtual bool GetIsPointed();

	virtual bool ContainsPoint(int MouseX, int MouseY);

	void SetLocation(int nX, int nY);
	Location GetLocation();

	void SetUIID(int New);
	int GetUIID();
	

	
};

