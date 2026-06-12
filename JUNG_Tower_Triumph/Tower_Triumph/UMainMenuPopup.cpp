#include "UMainMenuPopup.h"
#include "resource.h"

UMainMenuPopup::UMainMenuPopup() {
    bVisible = false;
    charPopup = nullptr;
    invPopup = nullptr;
    exitPopup = nullptr;
    navLayer = nullptr;
}

UMainMenuPopup::~UMainMenuPopup() {}

void UMainMenuPopup::Init() {}

void UMainMenuPopup::Setup(HINSTANCE hInst, RECT clientRect) {
    Size.sX = clientRect.right;
    Size.sY = clientRect.bottom;
    Loc.x = clientRect.right / 2;
    Loc.y = clientRect.bottom / 2;

    // 1. Navigation Layer (Visible on ESC)
    navLayer = new UContainerWidget();
    navLayer->SetSize(Size.sX, Size.sY);
    navLayer->SetLocation(Loc.x, Loc.y);
    AddChild(navLayer);

    btnOpenChar = new UButton();
    btnOpenChar->SetText(L"CHARACTER");
    btnOpenChar->SetFont(48, FW_BOLD, L"Gabriola");
    btnOpenChar->SetLocation(200, 100);
    btnOpenChar->SetSize(300, 80);
    btnOpenChar->SetOnClick([this]() {
        this->HideNavigation();
        this->charPopup->SetVisible(true);
    });
    navLayer->AddChild(btnOpenChar);

    btnOpenInv = new UButton();
    btnOpenInv->SetText(L"INVENTORY");
    btnOpenInv->SetFont(48, FW_BOLD, L"Gabriola");
    btnOpenInv->SetLocation(200, 200);
    btnOpenInv->SetSize(300, 80);
    btnOpenInv->SetOnClick([this]() {
        this->HideNavigation();
        this->invPopup->SetVisible(true);
    });
    navLayer->AddChild(btnOpenInv);

    btnExit = new UButton();
    btnExit->SetText(L"EXIT");
    btnExit->SetFont(48, FW_BOLD, L"Gabriola");
    btnExit->SetLocation(200, 300);
    btnExit->SetSize(300, 80);
    btnExit->SetOnClick([this]() {
        if (exitPopup) exitPopup->SetVisible(true);
    });
    navLayer->AddChild(btnExit);

    // 2. Sub-Popups
    charPopup = new UCharacterInfoPopup();
    charPopup->Setup(clientRect);
    charPopup->OnBackCallback = [this]() {
        this->charPopup->SetVisible(false);
        this->ShowNavigation();
    };
    AddChild(charPopup);

    invPopup = new UInventoryPopup();
    invPopup->Setup(clientRect);
    invPopup->OnBackCallback = [this]() {
        this->invPopup->SetVisible(false);
        this->ShowNavigation();
    };
    AddChild(invPopup);

    exitPopup = new UExitConfirmPopup();
    exitPopup->Setup(hInst, clientRect);
    exitPopup->SetOnYes([]() { PostQuitMessage(0); });
    exitPopup->SetOnNo([this]() { exitPopup->SetVisible(false); });
    AddChild(exitPopup);
}

void UMainMenuPopup::SetTeamData(const std::vector<Character*>& team) {
    if (charPopup) charPopup->SetTeamData(team);
}

void UMainMenuPopup::ShowNavigation() {
    navLayer->SetVisible(true);
}

void UMainMenuPopup::HideNavigation() {
    navLayer->SetVisible(false);
}

void UMainMenuPopup::Update() {
    if (!bVisible) return;
    UContainerWidget::Update();
}

bool UMainMenuPopup::HandleBack() {
    if (charPopup && charPopup->GetVisible()) {
        charPopup->SetVisible(false);
        ShowNavigation();
        return true;
    }
    else if (invPopup && invPopup->GetVisible()) {
        invPopup->SetVisible(false);
        ShowNavigation();
        return true;
    }
    else if (exitPopup && exitPopup->GetVisible()) {
        exitPopup->SetVisible(false);
        return true;
    }
    return false; // Already at navigation level, should close the whole menu
}

void UMainMenuPopup::Draw(HDC hMemDC, HDC hMemDC1) {
    if (!bVisible) return;
    
    // Darken background slightly when menu is open
    BLENDFUNCTION bf = { AC_SRC_OVER, 0, 150, 0 };
    HDC hTempDC = CreateCompatibleDC(hMemDC);
    HBITMAP hOld = (HBITMAP)SelectObject(hTempDC, CreateCompatibleBitmap(hMemDC, Size.sX, Size.sY));
    HBRUSH hB = CreateSolidBrush(RGB(0, 0, 0));
    RECT r = {0, 0, Size.sX, Size.sY};
    FillRect(hTempDC, &r, hB);
    AlphaBlend(hMemDC, 0, 0, Size.sX, Size.sY, hTempDC, 0, 0, Size.sX, Size.sY, bf);
    DeleteObject(SelectObject(hTempDC, hOld));
    DeleteObject(hB);
    DeleteDC(hTempDC);

    UContainerWidget::Draw(hMemDC, hMemDC1);
}
