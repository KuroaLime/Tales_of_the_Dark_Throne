#include "UContainerWidget.h"




void UContainerWidget::Init() {

}
void UContainerWidget::Draw(HDC hMemDC, HDC hMemDC1) {
	//배경 그림 그리기
	HBITMAP temp = (HBITMAP)SelectObject(hMemDC1, bBitmap[CurAni]);
	TransparentBlt(hMemDC, Loc.x - Size.sX/2, Loc.y - Size.sY / 2, Loc.x + Size.sX / 2, Loc.y + Size.sY / 2, hMemDC1, 0, 0, bMapinfo[CurAni].bmWidth, bMapinfo[CurAni].bmHeight, RGB(255, 255, 255));
	SelectObject(hMemDC1, temp);

	for (int i = 0; i < Contents.size(); i++) {
		Contents[i]->Draw(hMemDC, hMemDC1);
	}
}
void UContainerWidget::Update() {
	if (CurAni < AniCount) CurAni++;
	else CurAni = 0;

	for (int i = 0; i < Contents.size(); i++) {
		Contents[i]->Update();
	}
}
UWidget* UContainerWidget::FindWidgetUnderMouse(int MouseX, int MouseY) {
	for (int i = 0; i < Contents.size(); i++) {
		UWidget* FindChild  = Contents[i]->FindWidgetUnderMouse(MouseX,MouseY);
		if (FindChild != nullptr)
			return FindChild;
	}
	return nullptr;
}