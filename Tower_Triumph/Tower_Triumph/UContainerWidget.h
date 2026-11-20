#pragma once
#include"Widget.h"
#include <vector>

class UContainerWidget : public UWidget
{
protected:
	std::vector<UWidget*> Contents;

public:
    UContainerWidget();
    virtual ~UContainerWidget();

	virtual void Init() override;
	virtual void Draw(HDC hMemDC, HDC hMemDC1) override;
	virtual void Update() override;
	virtual void CancelPress() override;
	virtual void UpdateMouseHover(int MouseX, int MouseY) override;
	virtual UWidget* FindWidgetUnderMouse(int MouseX, int MouseY) override;

	void AddChild(UWidget* NewChild);
    void RemoveChild(UWidget* Child, bool bDelete = true);
    void ClearChildren(bool bDelete = true);
};

