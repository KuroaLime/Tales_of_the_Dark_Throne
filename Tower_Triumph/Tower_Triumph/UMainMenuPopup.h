#pragma once
#include "UContainerWidget.h"
#include "UButton.h"
#include "Character.h"
#include "UExitConfirmPopup.h"
#include <vector>

#include "TeamManager.h"
#include "AItemBase.h"

enum class EMenuTab {
    CharacterInfo,
    Inventory
};

enum class EInventoryState {
    Normal,
    ActionChoice,
    TargetSelect
};

class UMainMenuPopup : public UContainerWidget
{
private:
    EMenuTab currentTab;
    EInventoryState invState;

    std::vector<Character*> teamMembers;
    Character* selectedCharacter;
    AItemBase* selectedItem;

    // UI Components
    UButton* btnTabChar;
    UButton* btnTabInv;
    UButton* btnExit;
    UButton* btnClose;

    std::vector<UButton*> charSelectButtons;
    
    // Grid Inventory
    std::vector<UButton*> itemIconButtons;

    // Equipment Slots
    std::vector<UButton*> equipSlotButtons;
    
    // Floating Sub-menu (Square)
    UContainerWidget* itemActionPanel;
    UButton* btnItemAction1; // Equip / Use
    UButton* btnItemAction2; // Cancel
    
    // Target Selection
    UContainerWidget* targetSelectPanel;
    std::vector<UButton*> targetButtons;

    HBITMAP hBgBitmap; // Background image handle
    BITMAP bgInfo;

    UExitConfirmPopup* exitPopup;

    // Tooltip data
    bool bShowTooltip;
    std::wstring tooltipTitle;
    std::wstring tooltipDesc;
    Location tooltipLoc;

    // Helper for drawing
    void DrawCharacterInfo(HDC hMemDC);
    void DrawInventory(HDC hMemDC);
    void UpdateCharList();
    void UpdateInventoryGrid();
    void UpdateEquipmentSlots();
    void ShowActionMenu(AItemBase* item, int x, int y);
    void ShowTargetSelection();
    void HideSubMenus();
    void DrawTooltip(HDC hMemDC);

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
