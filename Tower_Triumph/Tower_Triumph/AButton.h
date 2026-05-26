#pragma once
#include<Windows.h>
#include"Structure.h"
#include<vector>



class AButton
{
protected:
	Location Loc;
	BuSIZE Size;
	bool IsPointed;
	bool isClicked;

	int AniCount;
	int CurAni;
	std::vector<HBITMAP> bBitmap;
	std::vector<BITMAP> bMapinfo;
	
public:
	AButton();

	virtual void SetFrame(const HINSTANCE& g_hInst, std::vector<WORD> FrameStr);

	virtual void Draw(HDC hMemDC, HDC hMemDC1);

	virtual void bCollision(int MouseX, int MouseY);
	virtual void bClick();
	virtual void Update();

	void SetLocation(int nX, int nY);
	Location GetLocation();
};

