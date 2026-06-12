#pragma once
#include "UContainerWidget.h"
#include "UButton.h"
#include "UCharacterInfoPopup.h"
#include "UInventoryPopup.h"
#include "UExitConfirmPopup.h"

class UMainMenuPopup : public UContainerWidget
{
private:
    UCharacterInfoPopup* charPopup;
    UInventoryPopup* invPopup;
    UExitConfirmPopup* exitPopup;

    UContainerWidget* navLayer;
    UButton* btnOpenChar;
    UButton* btnOpenInv;
    UButton* btnExit;

public:
    UMainMenuPopup();
    virtual ~UMainMenuPopup();

    virtual void Init() override;
    virtual void Draw(HDC hMemDC, HDC hMemDC1) override;
    virtual void Update() override;
    
    void Setup(HINSTANCE hInst, RECT clientRect);
    void SetTeamData(const std::vector<Character*>& team);
    
    void ShowNavigation();
    void HideNavigation();

    bool HandleBack(); // Returns true if it handled back navigation, false if it should close
};
