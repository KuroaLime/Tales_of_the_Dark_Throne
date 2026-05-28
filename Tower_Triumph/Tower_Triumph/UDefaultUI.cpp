#include "UDefaultUI.h"


UDefaultUI::UDefaultUI() {
	Loc.x = 0;
	Loc.y = 0;

	Size.sX = 100;
	Size.sY = 50;

	IsPointed = false;
	isClicked = false;

	AniCount = 0;
	CurAni = 0;
}

void UDefaultUI::SetFrame(const HINSTANCE& g_hInst, std::vector<WORD> FrameStr) {
	AniCount = FrameStr.size();
	bBitmap.resize(AniCount);
	bMapinfo.resize(AniCount);
	for (int i = 0; i < AniCount; i++) {
		bBitmap[i] = (HBITMAP)LoadBitmap(g_hInst, MAKEINTRESOURCE(FrameStr[i]));
		GetObject(bBitmap[i], sizeof(BITMAP), &bMapinfo[i]);
	}
	CurAni = 0;
}

void UDefaultUI::Draw(HDC hMemDC, HDC hMemDC1) {
	//HBITMAP temp = (HBITMAP)SelectObject(hMemDC1, bBitmap[CurAni]);
	//TransparentBlt(hMemDC, Loc.x - Size.sX/2, Loc.y - Size.sY / 2, Loc.x + Size.sX / 2, Loc.y + Size.sY / 2, hMemDC1, 0, 0, bMapinfo[CurAni].bmWidth, bMapinfo[CurAni].bmHeight, RGB(255, 255, 255));

	//SelectObject(hMemDC1, temp);
}
void UDefaultUI::SetIsPointed(bool New) {
	if (New) IsPointed = true;
	else IsPointed = false;
}
bool UDefaultUI::GetIsPointed() {
	return IsPointed;
}
bool UDefaultUI::bCollision(int MouseX, int MouseY) {
	RECT rects = { Loc.x - Size.sX / 2, Loc.y - Size.sY / 2, Loc.x + Size.sX / 2, Loc.y + Size.sY / 2 };
	if ((rects.left < MouseX && MouseX < rects.right) && rects.top < MouseY && MouseY < rects.bottom) {
		return true;

	}
	return false;
}
void UDefaultUI::bClick() {
	if (IsPointed) {
		if (isClicked) isClicked = false;
		else isClicked = true;
	}
}
bool UDefaultUI::GetbClick() {
	return isClicked;
}
void UDefaultUI::SetbClick(bool New) {
	isClicked = New;
}
void UDefaultUI::Update() {
	//애니메이션 등등
}

void UDefaultUI::SetLocation(int nX, int nY) {
	Loc.x = nX;
	Loc.y = nY;
}
Location UDefaultUI::GetLocation() {
	return Loc;
}


void UDefaultUI::SetUIID(int New) {
	UI_ID = New;
}
int UDefaultUI::GetUIID() {
	return UI_ID;
}