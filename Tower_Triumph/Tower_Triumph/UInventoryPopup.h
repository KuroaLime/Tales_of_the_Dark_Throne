#pragma once
#include "UContainerWidget.h"
#include "UButton.h"
#include "GameManager.h"
#include "ItemData.h"
#include "Character.h"
#include <functional>
#include <string>
#include <vector>

class UInventoryPopup : public UContainerWidget
{
private:
    std::vector<Character*> teamMembers;
    std::vector<UButton*> itemIconButtons;
    std::vector<UButton*> targetButtons;
    UContainerWidget* targetPanel;
    UButton* backButton;
    UButton* previousPageButton;
    UButton* nextPageButton;
    RECT clientBounds;
    RECT panelBounds;
    RECT listBounds;
    RECT detailBounds;
    RECT targetBounds;
    int selectedItemID;
    int detailItemID;
    int inventoryPage;
    bool refreshPending;
    std::wstring tooltipTitle;
    std::wstring tooltipDesc;

    void UpdateInventoryGrid();
    void ShowTargetSelection(int itemID);
    void HideTargetSelection();
    void SelectDetails(int itemID);
    void DrawDetails(HDC hMemDC);
    std::wstring MakeItemButtonText(int itemID, int count) const;
    std::wstring MakeItemTooltip(int itemID) const;
public:
    std::function<void()> OnBackCallback;
    UInventoryPopup();
    virtual ~UInventoryPopup();
    void Init() override;
    void Draw(HDC hMemDC, HDC hMemDC1) override;
    void Update() override;
    void UpdateMouseHover(int mouseX, int mouseY) override;
    UWidget* FindWidgetUnderMouse(int mouseX, int mouseY) override;
    void Setup(RECT clientRect);
    void SetTeamData(const std::vector<Character*>& team);
    void Refresh();
    // Returns true only when a target selection was dismissed.
    bool HandleBack();
    void ShowTooltip(const std::wstring& title, const std::wstring& desc, Location loc);
    void HideTooltip();
};
