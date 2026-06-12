#pragma once
#include "UContainerWidget.h"
#include "UButton.h"
#include <vector>
#include <string>
#include <functional>

class UInventoryPopup : public UContainerWidget
{
private:
    std::vector<UButton*> itemIconButtons;
    
    // Tooltip data
    bool bShowTooltip;
    std::wstring tooltipTitle;
    std::wstring tooltipDesc;
    Location tooltipLoc;

    void UpdateInventoryGrid();
    void DrawTooltip(HDC hMemDC);

public:
    UInventoryPopup();
    virtual ~UInventoryPopup();

    virtual void Init() override;
    virtual void Draw(HDC hMemDC, HDC hMemDC1) override;
    virtual void Update() override;

    void Setup(RECT clientRect);
    void SetTeamData(const std::vector<class Character*>& team); // Compatibility
    void Refresh();
    void ShowTooltip(const std::wstring& title, const std::wstring& desc, Location loc);
    void HideTooltip();

    std::function<void()> OnBackCallback;
};
