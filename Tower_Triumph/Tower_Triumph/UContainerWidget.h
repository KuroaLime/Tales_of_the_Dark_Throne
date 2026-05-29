#pragma once
#include"Widget.h"
#include"vector"
class UContainerWidget : public UWidget
{
protected:
	std::vector<UWidget*> Contents;

	int AniCount;
	int CurAni;
	std::vector<HBITMAP> bBackgorundBitmap;
	std::vector<BITMAP> bBackgroundMapinfo;
public:
	virtual void Init() override;
	virtual void Draw(HDC hMemDC, HDC hMemDC1) override;
	virtual void Update() override;
	virtual UWidget* FindWidgetUnderMouse(int MouseX, int MouseY) override;
};

