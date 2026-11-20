#include "USettingPopup.h"
#include "StarlightUI.h"

USettingPopup::USettingPopup()
    : btnClose(nullptr), btnSoundToggle(nullptr), bSoundOn(true), panelRect{}, clientBounds{}
{
    bVisible = false;
}

void USettingPopup::Init()
{
}

void USettingPopup::Setup(HINSTANCE, RECT clientRect)
{
    clientBounds = clientRect;
    panelRect = StarlightUI::Centered(clientRect, 480, 300);
    SetLocation((panelRect.left + panelRect.right) / 2, (panelRect.top + panelRect.bottom) / 2);
    SetSize(panelRect.right - panelRect.left, panelRect.bottom - panelRect.top);

    btnSoundToggle = new UButton();
    btnSoundToggle->SetLocation(Loc.x, panelRect.top + 142);
    btnSoundToggle->SetSize(Size.sX - 64, 54);
    btnSoundToggle->SetText(bSoundOn ? L"사운드: 켜짐" : L"사운드: 꺼짐");
    StarlightUI::StyleButton(btnSoundToggle);
    btnSoundToggle->SetSelected(bSoundOn);
    btnSoundToggle->SetOnClick([this]() {
        bSoundOn = !bSoundOn;
        btnSoundToggle->SetText(bSoundOn ? L"사운드: 켜짐" : L"사운드: 꺼짐");
        btnSoundToggle->SetSelected(bSoundOn);
    });
    AddChild(btnSoundToggle);

    btnClose = new UButton();
    btnClose->SetLocation(panelRect.right - 102, panelRect.bottom - 54);
    btnClose->SetSize(140, 44);
    btnClose->SetText(L"CLOSE");
    StarlightUI::StyleButton(btnClose);
    btnClose->SetOnClick([this]() { SetVisible(false); });
    AddChild(btnClose);
}

void USettingPopup::Draw(HDC hMemDC, HDC hMemDC1)
{
    if (!bVisible) return;
    StarlightUI::Dim(hMemDC, clientBounds);
    StarlightUI::Panel(hMemDC, panelRect, L"설정", L"- SETTINGS -");
    UContainerWidget::Draw(hMemDC, hMemDC1);
}
