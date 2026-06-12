#include "Maintenance_Stage.h"
#include "ItemData.h"

#include <algorithm>
#include <string>

Maintenance_Stage::Maintenance_Stage()
    : RewardApplied(false),
      ActiveTab(EMaintenanceTab::Reward),
      SelectedPartyIndex(0),
      SelectedSkillSlot(0),
      ShopMessage(L""),
      ShopMessageLife(0),
      SkillMessage(L""),
      SkillMessageLife(0)
{
    CurStageID = 3;
    NextStageID = -2;

    W = 0;
    H = 0;
    hBitBackground = NULL;
}

Maintenance_Stage::~Maintenance_Stage()
{
}

void Maintenance_Stage::Init(HWND hWnd, HINSTANCE const g_hInst)
{
    GetClientRect(hWnd, &rects);

    RewardApplied = false;
    ActiveTab = BattleRewardManager::Get().HasReward() ? EMaintenanceTab::Reward : EMaintenanceTab::Party;
    SelectedPartyIndex = 0;
    LevelResults.clear();
    ShopMessage.clear();
    ShopMessageLife = 0;
    SkillMessage.clear();
    SkillMessageLife = 0;
    SelectedSkillSlot = 0;
    NextStageID = -2;

    GameManager::Get().InitDefaultParty();

    Loc.first.x = rects.right / 2;
    Loc.first.y = rects.bottom / 2;
    Loc.second.sX = 0;
    Loc.second.sY = 0;
}

void Maintenance_Stage::Update(HWND hWnd)
{
    ApplyRewardOnce();

    if (ShopMessageLife > 0)
    {
        ShopMessageLife--;
    }

    if (SkillMessageLife > 0)
    {
        SkillMessageLife--;
    }
}


void Maintenance_Stage::Draw(HDC hMemDC, HDC hMemDC1)
{
    HBRUSH hBgBrush = CreateSolidBrush(RGB(18, 18, 24));
    FillRect(hMemDC, &rects, hBgBrush);
    DeleteObject(hBgBrush);

    RECT panel = GetMainPanelRect();
    DrawPanel(hMemDC, panel);

    DrawCenteredText(
        hMemDC,
        IsRewardMode() ? L"BATTLE REWARD" : L"TOWN / PREPARATION",
        panel.top + 16,
        50,
        38,
        FW_HEAVY,
        RGB(255, 220, 90)
    );

    if (IsRewardMode())
    {
        DrawRewardTab(hMemDC, panel);
        DrawContinueButton(hMemDC);
        return;
    }

    DrawTabs(hMemDC);

    switch (ActiveTab)
    {
    case EMaintenanceTab::Reward:
        DrawRewardTab(hMemDC, panel);
        break;

    case EMaintenanceTab::Party:
        DrawPartyTab(hMemDC, panel);
        break;

    case EMaintenanceTab::Inventory:
        DrawInventoryTab(hMemDC, panel);
        break;

    case EMaintenanceTab::Equipment:
        DrawEquipmentTab(hMemDC, panel);
        break;

    case EMaintenanceTab::Skill:
        DrawSkillTab(hMemDC, panel);
        break;

    case EMaintenanceTab::Shop:
        DrawShopTab(hMemDC, panel);
        break;

    default:
        DrawPartyTab(hMemDC, panel);
        break;
    }

    DrawContinueButton(hMemDC);
}

int Maintenance_Stage::Input(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_LBUTTONDOWN)
    {
        int mouseX = LOWORD(lParam);
        int mouseY = HIWORD(lParam);

        if (IsRewardMode())
        {
            RECT panel = GetMainPanelRect();

            if (CanProceedToNextStage() && IsPointInRect(mouseX, mouseY, GetNextStageButtonRect()))
            {
                return ProceedToNextStage();
            }

            if (IsPointInRect(mouseX, mouseY, GetContinueRect()))
            {
                return ReturnToMapFromReward();
            }

            if (!IsPointInRect(mouseX, mouseY, panel))
            {
                return ReturnToMapFromReward();
            }

            return -2;
        }

        if (IsPointInRect(mouseX, mouseY, GetContinueRect()))
        {
            BattleRewardManager::Get().Clear();
            return MAP_STAGE;
        }

        HandleMouseDown(mouseX, mouseY);
        return -2;
    }

    if (uMsg == WM_KEYDOWN)
    {
        if (IsRewardMode())
        {
            if ((wParam == VK_RETURN || wParam == VK_SPACE) && CanProceedToNextStage())
            {
                return ProceedToNextStage();
            }

            if (wParam == VK_ESCAPE)
            {
                return ReturnToMapFromReward();
            }

            return -2;
        }

        if (wParam == VK_RETURN || wParam == VK_SPACE || wParam == VK_ESCAPE)
        {
            BattleRewardManager::Get().Clear();
            return MAP_STAGE;
        }

        if (wParam == '1')
        {
            ActiveTab = EMaintenanceTab::Party;
        }
        else if (wParam == '2')
        {
            ActiveTab = EMaintenanceTab::Inventory;
        }
        else if (wParam == '3')
        {
            ActiveTab = EMaintenanceTab::Equipment;
        }
        else if (wParam == '4')
        {
            ActiveTab = EMaintenanceTab::Skill;
        }
        else if (wParam == '5')
        {
            ActiveTab = EMaintenanceTab::Shop;
        }
    }

    return -2;
}

void Maintenance_Stage::Comp_NextStage(int ID)
{
}

void Maintenance_Stage::BackgroundMove(int x, int y)
{
}

void Maintenance_Stage::ApplyRewardOnce()
{
    if (RewardApplied)
    {
        return;
    }

    RewardApplied = true;
    LevelResults.clear();

    BattleRewardManager& reward = BattleRewardManager::Get();

    if (!reward.HasReward())
    {
        return;
    }

    if (!reward.IsVictory())
    {
        return;
    }

    GameManager::Get().AddGold(reward.GetRewardGold());

    std::vector<Character*>& party = GameManager::Get().GetParty();

    for (Character* c : party)
    {
        if (c == nullptr)
        {
            continue;
        }

        SMaintenanceLevelResult result;
        result.CharacterName = c->GetName();
        result.OldLevel = c->GetLevel();
        result.OldExp = c->GetExp();
        result.OldStat = c->GetStat();

        c->AddExp(reward.GetRewardExp());

        result.NewLevel = c->GetLevel();
        result.NewExp = c->GetExp();
        result.NewStat = c->GetStat();
        result.LeveledUp = result.NewLevel > result.OldLevel;

        LevelResults.push_back(result);
    }

    const std::vector<SBattleRewardItem>& items = reward.GetRewardItems();

    for (const SBattleRewardItem& item : items)
    {
        GameManager::Get().AddItem(item.ItemID, item.Count);
    }

    GameManager::Get().UnlockBattleStage(reward.GetNextStageId());
}

RECT Maintenance_Stage::GetMainPanelRect() const
{
    RECT panel;
    panel.left = rects.right / 2 - 560;
    panel.top = rects.bottom / 2 - 320;
    panel.right = rects.right / 2 + 560;
    panel.bottom = rects.bottom / 2 + 320;

    if (panel.left < 20)
    {
        panel.left = 20;
    }

    if (panel.right > rects.right - 20)
    {
        panel.right = rects.right - 20;
    }

    if (panel.top < 20)
    {
        panel.top = 20;
    }

    if (panel.bottom > rects.bottom - 20)
    {
        panel.bottom = rects.bottom - 20;
    }

    return panel;
}

RECT Maintenance_Stage::GetTabRect(EMaintenanceTab tab) const
{
    RECT panel = GetMainPanelRect();

    int index = 0;
    int tabCount = IsRewardMode() ? 6 : 5;
    int tabW = IsRewardMode() ? 140 : 160;
    int tabH = 44;
    int gap = 8;

    if (IsRewardMode())
    {
        switch (tab)
        {
        case EMaintenanceTab::Reward: index = 0; break;
        case EMaintenanceTab::Party: index = 1; break;
        case EMaintenanceTab::Inventory: index = 2; break;
        case EMaintenanceTab::Equipment: index = 3; break;
        case EMaintenanceTab::Skill: index = 4; break;
        case EMaintenanceTab::Shop: index = 5; break;
        default: index = 0; break;
        }
    }
    else
    {
        switch (tab)
        {
        case EMaintenanceTab::Party: index = 0; break;
        case EMaintenanceTab::Inventory: index = 1; break;
        case EMaintenanceTab::Equipment: index = 2; break;
        case EMaintenanceTab::Skill: index = 3; break;
        case EMaintenanceTab::Shop: index = 4; break;
        default: index = 0; break;
        }
    }

    int totalW = tabW * tabCount + gap * (tabCount - 1);
    int startX = panel.left + (panel.right - panel.left - totalW) / 2;
    int y = panel.top + 78;

    RECT rc;
    rc.left = startX + index * (tabW + gap);
    rc.top = y;
    rc.right = rc.left + tabW;
    rc.bottom = rc.top + tabH;
    return rc;
}

RECT Maintenance_Stage::GetContinueRect() const
{
    RECT panel = GetMainPanelRect();

    RECT rc;
    rc.left = panel.right - 240;
    rc.top = panel.bottom - 58;
    rc.right = panel.right - 45;
    rc.bottom = panel.bottom - 20;
    return rc;
}

RECT Maintenance_Stage::GetNextStageButtonRect() const
{
    RECT panel = GetMainPanelRect();

    RECT rc;
    rc.left = panel.right - 460;
    rc.top = panel.bottom - 58;
    rc.right = panel.right - 260;
    rc.bottom = panel.bottom - 20;
    return rc;
}

RECT Maintenance_Stage::GetPartySelectRect(int index) const
{
    RECT panel = GetMainPanelRect();

    RECT rc;
    rc.left = panel.left + 55 + index * 180;
    rc.top = panel.top + 135;
    rc.right = rc.left + 160;
    rc.bottom = rc.top + 44;
    return rc;
}

RECT Maintenance_Stage::GetInventorySlotRect(int index) const
{
    RECT panel = GetMainPanelRect();

    int colCount = 4;
    int slotW = 235;
    int slotH = 54;
    int gapX = 18;
    int gapY = 14;

    int col = index % colCount;
    int row = index / colCount;

    RECT rc;
    rc.left = panel.left + 55 + col * (slotW + gapX);
    rc.top = panel.top + 220 + row * (slotH + gapY);
    rc.right = rc.left + slotW;
    rc.bottom = rc.top + slotH;
    return rc;
}

RECT Maintenance_Stage::GetEquipmentSlotRect(int index) const
{
    RECT panel = GetMainPanelRect();

    RECT rc;
    rc.left = panel.left + 610;
    rc.top = panel.top + 260 + index * 78;
    rc.right = panel.right - 70;
    rc.bottom = rc.top + 58;
    return rc;
}

RECT Maintenance_Stage::GetShopSlotRect(int index) const
{
    RECT panel = GetMainPanelRect();

    int colCount = 2;
    int slotW = 455;
    int slotH = 54;
    int gapX = 28;
    int gapY = 9;

    int col = index % colCount;
    int row = index / colCount;

    RECT rc;
    rc.left = panel.left + 65 + col * (slotW + gapX);
    rc.top = panel.top + 220 + row * (slotH + gapY);
    rc.right = rc.left + slotW;
    rc.bottom = rc.top + slotH;
    return rc;
}

RECT Maintenance_Stage::GetRosterSlotRect(int index) const
{
    RECT panel = GetMainPanelRect();

    int colCount = 5;
    int slotW = 180;
    int slotH = 48;
    int gapX = 18;
    int gapY = 10;

    int col = index % colCount;
    int row = index / colCount;

    RECT rc;
    rc.left = panel.left + 55 + col * (slotW + gapX);
    rc.top = panel.top + 250 + row * (slotH + gapY);
    rc.right = rc.left + slotW;
    rc.bottom = rc.top + slotH;
    return rc;
}

RECT Maintenance_Stage::GetSkillSlotRect(int index) const
{
    RECT panel = GetMainPanelRect();

    RECT rc;
    rc.left = panel.left + 65 + index * 310;
    rc.top = panel.top + 250;
    rc.right = rc.left + 280;
    rc.bottom = rc.top + 62;
    return rc;
}

RECT Maintenance_Stage::GetLearnedSkillRect(int index) const
{
    RECT panel = GetMainPanelRect();

    int colCount = 3;
    int slotW = 285;
    int slotH = 58;
    int gapX = 25;
    int gapY = 12;

    int col = index % colCount;
    int row = index / colCount;

    RECT rc;
    rc.left = panel.left + 65 + col * (slotW + gapX);
    rc.top = panel.top + 390 + row * (slotH + gapY);
    rc.right = rc.left + slotW;
    rc.bottom = rc.top + slotH;
    return rc;
}

bool Maintenance_Stage::IsPointInRect(int x, int y, const RECT& rc) const
{
    return x >= rc.left && x <= rc.right && y >= rc.top && y <= rc.bottom;
}

void Maintenance_Stage::DrawPanel(HDC hMemDC, const RECT& panel)
{
    HBRUSH hPanelBrush = CreateSolidBrush(RGB(34, 34, 46));
    FillRect(hMemDC, &panel, hPanelBrush);
    DeleteObject(hPanelBrush);

    HPEN hPanelPen = CreatePen(PS_SOLID, 3, RGB(220, 190, 90));
    HPEN hOldPen = (HPEN)SelectObject(hMemDC, hPanelPen);

    HBRUSH hOldBrush = (HBRUSH)SelectObject(hMemDC, GetStockObject(NULL_BRUSH));

    Rectangle(hMemDC, panel.left, panel.top, panel.right, panel.bottom);

    SelectObject(hMemDC, hOldBrush);
    SelectObject(hMemDC, hOldPen);
    DeleteObject(hPanelPen);
}

void Maintenance_Stage::DrawTabs(HDC hMemDC)
{
    struct TabInfo
    {
        EMaintenanceTab Tab;
        const wchar_t* Text;
    };

    TabInfo tabs[5] = {
        { EMaintenanceTab::Party, L"1. Party" },
        { EMaintenanceTab::Inventory, L"2. Inventory" },
        { EMaintenanceTab::Equipment, L"3. Equip" },
        { EMaintenanceTab::Skill, L"4. Skill" },
        { EMaintenanceTab::Shop, L"5. Shop" }
    };

    for (int i = 0; i < 5; ++i)
    {
        bool selected = ActiveTab == tabs[i].Tab;
        DrawBoxText(
            hMemDC,
            GetTabRect(tabs[i].Tab),
            tabs[i].Text,
            19,
            FW_BOLD,
            selected ? RGB(20, 20, 24) : RGB(230, 230, 230),
            selected ? RGB(255, 220, 90) : RGB(48, 48, 64),
            selected ? RGB(255, 240, 160) : RGB(120, 120, 140),
            DT_CENTER | DT_VCENTER | DT_SINGLELINE
        );
    }
}

void Maintenance_Stage::DrawContinueButton(HDC hMemDC)
{
    if (IsRewardMode())
    {
        if (CanProceedToNextStage())
        {
            DrawBoxText(
                hMemDC,
                GetNextStageButtonRect(),
                L"Next Stage",
                20,
                FW_BOLD,
                RGB(255, 255, 255),
                RGB(80, 90, 135),
                RGB(150, 180, 255),
                DT_CENTER | DT_VCENTER | DT_SINGLELINE
            );
        }

        DrawBoxText(
            hMemDC,
            GetContinueRect(),
            L"Back to Map",
            20,
            FW_BOLD,
            RGB(255, 255, 255),
            RGB(70, 105, 70),
            RGB(150, 230, 150),
            DT_CENTER | DT_VCENTER | DT_SINGLELINE
        );

        DrawLeftText(
            hMemDC,
            L"Click outside the reward panel to return to the world map.",
            GetMainPanelRect().left + 65,
            GetMainPanelRect().bottom - 52,
            560,
            26,
            17,
            FW_NORMAL,
            RGB(190, 190, 200)
        );
        return;
    }

    DrawBoxText(
        hMemDC,
        GetContinueRect(),
        L"Back to Map",
        20,
        FW_BOLD,
        RGB(255, 255, 255),
        RGB(70, 105, 70),
        RGB(150, 230, 150),
        DT_CENTER | DT_VCENTER | DT_SINGLELINE
    );
}


bool Maintenance_Stage::IsRewardMode() const
{
    return BattleRewardManager::Get().HasReward();
}

bool Maintenance_Stage::CanProceedToNextStage() const
{
    const BattleRewardManager& reward = BattleRewardManager::Get();

    if (!reward.HasReward() || !reward.IsVictory())
    {
        return false;
    }

    int clearedStageId = reward.GetClearedStageId();
    int nextStageId = reward.GetNextStageId();

    return nextStageId > clearedStageId && nextStageId <= 3;
}

int Maintenance_Stage::ProceedToNextStage()
{
    ApplyRewardOnce();

    int nextStageId = BattleRewardManager::Get().GetNextStageId();
    GameManager::Get().SetCurrentBattleStageId(nextStageId);
    BattleRewardManager::Get().Clear();
    return BATTLE_STAGE;
}

int Maintenance_Stage::ReturnToMapFromReward()
{
    ApplyRewardOnce();
    BattleRewardManager::Get().Clear();
    return MAP_STAGE;
}

void Maintenance_Stage::DrawRewardTab(HDC hMemDC, const RECT& panel)
{
    BattleRewardManager& reward = BattleRewardManager::Get();

    int left = panel.left + 65;
    int top = panel.top + 150;
    int width = panel.right - panel.left - 130;

    DrawLeftText(
        hMemDC,
        L"REWARD SUMMARY",
        left,
        top,
        width,
        32,
        26,
        FW_HEAVY,
        RGB(255, 220, 90)
    );

    top += 46;

    if (!reward.HasReward())
    {
        DrawLeftText(hMemDC, L"Town mode: no recent battle reward.", left, top, width, 32, 22, FW_NORMAL, RGB(230, 230, 230));
        return;
    }

    DrawLeftText(
        hMemDC,
        reward.IsVictory() ? L"Battle Result: Victory" : L"Battle Result: Defeat",
        left,
        top,
        width,
        30,
        22,
        FW_BOLD,
        reward.IsVictory() ? RGB(120, 220, 120) : RGB(255, 100, 100)
    );

    top += 38;
    DrawLeftText(hMemDC, L"Cleared Stage: " + std::to_wstring(reward.GetClearedStageId()), left, top, width, 28, 20, FW_NORMAL, RGB(230, 230, 230));

    top += 36;
    DrawLeftText(hMemDC, L"Gold +" + std::to_wstring(reward.GetRewardGold()), left, top, width, 30, 22, FW_BOLD, RGB(255, 220, 90));

    top += 34;
    DrawLeftText(hMemDC, L"EXP +" + std::to_wstring(reward.GetRewardExp()), left, top, width, 30, 22, FW_BOLD, RGB(120, 200, 255));

    top += 42;
    DrawLeftText(hMemDC, L"Items", left, top, width, 30, 22, FW_BOLD, RGB(230, 230, 230));

    const std::vector<SBattleRewardItem>& items = reward.GetRewardItems();
    top += 34;

    if (items.empty())
    {
        DrawLeftText(hMemDC, L"- None", left + 25, top, width - 25, 28, 20, FW_NORMAL, RGB(200, 200, 200));
    }
    else
    {
        for (size_t i = 0; i < items.size(); ++i)
        {
            std::wstring itemName = ItemData::GetName(items[i].ItemID);
            std::wstring itemText = L"- " + itemName + L" x" + std::to_wstring(items[i].Count);

            DrawLeftText(hMemDC, itemText, left + 25, top, width - 25, 28, 20, FW_NORMAL, RGB(230, 230, 230));
            top += 28;
        }
    }

    top += 24;
    DrawLeftText(hMemDC, L"Party Gold: " + std::to_wstring(GameManager::Get().GetGold()), left, top, width, 30, 22, FW_BOLD, RGB(255, 220, 90));

    int levelLeft = panel.left + 555;
    int levelTop = panel.top + 150;
    int levelWidth = panel.right - levelLeft - 65;

    DrawLeftText(hMemDC, L"LEVEL / EXP RESULT", levelLeft, levelTop, levelWidth, 32, 26, FW_HEAVY, RGB(120, 200, 255));
    levelTop += 46;

    if (LevelResults.empty())
    {
        DrawLeftText(hMemDC, L"No EXP was applied.", levelLeft, levelTop, levelWidth, 30, 20, FW_NORMAL, RGB(210, 210, 210));
        return;
    }

    for (size_t i = 0; i < LevelResults.size(); ++i)
    {
        const SMaintenanceLevelResult& result = LevelResults[i];

        DrawLeftText(
            hMemDC,
            result.CharacterName,
            levelLeft,
            levelTop,
            levelWidth,
            26,
            21,
            FW_BOLD,
            result.LeveledUp ? RGB(255, 220, 90) : RGB(230, 230, 230)
        );

        levelTop += 26;

        std::wstring levelText = L"Lv " + std::to_wstring(result.OldLevel) + L" -> " + std::to_wstring(result.NewLevel);
        levelText += L"    EXP " + std::to_wstring(result.OldExp) + L" -> " + std::to_wstring(result.NewExp);
        DrawLeftText(hMemDC, levelText, levelLeft + 18, levelTop, levelWidth - 18, 24, 18, FW_NORMAL, RGB(220, 220, 220));

        levelTop += 24;

        if (result.LeveledUp)
        {
            std::wstring statText = L"HP +" + std::to_wstring(result.NewStat.MaxHP - result.OldStat.MaxHP);
            statText += L"  MP +" + std::to_wstring(result.NewStat.MaxMP - result.OldStat.MaxMP);
            statText += L"  STR +" + std::to_wstring(result.NewStat.STR - result.OldStat.STR);
            DrawLeftText(hMemDC, statText, levelLeft + 18, levelTop, levelWidth - 18, 22, 17, FW_NORMAL, RGB(160, 230, 160));

            levelTop += 22;

            std::wstring statText2 = L"INT +" + std::to_wstring(result.NewStat.INT - result.OldStat.INT);
            statText2 += L"  DEF +" + std::to_wstring(result.NewStat.DEF - result.OldStat.DEF);
            statText2 += L"  SPD +" + std::to_wstring(result.NewStat.SPD - result.OldStat.SPD);
            DrawLeftText(hMemDC, statText2, levelLeft + 18, levelTop, levelWidth - 18, 22, 17, FW_NORMAL, RGB(160, 230, 160));

            levelTop += 28;
        }
        else
        {
            DrawLeftText(hMemDC, L"No Level Up", levelLeft + 18, levelTop, levelWidth - 18, 24, 17, FW_NORMAL, RGB(170, 170, 180));
            levelTop += 30;
        }

        if (levelTop > panel.bottom - 85)
        {
            break;
        }
    }
}

void Maintenance_Stage::DrawPartyTab(HDC hMemDC, const RECT& panel)
{
    const std::vector<Character*>& party = GameManager::Get().GetParty();
    const std::vector<Character*>& roster = GameManager::Get().GetRoster();

    DrawLeftText(hMemDC, L"ACTIVE PARTY  " + std::to_wstring((int)party.size()) + L" / " + std::to_wstring(GameManager::Get().GetMaxActivePartySize()), panel.left + 55, panel.top + 135, 500, 34, 26, FW_HEAVY, RGB(255, 220, 90));
    DrawLeftText(hMemDC, L"Click a roster character to add/remove. At least 1 and up to 5 characters can be active.", panel.left + 55, panel.top + 175, panel.right - panel.left - 110, 28, 18, FW_NORMAL, RGB(220, 220, 220));

    int activeTop = panel.top + 205;

    for (int i = 0; i < (int)party.size() && i < 5; ++i)
    {
        Character* c = party[i];
        std::wstring text = std::to_wstring(i + 1) + L". ";
        if (c != nullptr)
        {
            text += c->GetName() + L" / " + c->GetElementName();
        }
        else
        {
            text += L"Empty";
        }

        DrawLeftText(hMemDC, text, panel.left + 60 + i * 190, activeTop, 180, 30, 17, FW_BOLD, RGB(120, 200, 255));
    }

    DrawLeftText(hMemDC, L"ROSTER", panel.left + 55, panel.top + 222, 300, 28, 22, FW_HEAVY, RGB(230, 230, 230));

    for (int i = 0; i < (int)roster.size(); ++i)
    {
        Character* c = roster[i];
        if (c == nullptr)
        {
            continue;
        }

        bool active = GameManager::Get().IsInActiveParty(c);
        COLORREF fill = active ? RGB(45, 70, 95) : RGB(35, 35, 45);
        COLORREF border = active ? RGB(120, 200, 255) : RGB(90, 90, 100);
        COLORREF textColor = active ? RGB(235, 245, 255) : RGB(200, 200, 205);

        std::wstring label = c->GetName();
        if (active)
        {
            label += L"  ACTIVE";
        }

        DrawBoxText(hMemDC, GetRosterSlotRect(i), label, 16, FW_BOLD, textColor, fill, border, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    }

    if (!SkillMessage.empty() && SkillMessageLife > 0)
    {
        DrawLeftText(hMemDC, SkillMessage, panel.left + 65, panel.bottom - 105, panel.right - panel.left - 340, 30, 20, FW_BOLD, RGB(255, 220, 90));
    }
}

void Maintenance_Stage::DrawInventoryTab(HDC hMemDC, const RECT& panel)
{
    DrawPartySelectButtons(hMemDC, panel);

    Character* selected = GetSelectedCharacter();
    std::wstring selectedName = selected ? selected->GetName() : L"None";

    DrawLeftText(
        hMemDC,
        L"INVENTORY     GOLD: " + std::to_wstring(GameManager::Get().GetGold()) + L"     Target: " + selectedName,
        panel.left + 55,
        panel.top + 190,
        panel.right - panel.left - 110,
        30,
        24,
        FW_HEAVY,
        RGB(255, 220, 90)
    );

    const std::vector<InventoryItem>& inventory = GameManager::Get().GetInventory();

    if (inventory.empty())
    {
        DrawLeftText(hMemDC, L"No Items Found.", panel.left + 65, panel.top + 255, panel.right - panel.left - 130, 30, 22, FW_NORMAL, RGB(230, 230, 230));
        return;
    }

    int maxSlots = 20;
    int count = (int)std::min<size_t>((size_t)maxSlots, inventory.size());

    for (int i = 0; i < count; ++i)
    {
        const InventoryItem& inv = inventory[i];
        std::wstring itemText = ItemData::GetName(inv.ItemID) + L" x" + std::to_wstring(inv.Count);

        if (ItemData::IsEquipment(inv.ItemID))
        {
            itemText += L"  [Equip]";
        }
        else if (ItemData::IsConsumable(inv.ItemID))
        {
            itemText += L"  [Use]";
        }

        DrawBoxText(
            hMemDC,
            GetInventorySlotRect(i),
            itemText,
            18,
            FW_NORMAL,
            RGB(235, 235, 235),
            RGB(48, 48, 64),
            RGB(110, 110, 130),
            DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS
        );
    }

    DrawLeftText(
        hMemDC,
        L"Click equipment to equip it to the selected character. Click potion to use it.",
        panel.left + 55,
        panel.bottom - 95,
        panel.right - panel.left - 300,
        26,
        18,
        FW_NORMAL,
        RGB(180, 180, 190)
    );
}

void Maintenance_Stage::DrawEquipmentTab(HDC hMemDC, const RECT& panel)
{
    DrawPartySelectButtons(hMemDC, panel);

    Character* selected = GetSelectedCharacter();

    if (selected == nullptr)
    {
        DrawLeftText(hMemDC, L"No party member.", panel.left + 65, panel.top + 220, panel.right - panel.left - 130, 30, 22, FW_NORMAL, RGB(230, 230, 230));
        return;
    }

    CharacterStat stat = selected->GetStat();
    CharacterEquipment equip = selected->GetEquipment();

    int left = panel.left + 75;
    int top = panel.top + 225;
    int width = 470;

    DrawLeftText(hMemDC, selected->GetName(), left, top, width, 36, 30, FW_HEAVY, RGB(255, 220, 90));
    top += 54;
    DrawLeftText(hMemDC, L"Current Stats", left, top, width, 30, 24, FW_BOLD, RGB(120, 200, 255));
    top += 42;
    DrawLeftText(hMemDC, L"HP: " + std::to_wstring(stat.CurHP) + L" / " + std::to_wstring(stat.MaxHP), left, top, width, 28, 21, FW_NORMAL, RGB(230, 230, 230));
    top += 34;
    DrawLeftText(hMemDC, L"MP: " + std::to_wstring(stat.CurMP) + L" / " + std::to_wstring(stat.MaxMP), left, top, width, 28, 21, FW_NORMAL, RGB(230, 230, 230));
    top += 42;
    DrawLeftText(hMemDC, MakeStatLine(stat), left, top, width, 32, 23, FW_BOLD, RGB(120, 200, 255));

    int itemIDs[3] = { equip.WeaponID, equip.ArmorID, equip.AccessoryID };
    EEquipPart parts[3] = { EEquipPart::Weapon, EEquipPart::Armor, EEquipPart::Accessory };

    DrawLeftText(hMemDC, L"Equipped Items", panel.left + 610, panel.top + 218, panel.right - panel.left - 680, 32, 26, FW_HEAVY, RGB(255, 220, 90));

    for (int i = 0; i < 3; ++i)
    {
        std::wstring text = GetEquipPartName(parts[i]) + L": " + GetEquippedName(itemIDs[i]);

        if (itemIDs[i] >= 0)
        {
            text += L"  [Click to Unequip]";
        }

        DrawBoxText(
            hMemDC,
            GetEquipmentSlotRect(i),
            text,
            19,
            FW_NORMAL,
            RGB(235, 235, 235),
            itemIDs[i] >= 0 ? RGB(58, 58, 75) : RGB(42, 42, 55),
            itemIDs[i] >= 0 ? RGB(150, 150, 180) : RGB(90, 90, 110),
            DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS
        );
    }

    DrawLeftText(
        hMemDC,
        L"Use the Inventory tab to equip new items. Click an equipped slot here to unequip it.",
        panel.left + 75,
        panel.bottom - 95,
        panel.right - panel.left - 300,
        26,
        18,
        FW_NORMAL,
        RGB(180, 180, 190)
    );
}


void Maintenance_Stage::DrawSkillTab(HDC hMemDC, const RECT& panel)
{
    DrawPartySelectButtons(hMemDC, panel);

    Character* selected = GetSelectedCharacter();

    if (selected == nullptr)
    {
        DrawLeftText(hMemDC, L"No active party member.", panel.left + 65, panel.top + 220, panel.right - panel.left - 130, 30, 22, FW_NORMAL, RGB(230, 230, 230));
        return;
    }

    DrawLeftText(hMemDC, L"SKILL SETTING", panel.left + 65, panel.top + 195, 420, 34, 28, FW_HEAVY, RGB(255, 220, 90));
    DrawLeftText(hMemDC, selected->GetName() + L" / " + selected->GetElementName(), panel.left + 470, panel.top + 198, 450, 30, 22, FW_BOLD, RGB(120, 200, 255));

    DrawLeftText(hMemDC, L"Equipped Skills", panel.left + 65, panel.top + 225, 320, 28, 20, FW_BOLD, RGB(230, 230, 230));

    for (int i = 0; i < 3; ++i)
    {
        int skillID = selected->GetEquippedSkillID(i);
        const SSkillData* skill = SkillData::Find(skillID);

        std::wstring text = L"Slot " + std::to_wstring(i + 1) + L": ";
        COLORREF border = (i == SelectedSkillSlot) ? RGB(255, 220, 90) : RGB(90, 90, 100);
        COLORREF fill = (i == SelectedSkillSlot) ? RGB(70, 60, 30) : RGB(35, 35, 45);

        if (skill != nullptr)
        {
            text += skill->Name + L"  MP " + std::to_wstring(skill->MPCost);
        }
        else
        {
            text += L"Empty";
        }

        DrawBoxText(hMemDC, GetSkillSlotRect(i), text, 17, FW_BOLD, RGB(235, 235, 235), fill, border, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    }

    DrawLeftText(hMemDC, L"Learned Skills - click a skill to equip it into the selected slot.", panel.left + 65, panel.top + 350, panel.right - panel.left - 130, 28, 18, FW_NORMAL, RGB(220, 220, 220));

    std::vector<int> learned = selected->GetLearnedSkillIDs();

    for (int i = 0; i < (int)learned.size(); ++i)
    {
        int skillID = learned[i];
        const SSkillData* skill = SkillData::Find(skillID);

        if (skill == nullptr)
        {
            continue;
        }

        std::wstring text = skill->Name + L"\nMP " + std::to_wstring(skill->MPCost) + L" / R" + std::to_wstring(skill->Range);
        DrawBoxText(hMemDC, GetLearnedSkillRect(i), text, 15, FW_BOLD, RGB(240, 240, 240), RGB(35, 35, 45), skill->Color, DT_CENTER | DT_VCENTER | DT_WORDBREAK);
    }

    if (!SkillMessage.empty() && SkillMessageLife > 0)
    {
        DrawLeftText(hMemDC, SkillMessage, panel.left + 65, panel.bottom - 105, panel.right - panel.left - 340, 30, 20, FW_BOLD, RGB(255, 220, 90));
    }
}

void Maintenance_Stage::DrawShopTab(HDC hMemDC, const RECT& panel)
{
    int left = panel.left + 65;
    int top = panel.top + 150;
    int width = panel.right - panel.left - 130;

    int shopTier = GameManager::Get().GetMaxUnlockedBattleStageId();

    DrawLeftText(
        hMemDC,
        L"SHOP     GOLD: " + std::to_wstring(GameManager::Get().GetGold()) + L"     Shop Tier: " + std::to_wstring(shopTier),
        left,
        top,
        width,
        34,
        26,
        FW_HEAVY,
        RGB(255, 220, 90)
    );

    top += 42;
    DrawLeftText(
        hMemDC,
        L"Click an item to buy one. More items appear as later stages are unlocked.",
        left,
        top,
        width,
        28,
        19,
        FW_NORMAL,
        RGB(190, 190, 200)
    );

    std::vector<int> shopItems = GetShopItemIDs();

    for (int i = 0; i < (int)shopItems.size(); ++i)
    {
        int itemID = shopItems[i];
        int price = ItemData::GetPrice(itemID);

        std::wstring text = ItemData::GetName(itemID);
        text += L"     ";
        text += std::to_wstring(price);
        text += L"G";

        if (ItemData::IsConsumable(itemID))
        {
            text += L"     [Use]";
        }
        else if (ItemData::IsEquipment(itemID))
        {
            text += L"     [Equip]";
        }

        bool affordable = GameManager::Get().GetGold() >= price;

        DrawBoxText(
            hMemDC,
            GetShopSlotRect(i),
            text,
            17,
            FW_NORMAL,
            affordable ? RGB(245, 245, 245) : RGB(150, 150, 150),
            affordable ? RGB(48, 56, 64) : RGB(38, 38, 48),
            affordable ? RGB(130, 160, 130) : RGB(90, 90, 110),
            DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS
        );
    }

    if (!ShopMessage.empty() && ShopMessageLife > 0)
    {
        DrawLeftText(
            hMemDC,
            ShopMessage,
            panel.left + 65,
            panel.bottom - 105,
            panel.right - panel.left - 300,
            30,
            21,
            FW_BOLD,
            RGB(120, 220, 120)
        );
    }
}

void Maintenance_Stage::DrawPartySelectButtons(HDC hMemDC, const RECT& panel)
{
    std::vector<Character*>& party = GameManager::Get().GetParty();

    int count = (int)party.size();

    for (int i = 0; i < count && i < 5; ++i)
    {
        Character* c = party[i];

        std::wstring text = c ? c->GetName() : L"None";
        bool selected = i == SelectedPartyIndex;

        DrawBoxText(
            hMemDC,
            GetPartySelectRect(i),
            text,
            19,
            FW_BOLD,
            selected ? RGB(20, 20, 24) : RGB(230, 230, 230),
            selected ? RGB(255, 220, 90) : RGB(48, 48, 64),
            selected ? RGB(255, 240, 160) : RGB(120, 120, 140),
            DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS
        );
    }
}

void Maintenance_Stage::DrawBoxText(
    HDC hMemDC,
    const RECT& rc,
    const std::wstring& text,
    int fontSize,
    int fontWeight,
    COLORREF textColor,
    COLORREF fillColor,
    COLORREF borderColor,
    UINT format
)
{
    HBRUSH hBrush = CreateSolidBrush(fillColor);
    FillRect(hMemDC, &rc, hBrush);
    DeleteObject(hBrush);

    HPEN hPen = CreatePen(PS_SOLID, 2, borderColor);
    HPEN hOldPen = (HPEN)SelectObject(hMemDC, hPen);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hMemDC, GetStockObject(NULL_BRUSH));
    Rectangle(hMemDC, rc.left, rc.top, rc.right, rc.bottom);
    SelectObject(hMemDC, hOldBrush);
    SelectObject(hMemDC, hOldPen);
    DeleteObject(hPen);

    int oldBkMode = SetBkMode(hMemDC, TRANSPARENT);
    COLORREF oldColor = SetTextColor(hMemDC, textColor);

    HFONT hFont = CreateFontW(
        fontSize,
        0,
        0,
        0,
        fontWeight,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Arial"
    );

    HFONT hOldFont = (HFONT)SelectObject(hMemDC, hFont);

    RECT textRect = rc;
    textRect.left += 10;
    textRect.right -= 10;

    DrawTextW(hMemDC, text.c_str(), (int)text.length(), &textRect, format);

    SelectObject(hMemDC, hOldFont);
    DeleteObject(hFont);

    SetTextColor(hMemDC, oldColor);
    SetBkMode(hMemDC, oldBkMode);
}

void Maintenance_Stage::DrawCenteredText(
    HDC hMemDC,
    const std::wstring& text,
    int top,
    int height,
    int fontSize,
    int fontWeight,
    COLORREF color
)
{
    int oldBkMode = SetBkMode(hMemDC, TRANSPARENT);

    HFONT hFont = CreateFontW(
        fontSize,
        0,
        0,
        0,
        fontWeight,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Arial"
    );

    HFONT hOldFont = (HFONT)SelectObject(hMemDC, hFont);

    RECT textRect;
    textRect.left = 0;
    textRect.top = top;
    textRect.right = rects.right;
    textRect.bottom = top + height;

    SetTextColor(hMemDC, color);

    DrawTextW(hMemDC, text.c_str(), (int)text.length(), &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hMemDC, hOldFont);
    DeleteObject(hFont);

    SetBkMode(hMemDC, oldBkMode);
}

void Maintenance_Stage::DrawLeftText(
    HDC hMemDC,
    const std::wstring& text,
    int left,
    int top,
    int width,
    int height,
    int fontSize,
    int fontWeight,
    COLORREF color
)
{
    int oldBkMode = SetBkMode(hMemDC, TRANSPARENT);

    HFONT hFont = CreateFontW(
        fontSize,
        0,
        0,
        0,
        fontWeight,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Arial"
    );

    HFONT hOldFont = (HFONT)SelectObject(hMemDC, hFont);

    RECT textRect;
    textRect.left = left;
    textRect.top = top;
    textRect.right = left + width;
    textRect.bottom = top + height;

    SetTextColor(hMemDC, color);

    DrawTextW(hMemDC, text.c_str(), (int)text.length(), &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    SelectObject(hMemDC, hOldFont);
    DeleteObject(hFont);

    SetBkMode(hMemDC, oldBkMode);
}

void Maintenance_Stage::HandleMouseDown(int mouseX, int mouseY)
{
    if (IsPointInRect(mouseX, mouseY, GetTabRect(EMaintenanceTab::Party)))
    {
        ActiveTab = EMaintenanceTab::Party;
        return;
    }

    if (IsPointInRect(mouseX, mouseY, GetTabRect(EMaintenanceTab::Inventory)))
    {
        ActiveTab = EMaintenanceTab::Inventory;
        return;
    }

    if (IsPointInRect(mouseX, mouseY, GetTabRect(EMaintenanceTab::Equipment)))
    {
        ActiveTab = EMaintenanceTab::Equipment;
        return;
    }

    if (IsPointInRect(mouseX, mouseY, GetTabRect(EMaintenanceTab::Skill)))
    {
        ActiveTab = EMaintenanceTab::Skill;
        return;
    }

    if (IsPointInRect(mouseX, mouseY, GetTabRect(EMaintenanceTab::Shop)))
    {
        ActiveTab = EMaintenanceTab::Shop;
        return;
    }

    std::vector<Character*>& party = GameManager::Get().GetParty();

    for (int i = 0; i < (int)party.size() && i < 5; ++i)
    {
        if (IsPointInRect(mouseX, mouseY, GetPartySelectRect(i)))
        {
            SelectedPartyIndex = i;
            return;
        }
    }

    if (ActiveTab == EMaintenanceTab::Party)
    {
        HandlePartyClick(mouseX, mouseY);
    }
    else if (ActiveTab == EMaintenanceTab::Inventory)
    {
        HandleInventoryClick(mouseX, mouseY);
    }
    else if (ActiveTab == EMaintenanceTab::Equipment)
    {
        HandleEquipmentClick(mouseX, mouseY);
    }
    else if (ActiveTab == EMaintenanceTab::Skill)
    {
        HandleSkillClick(mouseX, mouseY);
    }
    else if (ActiveTab == EMaintenanceTab::Shop)
    {
        HandleShopClick(mouseX, mouseY);
    }
}

void Maintenance_Stage::HandlePartyClick(int mouseX, int mouseY)
{
    std::vector<Character*>& roster = GameManager::Get().GetRoster();

    for (int i = 0; i < (int)roster.size(); ++i)
    {
        if (!IsPointInRect(mouseX, mouseY, GetRosterSlotRect(i)))
        {
            continue;
        }

        Character* c = roster[i];
        bool wasActive = GameManager::Get().IsInActiveParty(c);

        if (!GameManager::Get().ToggleActivePartyMember(c))
        {
            SkillMessage = wasActive ? L"At least one active character is required." : L"Active party is already full.";
            SkillMessageLife = 180;
            return;
        }

        std::vector<Character*>& party = GameManager::Get().GetParty();
        for (int p = 0; p < (int)party.size(); ++p)
        {
            if (party[p] == c)
            {
                SelectedPartyIndex = p;
                break;
            }
        }

        if (SelectedPartyIndex >= (int)party.size())
        {
            SelectedPartyIndex = (int)party.size() - 1;
        }

        return;
    }
}

void Maintenance_Stage::HandleSkillClick(int mouseX, int mouseY)
{
    Character* selected = GetSelectedCharacter();

    if (selected == nullptr)
    {
        return;
    }

    for (int i = 0; i < 3; ++i)
    {
        if (IsPointInRect(mouseX, mouseY, GetSkillSlotRect(i)))
        {
            SelectedSkillSlot = i;
            return;
        }
    }

    std::vector<int> learned = selected->GetLearnedSkillIDs();

    for (int i = 0; i < (int)learned.size(); ++i)
    {
        if (!IsPointInRect(mouseX, mouseY, GetLearnedSkillRect(i)))
        {
            continue;
        }

        int skillID = learned[i];
        selected->SetEquippedSkillID(SelectedSkillSlot, skillID);
        SkillMessage = L"Equipped " + SkillData::GetName(skillID) + L" to slot " + std::to_wstring(SelectedSkillSlot + 1) + L".";
        SkillMessageLife = 180;
        return;
    }
}

void Maintenance_Stage::HandleInventoryClick(int mouseX, int mouseY)
{
    Character* target = GetSelectedCharacter();

    if (target == nullptr)
    {
        return;
    }

    const std::vector<InventoryItem>& inventory = GameManager::Get().GetInventory();

    int maxSlots = 20;
    int count = (int)std::min<size_t>((size_t)maxSlots, inventory.size());

    for (int i = 0; i < count; ++i)
    {
        if (!IsPointInRect(mouseX, mouseY, GetInventorySlotRect(i)))
        {
            continue;
        }

        int itemID = inventory[i].ItemID;

        if (ItemData::IsEquipment(itemID))
        {
            GameManager::Get().EquipItem(target, itemID);
            ActiveTab = EMaintenanceTab::Equipment;
        }
        else if (ItemData::IsConsumable(itemID))
        {
            GameManager::Get().UseConsumable(target, itemID);
        }

        return;
    }
}

void Maintenance_Stage::HandleEquipmentClick(int mouseX, int mouseY)
{
    Character* target = GetSelectedCharacter();

    if (target == nullptr)
    {
        return;
    }

    EEquipPart parts[3] = { EEquipPart::Weapon, EEquipPart::Armor, EEquipPart::Accessory };

    for (int i = 0; i < 3; ++i)
    {
        if (!IsPointInRect(mouseX, mouseY, GetEquipmentSlotRect(i)))
        {
            continue;
        }

        GameManager::Get().UnequipItem(target, parts[i]);
        return;
    }
}


void Maintenance_Stage::HandleShopClick(int mouseX, int mouseY)
{
    std::vector<int> shopItems = GetShopItemIDs();

    for (int i = 0; i < (int)shopItems.size(); ++i)
    {
        if (!IsPointInRect(mouseX, mouseY, GetShopSlotRect(i)))
        {
            continue;
        }

        int itemID = shopItems[i];
        int price = ItemData::GetPrice(itemID);

        if (GameManager::Get().BuyItem(itemID, 1))
        {
            ShopMessage = L"Purchased " + ItemData::GetName(itemID) + L" for " + std::to_wstring(price) + L"G.";
            ShopMessageLife = 180;
        }
        else
        {
            ShopMessage = L"Cannot buy " + ItemData::GetName(itemID) + L". Not enough gold or invalid item.";
            ShopMessageLife = 180;
        }

        return;
    }
}

Character* Maintenance_Stage::GetSelectedCharacter() const
{
    const std::vector<Character*>& party = GameManager::Get().GetParty();

    if (party.empty())
    {
        return nullptr;
    }

    int index = SelectedPartyIndex;

    if (index < 0)
    {
        index = 0;
    }

    if (index >= (int)party.size())
    {
        index = (int)party.size() - 1;
    }

    return party[index];
}

std::wstring Maintenance_Stage::GetEquipPartName(EEquipPart part) const
{
    switch (part)
    {
    case EEquipPart::Weapon:
        return L"Weapon";

    case EEquipPart::Armor:
        return L"Armor";

    case EEquipPart::Accessory:
        return L"Accessory";

    default:
        return L"Unknown";
    }
}

std::wstring Maintenance_Stage::GetEquippedName(int itemID) const
{
    if (itemID < 0)
    {
        return L"Empty";
    }

    return ItemData::GetName(itemID);
}


std::vector<int> Maintenance_Stage::GetShopItemIDs() const
{
    int tier = GameManager::Get().GetMaxUnlockedBattleStageId();

    std::vector<int> items;

    items.push_back(101);
    items.push_back(102);
    items.push_back(201);
    items.push_back(301);

    if (tier >= 2)
    {
        items.push_back(103);
        items.push_back(202);
        items.push_back(302);
        items.push_back(401);
    }

    if (tier >= 3)
    {
        items.push_back(203);
        items.push_back(402);
    }

    return items;
}

std::wstring Maintenance_Stage::MakeStatLine(const CharacterStat& stat) const
{
    std::wstring line;
    line += L"STR " + std::to_wstring(stat.STR);
    line += L"   INT " + std::to_wstring(stat.INT);
    line += L"   DEF " + std::to_wstring(stat.DEF);
    line += L"   SPD " + std::to_wstring(stat.SPD);
    return line;
}
