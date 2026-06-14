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

    int boxW = 400;
    int boxH = 200;
    int startX = Loc.x - boxW / 2;
    int startY = Loc.y - boxH / 2;

    HBRUSH hBrush = CreateSolidBrush(RGB(50, 50, 50));
    RECT rect = { startX, startY, startX + boxW, startY + boxH };
    FillRect(hMemDC, &rect, hBrush);
    DeleteObject(hBrush);

    HPEN hPen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
    HPEN hOldPen = (HPEN)SelectObject(hMemDC, hPen);
    MoveToEx(hMemDC, startX, startY, NULL);
    LineTo(hMemDC, startX + boxW, startY);
    LineTo(hMemDC, startX + boxW, startY + boxH);
    LineTo(hMemDC, startX, startY + boxH);
    LineTo(hMemDC, startX, startY);
    SelectObject(hMemDC, hOldPen);
    DeleteObject(hPen);

    HFONT hFont = CreateFont(24, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, L"Arial");
    HFONT hOldFont = (HFONT)SelectObject(hMemDC, hFont);
    SetTextColor(hMemDC, RGB(255, 255, 255));
    SetBkMode(hMemDC, TRANSPARENT);
    
    RECT textRect = { startX, startY + 40, startX + boxW, startY + 100 };
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
    btnYes->SetText(L"네");
    btnYes->SetLocation(Loc.x - btnW - spacing / 2, Loc.y + 40);
    btnYes->SetSize(btnW, btnH);
    AddChild(btnYes);

    btnNo = new UButton();
    btnNo->SetText(L"아니요");
    btnNo->SetLocation(Loc.x + spacing / 2, Loc.y + 40);
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
