#include "UMainMenuPopup.h"
#include "resource.h"

UMainMenuPopup::UMainMenuPopup()
    : currentTab(EMenuTab::CharacterInfo),
      invState(EInventoryState::Normal),
      selectedCharacter(nullptr),
      selectedItemID(-1),
      btnTabChar(nullptr),
      btnTabInv(nullptr),
      btnExit(nullptr),
      btnClose(nullptr),
      btnOpenCharPopup(nullptr),
      btnOpenInvPopup(nullptr),
      itemActionPanel(nullptr),
      btnItemAction1(nullptr),
      btnItemAction2(nullptr),
      targetSelectPanel(nullptr),
      profilePanel(nullptr),
      statePanel(nullptr),
      hBgBitmap(NULL),
      exitPopup(nullptr),
      charPopup(nullptr),
      invPopup(nullptr),
      bShowTooltip(false)
{
    bVisible = false;
    ZeroMemory(&bgInfo, sizeof(BITMAP));
}

UMainMenuPopup::~UMainMenuPopup()
{
}

void UMainMenuPopup::Init()
{
}

void UMainMenuPopup::Setup(HINSTANCE hInst, RECT clientRect)
{
    Size.sX = clientRect.right;
    Size.sY = clientRect.bottom;
    Loc.x = clientRect.right / 2;
    Loc.y = clientRect.bottom / 2;

    int topY = 88;
    int btnW = 145;
    int btnH = 38;
    int spacing = 16;

    int groupWidth = (btnW * 3) + (spacing * 2);
    int groupStartX = Loc.x - (groupWidth / 2);

    btnTabChar = new UButton();
    btnTabChar->SetText(L"상태");
    btnTabChar->SetLocation(groupStartX + (btnW / 2), topY);
    btnTabChar->SetSize(btnW, btnH);
    btnTabChar->SetOnClick([this]() { this->SelectTab(EMenuTab::CharacterInfo); });
    btnTabChar->SetSelected(true);
    AddChild(btnTabChar);

    btnTabInv = new UButton();
    btnTabInv->SetText(L"인벤토리");
    btnTabInv->SetLocation(groupStartX + btnW + spacing + (btnW / 2), topY);
    btnTabInv->SetSize(btnW, btnH);
    btnTabInv->SetOnClick([this]() { this->SelectTab(EMenuTab::Inventory); });
    AddChild(btnTabInv);

    btnExit = new UButton();
    btnExit->SetText(L"종료");
    btnExit->SetLocation(groupStartX + (btnW * 2) + (spacing * 2) + (btnW / 2), topY);
    btnExit->SetSize(btnW, btnH);
    btnExit->SetOnClick([this]() {
        if (exitPopup) {
            exitPopup->SetVisible(true);
        }
    });
    AddChild(btnExit);

    btnClose = new UButton();
    btnClose->SetText(L"X");
    btnClose->SetLocation(clientRect.right - 46, topY);
    btnClose->SetSize(42, 42);
    btnClose->SetOnClick([this]() { this->CloseSubPopups(); this->SetVisible(false); });
    AddChild(btnClose);

    btnOpenCharPopup = new UButton();
    btnOpenCharPopup->SetText(L"상세 보기");
    btnOpenCharPopup->SetLocation(Loc.x - 120, topY + 62);
    btnOpenCharPopup->SetSize(150, 34);
    btnOpenCharPopup->SetFont(18, FW_BOLD, L"Arial");
    btnOpenCharPopup->SetAnimationType(1);
    btnOpenCharPopup->SetOnClick([this]() { this->OpenCharacterPopup(); });
    AddChild(btnOpenCharPopup);

    btnOpenInvPopup = new UButton();
    btnOpenInvPopup->SetText(L"가방 보기");
    btnOpenInvPopup->SetLocation(Loc.x + 120, topY + 62);
    btnOpenInvPopup->SetSize(150, 34);
    btnOpenInvPopup->SetFont(18, FW_BOLD, L"Arial");
    btnOpenInvPopup->SetAnimationType(1);
    btnOpenInvPopup->SetOnClick([this]() { this->OpenInventoryPopup(); });
    AddChild(btnOpenInvPopup);

    itemActionPanel = new UContainerWidget();
    itemActionPanel->SetSize(180, 100);
    itemActionPanel->SetVisible(false);
    AddChild(itemActionPanel);

    btnItemAction1 = new UButton();
    btnItemAction1->SetSize(150, 40);
    btnItemAction1->SetLocation(90, 25);
    itemActionPanel->AddChild(btnItemAction1);

    btnItemAction2 = new UButton();
    btnItemAction2->SetText(L"취소");
    btnItemAction2->SetSize(150, 40);
    btnItemAction2->SetLocation(90, 75);
    btnItemAction2->SetOnClick([this]() { this->HideSubMenus(); });
    itemActionPanel->AddChild(btnItemAction2);

    targetSelectPanel = new UContainerWidget();
    targetSelectPanel->SetSize(640, 220);
    targetSelectPanel->SetLocation(Loc.x, Loc.y);
    targetSelectPanel->SetVisible(false);
    AddChild(targetSelectPanel);

    profilePanel = new UProfilePanel();
    profilePanel->Setup(Loc.x - 400, Loc.y + 10);
    profilePanel->SetVisible(false);
    AddChild(profilePanel);

    statePanel = new UStatePanel();
    statePanel->Setup(Loc.x + 80, Loc.y + 85);
    statePanel->SetVisible(false);
    AddChild(statePanel);

    exitPopup = new UExitConfirmPopup();
    exitPopup->Setup(hInst, clientRect);
    exitPopup->SetOnYes([]() { PostQuitMessage(0); });
    exitPopup->SetOnNo([this]() { exitPopup->SetVisible(false); });
    AddChild(exitPopup);

    charPopup = new UCharacterInfoPopup();
    charPopup->Setup(clientRect);
    charPopup->OnBackCallback = [this]() {
        if (charPopup) {
            charPopup->SetVisible(false);
        }
    };
    charPopup->SetVisible(false);
    AddChild(charPopup);

    invPopup = new UInventoryPopup();
    invPopup->Setup(clientRect);
    invPopup->OnBackCallback = [this]() {
        if (invPopup) {
            invPopup->SetVisible(false);
        }
    };
    invPopup->SetVisible(false);
    AddChild(invPopup);

    for (int i = 0; i < 3; ++i) {
        UButton* btn = new UButton();
        btn->SetSize(240, 54);
        btn->SetVisible(false);
        AddChild(btn);
        equipSlotButtons.push_back(btn);
    }
}

void UMainMenuPopup::SetBackgroundImage(HBITMAP hBitmap)
{
    hBgBitmap = hBitmap;
    if (hBgBitmap) {
        GetObject(hBgBitmap, sizeof(BITMAP), &bgInfo);
    }
}

void UMainMenuPopup::SetTeamData(const std::vector<Character*>& team)
{
    teamMembers = team;
    if (!teamMembers.empty() && selectedCharacter == nullptr) {
        selectedCharacter = teamMembers[0];
    }

    UpdateCharList();
    UpdateInventoryGrid();
    UpdateEquipmentSlots();
    RefreshFriendStylePanels();

    if (charPopup) {
        charPopup->SetTeamData(teamMembers);
    }
    if (invPopup) {
        invPopup->SetTeamData(teamMembers);
        invPopup->Refresh();
    }
}

void UMainMenuPopup::UpdateCharList()
{
    for (auto b : charSelectButtons) {
        RemoveChild(b, true);
    }
    charSelectButtons.clear();

    int panelRight = Loc.x + Size.sX / 2 - 70;
    int listStartX = panelRight - 85;
    int listStartY = Loc.y - Size.sY / 2 + 185;

    int btnW = 130;
    int btnH = 34;

    for (size_t i = 0; i < teamMembers.size(); ++i) {
        UButton* btn = new UButton();
        btn->SetText(teamMembers[i]->GetName());
        btn->SetLocation(listStartX, listStartY + (int)i * 44);
        btn->SetSize(btnW, btnH);

        Character* charPtr = teamMembers[i];
        btn->SetOnClick([this, charPtr, btn]() {
            this->SelectCharacter(charPtr);
            for (auto b : charSelectButtons) {
                b->SetSelected(false);
            }
            btn->SetSelected(true);
        });

        if (charPtr == selectedCharacter) {
            btn->SetSelected(true);
        }

        AddChild(btn);
        charSelectButtons.push_back(btn);
    }

    SelectTab(currentTab);
}

void UMainMenuPopup::UpdateInventoryGrid()
{
    for (auto b : itemIconButtons) {
        RemoveChild(b, true);
    }
    itemIconButtons.clear();

    if (currentTab != EMenuTab::Inventory) {
        return;
    }

    int boxLeft = Loc.x - Size.sX / 2 + 105;
    int boxTop = Loc.y - Size.sY / 2 + 245;

    const auto& inventory = GameManager::Get().GetInventory();

    int colCount = 5;
    int slotW = 160;
    int slotH = 66;
    int spacingX = 22;
    int spacingY = 22;

    for (size_t i = 0; i < inventory.size(); ++i) {
        int itemID = inventory[i].ItemID;
        int count = inventory[i].Count;

        int col = (int)i % colCount;
        int row = (int)i / colCount;

        UButton* btn = new UButton();
        btn->SetText(MakeItemButtonText(itemID, count));
        btn->SetLocation(
            boxLeft + col * (slotW + spacingX) + slotW / 2,
            boxTop + row * (slotH + spacingY) + slotH / 2
        );
        btn->SetSize(slotW, slotH);

        btn->SetOnHover([this, itemID, btn]() {
            this->ShowTooltip(ItemData::GetName(itemID), this->MakeItemTooltip(itemID), btn->GetLocation());
        });
        btn->SetOnUnhover([this]() { this->HideTooltip(); });

        btn->SetOnClick([this, itemID, btn]() {
            Location loc = btn->GetLocation();
            this->ShowActionMenu(itemID, loc.x, loc.y);
        });

        AddChild(btn);
        itemIconButtons.push_back(btn);
    }
}

void UMainMenuPopup::UpdateEquipmentSlots()
{
    // 상태 화면에서는 UStatePanel 안에 장비 정보가 이미 표시된다.
    // 별도의 큰 장비 버튼을 같이 띄우면 캐릭터 선택 버튼과 겹치므로 숨긴다.
    for (auto b : equipSlotButtons)
    {
        if (b)
        {
            b->SetVisible(false);
        }
    }
}

void UMainMenuPopup::ShowActionMenu(int itemID, int x, int y)
{
    selectedItemID = itemID;
    invState = EInventoryState::TargetSelect;

    if (itemActionPanel) {
        itemActionPanel->SetVisible(false);
    }

    if (ItemData::IsEquipment(itemID) || ItemData::IsConsumable(itemID)) {
        ShowTargetSelection();
        return;
    }

    HideSubMenus();
}

void UMainMenuPopup::ShowTargetSelection()
{
    invState = EInventoryState::TargetSelect;

    if (itemActionPanel) {
        itemActionPanel->SetVisible(false);
    }

    targetSelectPanel->ClearChildren(true);
    targetButtons.clear();

    int panelCenterX = Loc.x;
    int panelCenterY = Loc.y;

    // 5명 + 취소 버튼을 한 줄에 모두 넣으면 패널 밖으로 튀어나가므로 3열 2행으로 배치한다.
    const int panelW = 820;
    const int panelH = 300;
    const int colCount = 3;
    const int btnW = 160;
    const int btnH = 58;
    const int gapX = 42;
    const int gapY = 24;

    targetSelectPanel->SetSize(panelW, panelH);
    targetSelectPanel->SetLocation(panelCenterX, panelCenterY + 25);

    int totalW = colCount * btnW + (colCount - 1) * gapX;
    int startX = panelCenterX - totalW / 2 + btnW / 2;
    int startY = panelCenterY + 15;

    for (size_t i = 0; i < teamMembers.size(); ++i) {
        Character* target = teamMembers[i];
        UButton* btn = new UButton();
        btn->SetText(target ? target->GetName() : L"None");

        int col = (int)i % colCount;
        int row = (int)i / colCount;

        btn->SetLocation(startX + col * (btnW + gapX), startY + row * (btnH + gapY));
        btn->SetSize(btnW, btnH);
        btn->SetFont(15, FW_BOLD, L"Malgun Gothic");
        btn->SetAnimationType(0);

        btn->SetOnClick([this, target]() {
            if (target == nullptr || selectedItemID < 0) {
                this->HideSubMenus();
                return;
            }

            if (ItemData::IsEquipment(selectedItemID)) {
                GameManager::Get().EquipItem(target, selectedItemID);
            }
            else if (ItemData::IsConsumable(selectedItemID)) {
                GameManager::Get().UseConsumable(target, selectedItemID);
            }

            this->HideSubMenus();
            this->UpdateInventoryGrid();
            this->UpdateEquipmentSlots();
            this->RefreshFriendStylePanels();
            if (invPopup) {
                invPopup->Refresh();
            }
        });

        targetSelectPanel->AddChild(btn);
        targetButtons.push_back(btn);
    }

    int cancelIndex = (int)teamMembers.size();
    int cancelCol = cancelIndex % colCount;
    int cancelRow = cancelIndex / colCount;

    UButton* cancel = new UButton();
    cancel->SetText(L"취소");
    cancel->SetLocation(startX + cancelCol * (btnW + gapX), startY + cancelRow * (btnH + gapY));
    cancel->SetSize(btnW, btnH);
    cancel->SetFont(15, FW_BOLD, L"Malgun Gothic");
    cancel->SetAnimationType(0);
    cancel->SetOnClick([this]() { this->HideSubMenus(); });
    targetSelectPanel->AddChild(cancel);
    targetButtons.push_back(cancel);

    targetSelectPanel->SetVisible(true);
}

void UMainMenuPopup::HideSubMenus()
{
    if (itemActionPanel) {
        itemActionPanel->SetVisible(false);
    }
    if (targetSelectPanel) {
        targetSelectPanel->SetVisible(false);
    }

    targetButtons.clear();
    selectedItemID = -1;
    invState = EInventoryState::Normal;
}

void UMainMenuPopup::SelectTab(EMenuTab tab)
{
    currentTab = tab;
    HideSubMenus();

    btnTabChar->SetSelected(currentTab == EMenuTab::CharacterInfo);
    btnTabInv->SetSelected(currentTab == EMenuTab::Inventory);

    bool showCharLists = (currentTab == EMenuTab::CharacterInfo);
    for (auto btn : charSelectButtons) {
        btn->SetVisible(showCharLists);
    }

    if (profilePanel) {
        profilePanel->SetVisible(showCharLists);
    }

    if (statePanel) {
        statePanel->SetVisible(showCharLists);
    }

    RefreshFriendStylePanels();

    if (currentTab == EMenuTab::Inventory) {
        UpdateInventoryGrid();
        for (auto b : equipSlotButtons) {
            b->SetVisible(false);
        }
    }
    else {
        for (auto b : itemIconButtons) {
            b->SetVisible(false);
        }
        UpdateEquipmentSlots();
    }
}

void UMainMenuPopup::SelectCharacter(Character* character)
{
    selectedCharacter = character;
    UpdateEquipmentSlots();
    RefreshFriendStylePanels();
}

void UMainMenuPopup::RefreshFriendStylePanels()
{
    if (profilePanel) {
        profilePanel->SetCharacter(selectedCharacter);
    }

    if (statePanel) {
        statePanel->SetCharacter(selectedCharacter);
    }
}

void UMainMenuPopup::ShowNavigation()
{
    CloseSubPopups();
    HideTooltip();
    SelectTab(currentTab);
    SetVisible(true);
}

void UMainMenuPopup::OpenCharacterPopup()
{
    if (charPopup == nullptr) {
        return;
    }

    HideSubMenus();
    HideTooltip();
    if (invPopup) {
        invPopup->SetVisible(false);
    }

    charPopup->SetTeamData(teamMembers);
    RemoveChild(charPopup, false);
    AddChild(charPopup);
    charPopup->SetVisible(true);
}

void UMainMenuPopup::OpenInventoryPopup()
{
    if (invPopup == nullptr) {
        return;
    }

    HideSubMenus();
    HideTooltip();
    if (charPopup) {
        charPopup->SetVisible(false);
    }

    invPopup->SetTeamData(teamMembers);
    invPopup->Refresh();
    RemoveChild(invPopup, false);
    AddChild(invPopup);
    invPopup->SetVisible(true);
}

void UMainMenuPopup::CloseSubPopups()
{
    HideSubMenus();

    if (charPopup) {
        charPopup->SetVisible(false);
    }
    if (invPopup) {
        invPopup->SetVisible(false);
    }
    if (exitPopup) {
        exitPopup->SetVisible(false);
    }
}

bool UMainMenuPopup::HandleBack()
{
    if (exitPopup && exitPopup->GetVisible()) {
        exitPopup->SetVisible(false);
        return true;
    }

    if (charPopup && charPopup->GetVisible()) {
        charPopup->SetVisible(false);
        return true;
    }

    if (invPopup && invPopup->GetVisible()) {
        invPopup->SetVisible(false);
        return true;
    }

    if ((targetSelectPanel && targetSelectPanel->GetVisible()) ||
        (itemActionPanel && itemActionPanel->GetVisible())) {
        HideSubMenus();
        return true;
    }

    return false;
}

void UMainMenuPopup::ShowTooltip(const std::wstring& title, const std::wstring& desc, Location loc)
{
    bShowTooltip = true;
    tooltipTitle = title;
    tooltipDesc = desc;
    tooltipLoc = loc;
}

void UMainMenuPopup::HideTooltip()
{
    bShowTooltip = false;
}

void UMainMenuPopup::Update()
{
    UContainerWidget::Update();
}

void UMainMenuPopup::Draw(HDC hMemDC, HDC hMemDC1)
{
    if (!bVisible) {
        return;
    }

    RECT overlay = {
        Loc.x - Size.sX / 2,
        Loc.y - Size.sY / 2,
        Loc.x + Size.sX / 2,
        Loc.y + Size.sY / 2
    };

    HBRUSH overlayBrush = CreateSolidBrush(RGB(8, 8, 12));
    FillRect(hMemDC, &overlay, overlayBrush);
    DeleteObject(overlayBrush);

    RECT mainPanel = {
        Loc.x - Size.sX / 2 + 45,
        Loc.y - Size.sY / 2 + 45,
        Loc.x + Size.sX / 2 - 45,
        Loc.y + Size.sY / 2 - 45
    };

    HBRUSH panelBrush = CreateSolidBrush(RGB(19, 23, 31));
    HPEN panelPen = CreatePen(PS_SOLID, 3, RGB(176, 146, 78));
    HBRUSH oldBrush = (HBRUSH)SelectObject(hMemDC, panelBrush);
    HPEN oldPen = (HPEN)SelectObject(hMemDC, panelPen);
    RoundRect(hMemDC, mainPanel.left, mainPanel.top, mainPanel.right, mainPanel.bottom, 14, 14);
    SelectObject(hMemDC, oldBrush);
    SelectObject(hMemDC, oldPen);
    DeleteObject(panelBrush);
    DeleteObject(panelPen);

    if (currentTab == EMenuTab::CharacterInfo) {
        DrawCharacterInfo(hMemDC);
    }
    else {
        DrawInventory(hMemDC);
    }

    DrawSubMenuBackgrounds(hMemDC);
    UContainerWidget::Draw(hMemDC, hMemDC1);

    if (bShowTooltip) {
        DrawTooltip(hMemDC);
    }
}

void UMainMenuPopup::DrawCharacterInfo(HDC hMemDC)
{
    if (!selectedCharacter) {
        return;
    }

    int boxLeft = Loc.x - Size.sX / 2 + 80;
    int boxTop = Loc.y - Size.sY / 2 + 160;
    int width = Size.sX - 160;

    SetBkMode(hMemDC, TRANSPARENT);
    SetTextColor(hMemDC, RGB(230, 230, 240));

    HFONT hFont = CreateFontW(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, L"Arial");
    HFONT oldF = (HFONT)SelectObject(hMemDC, hFont);

    std::wstring guide = L"캐릭터를 선택하면 능력치와 장비 정보를 확인할 수 있습니다.";
    RECT guideRect = { boxLeft + 20, boxTop + 5, boxLeft + width - 20, boxTop + 38 };
    DrawTextW(hMemDC, guide.c_str(), -1, &guideRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hMemDC, oldF);
    DeleteObject(hFont);
}

void UMainMenuPopup::DrawInventory(HDC hMemDC)
{
    int boxLeft = Loc.x - Size.sX / 2 + 105;
    int boxTop = Loc.y - Size.sY / 2 + 170;

    int oldBkMode = SetBkMode(hMemDC, TRANSPARENT);
    COLORREF oldColor = SetTextColor(hMemDC, RGB(238, 232, 214));

    HFONT hFont = CreateFontW(25, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, L"Arial");
    HFONT oldF = (HFONT)SelectObject(hMemDC, hFont);

    std::wstring title = L"공용 인벤토리    골드: " + std::to_wstring(GameManager::Get().GetGold());
    TextOutW(hMemDC, boxLeft, boxTop, title.c_str(), (int)title.length());

    SelectObject(hMemDC, oldF);
    DeleteObject(hFont);

    hFont = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, L"Arial");
    oldF = (HFONT)SelectObject(hMemDC, hFont);
    SetTextColor(hMemDC, RGB(215, 215, 225));
    std::wstring guide = L"장비 또는 소비 아이템을 선택한 뒤 대상 캐릭터를 고르세요.";
    TextOutW(hMemDC, boxLeft, boxTop + 38, guide.c_str(), (int)guide.length());

    if (GameManager::Get().GetInventory().empty()) {
        std::wstring emptyText = L"아이템이 없습니다.";
        TextOutW(hMemDC, boxLeft, boxTop + 95, emptyText.c_str(), (int)emptyText.length());
    }

    SelectObject(hMemDC, oldF);
    DeleteObject(hFont);

    SetTextColor(hMemDC, oldColor);
    SetBkMode(hMemDC, oldBkMode);
}

void UMainMenuPopup::DrawSubMenuBackgrounds(HDC hMemDC)
{
    if (targetSelectPanel && targetSelectPanel->GetVisible()) {
        Location pLoc = targetSelectPanel->GetLocation();
        BuSIZE pSize = targetSelectPanel->GetSize();

        RECT pr = {
            pLoc.x - pSize.sX / 2,
            pLoc.y - pSize.sY / 2,
            pLoc.x + pSize.sX / 2,
            pLoc.y + pSize.sY / 2
        };

        HBRUSH hB = CreateSolidBrush(RGB(28, 30, 44));
        FillRect(hMemDC, &pr, hB);
        DeleteObject(hB);
        {
            HPEN hPen = CreatePen(PS_SOLID, 2, RGB(176, 146, 78));
            HPEN oldPen = (HPEN)SelectObject(hMemDC, hPen);
            HBRUSH oldBrush = (HBRUSH)SelectObject(hMemDC, GetStockObject(NULL_BRUSH));
            Rectangle(hMemDC, pr.left, pr.top, pr.right, pr.bottom);
            SelectObject(hMemDC, oldBrush);
            SelectObject(hMemDC, oldPen);
            DeleteObject(hPen);
        }

        SetBkMode(hMemDC, TRANSPARENT);
        SetTextColor(hMemDC, RGB(238, 232, 214));

        HFONT hFont = CreateFontW(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, L"Arial");
        HFONT oldF = (HFONT)SelectObject(hMemDC, hFont);

        std::wstring title = L"대상 선택: " + ItemData::GetName(selectedItemID);
        RECT titleRect = { pr.left, pr.top + 25, pr.right, pr.top + 60 };
        DrawTextW(hMemDC, title.c_str(), (int)title.length(), &titleRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hMemDC, oldF);
        DeleteObject(hFont);
    }
}

void UMainMenuPopup::DrawTooltip(HDC hMemDC)
{
    int tw = 280;
    int th = 150;
    int tx = tooltipLoc.x + 40;
    int ty = tooltipLoc.y - 60;

    if (tx + tw > Loc.x + Size.sX / 2 - 20) {
        tx = tooltipLoc.x - tw - 40;
    }
    if (ty < 20) {
        ty = 20;
    }

    RECT tr = { tx, ty, tx + tw, ty + th };
    HBRUSH hB = CreateSolidBrush(RGB(19, 23, 31));
    FillRect(hMemDC, &tr, hB);
    DeleteObject(hB);
    {
        HPEN hPen = CreatePen(PS_SOLID, 2, RGB(176, 146, 78));
        HPEN oldPen = (HPEN)SelectObject(hMemDC, hPen);
        HBRUSH oldBrush = (HBRUSH)SelectObject(hMemDC, GetStockObject(NULL_BRUSH));
        Rectangle(hMemDC, tr.left, tr.top, tr.right, tr.bottom);
        SelectObject(hMemDC, oldBrush);
        SelectObject(hMemDC, oldPen);
        DeleteObject(hPen);
    }

    SetTextColor(hMemDC, RGB(238, 232, 214));
    SetBkMode(hMemDC, TRANSPARENT);

    HFONT hFont = CreateFontW(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, L"Arial");
    HFONT oldF = (HFONT)SelectObject(hMemDC, hFont);
    TextOutW(hMemDC, tx + 10, ty + 10, tooltipTitle.c_str(), (int)tooltipTitle.length());
    DeleteObject(hFont);

    hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, L"Arial");
    SelectObject(hMemDC, hFont);
    RECT textR = { tx + 10, ty + 40, tx + tw - 10, ty + th - 10 };
    DrawTextW(hMemDC, tooltipDesc.c_str(), -1, &textR, DT_LEFT | DT_WORDBREAK);

    SelectObject(hMemDC, oldF);
    DeleteObject(hFont);
}

std::wstring UMainMenuPopup::GetEquipName(int itemID) const
{
    if (itemID < 0) {
        return L"Empty";
    }

    return ItemData::GetName(itemID);
}

std::wstring UMainMenuPopup::MakeItemTooltip(int itemID) const
{
    std::wstring text = ItemData::GetDescription(itemID);
    SItemStatBonus bonus = ItemData::GetBonus(itemID);

    if (ItemData::IsEquipment(itemID)) {
        text += L"\n";
        text += L"HP+" + std::to_wstring(bonus.MaxHP) + L" MP+" + std::to_wstring(bonus.MaxMP);
        text += L"\nSTR+" + std::to_wstring(bonus.STR) + L" INT+" + std::to_wstring(bonus.INT);
        text += L"\nDEF+" + std::to_wstring(bonus.DEF) + L" SPD+" + std::to_wstring(bonus.SPD);
    }
    else if (ItemData::IsConsumable(itemID)) {
        text += L"\nHeal HP: " + std::to_wstring(ItemData::GetHealHP(itemID));
        text += L"\nHeal MP: " + std::to_wstring(ItemData::GetHealMP(itemID));
    }

    return text;
}

std::wstring UMainMenuPopup::MakeItemButtonText(int itemID, int count) const
{
    std::wstring name = ItemData::GetName(itemID);

    if (name == L"Small Potion") {
        name = L"Potion";
    }
    else if (name == L"Leather Armor") {
        name = L"Leather Armor";
    }

    std::wstring text = name;

    if (count > 1) {
        text += L" x" + std::to_wstring(count);
    }

    if (text.length() > 17) {
        text = text.substr(0, 16) + L"...";
    }

    return text;
}
