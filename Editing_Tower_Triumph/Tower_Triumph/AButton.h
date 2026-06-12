#pragma once
#include<Windows.h>
#include"Structure.h"
#include<vector>



class AButton
{
protected:
	int ButtonID;
	int TargetPointID;
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

	virtual void SetIsPointed(bool New);
	virtual bool GetIsPointed();
	virtual bool bCollision(int MouseX, int MouseY);
	virtual void bClick();
	virtual void Update();

	bool GetbClick();
	void SetbClick(bool New);

	void SetLocation(int nX, int nY);
	Location GetLocation();

	void SetButtonID(int New);
	int GetButtonID();

	void SetTargetPointID(int New);
	int GetTargetPointID();
};

