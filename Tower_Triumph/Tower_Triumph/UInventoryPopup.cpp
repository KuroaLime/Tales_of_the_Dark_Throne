#include "UInventoryPopup.h"
#include "StarlightUI.h"
#include <algorithm>

namespace
{
    constexpr int ItemsPerPage = 10;
    std::wstring SignedBonus(int value)
    {
        return (value >= 0 ? L"+" : L"") + std::to_wstring(value);
    }
}

UInventoryPopup::UInventoryPopup()
    : targetPanel(nullptr), backButton(nullptr), previousPageButton(nullptr), nextPageButton(nullptr),
      clientBounds{}, panelBounds{}, listBounds{}, detailBounds{}, targetBounds{},
      selectedItemID(-1), detailItemID(-1), inventoryPage(0), refreshPending(false)
{
    bVisible = false;
}
UInventoryPopup::~UInventoryPopup() {}
void UInventoryPopup::Init() {}

void UInventoryPopup::Setup(RECT clientRect)
{
    ClearChildren(true);
    itemIconButtons.clear();
    targetButtons.clear();
    clientBounds = clientRect;
    panelBounds = StarlightUI::Centered(clientRect, 1000, 620);
    SetSize(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);
    SetLocation((clientRect.left + clientRect.right) / 2, (clientRect.top + clientRect.bottom) / 2);
    const int left = panelBounds.left, top = panelBounds.top;
    listBounds = { left + 28, top + 160, left + 492, top + 430 };
    detailBounds = { left + 510, top + 160, panelBounds.right - 28, top + 430 };
    targetBounds = { left + 28, top + 446, panelBounds.right - 28, panelBounds.bottom - 24 };

    backButton = new UButton();
    backButton->SetText(L"뒤로");
    backButton->SetSize(100, 38);
    backButton->SetLocation(panelBounds.right - 78, top + 52);
    StarlightUI::StyleButton(backButton);
    backButton->SetOnClick([this]() {
        HideTargetSelection();
        if (OnBackCallback) OnBackCallback();
        else SetVisible(false);
    });
    AddChild(backButton);
    previousPageButton = new UButton();
    previousPageButton->SetText(L"이전");
    previousPageButton->SetSize(70, 24);
    previousPageButton->SetLocation(listBounds.left + 47, listBounds.bottom - 17);
    StarlightUI::StyleButton(previousPageButton, false, 14);
    previousPageButton->SetOnClick([this]() {
        if (selectedItemID < 0 && inventoryPage > 0) { --inventoryPage; refreshPending = true; }
    });
    AddChild(previousPageButton);
    nextPageButton = new UButton();
    nextPageButton->SetText(L"다음");
    nextPageButton->SetSize(70, 24);
    nextPageButton->SetLocation(listBounds.right - 47, listBounds.bottom - 17);
    StarlightUI::StyleButton(nextPageButton, false, 14);
    nextPageButton->SetOnClick([this]() {
        const int pages = (int(GameManager::Get().GetInventory().size()) + ItemsPerPage - 1) / ItemsPerPage;
        if (selectedItemID < 0 && inventoryPage + 1 < pages) { ++inventoryPage; refreshPending = true; }
    });
    AddChild(nextPageButton);
    targetPanel = new UContainerWidget();
    targetPanel->SetLocation((targetBounds.left + targetBounds.right) / 2, (targetBounds.top + targetBounds.bottom) / 2);
    targetPanel->SetSize(targetBounds.right - targetBounds.left, targetBounds.bottom - targetBounds.top);
    targetPanel->SetVisible(false);
    AddChild(targetPanel);
    selectedItemID = -1;
    refreshPending = false;
    UpdateInventoryGrid();
}

void UInventoryPopup::SetTeamData(const std::vector<Character*>& team)
{
    HideTargetSelection();
    teamMembers = team;
}

void UInventoryPopup::Refresh()
{
    HideTargetSelection();
    // Rebuild outside button callbacks so their owning widgets stay alive until Click returns.
    refreshPending = true;
}

void UInventoryPopup::UpdateInventoryGrid()
{
    for (UButton* button : itemIconButtons) RemoveChild(button, true);
    itemIconButtons.clear();
    const auto& inventory = GameManager::Get().GetInventory();
    const int pages = (std::max)(1, (int(inventory.size()) + ItemsPerPage - 1) / ItemsPerPage);
    inventoryPage = (std::max)(0, (std::min)(inventoryPage, pages - 1));
    bool detailExists = false;
    for (const InventoryItem& item : inventory) if (item.ItemID == detailItemID) detailExists = true;
    if (!detailExists) detailItemID = inventory.empty() ? -1 : inventory.front().ItemID;
    SelectDetails(detailItemID);
    const int start = inventoryPage * ItemsPerPage;
    const int end = (std::min)(int(inventory.size()), start + ItemsPerPage);
    const int width = (listBounds.right - listBounds.left - 36) / 2;
    for (int i = start; i < end; ++i)
    {
        const int itemID = inventory[i].ItemID;
        UButton* button = new UButton();
        const int position = i - start;
        button->SetText(MakeItemButtonText(itemID, inventory[i].Count));
        button->SetSize(width, 40);
        button->SetLocation(listBounds.left + 12 + width / 2 + (position % 2) * (width + 12),
            listBounds.top + 30 + (position / 2) * 44);
        StarlightUI::StyleButton(button, false, 16);
        button->SetSelected(itemID == detailItemID);
        button->SetOnHover([this, itemID]() { if (selectedItemID < 0) SelectDetails(itemID); });
        button->SetOnClick([this, itemID]() {
            if (selectedItemID >= 0) return;
            SelectDetails(itemID);
            if (ItemData::IsEquipment(itemID) || ItemData::IsConsumable(itemID)) ShowTargetSelection(itemID);
        });
        AddChild(button);
        itemIconButtons.push_back(button);
    }
    previousPageButton->SetVisible(pages > 1);
    nextPageButton->SetVisible(pages > 1);
    previousPageButton->SetEnabled(inventoryPage > 0);
    nextPageButton->SetEnabled(inventoryPage + 1 < pages);
}

void UInventoryPopup::SelectDetails(int itemID)
{
    detailItemID = itemID;
    tooltipTitle = itemID >= 0 ? ItemData::GetName(itemID) : L"아이템 정보";
    tooltipDesc = itemID >= 0 ? ItemData::GetDescription(itemID) : L"아이템을 얻으면 이곳에서 설명과 효과를 확인할 수 있습니다.";
    const auto& inventory = GameManager::Get().GetInventory();
    const int start = inventoryPage * ItemsPerPage;
    for (size_t i = 0; i < itemIconButtons.size(); ++i)
        itemIconButtons[i]->SetSelected(start + int(i) < int(inventory.size()) && inventory[start + i].ItemID == itemID);
}

void UInventoryPopup::ShowTargetSelection(int itemID)
{
    if (!targetPanel) return;
    selectedItemID = itemID;
    SelectDetails(itemID);
    targetPanel->ClearChildren(true);
    targetButtons.clear();
    const int columns = 3;
    const int width = (targetBounds.right - targetBounds.left - 48) / columns;
    for (size_t i = 0; i < teamMembers.size() + 1; ++i)
    {
        const bool cancel = i == teamMembers.size();
        Character* target = cancel ? nullptr : teamMembers[i];
        UButton* button = new UButton();
        button->SetText(cancel ? L"취소" : (target ? target->GetName() : L"None"));
        button->SetSize(width, 38);
        button->SetLocation(targetBounds.left + 12 + width / 2 + int(i % columns) * (width + 12),
            targetBounds.top + 65 + int(i / columns) * 46);
        StarlightUI::StyleButton(button, !cancel, 16);
        button->SetOnClick([this, target, cancel]() {
            if (cancel || !target || selectedItemID < 0) { HideTargetSelection(); return; }
            if (ItemData::IsEquipment(selectedItemID)) GameManager::Get().EquipItem(target, selectedItemID);
            else if (ItemData::IsConsumable(selectedItemID)) GameManager::Get().UseConsumable(target, selectedItemID);
            Refresh();
        });
        targetPanel->AddChild(button);
        targetButtons.push_back(button);
    }
    targetPanel->SetVisible(true);
}

void UInventoryPopup::HideTargetSelection()
{
    selectedItemID = -1;
    // Keep the children alive while their click callback is on the stack.
    if (targetPanel) targetPanel->SetVisible(false);
    for (UButton* button : targetButtons) { button->SetIsPointed(false); button->SetIsPressed(false); }
}

bool UInventoryPopup::HandleBack()
{
    if (!targetPanel || !targetPanel->GetVisible()) return false;
    HideTargetSelection();
    return true;
}

void UInventoryPopup::Update()
{
    if (!bVisible) return;
    if (refreshPending) { refreshPending = false; UpdateInventoryGrid(); }
    UContainerWidget::Update();
}

void UInventoryPopup::UpdateMouseHover(int mouseX, int mouseY)
{
    const bool choosing = bVisible && targetPanel && targetPanel->GetVisible();
    for (UWidget* child : Contents)
    {
        const bool active = bVisible && child->GetVisible() && (!choosing || child == targetPanel || child == backButton);
        child->UpdateMouseHover(active ? mouseX : -10000, active ? mouseY : -10000);
    }
    IsPointed = bVisible && ContainsPoint(mouseX, mouseY);
}

UWidget* UInventoryPopup::FindWidgetUnderMouse(int mouseX, int mouseY)
{
    if (!bVisible) return nullptr;
    if (targetPanel && targetPanel->GetVisible())
    {
        if (backButton->ContainsPoint(mouseX, mouseY)) return backButton;
        UWidget* target = targetPanel->FindWidgetUnderMouse(mouseX, mouseY);
        return target ? target : this;
    }
    UWidget* widget = UContainerWidget::FindWidgetUnderMouse(mouseX, mouseY);
    return widget ? widget : this;
}

void UInventoryPopup::DrawDetails(HDC hMemDC)
{
    StarlightUI::Surface(hMemDC, detailBounds);
    StarlightUI::Text(hMemDC, tooltipTitle, { detailBounds.left + 18, detailBounds.top + 14,
        detailBounds.right - 18, detailBounds.top + 43 }, 21, StarlightUI::Gold);
    StarlightUI::Text(hMemDC, tooltipDesc, { detailBounds.left + 18, detailBounds.top + 52,
        detailBounds.right - 18, detailBounds.top + 128 }, 17, StarlightUI::Ink);
    if (detailItemID < 0) return;
    const bool equipment = ItemData::IsEquipment(detailItemID);
    const bool consumable = ItemData::IsConsumable(detailItemID);
    if (!equipment && !consumable) return;
    const SItemStatBonus bonus = ItemData::GetBonus(detailItemID);
    const wchar_t* names[] = { L"HP", L"MP", L"STR", L"INT", L"DEF", L"SPD" };
    const int values[] = { bonus.MaxHP, bonus.MaxMP, bonus.STR, bonus.INT, bonus.DEF, bonus.SPD };
    const int columns = equipment ? 3 : 2;
    const int count = equipment ? 6 : 2;
    const int width = (detailBounds.right - detailBounds.left - 36 - (columns - 1) * 10) / columns;
    for (int i = 0; i < count; ++i)
    {
        const int x = detailBounds.left + 18 + (i % columns) * (width + 10);
        const int y = detailBounds.top + 140 + (i / columns) * 56;
        StarlightUI::Surface(hMemDC, { x, y, x + width, y + 48 });
        const std::wstring label = equipment ? names[i] : (i == 0 ? L"Heal HP" : L"Heal MP");
        const std::wstring value = equipment ? SignedBonus(values[i]) :
            std::to_wstring(i == 0 ? ItemData::GetHealHP(detailItemID) : ItemData::GetHealMP(detailItemID));
        StarlightUI::Text(hMemDC, label, { x + 9, y + 4, x + width - 9, y + 23 }, 14, StarlightUI::Muted);
        StarlightUI::Text(hMemDC, value, { x + 9, y + 23, x + width - 9, y + 46 }, 19, StarlightUI::Ink);
    }
}

void UInventoryPopup::Draw(HDC hMemDC, HDC hMemDC1)
{
    if (!bVisible) return;
    StarlightUI::Dim(hMemDC, clientBounds);
    StarlightUI::Panel(hMemDC, panelBounds, L"인벤토리", L"여행자의 소지품");
    StarlightUI::Text(hMemDC, L"골드: " + std::to_wstring(GameManager::Get().GetGold()),
        { panelBounds.left + 28, panelBounds.top + 103, panelBounds.right - 28, panelBounds.top + 128 }, 19, StarlightUI::Gold);
    StarlightUI::Text(hMemDC, L"장비 또는 소비 아이템을 선택한 뒤 대상을 고르세요.",
        { panelBounds.left + 28, panelBounds.top + 132, panelBounds.right - 28, panelBounds.top + 155 }, 16, StarlightUI::Muted);
    StarlightUI::Surface(hMemDC, listBounds);
    const auto& inventory = GameManager::Get().GetInventory();
    if (inventory.empty()) StarlightUI::Text(hMemDC, L"아이템이 없습니다.", listBounds, 18,
        StarlightUI::Muted, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    const int pages = (int(inventory.size()) + ItemsPerPage - 1) / ItemsPerPage;
    if (pages > 1) StarlightUI::Text(hMemDC, std::to_wstring(inventoryPage + 1) + L" / " + std::to_wstring(pages),
        { listBounds.left + 90, listBounds.bottom - 31, listBounds.right - 90, listBounds.bottom - 4 },
        15, StarlightUI::Muted, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    DrawDetails(hMemDC);
    StarlightUI::Surface(hMemDC, targetBounds);
    if (targetPanel && targetPanel->GetVisible())
        StarlightUI::Text(hMemDC, L"대상 선택: " + ItemData::GetName(selectedItemID),
            { targetBounds.left + 16, targetBounds.top + 12, targetBounds.right - 16, targetBounds.top + 38 },
            18, StarlightUI::Gold);
    else
        StarlightUI::Text(hMemDC, L"장비 또는 소비 아이템을 선택한 뒤 대상을 고르세요.", targetBounds,
            17, StarlightUI::Muted, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    UContainerWidget::Draw(hMemDC, hMemDC1);
}

void UInventoryPopup::ShowTooltip(const std::wstring& title, const std::wstring& desc, Location)
{
    tooltipTitle = title;
    tooltipDesc = desc;
}
void UInventoryPopup::HideTooltip()
{
    // Item details stay visible when the pointer leaves the list.
}
std::wstring UInventoryPopup::MakeItemButtonText(int itemID, int count) const
{
    return ItemData::GetName(itemID) + (count > 1 ? L" x" + std::to_wstring(count) : L"");
}
std::wstring UInventoryPopup::MakeItemTooltip(int itemID) const
{
    std::wstring text = ItemData::GetDescription(itemID);
    const SItemStatBonus bonus = ItemData::GetBonus(itemID);
    if (ItemData::IsEquipment(itemID))
        text += L"\nHP" + SignedBonus(bonus.MaxHP) + L" MP" + SignedBonus(bonus.MaxMP) +
            L"\nSTR" + SignedBonus(bonus.STR) + L" INT" + SignedBonus(bonus.INT) +
            L"\nDEF" + SignedBonus(bonus.DEF) + L" SPD" + SignedBonus(bonus.SPD);
    else if (ItemData::IsConsumable(itemID))
        text += L"\nHeal HP: " + std::to_wstring(ItemData::GetHealHP(itemID)) +
            L"\nHeal MP: " + std::to_wstring(ItemData::GetHealMP(itemID));
    return text;
}
