#include "UExitConfirmPopup.h"

UExitConfirmPopup::UExitConfirmPopup()
    : btnYes(nullptr), btnNo(nullptr)
{
    bVisible = false;
    message = L"정말 게임을 종료하시겠습니까?";
}

UExitConfirmPopup::~UExitConfirmPopup()
{
}

void UExitConfirmPopup::Init()
{
}

void UExitConfirmPopup::Draw(HDC hMemDC, HDC hMemDC1)
{
    if (!bVisible) return;

    int boxW = 420;
    int boxH = 210;
    int startX = Loc.x - boxW / 2;
    int startY = Loc.y - boxH / 2;

    RECT shadow = { startX + 4, startY + 4, startX + boxW + 4, startY + boxH + 4 };
    HBRUSH shadowBrush = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(hMemDC, &shadow, shadowBrush);
    DeleteObject(shadowBrush);

    RECT rect = { startX, startY, startX + boxW, startY + boxH };
    HBRUSH bgBrush = CreateSolidBrush(RGB(19, 23, 31));
    HPEN borderPen = CreatePen(PS_SOLID, 3, RGB(176, 146, 78));
    HBRUSH oldBrush = (HBRUSH)SelectObject(hMemDC, bgBrush);
    HPEN oldPen = (HPEN)SelectObject(hMemDC, borderPen);
    RoundRect(hMemDC, rect.left, rect.top, rect.right, rect.bottom, 12, 12);
    SelectObject(hMemDC, oldBrush);
    SelectObject(hMemDC, oldPen);
    DeleteObject(bgBrush);
    DeleteObject(borderPen);

    RECT titleBar = { startX + 18, startY + 16, startX + boxW - 18, startY + 50 };
    HBRUSH titleBrush = CreateSolidBrush(RGB(48, 38, 24));
    FillRect(hMemDC, &titleBar, titleBrush);
    DeleteObject(titleBrush);

    HFONT hFont = CreateFontW(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, L"Malgun Gothic");
    HFONT hOldFont = (HFONT)SelectObject(hMemDC, hFont);
    SetTextColor(hMemDC, RGB(245, 220, 150));
    SetBkMode(hMemDC, TRANSPARENT);
    DrawTextW(hMemDC, L"종료", -1, &titleBar, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hMemDC, hOldFont);
    DeleteObject(hFont);

    hFont = CreateFontW(21, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, L"Malgun Gothic");
    hOldFont = (HFONT)SelectObject(hMemDC, hFont);
    SetTextColor(hMemDC, RGB(238, 232, 214));
    RECT textRect = { startX + 20, startY + 70, startX + boxW - 20, startY + 125 };
    DrawTextW(hMemDC, message.c_str(), -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hMemDC, hOldFont);
    DeleteObject(hFont);

    UContainerWidget::Draw(hMemDC, hMemDC1);
}

void UExitConfirmPopup::Setup(HINSTANCE hInst, RECT clientRect)
{
    Loc.x = (clientRect.right - clientRect.left) / 2;
    Loc.y = (clientRect.bottom - clientRect.top) / 2;

    int btnW = 100;
    int btnH = 40;
    int spacing = 40;

    btnYes = new UButton();
    btnYes->SetText(L"예");
    btnYes->SetLocation(Loc.x - btnW / 2 - spacing / 2, Loc.y + 58);
    btnYes->SetSize(btnW, btnH);
    AddChild(btnYes);

    btnNo = new UButton();
    btnNo->SetText(L"아니오");
    btnNo->SetLocation(Loc.x + btnW / 2 + spacing / 2, Loc.y + 58);
    btnNo->SetSize(btnW, btnH);
    AddChild(btnNo);
}

void UExitConfirmPopup::SetOnYes(std::function<void()> callback)
{
    if (btnYes) btnYes->SetOnClick(callback);
}

void UExitConfirmPopup::SetOnNo(std::function<void()> callback)
{
    if (btnNo) btnNo->SetOnClick(callback);
}
