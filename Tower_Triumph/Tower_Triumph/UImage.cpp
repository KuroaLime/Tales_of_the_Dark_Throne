#include "UImage.h"

void UImage::Draw(HDC hMemDC, HDC hMemDC1)
{
	if (!bVisible || bBitmap.empty()) return;

	HBITMAP temp = (HBITMAP)SelectObject(hMemDC1, bBitmap[CurAni]);
	TransparentBlt(hMemDC, Loc.x - Size.sX / 2, Loc.y - Size.sY / 2, Size.sX, Size.sY,
		hMemDC1, 0, 0, bMapinfo[CurAni].bmWidth, bMapinfo[CurAni].bmHeight, RGB(255, 255, 255));
	SelectObject(hMemDC1, temp);
}
