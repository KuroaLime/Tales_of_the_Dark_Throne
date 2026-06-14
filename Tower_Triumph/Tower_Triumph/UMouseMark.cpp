#include "UMouseMark.h"
#include "URegionMarker.h"
#include <cmath>
int Win_xS = 1280;
int Win_yS= 720;

UMouseMark::UMouseMark()
{
	targetMarker = nullptr;
	angle = 0.0f;
	currentMousePos = { 0, 0 };
	cameraX = 0;
	cameraY = 0;
	currentZoom = 1.0f;
	bVisible = true;
}

void UMouseMark::Update()
{
	angle += 0.05f;
	if (angle > 2.0f * 3.141592f) angle -= 2.0f * 3.141592f;

	if (targetMarker)
	{
		Location iconLoc = targetMarker->GetIconLocation();
		Loc.x = (int)((iconLoc.x - cameraX) * currentZoom);
		Loc.y = (int)((iconLoc.y - cameraY) * currentZoom);
	}
	else
	{
		Loc.x = currentMousePos.x;
		Loc.y = currentMousePos.y;
	}
}

void UMouseMark::Draw(HDC hMemDC, HDC hMemDC1)
{
	if (!bVisible || bBitmap.empty()) return;

	HBITMAP hBit = bBitmap[0];
	BITMAP info = bMapinfo[0];

	XFORM oldForm;
	int oldMode = GetGraphicsMode(hMemDC);
	GetWorldTransform(hMemDC, &oldForm);
	SetGraphicsMode(hMemDC, GM_ADVANCED);

	int oldStretchMode = SetStretchBltMode(hMemDC, COLORONCOLOR);
	float cosA = static_cast<float>(cos(angle));
	float sinA = static_cast<float>(sin(angle));

	XFORM xForm;
	xForm.eM11 = cosA;
	xForm.eM12 = sinA;
	xForm.eM21 = -sinA;
	xForm.eM22 = cosA;
	xForm.eDx = (float)Loc.x;
	xForm.eDy = (float)Loc.y;

	SetWorldTransform(hMemDC, &xForm);

	HBITMAP hOld = (HBITMAP)SelectObject(hMemDC1, hBit);
	TransparentBlt(hMemDC, -Win_xS / 30, -Win_xS / 30, Win_xS / 15 , Win_xS / 15,
		hMemDC1, 0, 0, info.bmWidth, info.bmHeight, RGB(0,0,0));

	SelectObject(hMemDC1, hOld);

	SetStretchBltMode(hMemDC, oldStretchMode);
	SetWorldTransform(hMemDC, &oldForm);
	SetGraphicsMode(hMemDC, oldMode);
}
