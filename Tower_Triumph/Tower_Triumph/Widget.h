#pragma once
#include<windows.h>
#include<vector>
#include<string>
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
	bool bVisible;
public:
	UWidget();
	virtual ~UWidget() {}
	virtual void Init();
	virtual void Draw(HDC hMemDC, HDC hMemDC1);
	virtual void Update();
	virtual void CancelPress() {}
	virtual UWidget* FindWidgetUnderMouse(int MouseX, int MouseY);
	
	virtual void SetFrame(const HINSTANCE& g_hInst, std::vector<WORD> FrameStr);
	virtual void SetFrameFromFile(std::vector<std::wstring> FramePaths);
	
	virtual void SetIsPointed(bool New);
	virtual bool GetIsPointed();

	virtual bool ContainsPoint(int MouseX, int MouseY);
	virtual void UpdateMouseHover(int MouseX, int MouseY);

	void SetLocation(int nX, int nY);
	Location GetLocation();

	void SetSize(int sX, int sY);
	BuSIZE GetSize();

	void SetUIID(int New);
	int GetUIID();

	std::vector<HBITMAP>& GetBitmap() { return bBitmap; }
	std::vector<BITMAP>& GetMapInfo() { return bMapinfo; }

	bool GetVisible() const { return bVisible; }
	virtual void SetVisible(bool bNew) { bVisible = bNew; }
};

