#pragma once

#include "UContainerWidget.h"
#include "UButton.h"
#include "Character.h"
#include "UExitConfirmPopup.h"
#include "GameManager.h"
#include "ItemData.h"
#include "UProfilePanel.h"
#include "UStatePanel.h"
#include "UCharacterInfoPopup.h"
#include "UInventoryPopup.h"
#include "UTextLabel.h"

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

    UButton* btnOpenChar;
    UButton* btnOpenInv;
    UButton* btnExit;
    UButton* btnClose;
    UButton* btnOpenCharDetail;
    UButton* btnOpenInvDetail;

    // 친구 코드처럼 화면 단위를 레이어로 나눠 SetVisible로 전환한다.
    UContainerWidget* navLayer;
    UContainerWidget* charLayer;
    UContainerWidget* invLayer;

    UTextLabel* guideLabel;
    UTextLabel* invTitleLabel;
    UTextLabel* invGuideLabel;
    UTextLabel* invEmptyLabel;

    std::vector<UButton*> charSelectButtons;
    std::vector<UButton*> itemIconButtons;
    std::vector<UButton*> equipSlotButtons;

    UContainerWidget* itemActionPanel;
    UButton* btnItemAction1;
    UButton* btnItemAction2;

    UContainerWidget* targetSelectPanel;
    std::vector<UButton*> targetButtons;

    UProfilePanel* profilePanel;
    UStatePanel* statePanel;

    HBITMAP hBgBitmap;
    BITMAP bgInfo;

    UExitConfirmPopup* exitPopup;
    UCharacterInfoPopup* charPopup;
    UInventoryPopup* invPopup;

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
    void RefreshFriendStylePanels();
    void CloseSubPopups();

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
    void ShowNavigation();
    void OpenCharacterPopup();
    void OpenInventoryPopup();
    bool HandleBack();
    void SelectCharacter(Character* character);
    void ShowTooltip(const std::wstring& title, const std::wstring& desc, Location loc);
    void HideTooltip();
};
