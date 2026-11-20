#pragma once
#include "UContainerWidget.h"
#include "UButton.h"
#include "StarlightUI.h"
#include <string>

class UNoticePopup : public UContainerWidget
{
private:
    std::wstring TitleText;
    std::wstring MessageText;
    UButton* btnOK;
    RECT clientRect;
    RECT panelRect;

public:
    UNoticePopup()
        : btnOK(nullptr), clientRect{ 0, 0, 1280, 720 }, panelRect{ 370, 210, 910, 510 }
    {
        bVisible = false;
    }

    virtual ~UNoticePopup()
    {
    }

    void Setup(const RECT& bounds)
    {
        clientRect = bounds;
        panelRect = StarlightUI::Centered(clientRect, 540, 300);
        SetLocation((clientRect.left + clientRect.right) / 2, (clientRect.top + clientRect.bottom) / 2);
        // Preserve the existing full-client input shield while the notice is open.
        SetSize(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);

        btnOK = new UButton();
        btnOK->SetText(L"확인");
        btnOK->SetSize(140, 44);
        btnOK->SetLocation(panelRect.right - 102, panelRect.bottom - 54);
        StarlightUI::StyleButton(btnOK, true);
        btnOK->SetOnClick([this]() { CloseNotice(); });
        AddChild(btnOK);
        SetVisible(false);
    }

    void ShowNotice(const std::wstring& title, const std::wstring& message)
    {
        TitleText = title;
        MessageText = message;
        SetVisible(true);
    }

    void CloseNotice()
    {
        SetVisible(false);
        if (btnOK) btnOK->SetIsPressed(false);
    }

    bool IsOpened() const
    {
        return bVisible;
    }

    virtual void Draw(HDC hMemDC, HDC hMemDC1) override
    {
        if (!bVisible) return;
        StarlightUI::Dim(hMemDC, clientRect, 140);
        StarlightUI::Panel(hMemDC, panelRect, TitleText);
        RECT messageRect = { panelRect.left + 32, panelRect.top + 108,
            panelRect.right - 32, panelRect.bottom - 100 };
        StarlightUI::Text(hMemDC, MessageText, messageRect, 20);
        UContainerWidget::Draw(hMemDC, hMemDC1);
    }
};
