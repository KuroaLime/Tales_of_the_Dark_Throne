#include "UMainMenuPopup.h"
#include "AArmor.h"
#include "AUsable.h"
#include "resource.h"

UMainMenuPopup::UMainMenuPopup()
    : currentTab(EMenuTab::CharacterInfo), invState(EInventoryState::Normal), 
      selectedCharacter(nullptr), selectedItem(nullptr), hBgBitmap(NULL), exitPopup(nullptr),
      bShowTooltip(false)
{
    bVisible = false;
    itemActionPanel = nullptr;
    targetSelectPanel = nullptr;
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

    int topY = 60;
    int btnW = 180;
    int btnH = 50;
    int spacing = 20;

    int groupWidth = (btnW * 3) + (spacing * 2);
    int groupStartX = Loc.x - (groupWidth / 2);

    btnTabChar = new UButton();
    btnTabChar->SetText(L"CHARACTER");
    btnTabChar->SetLocation(groupStartX + (btnW / 2), topY);
    btnTabChar->SetSize(btnW, btnH);
    btnTabChar->SetOnClick([this]() { this->SelectTab(EMenuTab::CharacterInfo); });
    btnTabChar->SetSelected(true); 
    AddChild(btnTabChar);

    btnTabInv = new UButton();
    btnTabInv->SetText(L"INVENTORY");
    btnTabInv->SetLocation(groupStartX + btnW + spacing + (btnW / 2), topY);
    btnTabInv->SetSize(btnW, btnH);
    btnTabInv->SetOnClick([this]() { this->SelectTab(EMenuTab::Inventory); });
    AddChild(btnTabInv);

    btnExit = new UButton();
    btnExit->SetText(L"EXIT");
    btnExit->SetLocation(groupStartX + (btnW * 2) + (spacing * 2) + (btnW / 2), topY);
    btnExit->SetSize(btnW, btnH);
    btnExit->SetOnClick([this]() { 
        if (exitPopup) exitPopup->SetVisible(true);
    });
    AddChild(btnExit);

    btnClose = new UButton();
    btnClose->SetText(L"X");
    btnClose->SetLocation(clientRect.right - 50, topY);
    btnClose->SetSize(50, 50);
    btnClose->SetOnClick([this]() { this->SetVisible(false); });
    AddChild(btnClose);

    // 아이템 액션 패널 (사각형 서브 메뉴)
    itemActionPanel = new UContainerWidget();
    itemActionPanel->SetSize(160, 100);
    itemActionPanel->SetVisible(false);
    AddChild(itemActionPanel);

    btnItemAction1 = new UButton();
    btnItemAction1->SetSize(140, 40);
    btnItemAction1->SetLocation(80, 25);
    itemActionPanel->AddChild(btnItemAction1);

    btnItemAction2 = new UButton();
    btnItemAction2->SetText(L"CANCEL");
    btnItemAction2->SetSize(140, 40);
    btnItemAction2->SetLocation(80, 75);
    btnItemAction2->SetOnClick([this]() { this->HideSubMenus(); });
    itemActionPanel->AddChild(btnItemAction2);

    // 타겟 선택 패널
    targetSelectPanel = new UContainerWidget();
    targetSelectPanel->SetSize(600, 200);
    targetSelectPanel->SetLocation(Loc.x, Loc.y);
    targetSelectPanel->SetVisible(false);
    AddChild(targetSelectPanel);

    exitPopup = new UExitConfirmPopup();
    exitPopup->Setup(hInst, clientRect);
    exitPopup->SetOnYes([]() { PostQuitMessage(0); });
    exitPopup->SetOnNo([this]() { exitPopup->SetVisible(false); });
    AddChild(exitPopup);

    // 장비 슬롯 버튼 초기화
    for (int i = 0; i < 5; ++i) {
        UButton* btn = new UButton();
        btn->SetSize(64, 64);
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
    if (!teamMembers.empty() && !selectedCharacter) {
        selectedCharacter = teamMembers[0];
    }
    UpdateCharList();
}

void UMainMenuPopup::UpdateCharList()
{
    for (auto b : charSelectButtons) RemoveChild(b, true);
    charSelectButtons.clear();
    
    int boxRight = Loc.x + Size.sX / 2 - 50;
    int boxTop = Loc.y - Size.sY / 2 + 120;
    
    int btnW = 180;
    int listStartX = boxRight - (btnW / 2) - 20; 
    int listStartY = boxTop + 50;

    for (size_t i = 0; i < teamMembers.size(); ++i) {
        UButton* btn = new UButton();
        btn->SetText(teamMembers[i]->GetName()); 
        btn->SetLocation(listStartX, listStartY + (int)i * 60);
        btn->SetSize(btnW, 50);
        
        Character* charPtr = teamMembers[i];
        btn->SetOnClick([this, charPtr, btn]() { 
            this->SelectCharacter(charPtr); 
            for (auto b : charSelectButtons) b->SetSelected(false);
            btn->SetSelected(true);
        });

        if (charPtr == selectedCharacter) btn->SetSelected(true);

        AddChild(btn);
        charSelectButtons.push_back(btn);
    }
    
    SelectTab(currentTab);
}

void UMainMenuPopup::UpdateInventoryGrid()
{
    for (auto b : itemIconButtons) RemoveChild(b, true);
    itemIconButtons.clear();

    if (currentTab != EMenuTab::Inventory) return;

    int boxLeft = Loc.x - Size.sX / 2 + 80;
    int boxTop = Loc.y - Size.sY / 2 + 180;
    
    const auto& inventory = TeamManager::Get().GetInventory();
    
    int colCount = 8;
    int slotSize = 80;
    int spacing = 15;

    for (size_t i = 0; i < inventory.size(); ++i) {
        AItemBase* item = inventory[i];
        int col = (int)i % colCount;
        int row = (int)i / colCount;

        UButton* btn = new UButton();
        btn->SetLocation(boxLeft + col * (slotSize + spacing), boxTop + row * (slotSize + spacing));
        btn->SetSize(slotSize, slotSize);
        
        btn->SetOnHover([this, item, btn]() {
            this->ShowTooltip(item->GetName(), item->GetDescription(), btn->GetLocation());
        });
        btn->SetOnUnhover([this]() { this->HideTooltip(); });

        btn->SetOnClick([this, item, btn]() { 
            Location loc = btn->GetLocation();
            this->ShowActionMenu(item, loc.x, loc.y); 
        });
        
        AddChild(btn);
        itemIconButtons.push_back(btn);
    }
}

void UMainMenuPopup::UpdateEquipmentSlots()
{
    if (!selectedCharacter || currentTab != EMenuTab::CharacterInfo) {
        for (auto b : equipSlotButtons) b->SetVisible(false);
        return;
    }

    int boxLeft = Loc.x - Size.sX / 2 + 50;
    int boxTop = Loc.y - Size.sY / 2 + 120;
    int col1StartX = boxLeft + 30;
    int spriteCenterX = col1StartX + 64; // Approx center of 128px sprite
    
    int slotX_Left = spriteCenterX - 100;
    int slotX_Right = spriteCenterX + 100;
    int startY = boxTop + 60;

    CharacterEquipment equip = selectedCharacter->GetEquipment();
    AArmor* items[5] = { equip.Helmet, equip.Armor, equip.Pants, equip.Weapon, equip.Accessory };
    std::wstring slotNames[5] = { L"HELMET", L"ARMOR", L"PANTS", L"WEAPON", L"ACCESSORY" };

    for (int i = 0; i < 5; ++i) {
        UButton* btn = equipSlotButtons[i];
        btn->SetVisible(true);
        btn->SetLocation((i < 3) ? slotX_Left : slotX_Right, startY + (i % 3) * 80);
        
        AArmor* item = items[i];
        if (item) {
            btn->SetText(L"");
            btn->SetOnHover([this, item, btn]() {
                CharacterStat bonus = item->GetStatBonus();
                std::wstring statStr = L"STR+" + std::to_wstring(bonus.STR) + L" DEF+" + std::to_wstring(bonus.DEF);
                this->ShowTooltip(item->GetName(), item->GetDescription() + L"\n" + statStr, btn->GetLocation());
            });
            btn->SetOnRightClick([this, item]() {
                TeamManager::Get().UnequipArmor(selectedCharacter, item->GetPart());
                this->UpdateEquipmentSlots();
            });
        } else {
            btn->SetText(slotNames[i].substr(0, 1)); // Show first letter as placeholder
            btn->SetOnHover([this, slotNames, i, btn]() {
                this->ShowTooltip(slotNames[i], L"No item equipped.", btn->GetLocation());
            });
            btn->SetOnRightClick(nullptr);
        }
        btn->SetOnUnhover([this]() { this->HideTooltip(); });
    }
}

void UMainMenuPopup::ShowActionMenu(AItemBase* item, int x, int y)
{
    selectedItem = item;
    invState = EInventoryState::ActionChoice;
    
    itemActionPanel->SetLocation(x + 100, y); // 아이템 버튼 옆에 표시
    itemActionPanel->SetVisible(true);

    if (item->GetItemType() == EItemType::Armor) {
        btnItemAction1->SetText(L"EQUIP");
        btnItemAction1->SetOnClick([this]() { this->ShowTargetSelection(); });
    } else {
        btnItemAction1->SetText(L"USE");
        btnItemAction1->SetOnClick([this]() { 
            MessageBoxW(NULL, L"전투 중에 사용 가능합니다.", L"알림", MB_OK);
            this->HideSubMenus();
        });
    }
}

void UMainMenuPopup::ShowTargetSelection()
{
    invState = EInventoryState::TargetSelect;
    itemActionPanel->SetVisible(false);
    targetSelectPanel->ClearChildren(true);
    targetButtons.clear();

    int startX = 80;
    for (size_t i = 0; i < teamMembers.size(); ++i) {
        Character* target = teamMembers[i];
        UButton* btn = new UButton();
        btn->SetText(target->GetName());
        btn->SetLocation(startX + (int)i * 150, 100);
        btn->SetSize(120, 120);
        
        btn->SetOnClick([this, target]() {
            if (AArmor* armor = dynamic_cast<AArmor*>(selectedItem)) {
                TeamManager::Get().EquipArmor(target, armor);
                MessageBoxW(NULL, (target->GetName() + L"에게 장착했습니다.").c_str(), L"장착 완료", MB_OK);
            }
            this->HideSubMenus();
            this->UpdateInventoryGrid();
            this->UpdateEquipmentSlots();
        });
        targetSelectPanel->AddChild(btn);
        targetButtons.push_back(btn);
    }
    targetSelectPanel->SetVisible(true);
}

void UMainMenuPopup::HideSubMenus()
{
    if (itemActionPanel) itemActionPanel->SetVisible(false);
    if (targetSelectPanel) targetSelectPanel->SetVisible(false);
    invState = EInventoryState::Normal;
}

void UMainMenuPopup::SelectTab(EMenuTab tab)
{
    currentTab = tab;
    invState = EInventoryState::Normal;
    HideSubMenus();

    btnTabChar->SetSelected(currentTab == EMenuTab::CharacterInfo);
    btnTabInv->SetSelected(currentTab == EMenuTab::Inventory);

    bool showCharLists = (currentTab == EMenuTab::CharacterInfo);
    for (auto btn : charSelectButtons) btn->SetVisible(showCharLists);

    if (currentTab == EMenuTab::Inventory) {
        UpdateInventoryGrid();
        for (auto b : equipSlotButtons) b->SetVisible(false);
    } else {
        for (auto b : itemIconButtons) b->SetVisible(false);
        UpdateEquipmentSlots();
    }
}

void UMainMenuPopup::SelectCharacter(Character* character)
{
    selectedCharacter = character;
    UpdateEquipmentSlots();
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
    if (!bVisible) return;

    BLENDFUNCTION bf;
    bf.BlendOp = AC_SRC_OVER;
    bf.BlendFlags = 0;
    bf.SourceConstantAlpha = 200;
    bf.AlphaFormat = 0;

    if (hBgBitmap) {
        HDC hTempDC = CreateCompatibleDC(hMemDC);
        HBITMAP hOldBit = (HBITMAP)SelectObject(hTempDC, hBgBitmap);
        AlphaBlend(hMemDC, Loc.x - Size.sX / 2, Loc.y - Size.sY / 2, Size.sX, Size.sY,
                   hTempDC, 0, 0, bgInfo.bmWidth, bgInfo.bmHeight, bf);
        SelectObject(hTempDC, hOldBit);
        DeleteDC(hTempDC);
    }

    RECT r = { Loc.x - Size.sX / 2 + 50, Loc.y - Size.sY / 2 + 120, Loc.x + Size.sX / 2 - 50, Loc.y + Size.sY / 2 - 50 };
    HBRUSH borderBrush = CreateSolidBrush(RGB(150, 150, 150));
    FrameRect(hMemDC, &r, borderBrush);
    DeleteObject(borderBrush);

    if (currentTab == EMenuTab::CharacterInfo) {
        DrawCharacterInfo(hMemDC);
    } else {
        DrawInventory(hMemDC);
    }

    UContainerWidget::Draw(hMemDC, hMemDC1);
    
    if (bShowTooltip) DrawTooltip(hMemDC);

    // 서브 메뉴 배경 (불투명)
    if (itemActionPanel && itemActionPanel->GetVisible()) {
        Location pLoc = itemActionPanel->GetLocation();
        BuSIZE pSize = itemActionPanel->GetSize();
        RECT pr = { pLoc.x - pSize.sX / 2, pLoc.y - pSize.sY / 2, pLoc.x + pSize.sX / 2, pLoc.y + pSize.sY / 2 };
        HBRUSH hB = CreateSolidBrush(RGB(50, 50, 70));
        FillRect(hMemDC, &pr, hB);
        DeleteObject(hB);
        FrameRect(hMemDC, &pr, (HBRUSH)GetStockObject(WHITE_BRUSH));
    }
}

void UMainMenuPopup::DrawCharacterInfo(HDC hMemDC)
{
    if (!selectedCharacter) return;

    int boxLeft = Loc.x - Size.sX / 2 + 50;
    int boxRight = Loc.x + Size.sX / 2 - 50;
    int boxTop = Loc.y - Size.sY / 2 + 120;
    
    int infoAreaEndX = boxRight - 220; 
    int col1Width = (infoAreaEndX - boxLeft) * 0.4;
    int col1StartX = boxLeft + 30;
    int col2StartX = boxLeft + col1Width + 120; // Pushed right for equipment slots

    HBITMAP hSprite = selectedCharacter->GetSpriteIdle();
    if (hSprite) {
        BITMAP bm;
        GetObject(hSprite, sizeof(BITMAP), &bm);
        HDC hSpriteDC = CreateCompatibleDC(hMemDC);
        HBITMAP hOld = (HBITMAP)SelectObject(hSpriteDC, hSprite);
        int frameW = bm.bmWidth / CHARACTER_SHEET_COL;
        int frameH = bm.bmHeight / CHARACTER_SHEET_ROW;
        TransparentBlt(hMemDC, col1StartX, boxTop + 40, frameW * 2, frameH * 2,
                       hSpriteDC, 0, 0, frameW, frameH, RGB(255, 0, 255));
        SelectObject(hSpriteDC, hOld);
        DeleteDC(hSpriteDC);
    }

    SetTextColor(hMemDC, RGB(255, 255, 255));
    SetBkMode(hMemDC, TRANSPARENT);
    
    HFONT hFont = CreateFont(28, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, L"Arial");
    HFONT oldF = (HFONT)SelectObject(hMemDC, hFont);
    TextOutW(hMemDC, col1StartX + 10, boxTop + 220, selectedCharacter->GetName().c_str(), (int)selectedCharacter->GetName().length());
    DeleteObject(hFont);

    hFont = CreateFont(20, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, L"Arial");
    SelectObject(hMemDC, hFont);
    CharacterStat stat = selectedCharacter->GetStat();
    int statY = boxTop + 50;
    TextOutW(hMemDC, col2StartX, statY, (L"HP: " + std::to_wstring(stat.CurHP) + L" / " + std::to_wstring(stat.MaxHP)).c_str(), 15);
    TextOutW(hMemDC, col2StartX, statY + 30, (L"MP: " + std::to_wstring(stat.CurMP) + L" / " + std::to_wstring(stat.MaxMP)).c_str(), 15);
    TextOutW(hMemDC, col2StartX, statY + 60, (L"STR: " + std::to_wstring(stat.STR)).c_str(), 10);
    TextOutW(hMemDC, col2StartX, statY + 90, (L"INT: " + std::to_wstring(stat.INT)).c_str(), 10);
    TextOutW(hMemDC, col2StartX, statY + 120, (L"DEF: " + std::to_wstring(stat.DEF)).c_str(), 10);
    TextOutW(hMemDC, col2StartX, statY + 150, (L"SPD: " + std::to_wstring(stat.SPD)).c_str(), 10);
    
    DeleteObject(hFont);
    SelectObject(hMemDC, oldF);
}

void UMainMenuPopup::DrawInventory(HDC hMemDC)
{
    int boxLeft = Loc.x - Size.sX / 2 + 80;
    int boxTop = Loc.y - Size.sY / 2 + 150;
    
    SetTextColor(hMemDC, RGB(255, 255, 255));
    SetBkMode(hMemDC, TRANSPARENT);
    
    HFONT hFont = CreateFont(24, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, L"Arial");
    HFONT oldF = (HFONT)SelectObject(hMemDC, hFont);
    TextOutW(hMemDC, boxLeft, boxTop - 40, L"TEAM INVENTORY", 14);
    
    if (TeamManager::Get().GetInventory().empty()) {
        TextOutW(hMemDC, boxLeft, boxTop + 50, L"No Items Found.", 15);
    }

    SelectObject(hMemDC, oldF);
    DeleteObject(hFont);
}

void UMainMenuPopup::DrawTooltip(HDC hMemDC)
{
    int tw = 200;
    int th = 100;
    int tx = tooltipLoc.x + 40;
    int ty = tooltipLoc.y - 50;

    RECT tr = { tx, ty, tx + tw, ty + th };
    HBRUSH hB = CreateSolidBrush(RGB(30, 30, 30));
    FillRect(hMemDC, &tr, hB);
    DeleteObject(hB);
    FrameRect(hMemDC, &tr, (HBRUSH)GetStockObject(WHITE_BRUSH));

    SetTextColor(hMemDC, RGB(255, 255, 255));
    HFONT hFont = CreateFont(18, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, L"Arial");
    HFONT oldF = (HFONT)SelectObject(hMemDC, hFont);
    TextOutW(hMemDC, tx + 10, ty + 10, tooltipTitle.c_str(), (int)tooltipTitle.length());
    DeleteObject(hFont);

    hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, L"Arial");
    SelectObject(hMemDC, hFont);
    RECT textR = { tx + 10, ty + 40, tx + tw - 10, ty + th - 10 };
    DrawTextW(hMemDC, tooltipDesc.c_str(), -1, &textR, DT_LEFT | DT_WORDBREAK);
    
    SelectObject(hMemDC, oldF);
    DeleteObject(hFont);
}
