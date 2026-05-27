#include "AButton.h"


AButton::AButton() {
	Loc.x = 0;
	Loc.y = 0;

	Size.sX = 100;
	Size.sY = 50;

	IsPointed = false;
	isClicked = false;

	AniCount = 0;
	CurAni = 0;
}

void AButton::SetFrame(const HINSTANCE& g_hInst, std::vector<WORD> FrameStr) {
	AniCount = FrameStr.size();
	bBitmap.resize(AniCount);
	bMapinfo.resize(AniCount);
	for (int i = 0; i < AniCount; i++) {
		bBitmap[i] = (HBITMAP)LoadBitmap(g_hInst, MAKEINTRESOURCE(FrameStr[i]));
		GetObject(bBitmap[i], sizeof(BITMAP), &bMapinfo[i]);
	}
	CurAni = 0;
}

void AButton::Draw(HDC hMemDC, HDC hMemDC1) {
	//HBITMAP temp = (HBITMAP)SelectObject(hMemDC1, bBitmap[CurAni]);
	//TransparentBlt(hMemDC, Loc.x - Size.sX/2, Loc.y - Size.sY / 2, Loc.x + Size.sX / 2, Loc.y + Size.sY / 2, hMemDC1, 0, 0, bMapinfo[CurAni].bmWidth, bMapinfo[CurAni].bmHeight, RGB(255, 255, 255));
	HBRUSH bBrush;
	if(!IsPointed)
	{
		bBrush = CreateSolidBrush(RGB(255, 0, 0));
	}
	else {
		bBrush = CreateSolidBrush(RGB(0, 255, 0));
	}
		RECT rects = { Loc.x - Size.sX / 2, Loc.y - Size.sY / 2, Loc.x + Size.sX / 2, Loc.y + Size.sY / 2 };
		FillRect(hMemDC1, &rects, bBrush);
		DeleteObject(bBrush);
	
	//SelectObject(hMemDC1, temp);
}
void AButton::SetIsPointed(bool New) {
	if (New) IsPointed = true;
	else IsPointed = false;
}
bool AButton::GetIsPointed() {
	return IsPointed;
}
bool AButton::bCollision(int MouseX, int MouseY) {
	RECT rects = { Loc.x - Size.sX / 2, Loc.y - Size.sY / 2, Loc.x + Size.sX / 2, Loc.y + Size.sY / 2 };
	if ((rects.left < MouseX && MouseX < rects.right) && rects.top < MouseY && MouseY < rects.bottom) {
		return true;

	}
	return false;
}
void AButton::bClick() {
	if (IsPointed) {
		if (isClicked) isClicked = false;
		else isClicked = true;
	}
}
bool AButton::GetbClick() {
	return isClicked;
}
void AButton::SetbClick(bool New) {
	isClicked = New;
}
void AButton::Update() {
	//애니메이션 등등
}

void AButton::SetLocation(int nX, int nY) {
	Loc.x = nX;
	Loc.y = nY;
}
Location AButton::GetLocation() {
	return Loc;
}


void AButton::SetButtonID(int New) {
	ButtonID = New;
}
int AButton::GetButtonID() {
	return ButtonID;
}
void AButton::SetTargetPointID(int New) {
	TargetPointID = New;
}
int AButton::GetTargetPointID() {
	return TargetPointID;
}