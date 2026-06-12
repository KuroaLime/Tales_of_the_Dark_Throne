#include "UInventoryPopup.h"
#include "GameManager.h"
#include "ItemData.h"

UInventoryPopup::UInventoryPopup() : bShowTooltip(false) {
    bVisible = false;
}

UInventoryPopup::~UInventoryPopup() {}

void UInventoryPopup::Init() {}

void UInventoryPopup::Setup(RECT clientRect) {
    Size.sX = clientRect.right;
    Size.sY = clientRect.bottom;
    Loc.x = clientRect.right / 2;
    Loc.y = clientRect.bottom / 2;

    UButton* btnBack = new UButton();
    btnBack->SetText(L"BACK");
    btnBack->SetSize(100, 50);
    btnBack->SetLocation(clientRect.right - 80, 60);
    btnBack->SetOnClick([this]() { if (OnBackCallback) OnBackCallback(); });
    AddChild(btnBack);

    UpdateInventoryGrid();
}

void UInventoryPopup::SetTeamData(const std::vector<Character*>& team) {
    // Original's inventory is shared, so team data isn't strictly needed for the grid itself
    // but we keep it for compatibility with the caller.
    Refresh();
}

void UInventoryPopup::Refresh() {
    UpdateInventoryGrid();
}

void UInventoryPopup::UpdateInventoryGrid() {
    for (auto b : itemIconButtons) RemoveChild(b, true);
    itemIconButtons.clear();

    int boxLeft = Loc.x - 400;
    int boxTop = Loc.y - 200;
    const auto& inventory = GameManager::Get().GetInventory();
    int colCount = 8;
    int slotSize = 80;
    int spacing = 15;

    for (size_t i = 0; i < inventory.size(); ++i) {
        int itemID = inventory[i].ItemID;
        int count = inventory[i].Count;

        int col = (int)i % colCount;
        int row = (int)i / colCount;

        UButton* btn = new UButton();
        btn->SetLocation(boxLeft + col * (slotSize + spacing), boxTop + row * (slotSize + spacing));
        btn->SetSize(slotSize, slotSize);
        
        std::wstring name = ItemData::GetName(itemID);
        std::wstring btnText = name.substr(0, 2);
        if (count > 1) btnText += L" x" + std::to_wstring(count);
        btn->SetText(btnText);
        
        btn->SetOnHover([this, itemID, name, btn]() {
            this->ShowTooltip(name, ItemData::GetDescription(itemID), btn->GetLocation());
        });
        btn->SetOnUnhover([this]() { this->HideTooltip(); });

        AddChild(btn);
        itemIconButtons.push_back(btn);
    }
}

void UInventoryPopup::Update() {
    UContainerWidget::Update();
}

void UInventoryPopup::Draw(HDC hMemDC, HDC hMemDC1) {
    if (!bVisible) return;
    
    // Draw semi-transparent background for inventory
    RECT r = { Loc.x - 450, Loc.y - 250, Loc.x + 450, Loc.y + 250 };
    HBRUSH hB = CreateSolidBrush(RGB(20, 20, 40));
    FillRect(hMemDC, &r, hB);
    DeleteObject(hB);

    UContainerWidget::Draw(hMemDC, hMemDC1);
    if (bShowTooltip) DrawTooltip(hMemDC);
}

void UInventoryPopup::ShowTooltip(const std::wstring& title, const std::wstring& desc, Location loc) {
    bShowTooltip = true; tooltipTitle = title; tooltipDesc = desc; tooltipLoc = loc;
}

void UInventoryPopup::HideTooltip() { bShowTooltip = false; }

void UInventoryPopup::DrawTooltip(HDC hMemDC) {
    int tw = 200;
    int th = 100;
    int tx = tooltipLoc.x + 40; 
    int ty = tooltipLoc.y - 50;

    RECT tr = { tx, ty, tx + tw, ty + th };
    HBRUSH hB = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(hMemDC, &tr, hB);
    DeleteObject(hB);
    
    HPEN hP = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
    HPEN oldP = (HPEN)SelectObject(hMemDC, hP);
    MoveToEx(hMemDC, tr.left, tr.top, NULL);
    LineTo(hMemDC, tr.right, tr.top);
    LineTo(hMemDC, tr.right, tr.bottom);
    LineTo(hMemDC, tr.left, tr.bottom);
    LineTo(hMemDC, tr.left, tr.top);
    SelectObject(hMemDC, oldP);
    DeleteObject(hP);

    SetTextColor(hMemDC, RGB(255, 255, 255));
    SetBkMode(hMemDC, TRANSPARENT);
    
    RECT titleRect = { tx + 5, ty + 5, tx + tw - 5, ty + 30 };
    DrawTextW(hMemDC, tooltipTitle.c_str(), -1, &titleRect, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);

    RECT descRect = { tx + 5, ty + 35, tx + tw - 5, ty + th - 5 };
    DrawTextW(hMemDC, tooltipDesc.c_str(), -1, &descRect, DT_LEFT | DT_WORDBREAK);
}
