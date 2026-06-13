#include "UInventoryPopup.h"

UInventoryPopup::UInventoryPopup()
    : targetPanel(nullptr),
      backButton(nullptr),
      titleLabel(nullptr),
      goldLabel(nullptr),
      guideLabel(nullptr),
      emptyLabel(nullptr),
      selectedItemID(-1),
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

    titleLabel = new UTextLabel();
    titleLabel->SetText(L"인벤토리");
    titleLabel->SetLocation(Loc.x, 45);
    titleLabel->SetSize(Size.sX, 50);
    titleLabel->SetFont(34, FW_HEAVY, L"Arial");
    titleLabel->SetColor(RGB(255, 230, 120));
    titleLabel->SetAlign(DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    AddChild(titleLabel);

    goldLabel = new UTextLabel();
    goldLabel->SetLocation(Loc.x - 70, Loc.y - 226);
    goldLabel->SetSize(860, 28);
    goldLabel->SetFont(18, FW_BOLD, L"Arial");
    goldLabel->SetColor(RGB(225, 225, 235));
    goldLabel->SetAlign(DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    AddChild(goldLabel);

    guideLabel = new UTextLabel();
    guideLabel->SetText(L"장비 또는 소비 아이템을 선택한 뒤 대상을 고르세요.");
    guideLabel->SetLocation(Loc.x - 70, Loc.y - 201);
    guideLabel->SetSize(860, 28);
    guideLabel->SetFont(18, FW_BOLD, L"Arial");
    guideLabel->SetColor(RGB(225, 225, 235));
    guideLabel->SetAlign(DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    AddChild(guideLabel);

    emptyLabel = new UTextLabel();
    emptyLabel->SetText(L"아이템이 없습니다.");
    emptyLabel->SetLocation(Loc.x, Loc.y - 20);
    emptyLabel->SetSize(300, 30);
    emptyLabel->SetFont(18, FW_BOLD, L"Arial");
    emptyLabel->SetColor(RGB(225, 225, 235));
    emptyLabel->SetAlign(DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    emptyLabel->SetVisible(false);
    AddChild(emptyLabel);

    backButton = new UButton();
    backButton->SetText(L"뒤로");
    backButton->SetSize(120, 46);
    backButton->SetLocation(clientRect.right - 90, 58);
    backButton->SetFont(18, FW_BOLD, L"Malgun Gothic");
    backButton->SetAnimationType(1);
    backButton->SetOnClick([this]() {
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
    AddChild(backButton);

    targetPanel = new UContainerWidget();
    targetPanel->SetSize(720, 240);
    targetPanel->SetLocation(Loc.x, Loc.y + 120);
    targetPanel->SetVisible(false);
    AddChild(targetPanel);

    UpdateInventoryGrid();
}

void UInventoryPopup::SetTeamData(const std::vector<Character*>& team)
{
    teamMembers = team;
}

void UInventoryPopup::Refresh()
{
    HideTargetSelection();
    UpdateInventoryGrid();
}

void UInventoryPopup::UpdateInventoryGrid()
{
    for (UButton* button : itemIconButtons)
    {
        RemoveChild(button, true);
    }
    itemIconButtons.clear();

    const std::vector<InventoryItem>& inventory = GameManager::Get().GetInventory();

    if (goldLabel)
    {
        goldLabel->SetText(L"골드: " + std::to_wstring(GameManager::Get().GetGold()));
    }
    if (emptyLabel)
    {
        emptyLabel->SetVisible(inventory.empty());
    }

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
        itemIconButtons.push_back(button);
    }
}

void UInventoryPopup::ShowTargetSelection(int itemID)
{
    selectedItemID = itemID;
    HideTooltip();

    if (targetPanel == nullptr)
    {
        return;
    }

    targetPanel->ClearChildren(true);
    targetButtons.clear();

    int panelX = Loc.x;
    int panelY = Loc.y + 120;

    const int panelW = 820;
    const int panelH = 270;
    const int colCount = 3;
    const int btnW = 160;
    const int btnH = 58;
    const int gapX = 42;
    const int gapY = 22;

    targetPanel->SetSize(panelW, panelH);
    targetPanel->SetLocation(panelX, panelY);

    UTextLabel* targetTitleLabel = new UTextLabel();
    targetTitleLabel->SetText(L"대상 선택: " + ItemData::GetName(selectedItemID));
    targetTitleLabel->SetLocation(panelX, panelY - panelH / 2 + 40);
    targetTitleLabel->SetSize(panelW, 36);
    targetTitleLabel->SetFont(24, FW_HEAVY, L"Arial");
    targetTitleLabel->SetColor(RGB(255, 230, 120));
    targetTitleLabel->SetAlign(DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    targetPanel->AddChild(targetTitleLabel);

    int totalW = colCount * btnW + (colCount - 1) * gapX;
    int startX = panelX - totalW / 2 + btnW / 2;
    int startY = panelY + 18;

    for (size_t i = 0; i < teamMembers.size(); ++i)
    {
        Character* target = teamMembers[i];
        UButton* button = new UButton();
        button->SetText(target ? target->GetName() : L"None");

        int col = (int)i % colCount;
        int row = (int)i / colCount;

        button->SetSize(btnW, btnH);
        button->SetLocation(startX + col * (btnW + gapX), startY + row * (btnH + gapY));
        button->SetFont(15, FW_BOLD, L"Malgun Gothic");
        button->SetAnimationType(0);
        button->SetOnClick([this, target]() {
            if (target == nullptr || selectedItemID < 0)
            {
                HideTargetSelection();
                return;
            }

            if (ItemData::IsEquipment(selectedItemID))
            {
                GameManager::Get().EquipItem(target, selectedItemID);
            }
            else if (ItemData::IsConsumable(selectedItemID))
            {
                GameManager::Get().UseConsumable(target, selectedItemID);
            }

            Refresh();
        });

        targetPanel->AddChild(button);
        targetButtons.push_back(button);
    }

    int cancelIndex = (int)teamMembers.size();
    int cancelCol = cancelIndex % colCount;
    int cancelRow = cancelIndex / colCount;

    UButton* cancel = new UButton();
    cancel->SetText(L"취소");
    cancel->SetSize(btnW, btnH);
    cancel->SetLocation(startX + cancelCol * (btnW + gapX), startY + cancelRow * (btnH + gapY));
    cancel->SetFont(15, FW_BOLD, L"Malgun Gothic");
    cancel->SetAnimationType(0);
    cancel->SetOnClick([this]() { HideTargetSelection(); });
    targetPanel->AddChild(cancel);
    targetButtons.push_back(cancel);

    RemoveChild(targetPanel, false);
    AddChild(targetPanel);
    targetPanel->SetVisible(true);
}

void UInventoryPopup::HideTargetSelection()
{
    selectedItemID = -1;
    if (targetPanel)
    {
        targetPanel->SetVisible(false);
        targetPanel->ClearChildren(true);
    }
    targetButtons.clear();
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

    bool bTargetVisible = (targetPanel != nullptr && targetPanel->GetVisible());
    if (bTargetVisible)
    {
        targetPanel->SetVisible(false);
    }

    UContainerWidget::Draw(hMemDC, hMemDC1);

    if (bTargetVisible)
    {
        targetPanel->SetVisible(true);
        DrawTargetPanelBackground(hMemDC);
        targetPanel->Draw(hMemDC, hMemDC1);
    }

    if (bShowTooltip)
    {
        DrawTooltip(hMemDC);
    }
}

void UInventoryPopup::DrawTargetPanelBackground(HDC hMemDC)
{
    if (targetPanel == nullptr || !targetPanel->GetVisible())
    {
        return;
    }

    Location loc = targetPanel->GetLocation();
    BuSIZE size = targetPanel->GetSize();
    RECT rc = { loc.x - size.sX / 2, loc.y - size.sY / 2, loc.x + size.sX / 2, loc.y + size.sY / 2 };

    HBRUSH bgBrush = CreateSolidBrush(RGB(28, 30, 44));
    FillRect(hMemDC, &rc, bgBrush);
    DeleteObject(bgBrush);

    HBRUSH frameBrush = CreateSolidBrush(RGB(176, 146, 78));
    FrameRect(hMemDC, &rc, frameBrush);
    DeleteObject(frameBrush);

}

void UInventoryPopup::ShowTooltip(const std::wstring& title, const std::wstring& desc, Location loc)
{
    bShowTooltip = true;
    tooltipTitle = title;
    tooltipDesc = desc;
    tooltipLoc = loc;
}

void UInventoryPopup::HideTooltip()
{
    bShowTooltip = false;
}

void UInventoryPopup::DrawTooltip(HDC hMemDC)
{
    int tx = tooltipLoc.x + 45;
    int ty = tooltipLoc.y - 55;
    RECT rc = { tx, ty, tx + 300, ty + 150 };

    if (rc.right > Size.sX - 10)
    {
        tx = tooltipLoc.x - 345;
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
    DrawTextW(hMemDC, tooltipTitle.c_str(), -1, &titleRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    SetTextColor(hMemDC, RGB(230, 230, 235));
    RECT descRc = { rc.left + 10, rc.top + 40, rc.right - 10, rc.bottom - 8 };
    DrawTextW(hMemDC, tooltipDesc.c_str(), -1, &descRc, DT_LEFT | DT_WORDBREAK);
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
