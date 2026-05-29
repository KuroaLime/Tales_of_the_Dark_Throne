#include "UCanvasPanel.h"
#include "UContainerWidget.h"



void UCanvasPanel::Init(){

}
void UCanvasPanel::Draw(HDC hMemDC, HDC hMemDC1){
	for (int i = 0; i < Containers.size(); i++) {
		Containers[i]->Draw(hMemDC, hMemDC1);
	}
}
void UCanvasPanel::Update(){
	for (int i = 0; i < Containers.size(); i++) {
		Containers[i]->Update();
	}
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
void UCanvasPanel::AddChildWindow(UContainerWidget content) {
	Containers.push_back(&content);
}