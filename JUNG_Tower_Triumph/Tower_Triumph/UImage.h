#pragma once
#include "Widget.h"

class UImage : public UWidget
{
public:
	virtual void Draw(HDC hMemDC, HDC hMemDC1) override;
};
