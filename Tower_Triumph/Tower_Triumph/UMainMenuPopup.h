#pragma once

#include "UContainerWidget.h"
#include "UButton.h"
#include "Character.h"
#include "UExitConfirmPopup.h"
#include "GameManager.h"
#include "ItemData.h"

#include <vector>
#include <string>

class UMainMenuPopup;

enum class EMenuTab {
    CharacterInfo,
    Inventory
};

enum class EInventoryState {
    Normal,
    TargetSelect
};

class UMainMenuPopup : public UContainerWidget
{
private:
    EMenuTab currentTab;
    EInventoryState invState;

    std::vector<Character*> teamMembers;
    Character* selectedCharacter;
    int selectedItemID;

    UButton* btnTabChar;
    UButton* btnTabInv;
    UButton* btnExit;
    UButton* btnClose;

    std::vector<UButton*> charSelectButtons;
    std::vector<UButton*> itemIconButtons;
    std::vector<UButton*> equipSlotButtons;

    UContainerWidget* itemActionPanel;
    UButton* btnItemAction1;
    UButton* btnItemAction2;

    UContainerWidget* targetSelectPanel;
    std::vector<UButton*> targetButtons;

    HBITMAP hBgBitmap;
    BITMAP bgInfo;

    UExitConfirmPopup* exitPopup;

    bool bShowTooltip;
    std::wstring tooltipTitle;
    std::wstring tooltipDesc;
    Location tooltipLoc;

private:
    void DrawCharacterInfo(HDC hMemDC);
    void DrawInventory(HDC hMemDC);

    void UpdateCharList();
    void UpdateInventoryGrid();
    void UpdateEquipmentSlots();

    void ShowActionMenu(int itemID, int x, int y);
    void ShowTargetSelection();
    void HideSubMenus();

    void DrawSubMenuBackgrounds(HDC hMemDC);
    void DrawTooltip(HDC hMemDC);

    std::wstring GetEquipName(int itemID) const;
    std::wstring MakeItemTooltip(int itemID) const;
    std::wstring MakeItemButtonText(int itemID, int count) const;

public:
    UMainMenuPopup();
    virtual ~UMainMenuPopup();

    virtual void Init() override;
    virtual void Draw(HDC hMemDC, HDC hMemDC1) override;
    virtual void Update() override;

    void Setup(HINSTANCE hInst, RECT clientRect);
    void SetBackgroundImage(HBITMAP hBitmap);
    void SetTeamData(const std::vector<Character*>& team);

    void SelectTab(EMenuTab tab);
    void SelectCharacter(Character* character);
    void ShowTooltip(const std::wstring& title, const std::wstring& desc, Location loc);
    void HideTooltip();
};
