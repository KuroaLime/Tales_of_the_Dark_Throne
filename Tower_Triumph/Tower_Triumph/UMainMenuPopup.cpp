#include "UMainMenuPopup.h"
#include "StarlightUI.h"
#include <algorithm>

namespace {
UButton* MenuButton(UContainerWidget* parent, const std::wstring& text,
    int x, int y, int width, const std::function<void()>& action, bool primary = false)
{
    auto button = new UButton();
    button->SetText(text);
    button->SetLocation(x, y);
    button->SetSize(width, 40);
    StarlightUI::StyleButton(button, primary, 17);
    button->SetOnClick(action);
    parent->AddChild(button);
    return button;
}
UTextLabel* MenuLabel(UContainerWidget* parent, const std::wstring& text, const RECT& bounds,
    int height = 18, COLORREF color = RGB(187, 179, 201))
{
    auto label = new UTextLabel();
    label->SetText(text);
    label->SetLocation((bounds.left + bounds.right) / 2, (bounds.top + bounds.bottom) / 2);
    label->SetSize(bounds.right - bounds.left, bounds.bottom - bounds.top);
    label->SetFont(height, FW_NORMAL, L"Malgun Gothic");
    label->SetColor(color);
    label->SetAlign(DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    parent->AddChild(label);
    return label;
}
std::wstring SignedBonus(int value)
{
    return (value >= 0 ? L"+" : L"") + std::to_wstring(value);
}
}

UMainMenuPopup::UMainMenuPopup() { bVisible = false; }

void UMainMenuPopup::Setup(HINSTANCE instance, RECT clientRect)
{
    Size.sX = clientRect.right - clientRect.left;
    Size.sY = clientRect.bottom - clientRect.top;
    Loc.x = (clientRect.left + clientRect.right) / 2;
    Loc.y = (clientRect.top + clientRect.bottom) / 2;
    panelBounds = StarlightUI::Centered(clientRect, 1000, 620);
    const int left = panelBounds.left, top = panelBounds.top, right = panelBounds.right;
    navLayer = new UContainerWidget();
    charLayer = new UContainerWidget();
    invLayer = new UContainerWidget();
    for (auto layer : { navLayer, charLayer, invLayer }) {
        layer->SetLocation(Loc.x, Loc.y);
        layer->SetSize(0, 0);
        AddChild(layer);
    }
    btnOpenChar = MenuButton(navLayer, L"상태", left + 88, top + 113, 120,
        [this]() { SelectTab(EMenuTab::CharacterInfo); });
    btnOpenInv = MenuButton(navLayer, L"인벤토리", left + 222, top + 113, 128,
        [this]() { SelectTab(EMenuTab::Inventory); });
    btnExit = MenuButton(navLayer, L"종료", left + 356, top + 113, 120,
        [this]() { HideSubMenus(); exitPopup->SetVisible(true); });
    btnOpenCharDetail = MenuButton(navLayer, L"상세 보기", right - 260, top + 113, 142,
        [this]() { OpenCharacterPopup(); });
    btnOpenInvDetail = MenuButton(navLayer, L"가방 보기", right - 100, top + 113, 142,
        [this]() { OpenInventoryPopup(); });
    btnClose = MenuButton(navLayer, L"X", right - 42, top + 43, 36,
        [this]() { CloseSubPopups(); SetVisible(false); });
    guideLabel = MenuLabel(charLayer, L"캐릭터를 선택하면 능력치와 장비 정보를 확인할 수 있습니다.",
        RECT{ left + 28, top + 150, right - 28, top + 182 });
    invTitleLabel = MenuLabel(invLayer, L"", RECT{ left + 28, top + 147, right - 28, top + 177 }, 19, StarlightUI::Gold);
    invGuideLabel = MenuLabel(invLayer, L"장비 또는 소비 아이템을 선택한 뒤 대상 캐릭터를 고르세요.",
        RECT{ left + 28, top + 180, right - 28, top + 210 });
    invEmptyLabel = MenuLabel(invLayer, L"아이템이 없습니다.",
        RECT{ left + 28, top + 254, left + 466, top + 290 });
    profilePanel = new UProfilePanel();
    profilePanel->Setup(left + 353, top + 390);
    profilePanel->SetPanelBounds(RECT{ left + 228, top + 194, left + 478, panelBounds.bottom - 28 });
    charLayer->AddChild(profilePanel);
    statePanel = new UStatePanel();
    statePanel->Setup(left + 736, top + 390);
    statePanel->SetPanelBounds(RECT{ left + 500, top + 194, right - 28, panelBounds.bottom - 28 });
    charLayer->AddChild(statePanel);
    detailBounds = RECT{ left + 500, top + 226, right - 28, panelBounds.bottom - 28 };
    btnInspectAction = MenuButton(invLayer, L"대상 선택", (detailBounds.left + detailBounds.right) / 2,
        detailBounds.bottom - 38, 150, [this]() {
            if (inspectedItemID >= 0) ShowActionMenu(inspectedItemID, 0, 0);
        }, true);
    targetSelectPanel = new UContainerWidget();
    targetSelectPanel->SetLocation(Loc.x, Loc.y);
    targetSelectPanel->SetSize(700, 300);
    targetSelectPanel->SetVisible(false);
    AddChild(targetSelectPanel);
    exitPopup = new UExitConfirmPopup();
    exitPopup->Setup(instance, clientRect);
    exitPopup->SetOnYes([]() { PostQuitMessage(0); });
    exitPopup->SetOnNo([this]() { exitPopup->SetVisible(false); });
    AddChild(exitPopup);
    charPopup = new UCharacterInfoPopup();
    charPopup->Setup(clientRect);
    charPopup->OnBackCallback = [this]() {
        charPopup->SetVisible(false);
        navLayer->SetVisible(true);
        SelectTab(currentTab);
    };
    charPopup->SetVisible(false);
    AddChild(charPopup);
    invPopup = new UInventoryPopup();
    invPopup->Setup(clientRect);
    invPopup->OnBackCallback = [this]() {
        invPopup->SetVisible(false);
        navLayer->SetVisible(true);
        SelectTab(currentTab);
    };
    invPopup->SetVisible(false);
    AddChild(invPopup);
    SelectTab(currentTab);
}

void UMainMenuPopup::SetTeamData(const std::vector<Character*>& team)
{
    teamMembers = team;
    if (std::find(teamMembers.begin(), teamMembers.end(), selectedCharacter) == teamMembers.end())
        selectedCharacter = teamMembers.empty() ? nullptr : teamMembers.front();
    UpdateCharList();
    RefreshFriendStylePanels();
    if (charPopup) charPopup->SetTeamData(teamMembers);
    if (invPopup) { invPopup->SetTeamData(teamMembers); invPopup->Refresh(); }
    if (currentTab == EMenuTab::Inventory) UpdateInventoryGrid();
}

void UMainMenuPopup::UpdateCharList()
{
    for (auto button : charSelectButtons) charLayer->RemoveChild(button, true);
    charSelectButtons.clear();
    for (size_t i = 0; i < teamMembers.size(); ++i) {
        Character* character = teamMembers[i];
        auto button = MenuButton(charLayer, character ? character->GetName() : L"None",
            panelBounds.left + 118, panelBounds.top + 222 + (int)i * 56, 180,
            [this, character]() { SelectCharacter(character); });
        button->SetSelected(character == selectedCharacter);
        charSelectButtons.push_back(button);
    }
}

void UMainMenuPopup::UpdateInventoryGrid()
{
    for (auto button : itemIconButtons) invLayer->RemoveChild(button, true);
    itemIconButtons.clear();
    itemButtonIDs.clear();
    const auto& inventory = GameManager::Get().GetInventory();
    invTitleLabel->SetText(L"공용 인벤토리    골드: " + std::to_wstring(GameManager::Get().GetGold()));
    invEmptyLabel->SetVisible(inventory.empty());
    bool found = false;
    for (const auto& item : inventory) if (item.ItemID == inspectedItemID) found = true;
    if (!found) inspectedItemID = inventory.empty() ? -1 : inventory.front().ItemID;
    for (size_t i = 0; i < inventory.size(); ++i) {
        const int itemID = inventory[i].ItemID;
        const int x = panelBounds.left + 134 + ((int)i % 2) * 230;
        const int y = panelBounds.top + 250 + ((int)i / 2) * 65;
        auto button = MenuButton(invLayer, MakeItemButtonText(itemID, inventory[i].Count), x, y, 212,
            [this, itemID]() { InspectItem(itemID); ShowActionMenu(itemID, 0, 0); });
        button->SetSize(212, 52);
        StarlightUI::StyleButton(button, false, 16);
        button->SetOnHover([this, itemID]() { if (invState == EInventoryState::Normal) InspectItem(itemID); });
        button->SetSelected(itemID == inspectedItemID);
        itemIconButtons.push_back(button);
        itemButtonIDs.push_back(itemID);
    }
    btnInspectAction->SetVisible(inspectedItemID >= 0);
}

void UMainMenuPopup::InspectItem(int itemID)
{
    inspectedItemID = itemID;
    for (size_t i = 0; i < itemIconButtons.size(); ++i)
        itemIconButtons[i]->SetSelected(itemButtonIDs[i] == inspectedItemID);
}

void UMainMenuPopup::ShowActionMenu(int itemID, int, int)
{
    selectedItemID = itemID;
    if (ItemData::IsEquipment(itemID) || ItemData::IsConsumable(itemID)) ShowTargetSelection();
    else HideSubMenus();
}

void UMainMenuPopup::ShowTargetSelection()
{
    invState = EInventoryState::TargetSelect;
    targetSelectPanel->ClearChildren(true);
    const int x = Loc.x - 212, y = Loc.y - 30;
    for (size_t i = 0; i < teamMembers.size(); ++i) {
        Character* target = teamMembers[i];
        MenuButton(targetSelectPanel, target ? target->GetName() : L"None",
            x + ((int)i % 3) * 212, y + ((int)i / 3) * 66, 196, [this, target]() {
                if (!target || selectedItemID < 0) { HideSubMenus(); return; }
                if (ItemData::IsEquipment(selectedItemID)) GameManager::Get().EquipItem(target, selectedItemID);
                else if (ItemData::IsConsumable(selectedItemID)) GameManager::Get().UseConsumable(target, selectedItemID);
                HideSubMenus();
                UpdateInventoryGrid();
                RefreshFriendStylePanels();
                if (invPopup) invPopup->Refresh();
            });
    }
    MenuButton(targetSelectPanel, L"취소", Loc.x, Loc.y + 111, 140, [this]() { HideSubMenus(); });
    targetSelectPanel->SetVisible(true);
}

void UMainMenuPopup::HideSubMenus()
{
    if (targetSelectPanel) targetSelectPanel->SetVisible(false);
    selectedItemID = -1;
    invState = EInventoryState::Normal;
}

void UMainMenuPopup::SelectTab(EMenuTab tab)
{
    currentTab = tab;
    HideSubMenus();
    const bool character = currentTab == EMenuTab::CharacterInfo;
    btnOpenChar->SetSelected(character);
    btnOpenInv->SetSelected(!character);
    charLayer->SetVisible(character);
    invLayer->SetVisible(!character);
    RefreshFriendStylePanels();
    if (!character) UpdateInventoryGrid();
}

void UMainMenuPopup::SelectCharacter(Character* character)
{
    selectedCharacter = character;
    for (size_t i = 0; i < charSelectButtons.size(); ++i)
        charSelectButtons[i]->SetSelected(teamMembers[i] == character);
    RefreshFriendStylePanels();
}
void UMainMenuPopup::RefreshFriendStylePanels()
{
    if (profilePanel) profilePanel->SetCharacter(selectedCharacter);
    if (statePanel) statePanel->SetCharacter(selectedCharacter);
}
void UMainMenuPopup::ShowNavigation() { CloseSubPopups(); SetVisible(true); }

void UMainMenuPopup::OpenCharacterPopup()
{
    if (!charPopup) return;
    HideSubMenus();
    invPopup->SetVisible(false);
    exitPopup->SetVisible(false);
    navLayer->SetVisible(false);
    charLayer->SetVisible(false);
    invLayer->SetVisible(false);
    charPopup->SetTeamData(teamMembers);
    charPopup->SetVisible(true);
}
void UMainMenuPopup::OpenInventoryPopup()
{
    if (!invPopup) return;
    HideSubMenus();
    charPopup->SetVisible(false);
    exitPopup->SetVisible(false);
    navLayer->SetVisible(false);
    charLayer->SetVisible(false);
    invLayer->SetVisible(false);
    invPopup->SetTeamData(teamMembers);
    invPopup->Refresh();
    invPopup->SetVisible(true);
}
void UMainMenuPopup::CloseSubPopups()
{
    HideSubMenus();
    if (charPopup) charPopup->SetVisible(false);
    if (invPopup) invPopup->SetVisible(false);
    if (exitPopup) exitPopup->SetVisible(false);
    if (navLayer) navLayer->SetVisible(true);
    SelectTab(currentTab);
}

bool UMainMenuPopup::HandleBack()
{
    if (exitPopup && exitPopup->GetVisible()) { exitPopup->SetVisible(false); return true; }
    if (charPopup && charPopup->GetVisible()) {
        charPopup->SetVisible(false);
        navLayer->SetVisible(true);
        SelectTab(currentTab);
        return true;
    }
    if (invPopup && invPopup->GetVisible()) {
        if (invPopup->HandleBack()) return true;
        invPopup->SetVisible(false);
        navLayer->SetVisible(true);
        SelectTab(currentTab);
        return true;
    }
    if (targetSelectPanel && targetSelectPanel->GetVisible()) { HideSubMenus(); return true; }
    return false;
}
UContainerWidget* UMainMenuPopup::ActiveModal() const
{
    if (exitPopup && exitPopup->GetVisible()) return exitPopup;
    if (charPopup && charPopup->GetVisible()) return charPopup;
    if (invPopup && invPopup->GetVisible()) return invPopup;
    if (targetSelectPanel && targetSelectPanel->GetVisible()) return targetSelectPanel;
    return nullptr;
}
UWidget* UMainMenuPopup::FindWidgetUnderMouse(int x, int y)
{
    if (!bVisible) return nullptr;
    if (auto modal = ActiveModal()) {
        if (auto hit = modal->FindWidgetUnderMouse(x, y)) return hit;
        return this;
    }
    if (auto hit = UContainerWidget::FindWidgetUnderMouse(x, y)) return hit;
    return this;
}
void UMainMenuPopup::UpdateMouseHover(int x, int y)
{
    UContainerWidget* modal = bVisible ? ActiveModal() : nullptr;
    for (auto layer : { navLayer, charLayer, invLayer, targetSelectPanel })
        if (layer) layer->UpdateMouseHover(!bVisible || modal ? -10000 : x, !bVisible || modal ? -10000 : y);
    for (UContainerWidget* child : { static_cast<UContainerWidget*>(exitPopup), static_cast<UContainerWidget*>(charPopup), static_cast<UContainerWidget*>(invPopup) })
        if (child) child->UpdateMouseHover(-10000, -10000);
    if (modal) modal->UpdateMouseHover(x, y);
}
void UMainMenuPopup::Update()
{
    if (!bVisible) return;
    if (auto modal = ActiveModal()) { modal->Update(); return; }
    navLayer->Update();
    (currentTab == EMenuTab::CharacterInfo ? charLayer : invLayer)->Update();
}
void UMainMenuPopup::Draw(HDC hdc, HDC scratch)
{
    if (!bVisible) return;
    if (charPopup && charPopup->GetVisible()) { charPopup->Draw(hdc, scratch); return; }
    if (invPopup && invPopup->GetVisible()) { invPopup->Draw(hdc, scratch); return; }
    const RECT client = { Loc.x - Size.sX / 2, Loc.y - Size.sY / 2,
        Loc.x + Size.sX / 2, Loc.y + Size.sY / 2 };
    StarlightUI::Dim(hdc, client, 100);
    StarlightUI::Panel(hdc, panelBounds, L"메뉴", L"JOURNEY");
    if (currentTab == EMenuTab::Inventory) DrawInventory(hdc);
    navLayer->Draw(hdc, scratch);
    (currentTab == EMenuTab::CharacterInfo ? charLayer : invLayer)->Draw(hdc, scratch);
    if (targetSelectPanel->GetVisible()) {
        StarlightUI::Dim(hdc, panelBounds, 150);
        RECT target = { Loc.x - 350, Loc.y - 150, Loc.x + 350, Loc.y + 150 };
        StarlightUI::Panel(hdc, target, L"대상 선택: " + ItemData::GetName(selectedItemID));
        targetSelectPanel->Draw(hdc, scratch);
    }
    if (exitPopup->GetVisible()) exitPopup->Draw(hdc, scratch);
}
void UMainMenuPopup::DrawInventory(HDC hdc)
{
    invTitleLabel->SetText(L"공용 인벤토리    골드: " + std::to_wstring(GameManager::Get().GetGold()));
    if (inspectedItemID < 0) return;
    StarlightUI::Surface(hdc, detailBounds);
    RECT title = { detailBounds.left + 22, detailBounds.top + 18, detailBounds.right - 22, detailBounds.top + 52 };
    StarlightUI::Text(hdc, ItemData::GetName(inspectedItemID), title, 22, StarlightUI::Gold);
    RECT description = { detailBounds.left + 22, detailBounds.top + 68, detailBounds.right - 22, detailBounds.bottom - 78 };
    StarlightUI::Text(hdc, MakeItemTooltip(inspectedItemID), description, 18, StarlightUI::Ink);
}
std::wstring UMainMenuPopup::MakeItemTooltip(int itemID) const
{
    std::wstring text = ItemData::GetDescription(itemID);
    const SItemStatBonus bonus = ItemData::GetBonus(itemID);
    if (ItemData::IsEquipment(itemID)) {
        text += L"\n\nHP" + SignedBonus(bonus.MaxHP) + L"   MP" + SignedBonus(bonus.MaxMP);
        text += L"\nSTR" + SignedBonus(bonus.STR) + L"   INT" + SignedBonus(bonus.INT);
        text += L"\nDEF" + SignedBonus(bonus.DEF) + L"   SPD" + SignedBonus(bonus.SPD);
    }
    else if (ItemData::IsConsumable(itemID)) {
        text += L"\n\nHeal HP: " + std::to_wstring(ItemData::GetHealHP(itemID));
        text += L"\nHeal MP: " + std::to_wstring(ItemData::GetHealMP(itemID));
    }
    return text;
}
std::wstring UMainMenuPopup::MakeItemButtonText(int itemID, int count) const
{
    std::wstring text = ItemData::GetName(itemID);
    if (count > 1) text += L" x" + std::to_wstring(count);
    return text;
}
