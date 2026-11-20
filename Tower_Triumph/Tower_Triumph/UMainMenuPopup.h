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
#include <string>
#include <vector>

enum class EMenuTab { CharacterInfo, Inventory };
enum class EInventoryState { Normal, TargetSelect };
class UMainMenuPopup : public UContainerWidget
{
private:
    EMenuTab currentTab = EMenuTab::CharacterInfo;
    EInventoryState invState = EInventoryState::Normal;
    RECT panelBounds{}, detailBounds{};
    std::vector<Character*> teamMembers;
    Character* selectedCharacter = nullptr;
    int selectedItemID = -1, inspectedItemID = -1;
    UButton *btnOpenChar = nullptr, *btnOpenInv = nullptr, *btnExit = nullptr, *btnClose = nullptr;
    UButton *btnOpenCharDetail = nullptr, *btnOpenInvDetail = nullptr, *btnInspectAction = nullptr;
    UContainerWidget *navLayer = nullptr, *charLayer = nullptr, *invLayer = nullptr;
    UTextLabel *guideLabel = nullptr, *invTitleLabel = nullptr, *invGuideLabel = nullptr, *invEmptyLabel = nullptr;
    std::vector<UButton*> charSelectButtons, itemIconButtons;
    std::vector<int> itemButtonIDs;
    UContainerWidget* targetSelectPanel = nullptr;
    UProfilePanel* profilePanel = nullptr;
    UStatePanel* statePanel = nullptr;
    UExitConfirmPopup* exitPopup = nullptr;
    UCharacterInfoPopup* charPopup = nullptr;
    UInventoryPopup* invPopup = nullptr;
    void UpdateCharList();
    void UpdateInventoryGrid();
    void InspectItem(int itemID);
    void ShowActionMenu(int itemID, int x, int y);
    void ShowTargetSelection();
    void HideSubMenus();
    void RefreshFriendStylePanels();
    void CloseSubPopups();
    void DrawInventory(HDC hdc);
    UContainerWidget* ActiveModal() const;
    std::wstring MakeItemTooltip(int itemID) const;
    std::wstring MakeItemButtonText(int itemID, int count) const;
public:
    UMainMenuPopup();
    virtual ~UMainMenuPopup() = default;
    virtual void Init() override {}
    virtual void Draw(HDC hdc, HDC scratch) override;
    virtual void Update() override;
    virtual void UpdateMouseHover(int x, int y) override;
    virtual UWidget* FindWidgetUnderMouse(int x, int y) override;
    void Setup(HINSTANCE instance, RECT clientRect);
    void SetTeamData(const std::vector<Character*>& team);
    void SelectTab(EMenuTab tab);
    void ShowNavigation();
    void OpenCharacterPopup();
    void OpenInventoryPopup();
    bool HandleBack();
    void SelectCharacter(Character* character);
};
