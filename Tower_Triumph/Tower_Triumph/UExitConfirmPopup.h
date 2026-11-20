#pragma once
#include "UContainerWidget.h"
#include "UButton.h"
#include <string>

class UExitConfirmPopup : public UContainerWidget
{
private:
    UButton* btnYes;
    UButton* btnNo;
    std::wstring message;
    RECT panelRect;
    RECT clientBounds;

public:
    UExitConfirmPopup();
    virtual ~UExitConfirmPopup();
    virtual void Init() override;
    virtual void Draw(HDC hMemDC, HDC hMemDC1) override;
    
    void Setup(HINSTANCE hInst, RECT clientRect);
    void SetOnYes(std::function<void()> callback);
    void SetOnNo(std::function<void()> callback);
};
