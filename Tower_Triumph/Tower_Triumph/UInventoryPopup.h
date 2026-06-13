#pragma once

#include "UContainerWidget.h"
#include "UButton.h"
#include "GameManager.h"
#include "ItemData.h"
#include "Character.h"
#include "UTextLabel.h"

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

    UTextLabel* titleLabel;
    UTextLabel* goldLabel;
    UTextLabel* guideLabel;
    UTextLabel* emptyLabel;

    int selectedItemID;
    bool bShowTooltip;
    std::wstring tooltipTitle;
    std::wstring tooltipDesc;
    Location tooltipLoc;

private:
    void UpdateInventoryGrid();
    void ShowTargetSelection(int itemID);
    void HideTargetSelection();
    void DrawTargetPanelBackground(HDC hMemDC);
    void DrawTooltip(HDC hMemDC);

    std::wstring MakeItemButtonText(int itemID, int count) const;
    std::wstring MakeItemTooltip(int itemID) const;

public:
    std::function<void()> OnBackCallback;

public:
    UInventoryPopup();
    virtual ~UInventoryPopup();

    virtual void Init() override;
    virtual void Draw(HDC hMemDC, HDC hMemDC1) override;
    virtual void Update() override;

    void Setup(RECT clientRect);
    void SetTeamData(const std::vector<Character*>& team);
    void Refresh();
    void ShowTooltip(const std::wstring& title, const std::wstring& desc, Location loc);
    void HideTooltip();
};
