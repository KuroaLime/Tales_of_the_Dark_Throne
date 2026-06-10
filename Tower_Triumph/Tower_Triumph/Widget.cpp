#include "Widget.h"


UWidget::UWidget() {
	Loc.x = 0;
	Loc.y = 0;

	Size.sX = 100;
	Size.sY = 50;

	IsPointed = false;
	bVisible = true;

	AniCount = 0;
	CurAni = 0;
}


void UWidget::Init() {

}
void UWidget::Draw(HDC hMemDC, HDC hMemDC1) {

}
void UWidget::Update() {
}
UWidget* UWidget::FindWidgetUnderMouse(int MouseX, int MouseY) {
	if (ContainsPoint(MouseX, MouseY)) {
		return this;
	}
	return nullptr;
}


void UWidget::SetFrame(const HINSTANCE& g_hInst, std::vector<WORD> FrameStr) {
	AniCount = FrameStr.size();
	bBitmap.resize(AniCount);
	bMapinfo.resize(AniCount);
	for (int i = 0; i < AniCount; i++) {
		bBitmap[i] = (HBITMAP)LoadBitmap(g_hInst, MAKEINTRESOURCE(FrameStr[i]));
		GetObject(bBitmap[i], sizeof(BITMAP), &bMapinfo[i]);
	}
	CurAni = 0;
}

void UWidget::SetIsPointed(bool New) {
	if (New) IsPointed = true;
	else IsPointed = false;
}
bool UWidget::GetIsPointed() {
	return IsPointed;
}

bool UWidget::ContainsPoint(int MouseX, int MouseY) {
	RECT rects = { Loc.x - Size.sX / 2, Loc.y - Size.sY / 2, Loc.x + Size.sX / 2, Loc.y + Size.sY / 2 };
	if ((rects.left <= MouseX && MouseX <= rects.right) && rects.top <= MouseY && MouseY <= rects.bottom) {
		return true;
	}
	return false;
}

void UWidget::UpdateMouseHover(int MouseX, int MouseY) {
	if (!bVisible) {
		IsPointed = false;
		return;
	}
	IsPointed = ContainsPoint(MouseX, MouseY);
}

void UWidget::SetLocation(int nX, int nY) {
	Loc.x = nX;
	Loc.y = nY;
}
Location UWidget::GetLocation() {
	return Loc;
}

void UWidget::SetSize(int sX, int sY) {
	Size.sX = sX;
	Size.sY = sY;
}
BuSIZE UWidget::GetSize() {
	return Size;
}

void UWidget::SetUIID(int New) {
	UI_ID = New;
}
int UWidget::GetUIID() {
	return UI_ID;
}

