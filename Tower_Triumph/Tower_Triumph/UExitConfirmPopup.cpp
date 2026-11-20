#include "UExitConfirmPopup.h"
#include "StarlightUI.h"

UExitConfirmPopup::UExitConfirmPopup()
    : btnYes(nullptr), btnNo(nullptr), panelRect{}, clientBounds{}
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
    StarlightUI::Dim(hMemDC, clientBounds, 140);
    StarlightUI::Panel(hMemDC, panelRect, L"종료");
    RECT messageRect = { panelRect.left + 32, panelRect.top + 108,
        panelRect.right - 32, panelRect.bottom - 100 };
    StarlightUI::Text(hMemDC, message, messageRect, 21);
    UContainerWidget::Draw(hMemDC, hMemDC1);
}

void UExitConfirmPopup::Setup(HINSTANCE, RECT clientRect)
{
    clientBounds = clientRect;
    panelRect = StarlightUI::Centered(clientRect, 500, 290);
    SetLocation((panelRect.left + panelRect.right) / 2, (panelRect.top + panelRect.bottom) / 2);
    SetSize(panelRect.right - panelRect.left, panelRect.bottom - panelRect.top);

    btnYes = new UButton();
    btnYes->SetText(L"네");
    btnYes->SetLocation(panelRect.right - 102, panelRect.bottom - 54);
    btnYes->SetSize(140, 44);
    StarlightUI::StyleButton(btnYes, true);
    AddChild(btnYes);

    btnNo = new UButton();
    btnNo->SetText(L"아니요");
    btnNo->SetLocation(panelRect.right - 254, panelRect.bottom - 54);
    btnNo->SetSize(140, 44);
    StarlightUI::StyleButton(btnNo);
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
