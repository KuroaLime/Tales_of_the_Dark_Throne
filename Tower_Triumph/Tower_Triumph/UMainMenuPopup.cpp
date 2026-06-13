#include "UMainMenuPopup.h"
#include "resource.h"

UMainMenuPopup::UMainMenuPopup()
    : currentTab(EMenuTab::CharacterInfo),
      invState(EInventoryState::Normal),
      selectedCharacter(nullptr),
      selectedItemID(-1),
      btnOpenChar(nullptr),
      btnOpenInv(nullptr),
      btnExit(nullptr),
      btnClose(nullptr),
      btnOpenCharDetail(nullptr),
      btnOpenInvDetail(nullptr),
      navLayer(nullptr),
      charLayer(nullptr),
      invLayer(nullptr),
      guideLabel(nullptr),
      invTitleLabel(nullptr),
      invGuideLabel(nullptr),
      invEmptyLabel(nullptr),
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

    // 친구 코드 방식: UI를 직접 그리는 대신 레이어 컨테이너에 등록해두고
    // 상태 전환은 SetVisible로 처리한다.
    navLayer = new UContainerWidget();
    navLayer->SetSize(0, 0);
    navLayer->SetLocation(Loc.x, Loc.y);
    AddChild(navLayer);

    charLayer = new UContainerWidget();
    charLayer->SetSize(0, 0);
    charLayer->SetLocation(Loc.x, Loc.y);
    AddChild(charLayer);

    invLayer = new UContainerWidget();
    invLayer->SetSize(0, 0);
    invLayer->SetLocation(Loc.x, Loc.y);
    invLayer->SetVisible(false);
    AddChild(invLayer);

    int topY = 88;
    int btnW = 145;
    int btnH = 38;
    int spacing = 16;

    int groupWidth = (btnW * 3) + (spacing * 2);
    int groupStartX = Loc.x - (groupWidth / 2);

    btnOpenChar = new UButton();
    btnOpenChar->SetText(L"상태");
    btnOpenChar->SetLocation(groupStartX + (btnW / 2), topY);
    btnOpenChar->SetSize(btnW, btnH);
    btnOpenChar->SetOnClick([this]() { this->SelectTab(EMenuTab::CharacterInfo); });
    btnOpenChar->SetSelected(true);
    navLayer->AddChild(btnOpenChar);

    btnOpenInv = new UButton();
    btnOpenInv->SetText(L"인벤토리");
    btnOpenInv->SetLocation(groupStartX + btnW + spacing + (btnW / 2), topY);
    btnOpenInv->SetSize(btnW, btnH);
    btnOpenInv->SetOnClick([this]() { this->SelectTab(EMenuTab::Inventory); });
    navLayer->AddChild(btnOpenInv);

    btnExit = new UButton();
    btnExit->SetText(L"종료");
    btnExit->SetLocation(groupStartX + (btnW * 2) + (spacing * 2) + (btnW / 2), topY);
    btnExit->SetSize(btnW, btnH);
    btnExit->SetOnClick([this]() {
        if (exitPopup) {
            exitPopup->SetVisible(true);
        }
    });
    navLayer->AddChild(btnExit);

    btnClose = new UButton();
    btnClose->SetText(L"X");
    btnClose->SetLocation(clientRect.right - 46, topY);
    btnClose->SetSize(42, 42);
    btnClose->SetOnClick([this]() { this->CloseSubPopups(); this->SetVisible(false); });
    navLayer->AddChild(btnClose);

    btnOpenCharDetail = new UButton();
    btnOpenCharDetail->SetText(L"상세 보기");
    btnOpenCharDetail->SetLocation(Loc.x - 120, topY + 62);
    btnOpenCharDetail->SetSize(150, 34);
    btnOpenCharDetail->SetFont(18, FW_BOLD, L"Arial");
    btnOpenCharDetail->SetAnimationType(1);
    btnOpenCharDetail->SetOnClick([this]() { this->OpenCharacterPopup(); });
    navLayer->AddChild(btnOpenCharDetail);

    btnOpenInvDetail = new UButton();
    btnOpenInvDetail->SetText(L"가방 보기");
    btnOpenInvDetail->SetLocation(Loc.x + 120, topY + 62);
    btnOpenInvDetail->SetSize(150, 34);
    btnOpenInvDetail->SetFont(18, FW_BOLD, L"Arial");
    btnOpenInvDetail->SetAnimationType(1);
    btnOpenInvDetail->SetOnClick([this]() { this->OpenInventoryPopup(); });
    navLayer->AddChild(btnOpenInvDetail);

    guideLabel = new UTextLabel();
    guideLabel->SetText(L"캐릭터를 선택하면 능력치와 장비 정보를 확인할 수 있습니다.");
    guideLabel->SetLocation(Loc.x, Loc.y - Size.sY / 2 + 181);
    guideLabel->SetSize(Size.sX - 160, 34);
    guideLabel->SetFont(20, FW_BOLD, L"Arial");
    guideLabel->SetColor(RGB(230, 230, 240));
    guideLabel->SetAlign(DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    charLayer->AddChild(guideLabel);

    int invTextLeft = Loc.x - Size.sX / 2 + 105;

    invTitleLabel = new UTextLabel();
    invTitleLabel->SetLocation(invTextLeft + 430, Loc.y - Size.sY / 2 + 184);
    invTitleLabel->SetSize(860, 34);
    invTitleLabel->SetFont(25, FW_BOLD, L"Arial");
    invTitleLabel->SetColor(RGB(238, 232, 214));
    invTitleLabel->SetAlign(DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    invLayer->AddChild(invTitleLabel);

    invGuideLabel = new UTextLabel();
    invGuideLabel->SetText(L"장비 또는 소비 아이템을 선택한 뒤 대상 캐릭터를 고르세요.");
    invGuideLabel->SetLocation(invTextLeft + 430, Loc.y - Size.sY / 2 + 222);
    invGuideLabel->SetSize(860, 30);
    invGuideLabel->SetFont(18, FW_NORMAL, L"Arial");
    invGuideLabel->SetColor(RGB(215, 215, 225));
    invGuideLabel->SetAlign(DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    invLayer->AddChild(invGuideLabel);

    invEmptyLabel = new UTextLabel();
    invEmptyLabel->SetText(L"아이템이 없습니다.");
    invEmptyLabel->SetLocation(invTextLeft + 430, Loc.y - Size.sY / 2 + 277);
    invEmptyLabel->SetSize(860, 30);
    invEmptyLabel->SetFont(18, FW_NORMAL, L"Arial");
    invEmptyLabel->SetColor(RGB(215, 215, 225));
    invEmptyLabel->SetAlign(DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    invEmptyLabel->SetVisible(false);
    invLayer->AddChild(invEmptyLabel);

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
    charLayer->AddChild(profilePanel);

    statePanel = new UStatePanel();
    statePanel->Setup(Loc.x + 80, Loc.y + 85);
    statePanel->SetVisible(false);
    charLayer->AddChild(statePanel);

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
        if (navLayer) {
            navLayer->SetVisible(true);
        }
        SelectTab(currentTab);
    };
    charPopup->SetVisible(false);
    AddChild(charPopup);

    invPopup = new UInventoryPopup();
    invPopup->Setup(clientRect);
    invPopup->OnBackCallback = [this]() {
        if (invPopup) {
            invPopup->SetVisible(false);
        }
        if (navLayer) {
            navLayer->SetVisible(true);
        }
        SelectTab(currentTab);
    };
    invPopup->SetVisible(false);
    AddChild(invPopup);

    for (int i = 0; i < 3; ++i) {
        UButton* btn = new UButton();
        btn->SetSize(240, 54);
        btn->SetVisible(false);
        charLayer->AddChild(btn);
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
        if (charLayer) charLayer->RemoveChild(b, true);
        else RemoveChild(b, true);
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

        if (charLayer) charLayer->AddChild(btn);
        else AddChild(btn);
        charSelectButtons.push_back(btn);
    }

    SelectTab(currentTab);
}

void UMainMenuPopup::UpdateInventoryGrid()
{
    for (auto b : itemIconButtons) {
        if (invLayer) invLayer->RemoveChild(b, true);
        else RemoveChild(b, true);
    }
    itemIconButtons.clear();

    if (currentTab != EMenuTab::Inventory) {
        return;
    }

    int boxLeft = Loc.x - Size.sX / 2 + 105;
    int boxTop = Loc.y - Size.sY / 2 + 245;

    const auto& inventory = GameManager::Get().GetInventory();

    if (invTitleLabel) {
        invTitleLabel->SetText(L"공용 인벤토리    골드: " + std::to_wstring(GameManager::Get().GetGold()));
    }
    if (invEmptyLabel) {
        invEmptyLabel->SetVisible(inventory.empty());
    }

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

        if (invLayer) invLayer->AddChild(btn);
        else AddChild(btn);
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

    UTextLabel* targetTitleLabel = new UTextLabel();
    targetTitleLabel->SetText(L"대상 선택: " + ItemData::GetName(selectedItemID));
    targetTitleLabel->SetLocation(panelCenterX, panelCenterY - 85);
    targetTitleLabel->SetSize(panelW, 36);
    targetTitleLabel->SetFont(24, FW_BOLD, L"Arial");
    targetTitleLabel->SetColor(RGB(238, 232, 214));
    targetTitleLabel->SetAlign(DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    targetSelectPanel->AddChild(targetTitleLabel);

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

    btnOpenChar->SetSelected(currentTab == EMenuTab::CharacterInfo);
    btnOpenInv->SetSelected(currentTab == EMenuTab::Inventory);

    bool showCharLists = (currentTab == EMenuTab::CharacterInfo);
    if (charLayer) {
        charLayer->SetVisible(showCharLists);
    }
    if (invLayer) {
        invLayer->SetVisible(!showCharLists);
    }

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
    if (navLayer) { navLayer->SetVisible(true); }
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

    if (navLayer) { navLayer->SetVisible(false); }
    if (charLayer) { charLayer->SetVisible(false); }
    if (invLayer) { invLayer->SetVisible(false); }

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

    if (navLayer) { navLayer->SetVisible(false); }
    if (charLayer) { charLayer->SetVisible(false); }
    if (invLayer) { invLayer->SetVisible(false); }

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

    if (navLayer) {
        navLayer->SetVisible(true);
    }
    SelectTab(currentTab);
}


bool UMainMenuPopup::HandleBack()
{
    if (exitPopup && exitPopup->GetVisible()) {
        exitPopup->SetVisible(false);
        return true;
    }

    if (charPopup && charPopup->GetVisible()) {
        charPopup->SetVisible(false);
        if (navLayer) { navLayer->SetVisible(true); }
        SelectTab(currentTab);
        return true;
    }

    if (invPopup && invPopup->GetVisible()) {
        invPopup->SetVisible(false);
        if (navLayer) { navLayer->SetVisible(true); }
        SelectTab(currentTab);
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

    // 텍스트/버튼/패널은 Setup에서 만든 레이어 위젯들이 그린다.
    DrawSubMenuBackgrounds(hMemDC);
    UContainerWidget::Draw(hMemDC, hMemDC1);

    if (bShowTooltip) {
        DrawTooltip(hMemDC);
    }
}

void UMainMenuPopup::DrawCharacterInfo(HDC hMemDC)
{
    // 친구식 레이어 구조로 전환됨: guideLabel / UProfilePanel / UStatePanel이 출력 담당.
}

void UMainMenuPopup::DrawInventory(HDC hMemDC)
{
    // 친구식 레이어 구조로 전환됨: invTitleLabel / invGuideLabel / invEmptyLabel / item buttons가 출력 담당.
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
