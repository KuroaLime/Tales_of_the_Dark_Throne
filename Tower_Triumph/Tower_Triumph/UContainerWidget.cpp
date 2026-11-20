#include "UContainerWidget.h"
#include <algorithm>

UContainerWidget::UContainerWidget()
{
}

UContainerWidget::~UContainerWidget()
{
	ClearChildren(true);
}

void UContainerWidget::Init() {

}
void UContainerWidget::Draw(HDC hMemDC, HDC hMemDC1) {
	if (!bVisible) return;

	if (!bBitmap.empty()) {
		HBITMAP temp = (HBITMAP)SelectObject(hMemDC1, bBitmap[CurAni]);
		TransparentBlt(hMemDC, Loc.x - Size.sX / 2, Loc.y - Size.sY / 2, Size.sX, Size.sY, hMemDC1, 0, 0, bMapinfo[CurAni].bmWidth, bMapinfo[CurAni].bmHeight, RGB(255, 255, 255));
		SelectObject(hMemDC1, temp);
	}

	for (int i = 0; i < (int)Contents.size(); i++) {
		Contents[i]->Draw(hMemDC, hMemDC1);
	}
}
void UContainerWidget::Update() {
	if (!bVisible) return;
	if (AniCount > 0) {
		CurAni = (CurAni + 1) % AniCount;
	}

	for (int i = 0; i < (int)Contents.size(); i++) {
		Contents[i]->Update();
	}
}
void UContainerWidget::CancelPress() {
	for (auto child : Contents) child->CancelPress();
}
void UContainerWidget::UpdateMouseHover(int MouseX, int MouseY) {
	if (!bVisible) {
		IsPointed = false;
		for (auto child : Contents) child->UpdateMouseHover(-100000, -100000);
		return;
	}

	IsPointed = ContainsPoint(MouseX, MouseY);

	for (auto child : Contents) {
		child->UpdateMouseHover(MouseX, MouseY);
	}
}

UWidget* UContainerWidget::FindWidgetUnderMouse(int MouseX, int MouseY) {
	if (!bVisible) return nullptr;

	for (int i = (int)Contents.size() - 1; i >= 0; i--) {
		UWidget* FindChild = Contents[i]->FindWidgetUnderMouse(MouseX, MouseY);
		if (FindChild != nullptr)
			return FindChild;
	}

	if (ContainsPoint(MouseX, MouseY))
		return this;

	return nullptr;
}

void UContainerWidget::AddChild(UWidget* NewChild) {
	if (NewChild) {
		Contents.push_back(NewChild);
	}
};

void UContainerWidget::RemoveChild(UWidget* Child, bool bDelete)
{
	auto it = std::find(Contents.begin(), Contents.end(), Child);
	if (it != Contents.end())
	{
		UWidget* pWidget = *it;
		Contents.erase(it);
		if (bDelete && pWidget)
		{
			delete pWidget;
		}
	}
}

void UContainerWidget::ClearChildren(bool bDelete)
{
	if (bDelete)
	{
		std::vector<UWidget*> temp = Contents;
		Contents.clear(); 
		for (auto child : temp)
		{
			if (child) delete child;
		}
	}
	else
	{
		Contents.clear();
	}
}
