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
    std::vector<Character*> TeamMembers;
    std::vector<UButton*> ItemButtons;
    std::vector<UButton*> TargetButtons;

    UContainerWidget* TargetPanel;
    UButton* BackButton;

    int SelectedItemID;
    bool bShowTooltip;
    std::wstring TooltipTitle;
    std::wstring TooltipDesc;
    Location TooltipLoc;

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
