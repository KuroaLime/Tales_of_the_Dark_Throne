#pragma once
#include "UContainerWidget.h"
#include "UButton.h"
#include <string>

class UNoticePopup : public UContainerWidget
{
private:
    std::wstring TitleText;
    std::wstring MessageText;
    UButton* btnOK;

    int ClientWidth;
    int ClientHeight;
    int PanelWidth;
    int PanelHeight;

public:
    UNoticePopup()
        : btnOK(nullptr), ClientWidth(1280), ClientHeight(720), PanelWidth(440), PanelHeight(220)
    {
        bVisible = false;
    }

    virtual ~UNoticePopup()
    {
    }

    void Setup(const RECT& clientRect)
    {
        ClientWidth = clientRect.right - clientRect.left;
        ClientHeight = clientRect.bottom - clientRect.top;

        SetLocation(ClientWidth / 2, ClientHeight / 2);
        SetSize(ClientWidth, ClientHeight);

        btnOK = new UButton();
        btnOK->SetText(L"확인");
        btnOK->SetSize(112, 40);
        btnOK->SetLocation(ClientWidth / 2, ClientHeight / 2 + 68);
        btnOK->SetFont(18, FW_BOLD, L"Malgun Gothic");
        btnOK->SetAnimationType(0);
        btnOK->SetDrawTextFallbackBackground(true);
        btnOK->SetOnClick([this]() { this->CloseNotice(); });
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
        if (btnOK)
        {
            btnOK->SetIsPressed(false);
        }
    }

    bool IsOpened() const
    {
        return bVisible;
    }

    virtual void Draw(HDC hMemDC, HDC hMemDC1) override
    {
        if (!bVisible)
        {
            return;
        }

        // 전체 화면을 어둡게 덮어서 일반 MessageBox 대신 게임 내부 모달처럼 보이게 한다.
        RECT overlay = { 0, 0, ClientWidth, ClientHeight };
        HBRUSH overlayBrush = CreateSolidBrush(RGB(4, 4, 7));
        FillRect(hMemDC, &overlay, overlayBrush);
        DeleteObject(overlayBrush);

        int panelLeft = ClientWidth / 2 - PanelWidth / 2;
        int panelTop = ClientHeight / 2 - PanelHeight / 2;
        RECT shadowRect = { panelLeft + 4, panelTop + 4, panelLeft + PanelWidth + 4, panelTop + PanelHeight + 4 };
        HBRUSH shadowBrush = CreateSolidBrush(RGB(0, 0, 0));
        FillRect(hMemDC, &shadowRect, shadowBrush);
        DeleteObject(shadowBrush);

        RECT panelRect = { panelLeft, panelTop, panelLeft + PanelWidth, panelTop + PanelHeight };
        HBRUSH panelBrush = CreateSolidBrush(RGB(19, 23, 31));
        HPEN panelPen = CreatePen(PS_SOLID, 3, RGB(176, 146, 78));
        HBRUSH oldBrush = (HBRUSH)SelectObject(hMemDC, panelBrush);
        HPEN oldPen = (HPEN)SelectObject(hMemDC, panelPen);
        RoundRect(hMemDC, panelRect.left, panelRect.top, panelRect.right, panelRect.bottom, 12, 12);
        SelectObject(hMemDC, oldBrush);
        SelectObject(hMemDC, oldPen);
        DeleteObject(panelBrush);
        DeleteObject(panelPen);

        RECT titleRect = { panelRect.left + 14, panelRect.top + 12, panelRect.right - 14, panelRect.top + 48 };
        HBRUSH titleBrush = CreateSolidBrush(RGB(48, 38, 24));
        FillRect(hMemDC, &titleRect, titleBrush);
        DeleteObject(titleBrush);

        int oldBkMode = SetBkMode(hMemDC, TRANSPARENT);
        COLORREF oldTextColor = SetTextColor(hMemDC, RGB(245, 220, 150));

        HFONT titleFont = CreateFontW(
            22, 0, 0, 0, FW_BOLD,
            FALSE, FALSE, FALSE,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            L"Malgun Gothic"
        );
        HFONT oldFont = (HFONT)SelectObject(hMemDC, titleFont);

        RECT titleTextRect = titleRect;
        OffsetRect(&titleTextRect, 0, 2);
        DrawTextW(hMemDC, TitleText.c_str(), -1, &titleTextRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hMemDC, oldFont);
        DeleteObject(titleFont);

        HFONT bodyFont = CreateFontW(
            20, 0, 0, 0, FW_BOLD,
            FALSE, FALSE, FALSE,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            L"Malgun Gothic"
        );
        oldFont = (HFONT)SelectObject(hMemDC, bodyFont);
        SetTextColor(hMemDC, RGB(238, 232, 214));

        RECT bodyRect = { panelRect.left + 36, panelRect.top + 72, panelRect.right - 36, panelRect.bottom - 76 };
        DrawTextW(hMemDC, MessageText.c_str(), -1, &bodyRect, DT_CENTER | DT_VCENTER | DT_WORDBREAK);

        SelectObject(hMemDC, oldFont);
        DeleteObject(bodyFont);

        SetTextColor(hMemDC, oldTextColor);
        SetBkMode(hMemDC, oldBkMode);

        UContainerWidget::Draw(hMemDC, hMemDC1);
    }
};
