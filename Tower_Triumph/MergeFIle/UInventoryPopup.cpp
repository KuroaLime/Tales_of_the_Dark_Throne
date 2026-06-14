#include "UInventoryPopup.h"

UInventoryPopup::UInventoryPopup()
    : TargetPanel(nullptr),
      BackButton(nullptr),
      SelectedItemID(-1),
      bShowTooltip(false)
{
    bVisible = false;
}

UInventoryPopup::~UInventoryPopup()
{
}

void UInventoryPopup::Init()
{
}

void UInventoryPopup::Setup(RECT clientRect)
{
    Size.sX = clientRect.right;
    Size.sY = clientRect.bottom;
    Loc.x = clientRect.right / 2;
    Loc.y = clientRect.bottom / 2;

    BackButton = new UButton();
    BackButton->SetText(L"뒤로");
    BackButton->SetSize(120, 46);
    BackButton->SetLocation(clientRect.right - 90, 58);
    BackButton->SetFont(18, FW_BOLD, L"Malgun Gothic");
    BackButton->SetAnimationType(1);
    BackButton->SetOnClick([this]() {
        HideTargetSelection();
        if (OnBackCallback)
        {
            OnBackCallback();
        }
        else
        {
            SetVisible(false);
        }
    });
    AddChild(BackButton);

    TargetPanel = new UContainerWidget();
    TargetPanel->SetSize(720, 240);
    TargetPanel->SetLocation(Loc.x, Loc.y + 120);
    TargetPanel->SetVisible(false);
    AddChild(TargetPanel);

    UpdateInventoryGrid();
}

void UInventoryPopup::SetTeamData(const std::vector<Character*>& team)
{
    TeamMembers = team;
}

void UInventoryPopup::Refresh()
{
    HideTargetSelection();
    UpdateInventoryGrid();
}

void UInventoryPopup::UpdateInventoryGrid()
{
    for (UButton* button : ItemButtons)
    {
        RemoveChild(button, true);
    }
    ItemButtons.clear();

    const std::vector<InventoryItem>& inventory = GameManager::Get().GetInventory();

    int colCount = 5;
    int slotW = 170;
    int slotH = 64;
    int spacingX = 22;
    int spacingY = 20;
    int startX = Loc.x - ((colCount * slotW + (colCount - 1) * spacingX) / 2) + slotW / 2;
    int startY = Loc.y - 90;

    for (size_t i = 0; i < inventory.size(); ++i)
    {
        int itemID = inventory[i].ItemID;
        int count = inventory[i].Count;
        int col = (int)i % colCount;
        int row = (int)i / colCount;

        UButton* button = new UButton();
        button->SetText(MakeItemButtonText(itemID, count));
        button->SetSize(slotW, slotH);
        button->SetLocation(startX + col * (slotW + spacingX), startY + row * (slotH + spacingY));
        button->SetFont(16, FW_BOLD, L"Malgun Gothic");
        button->SetAnimationType(0);
        button->SetOnHover([this, itemID, button]() {
            ShowTooltip(ItemData::GetName(itemID), MakeItemTooltip(itemID), button->GetLocation());
        });
        button->SetOnUnhover([this]() { HideTooltip(); });
        button->SetOnClick([this, itemID]() {
            if (ItemData::IsEquipment(itemID) || ItemData::IsConsumable(itemID))
            {
                ShowTargetSelection(itemID);
            }
        });

        AddChild(button);
        ItemButtons.push_back(button);
    }
}

void UInventoryPopup::ShowTargetSelection(int itemID)
{
    SelectedItemID = itemID;
    HideTooltip();

    if (TargetPanel == nullptr)
    {
        return;
    }

    TargetPanel->ClearChildren(true);
    TargetButtons.clear();

    int panelX = Loc.x;
    int panelY = Loc.y + 120;

    const int panelW = 820;
    const int panelH = 270;
    const int colCount = 3;
    const int btnW = 160;
    const int btnH = 58;
    const int gapX = 42;
    const int gapY = 22;

    TargetPanel->SetSize(panelW, panelH);
    TargetPanel->SetLocation(panelX, panelY);

    int totalW = colCount * btnW + (colCount - 1) * gapX;
    int startX = panelX - totalW / 2 + btnW / 2;
    int startY = panelY + 18;

    for (size_t i = 0; i < TeamMembers.size(); ++i)
    {
        Character* target = TeamMembers[i];
        UButton* button = new UButton();
        button->SetText(target ? target->GetName() : L"None");

        int col = (int)i % colCount;
        int row = (int)i / colCount;

        button->SetSize(btnW, btnH);
        button->SetLocation(startX + col * (btnW + gapX), startY + row * (btnH + gapY));
        button->SetFont(15, FW_BOLD, L"Malgun Gothic");
        button->SetAnimationType(0);
        button->SetOnClick([this, target]() {
            if (target == nullptr || SelectedItemID < 0)
            {
                HideTargetSelection();
                return;
            }

            if (ItemData::IsEquipment(SelectedItemID))
            {
                GameManager::Get().EquipItem(target, SelectedItemID);
            }
            else if (ItemData::IsConsumable(SelectedItemID))
            {
                GameManager::Get().UseConsumable(target, SelectedItemID);
            }

            Refresh();
        });

        TargetPanel->AddChild(button);
        TargetButtons.push_back(button);
    }

    int cancelIndex = (int)TeamMembers.size();
    int cancelCol = cancelIndex % colCount;
    int cancelRow = cancelIndex / colCount;

    UButton* cancel = new UButton();
    cancel->SetText(L"취소");
    cancel->SetSize(btnW, btnH);
    cancel->SetLocation(startX + cancelCol * (btnW + gapX), startY + cancelRow * (btnH + gapY));
    cancel->SetFont(15, FW_BOLD, L"Malgun Gothic");
    cancel->SetAnimationType(0);
    cancel->SetOnClick([this]() { HideTargetSelection(); });
    TargetPanel->AddChild(cancel);
    TargetButtons.push_back(cancel);

    RemoveChild(TargetPanel, false);
    AddChild(TargetPanel);
    TargetPanel->SetVisible(true);
}

void UInventoryPopup::HideTargetSelection()
{
    SelectedItemID = -1;
    if (TargetPanel)
    {
        TargetPanel->SetVisible(false);
        TargetPanel->ClearChildren(true);
    }
    TargetButtons.clear();
}

void UInventoryPopup::Update()
{
    if (!bVisible)
    {
        return;
    }

    UContainerWidget::Update();
}

void UInventoryPopup::Draw(HDC hMemDC, HDC hMemDC1)
{
    if (!bVisible)
    {
        return;
    }

    RECT bg = { Loc.x - Size.sX / 2, Loc.y - Size.sY / 2, Loc.x + Size.sX / 2, Loc.y + Size.sY / 2 };
    HBRUSH bgBrush = CreateSolidBrush(RGB(8, 8, 12));
    FillRect(hMemDC, &bg, bgBrush);
    DeleteObject(bgBrush);

    RECT panel = { Loc.x - 540, Loc.y - 270, Loc.x + 540, Loc.y + 280 };
    HBRUSH panelBrush = CreateSolidBrush(RGB(19, 23, 31));
    FillRect(hMemDC, &panel, panelBrush);
    DeleteObject(panelBrush);

    HBRUSH frameBrush = CreateSolidBrush(RGB(176, 146, 78));
    FrameRect(hMemDC, &panel, frameBrush);
    DeleteObject(frameBrush);

    SetBkMode(hMemDC, TRANSPARENT);
    SetTextColor(hMemDC, RGB(255, 230, 120));
    HFONT titleFont = CreateFontW(34, 0, 0, 0, FW_HEAVY, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, L"Arial");
    HFONT oldFont = (HFONT)SelectObject(hMemDC, titleFont);
    RECT title = { 0, 20, Size.sX, 70 };
    DrawTextW(hMemDC, L"인벤토리", -1, &title, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hMemDC, oldFont);
    DeleteObject(titleFont);

    HFONT normalFont = CreateFontW(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, L"Arial");
    oldFont = (HFONT)SelectObject(hMemDC, normalFont);
    SetTextColor(hMemDC, RGB(225, 225, 235));
    std::wstring goldText = L"골드: " + std::to_wstring(GameManager::Get().GetGold());
    TextOutW(hMemDC, Loc.x - 500, Loc.y - 235, goldText.c_str(), (int)goldText.length());
    std::wstring guideText = L"장비 또는 소비 아이템을 선택한 뒤 대상을 고르세요.";
    TextOutW(hMemDC, Loc.x - 500, Loc.y - 210, guideText.c_str(), (int)guideText.length());

    if (GameManager::Get().GetInventory().empty())
    {
        std::wstring emptyText = L"아이템이 없습니다.";
        TextOutW(hMemDC, Loc.x - 80, Loc.y - 20, emptyText.c_str(), (int)emptyText.length());
    }

    SelectObject(hMemDC, oldFont);
    DeleteObject(normalFont);

    bool bTargetVisible = (TargetPanel != nullptr && TargetPanel->GetVisible());
    if (bTargetVisible)
    {
        TargetPanel->SetVisible(false);
    }

    UContainerWidget::Draw(hMemDC, hMemDC1);

    if (bTargetVisible)
    {
        TargetPanel->SetVisible(true);
        DrawTargetPanelBackground(hMemDC);
        TargetPanel->Draw(hMemDC, hMemDC1);
    }

    if (bShowTooltip)
    {
        DrawTooltip(hMemDC);
    }
}

void UInventoryPopup::DrawTargetPanelBackground(HDC hMemDC)
{
    if (TargetPanel == nullptr || !TargetPanel->GetVisible())
    {
        return;
    }

    Location loc = TargetPanel->GetLocation();
    BuSIZE size = TargetPanel->GetSize();
    RECT rc = { loc.x - size.sX / 2, loc.y - size.sY / 2, loc.x + size.sX / 2, loc.y + size.sY / 2 };

    HBRUSH bgBrush = CreateSolidBrush(RGB(28, 30, 44));
    FillRect(hMemDC, &rc, bgBrush);
    DeleteObject(bgBrush);

    HBRUSH frameBrush = CreateSolidBrush(RGB(176, 146, 78));
    FrameRect(hMemDC, &rc, frameBrush);
    DeleteObject(frameBrush);

    SetBkMode(hMemDC, TRANSPARENT);
    SetTextColor(hMemDC, RGB(255, 230, 120));
    HFONT font = CreateFontW(24, 0, 0, 0, FW_HEAVY, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, L"Arial");
    HFONT oldFont = (HFONT)SelectObject(hMemDC, font);
    std::wstring title = L"대상 선택: " + ItemData::GetName(SelectedItemID);
    RECT titleRc = { rc.left, rc.top + 22, rc.right, rc.top + 58 };
    DrawTextW(hMemDC, title.c_str(), -1, &titleRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hMemDC, oldFont);
    DeleteObject(font);
}

void UInventoryPopup::ShowTooltip(const std::wstring& title, const std::wstring& desc, Location loc)
{
    bShowTooltip = true;
    TooltipTitle = title;
    TooltipDesc = desc;
    TooltipLoc = loc;
}

void UInventoryPopup::HideTooltip()
{
    bShowTooltip = false;
}

void UInventoryPopup::DrawTooltip(HDC hMemDC)
{
    int tx = TooltipLoc.x + 45;
    int ty = TooltipLoc.y - 55;
    RECT rc = { tx, ty, tx + 300, ty + 150 };

    if (rc.right > Size.sX - 10)
    {
        tx = TooltipLoc.x - 345;
        rc = { tx, ty, tx + 300, ty + 150 };
    }

    HBRUSH bgBrush = CreateSolidBrush(RGB(19, 23, 31));
    FillRect(hMemDC, &rc, bgBrush);
    DeleteObject(bgBrush);

    HBRUSH frameBrush = CreateSolidBrush(RGB(176, 146, 78));
    FrameRect(hMemDC, &rc, frameBrush);
    DeleteObject(frameBrush);

    SetBkMode(hMemDC, TRANSPARENT);
    SetTextColor(hMemDC, RGB(255, 230, 120));
    RECT titleRc = { rc.left + 10, rc.top + 8, rc.right - 10, rc.top + 34 };
    DrawTextW(hMemDC, TooltipTitle.c_str(), -1, &titleRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    SetTextColor(hMemDC, RGB(230, 230, 235));
    RECT descRc = { rc.left + 10, rc.top + 40, rc.right - 10, rc.bottom - 8 };
    DrawTextW(hMemDC, TooltipDesc.c_str(), -1, &descRc, DT_LEFT | DT_WORDBREAK);
}

std::wstring UInventoryPopup::MakeItemButtonText(int itemID, int count) const
{
    std::wstring text = ItemData::GetName(itemID);

    if (count > 1)
    {
        text += L" x" + std::to_wstring(count);
    }

    if (text.length() > 18)
    {
        text = text.substr(0, 17) + L"...";
    }

    return text;
}

std::wstring UInventoryPopup::MakeItemTooltip(int itemID) const
{
    std::wstring text = ItemData::GetDescription(itemID);
    SItemStatBonus bonus = ItemData::GetBonus(itemID);

    if (ItemData::IsEquipment(itemID))
    {
        text += L"\n";
        text += L"HP+" + std::to_wstring(bonus.MaxHP) + L" MP+" + std::to_wstring(bonus.MaxMP);
        text += L"\nSTR+" + std::to_wstring(bonus.STR) + L" INT+" + std::to_wstring(bonus.INT);
        text += L"\nDEF+" + std::to_wstring(bonus.DEF) + L" SPD+" + std::to_wstring(bonus.SPD);
    }
    else if (ItemData::IsConsumable(itemID))
    {
        text += L"\nHeal HP: " + std::to_wstring(ItemData::GetHealHP(itemID));
        text += L"\nHeal MP: " + std::to_wstring(ItemData::GetHealMP(itemID));
    }

    return text;
}
