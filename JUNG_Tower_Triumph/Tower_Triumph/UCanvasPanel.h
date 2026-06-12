#pragma once
#include "Widget.h"
#include<vector>

class UCanvasPanel : public UWidget
{
protected:
	std::vector<class UContainerWidget*> Containers;
public:
	virtual void Init() override;
	virtual void Draw(HDC hMemDC, HDC hMemDC1) override;
	virtual void Update() override;
	virtual void UpdateMouseHover(int MouseX, int MouseY) override;
	virtual UWidget* FindWidgetUnderMouse(int MouseX, int MouseY) override;
	
	bool OnMouseEnter();
	bool OnMouseHover();
	bool OnMouseEnd();

	void AddChildWindow(UContainerWidget* content);
};

