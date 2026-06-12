#include "UCanvasPanel.h"
#include "UContainerWidget.h"



void UCanvasPanel::Init(){

}
void UCanvasPanel::Draw(HDC hMemDC, HDC hMemDC1){
	for (int i = 0; i < static_cast<int>(Containers.size()); i++) {
		Containers[i]->Draw(hMemDC, hMemDC1);
	}
}
void UCanvasPanel::Update(){
	for (int i = 0; i < static_cast<int>(Containers.size()); i++) {
		Containers[i]->Update();
	}
}
void UCanvasPanel::UpdateMouseHover(int MouseX, int MouseY) {
	bool bFoundTop = false;
	// Respect Z-order: Iterate from top-most to bottom-most
	for (int i = static_cast<int>(Containers.size()) - 1; i >= 0; i--) {
		if (!bFoundTop) {
			Containers[i]->UpdateMouseHover(MouseX, MouseY);
			// If we found a widget under mouse in this container, 
			// it should block hover for containers below it.
			// However, our current UpdateMouseHover handles full hierarchy.
			// For simplicity, let's just let everyone check, 
			// but we could optimize to block hover for lower containers.
			if (Containers[i]->FindWidgetUnderMouse(MouseX, MouseY)) {
				// bFoundTop = true; // Optional: block lower hover
			}
		}
		else {
			// Reset hover for lower containers
			Containers[i]->UpdateMouseHover(-9999, -9999);
		}
	}
}

UWidget* UCanvasPanel::FindWidgetUnderMouse(int MouseX, int MouseY) {
	for (int i = static_cast<int>(Containers.size()) - 1; i >= 0; i--) {
		UWidget* FindChild = Containers[i]->FindWidgetUnderMouse(MouseX, MouseY);
		if (FindChild != nullptr)
			return FindChild;
	}
	return nullptr;
}
bool UCanvasPanel::OnMouseEnter() {
	return true;
}
bool UCanvasPanel::OnMouseHover() {
	return true;
}
bool UCanvasPanel::OnMouseEnd() {
	return true;
}
void UCanvasPanel::AddChildWindow(UContainerWidget* content) {
	if (content) {
		Containers.push_back(content);
	}
}