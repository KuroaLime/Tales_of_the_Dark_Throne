#pragma once
#include "Widget.h"

class UImage : public UWidget
{
protected:
	COLORREF TransColor = RGB(255, 255, 255);
public:
	virtual void Draw(HDC hMemDC, HDC hMemDC1) override;
	void SetTransColor(COLORREF color) { TransColor = color; }
};
