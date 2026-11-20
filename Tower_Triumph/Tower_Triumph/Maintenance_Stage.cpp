#include "Maintenance_Stage.h"
#include "ItemData.h"
#include "StarlightUI.h"
#include "UMouseMark.h"
#include "resource.h"
#include <windowsx.h>

#include <algorithm>
#include <cmath>
#include <string>

namespace
{
    constexpr double TabPresentationDuration = 0.18;
    constexpr double CharacterPresentationDuration = 0.16;
    constexpr double EquipmentFeedbackDuration = 1.6;
    constexpr double SkillSelectionDuration = 0.14;
    constexpr double SkillReplacementDuration = 0.16;
    constexpr double SkillPulseDuration = 0.6;
    constexpr double ShopNoticeTransitionDuration = 0.16;
    constexpr double ShopFeedbackDuration = 2.6;
    constexpr double ShopNoticeHoldEnd = 2.2;
    constexpr double ShopNoticeExitDuration = 0.4;
    constexpr double ShopPulseHoldEnd = 1.1;
    constexpr double ShopPulseExitDuration = 0.5;
    constexpr double InventoryNoticeTransitionDuration = 0.16;
    constexpr double InventoryFeedbackDuration = 2.6;
    constexpr double InventoryNoticeHoldEnd = 2.2;
    constexpr double InventoryNoticeExitDuration = 0.4;
    constexpr double InventoryPulseHoldEnd = 1.1;
    constexpr double InventoryPulseExitDuration = 0.5;
    constexpr double InventoryBarDuration = 0.28;
    constexpr double PartyPresentationDuration = 0.16;
    constexpr double PartyFeedbackDuration = 2.6;
    constexpr double PartyNoticeHoldEnd = 2.2;
    constexpr double PartyNoticeExitDuration = 0.4;
    constexpr double PartyPulseHoldEnd = 1.1;
    constexpr double PartyPulseExitDuration = 0.5;

    std::wstring SignedValue(int value)
    {
        return (value >= 0 ? L"+" : L"") + std::to_wstring(value);
    }

    COLORREF MixColor(COLORREF from, COLORREF to, double amount)
    {
        return RGB(int(GetRValue(from) + (GetRValue(to) - GetRValue(from)) * amount + 0.5),
            int(GetGValue(from) + (GetGValue(to) - GetGValue(from)) * amount + 0.5),
            int(GetBValue(from) + (GetBValue(to) - GetBValue(from)) * amount + 0.5));
    }

    void StatSurface(HDC dc, const RECT& bounds, COLORREF fill, COLORREF border)
    {
        const int saved = SaveDC(dc);
        HBRUSH brush = CreateSolidBrush(fill);
        HPEN pen = CreatePen(PS_SOLID, 1, border);
        SelectObject(dc, brush); SelectObject(dc, pen);
        RoundRect(dc, bounds.left, bounds.top, bounds.right, bounds.bottom, 6, 6);
        RestoreDC(dc, saved); DeleteObject(pen); DeleteObject(brush);
    }
}

RECT Maintenance_Stage::GetTabPresentationRect() const
{
    const RECT panel = GetMainPanelRect();
    // Include the party/skill message footer, but never the header or tabs.
    return { panel.left + 16, panel.top + 150, panel.right - 16, panel.bottom - 8 };
}

void Maintenance_Stage::ResetTabPresentation()
{
    BodyPresentation.Reset();
    PresentedTabFrame.Reset();
    // Focus loss or a resized/failed framebuffer invalidates pixel snapshots,
    // while result messages, numbers and success pulses retain their lifetime.
    SkillFeedback.CancelTransition();
    ShopFeedback.CancelTransition();
    InventoryFeedback.CancelTransition();
    PartyFeedback.CancelTransition();
    LastTabPresentationUpdate = std::chrono::steady_clock::now();
}

void Maintenance_Stage::SelectTab(EMaintenanceTab tab)
{
    if (IsRewardMode() || tab == ActiveTab) return;
    BeginTabPresentation(BodyTransitionKind::Tab);
    ResetActionFeedback();
    ActiveTab = tab;
}

void Maintenance_Stage::SelectCharacter(int index)
{
    if (index == SelectedPartyIndex) return;
    if (ActiveTab == EMaintenanceTab::Equipment || ActiveTab == EMaintenanceTab::Skill)
        BeginTabPresentation(BodyTransitionKind::Character);
    ResetActionFeedback();
    SelectedPartyIndex = index;
}

void Maintenance_Stage::BeginTabPresentation(BodyTransitionKind kind)
{
    // Capture the last displayed mixture before clearing the action feedback.
    BodyPresentation.Begin(PresentedTabFrame, GetTabPresentationRect(), kind,
        kind == BodyTransitionKind::Character ? CharacterPresentationDuration : TabPresentationDuration);
    LastTabPresentationUpdate = std::chrono::steady_clock::now();
}

void Maintenance_Stage::AdvanceTabPresentation(double seconds)
{
    LastTabPresentationUpdate = std::chrono::steady_clock::now();
    if (IsRewardMode()) { ResetTabPresentation(); ResetActionFeedback(); return; }
    if (!std::isfinite(seconds) || seconds <= 0.0) return;
    EquipmentFeedback.Advance(seconds);
    SkillFeedback.Advance(seconds);
    ShopFeedback.Advance(seconds);
    InventoryFeedback.Advance(seconds);
    PartyFeedback.Advance(seconds);
    BodyPresentation.Advance(seconds);
}

bool Maintenance_Stage::IsTabTransitioning() const { return BodyPresentation.IsActive(BodyTransitionKind::Tab); }
double Maintenance_Stage::GetTabTransitionProgress() const { return BodyPresentation.Progress(BodyTransitionKind::Tab); }
bool Maintenance_Stage::IsCharacterTransitioning() const { return BodyPresentation.IsActive(BodyTransitionKind::Character); }
double Maintenance_Stage::GetCharacterTransitionProgress() const { return BodyPresentation.Progress(BodyTransitionKind::Character); }

void Maintenance_Stage::ResetActionFeedback()
{
    EquipmentFeedback.Reset();
    SkillFeedback.ResetPresentation();
    ShopFeedback.Reset();
    InventoryFeedback.Reset();
    PartyFeedback.Reset();
}

bool Maintenance_Stage::IsEquipmentFeedbackActive() const
{
    return ActiveTab == EMaintenanceTab::Equipment && EquipmentFeedback.IsActiveFor(GetSelectedCharacter());
}

double Maintenance_Stage::GetEquipmentFeedbackProgress() const { return IsEquipmentFeedbackActive() ? EquipmentFeedback.Progress() : 1.0; }
int Maintenance_Stage::GetEquipmentStatDelta(int index) const { return IsEquipmentFeedbackActive() ? EquipmentFeedback.StatDelta(index) : 0; }

void Maintenance_Stage::BeginEquipmentFeedback(Character* target, const CharacterStat& before, int part, bool equipmentChanged)
{
    if (EquipmentFeedback.Record(target, before, target->GetStat(), part, equipmentChanged))
        LastTabPresentationUpdate = std::chrono::steady_clock::now();
}

void Maintenance_Stage::BeginSkillFeedback(double duration)
{
    SkillFeedback.Begin(PresentedTabFrame, GetTabPresentationRect(), duration);
    LastTabPresentationUpdate = std::chrono::steady_clock::now();
}

bool Maintenance_Stage::IsSkillFeedbackTransitioning() const { return ActiveTab == EMaintenanceTab::Skill && SkillFeedback.IsTransitioning(); }
double Maintenance_Stage::GetSkillFeedbackProgress() const { return IsSkillFeedbackTransitioning() ? SkillFeedback.Progress() : 1.0; }

void Maintenance_Stage::CompositeSkillFeedback(HDC destination)
{
    if (!IsSkillFeedbackTransitioning()) return;
    const RECT body = GetTabPresentationRect();
    for (int i = 0; i < 3; ++i) if (!SkillFeedback.Composite(destination, body, GetSkillSlotRect(i))) return;
    const Character* selected = GetSelectedCharacter();
    if (selected)
        for (int i = 0; i < int(selected->GetLearnedSkillIDs().size()); ++i)
            if (!SkillFeedback.Composite(destination, body, GetLearnedSkillRect(i))) return;
    const RECT panel = GetMainPanelRect();
    SkillFeedback.Composite(destination, body, { panel.left + 28, panel.bottom - 51, panel.right - 28, panel.bottom - 17 });
}

bool Maintenance_Stage::IsShopFeedbackActive() const { return ActiveTab == EMaintenanceTab::Shop && ShopFeedback.IsActive(); }
double Maintenance_Stage::GetShopFeedbackProgress() const { return IsShopFeedbackActive() ? ShopFeedback.Progress() : 1.0; }
double Maintenance_Stage::GetShopNoticeOpacity() const { return IsShopFeedbackActive() ? ShopFeedback.Opacity() : 0.0; }
int Maintenance_Stage::GetShopGoldDelta() const { return IsShopFeedbackActive() ? ShopFeedback.GoldDelta() : 0; }
int Maintenance_Stage::GetShopQuantityDelta() const { return IsShopFeedbackActive() ? ShopFeedback.QuantityDelta() : 0; }
int Maintenance_Stage::GetShopShortfall() const { return IsShopFeedbackActive() ? ShopFeedback.MissingGold() : 0; }
int Maintenance_Stage::GetShopFeedbackItemID() const { return IsShopFeedbackActive() ? ShopFeedback.FeedbackItemID() : -1; }
double Maintenance_Stage::GetShopPulseStrength() const { return IsShopFeedbackActive() ? ShopFeedback.Pulse() : 0.0; }

void Maintenance_Stage::BeginShopFeedback(int itemID, bool purchased, int oldGold, int oldCount)
{
    const int price = ItemData::GetPrice(itemID);
    const std::wstring message = purchased ? L"Purchased " + ItemData::GetName(itemID) + L" for " + std::to_wstring(price) + L"G." :
        L"Cannot buy " + ItemData::GetName(itemID) + L". Not enough gold or invalid item.";
    ShopFeedback.Record(PresentedTabFrame, GetTabPresentationRect(), itemID, purchased,
        purchased ? GameManager::Get().GetGold() - oldGold : 0,
        purchased ? GameManager::Get().GetItemCount(itemID) - oldCount : 0,
        !purchased ? (std::max)(0, price - oldGold) : 0, message);
    LastTabPresentationUpdate = std::chrono::steady_clock::now();
}

void Maintenance_Stage::CompositeShopNotice(HDC destination)
{
    if (ActiveTab != EMaintenanceTab::Shop) return;
    const RECT panel = GetMainPanelRect();
    ShopFeedback.Composite(destination, GetTabPresentationRect(), { panel.left + 28, panel.bottom - 46, panel.right - 28, panel.bottom - 16 });
}

bool Maintenance_Stage::IsInventoryFeedbackActive() const
{
    return ActiveTab == EMaintenanceTab::Inventory && InventoryFeedback.IsActiveFor(GetSelectedCharacter());
}
double Maintenance_Stage::GetInventoryFeedbackProgress() const { return IsInventoryFeedbackActive() ? InventoryFeedback.Progress() : 1.0; }
double Maintenance_Stage::GetInventoryNoticeOpacity() const { return IsInventoryFeedbackActive() ? InventoryFeedback.Opacity() : 0.0; }
int Maintenance_Stage::GetInventoryHPDelta() const { return IsInventoryFeedbackActive() ? InventoryFeedback.HPDelta() : 0; }
int Maintenance_Stage::GetInventoryMPDelta() const { return IsInventoryFeedbackActive() ? InventoryFeedback.MPDelta() : 0; }
int Maintenance_Stage::GetInventoryQuantityDelta() const { return IsInventoryFeedbackActive() ? InventoryFeedback.QuantityDelta() : 0; }
int Maintenance_Stage::GetInventoryRemainingCount() const { return IsInventoryFeedbackActive() ? InventoryFeedback.RemainingCount() : 0; }
int Maintenance_Stage::GetInventoryFeedbackItemID() const { return IsInventoryFeedbackActive() ? InventoryFeedback.FeedbackItemID() : -1; }
bool Maintenance_Stage::WasInventoryUseSuccessful() const { return IsInventoryFeedbackActive() && InventoryFeedback.WasSuccessful(); }
double Maintenance_Stage::GetInventoryPulseStrength() const { return IsInventoryFeedbackActive() ? InventoryFeedback.Pulse() : 0.0; }

void Maintenance_Stage::UseInventoryConsumable(Character* target, int itemID)
{
    const CharacterStat before = target ? target->GetStat() : CharacterStat{};
    const int oldCount = GameManager::Get().GetItemCount(itemID);
    // Gameplay owns consumption, including the agreed full-HP/MP potion use.
    const bool used = GameManager::Get().UseConsumable(target, itemID);
    const CharacterStat after = target ? target->GetStat() : before;
    const int remaining = GameManager::Get().GetItemCount(itemID);
    const int hpChange = used ? after.CurHP - before.CurHP : 0;
    const int mpChange = used ? after.CurMP - before.CurMP : 0;
    std::wstring message;
    if (used)
    {
        message = L"Used " + ItemData::GetName(itemID) + L".";
        if (ItemData::GetHealHP(itemID) > 0) message += L" HP " + SignedValue(hpChange);
        if (ItemData::GetHealMP(itemID) > 0) message += L" MP " + SignedValue(mpChange);
        if (hpChange == 0 && mpChange == 0) message += L" (already full)";
        message += L". Bag: " + std::to_wstring(remaining) + L" (" + SignedValue(remaining - oldCount) + L").";
    }
    else
    {
        message = L"Cannot use " + ItemData::GetName(itemID) + L". ";
        if (!target) message += L"No character selected.";
        else if (!ItemData::IsValidItem(itemID) || !ItemData::IsConsumable(itemID)) message += L"Not a usable potion.";
        else if (oldCount <= 0) message += L"Not in bag.";
        else message += L"This item has no recovery effect.";
        message += L" Nothing consumed.";
    }
    InventoryFeedback.Record(PresentedTabFrame, GetTabPresentationRect(), target, itemID, before, after, oldCount, remaining, used, message);
    LastTabPresentationUpdate = std::chrono::steady_clock::now();
}

void Maintenance_Stage::CompositeInventoryNotice(HDC destination)
{
    if (!IsInventoryFeedbackActive()) return;
    const RECT panel = GetMainPanelRect();
    InventoryFeedback.Composite(destination, GetTabPresentationRect(), { panel.left + 28, panel.bottom - 39, panel.right - 28, panel.bottom - 12 });
}

bool Maintenance_Stage::IsPartyFeedbackActive() const { return ActiveTab == EMaintenanceTab::Party && PartyFeedback.IsActive(); }
double Maintenance_Stage::GetPartyFeedbackProgress() const { return IsPartyFeedbackActive() ? PartyFeedback.Progress() : 1.0; }
double Maintenance_Stage::GetPartyNoticeOpacity() const { return IsPartyFeedbackActive() ? PartyFeedback.Opacity() : 0.0; }
int Maintenance_Stage::GetPartyCountDelta() const { return IsPartyFeedbackActive() ? PartyFeedback.CountDelta() : 0; }
int Maintenance_Stage::GetPartyFeedbackCharacterID() const { return IsPartyFeedbackActive() ? PartyFeedback.FeedbackCharacterID() : -1; }
bool Maintenance_Stage::WasPartyChangeSuccessful() const { return IsPartyFeedbackActive() && PartyFeedback.WasSuccessful(); }
bool Maintenance_Stage::IsPartyPresentationTransitioning() const { return ActiveTab == EMaintenanceTab::Party && PartyFeedback.IsTransitioning(); }
double Maintenance_Stage::GetPartyPresentationProgress() const { return IsPartyPresentationTransitioning() ? PartyFeedback.TransitionProgress() : 1.0; }
double Maintenance_Stage::GetPartyPulseStrength() const { return IsPartyFeedbackActive() ? PartyFeedback.Pulse() : 0.0; }

void Maintenance_Stage::BeginPartyFeedback(Character* character, int oldCount, bool wasActive, bool changed)
{
    const std::wstring message = changed ? (wasActive ? L"Removed " : L"Added ") + character->GetName() +
        (wasActive ? L" from the active party." : L" to the active party.") :
        wasActive ? L"At least one active character is required." : L"Active party is already full.";
    PartyFeedback.Record(PresentedTabFrame, GetTabPresentationRect(), character ? character->GetCharacterID() : -1,
        changed ? int(GameManager::Get().GetParty().size()) - oldCount : 0, changed, message);
    LastTabPresentationUpdate = std::chrono::steady_clock::now();
}

void Maintenance_Stage::CompositePartyPresentation(HDC destination)
{
    if (!IsPartyPresentationTransitioning()) return;
    const RECT body = GetTabPresentationRect(), panel = GetMainPanelRect(), first = GetRosterSlotRect(0);
    const int rowHeight = first.bottom - first.top;
    if (!PartyFeedback.Composite(destination, body, { panel.left + 28, first.top, panel.left + 304, first.top + 4 * (rowHeight + 8) + rowHeight })) return;
    for (int i = 0; i < int(GameManager::Get().GetRoster().size()); ++i)
        if (!PartyFeedback.Composite(destination, body, GetRosterSlotRect(i))) return;
    PartyFeedback.Composite(destination, body, { panel.left + 28, panel.bottom - 34, panel.right - 28, panel.bottom - 10 });
}

void Maintenance_Stage::CompositeTabPresentation(HDC destination)
{
    if (!UIFrameSnapshot::IsRasterTarget(destination)) return;
    const RECT bounds = GetTabPresentationRect();
    if (PresentedTabFrame.HasFrame() && !PresentedTabFrame.Matches(bounds)) ResetTabPresentation();
    if (!BodyPresentation.Composite(destination, bounds)) { ResetTabPresentation(); return; }
    // Cache only the final displayed mixture, after all local and body fades.
    if (!PresentedTabFrame.Capture(destination, bounds)) ResetTabPresentation();
}

Maintenance_Stage::Maintenance_Stage()
    : RewardApplied(false),
      ActiveTab(EMaintenanceTab::Reward),
      SelectedPartyIndex(0),
      SelectedSkillSlot(0),
      EquipmentFeedback(EquipmentFeedbackDuration),
      SkillFeedback(SkillPulseDuration),
      ShopFeedback({ ShopNoticeTransitionDuration, ShopFeedbackDuration, ShopNoticeHoldEnd, ShopNoticeExitDuration, ShopPulseHoldEnd, ShopPulseExitDuration }),
      InventoryFeedback({ InventoryNoticeTransitionDuration, InventoryFeedbackDuration, InventoryNoticeHoldEnd, InventoryNoticeExitDuration, InventoryPulseHoldEnd, InventoryPulseExitDuration }, InventoryBarDuration),
      PartyFeedback({ PartyPresentationDuration, PartyFeedbackDuration, PartyNoticeHoldEnd, PartyNoticeExitDuration, PartyPulseHoldEnd, PartyPulseExitDuration })
{
    CurStageID = 3;
    NextStageID = -2;

    W = 0;
    H = 0;
    hBitBackground = NULL;
}

Maintenance_Stage::~Maintenance_Stage()
{
    if (MouseMark && MouseMark->IsVisible()) SetCursor(LoadCursor(nullptr, IDC_ARROW));
    delete MouseMark;
    if (hBitBackground) DeleteObject(hBitBackground);
}

void Maintenance_Stage::Init(HWND hWnd, HINSTANCE const g_hInst)
{
    ResetTabPresentation();
    ResetActionFeedback();
    GetClientRect(hWnd, &rects);
    if (hBitBackground) DeleteObject(hBitBackground);
    hBitBackground = static_cast<HBITMAP>(LoadImageW(nullptr,
        L"Source\\SMap\\background\\Map_Background.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION));
    if (!hBitBackground) hBitBackground = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP3));
    if (hBitBackground) GetObject(hBitBackground, sizeof(BackgroundInfo), &BackgroundInfo);
    delete MouseMark;
    MouseMark = new UMouseMark();
    MouseMark->SetFrame(g_hInst, { IDB_UI_MOUSECUSOR });
    MouseMark->SetVisible(false);
    RefreshPointer(hWnd);

    RewardApplied = false;
    ActiveTab = BattleRewardManager::Get().HasReward() ? EMaintenanceTab::Reward : EMaintenanceTab::Party;
    SelectedPartyIndex = 0;
    LevelResults.clear();
    SkillFeedback.Reset();
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
    const auto now = std::chrono::steady_clock::now();
    AdvanceTabPresentation(std::chrono::duration<double>(now - LastTabPresentationUpdate).count());
    RefreshPointer(hWnd);
    if (MouseMark) MouseMark->Update();
    ApplyRewardOnce();

    SkillFeedback.AdvanceMessageFrame();
}


void Maintenance_Stage::Draw(HDC dc, HDC scratch)
{
    HBRUSH background = CreateSolidBrush(StarlightUI::Navy);
    FillRect(dc, &rects, background); DeleteObject(background);
    if (hBitBackground && BackgroundInfo.bmWidth > 0 && BackgroundInfo.bmHeight > 0)
    {
        HDC source = CreateCompatibleDC(dc); HGDIOBJ old = SelectObject(source, hBitBackground);
        const double scale = (std::max)(double(rects.right - rects.left) / BackgroundInfo.bmWidth,
            double(rects.bottom - rects.top) / BackgroundInfo.bmHeight);
        const int width = int(BackgroundInfo.bmWidth * scale), height = int(BackgroundInfo.bmHeight * scale);
        const int saved = SaveDC(dc); SetStretchBltMode(dc, HALFTONE); SetBrushOrgEx(dc, 0, 0, nullptr);
        StretchBlt(dc, (rects.left + rects.right - width) / 2, (rects.top + rects.bottom - height) / 2,
            width, height, source, 0, 0, BackgroundInfo.bmWidth, BackgroundInfo.bmHeight, SRCCOPY);
        RestoreDC(dc, saved); SelectObject(source, old); DeleteDC(source);
    }
    StarlightUI::Dim(dc, rects, 135);
    const RECT panel = GetMainPanelRect(); DrawPanel(dc, panel);
    if (IsRewardMode())
    {
        ResetTabPresentation();
        DrawRewardTab(dc, panel); DrawNextStageButton(dc);
    }
    else
    {
        DrawTabs(dc);
        switch (ActiveTab)
        {
        case EMaintenanceTab::Reward: DrawRewardTab(dc, panel); break;
        case EMaintenanceTab::Party: DrawPartyTab(dc, panel); break;
        case EMaintenanceTab::Inventory: DrawInventoryTab(dc, panel); break;
        case EMaintenanceTab::Equipment: DrawEquipmentTab(dc, panel); break;
        case EMaintenanceTab::Skill: DrawSkillTab(dc, panel); break;
        case EMaintenanceTab::Shop: DrawShopTab(dc, panel); break;
        }
        CompositeSkillFeedback(dc);
        CompositeShopNotice(dc);
        CompositeInventoryNotice(dc);
        CompositePartyPresentation(dc);
        CompositeTabPresentation(dc);
    }
    DrawContinueButton(dc);
}

void Maintenance_Stage::DrawCursor(HDC dc, HDC scratch)
{
    if (MouseMark) MouseMark->Draw(dc, scratch);
}

void Maintenance_Stage::RefreshPointer(HWND hWnd)
{
    if (!MouseMark || !hWnd) return;
    POINT screen{}; GetCursorPos(&screen); POINT client = screen;
    const bool converted = ScreenToClient(hWnd, &client) != FALSE;
    const bool inside = converted && PtInRect(&rects, client) && GetForegroundWindow() == hWnd && WindowFromPoint(screen) == hWnd;
    PointerPosition = inside ? client : POINT{ -1, -1 };
    MouseMark->SetMousePos(client); MouseMark->SetVisible(inside && MouseMark->IsDrawable());
    if (inside) SetCursor(MouseMark->IsVisible() ? nullptr : LoadCursor(nullptr, IDC_ARROW));
}

bool Maintenance_Stage::HandleSetCursor(HWND hWnd, WPARAM, LPARAM lParam)
{
    if (LOWORD(lParam) != HTCLIENT) return false;
    RefreshPointer(hWnd);
    return MouseMark && MouseMark->IsVisible();
}

int Maintenance_Stage::Input(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_KILLFOCUS || uMsg == WM_CANCELMODE)
    {
        ResetTabPresentation();
        PointerPosition = { -1, -1 };
        if (MouseMark) MouseMark->SetVisible(false);
        if (hWnd) SetCursor(LoadCursor(nullptr, IDC_ARROW));
        return -2;
    }
    if (uMsg == WM_MOUSEMOVE)
    {
        PointerPosition = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        RefreshPointer(hWnd);
    }
    if (uMsg == WM_LBUTTONDOWN)
    {
        int mouseX = LOWORD(lParam);
        int mouseY = HIWORD(lParam);

        if (IsRewardMode())
        {
            ApplyRewardOnce();
            const int next = BattleRewardManager::Get().GetNextStageId();
            if (next >= 1 && next <= 5 && GameManager::Get().IsBattleStageUnlocked(next) &&
                IsPointInRect(mouseX, mouseY, GetNextStageRect()))
            {
                ApplyRewardOnce();
                int nextStageId = BattleRewardManager::Get().GetNextStageId();
                BattleRewardManager::Get().Clear();

                if (nextStageId >= 1 && nextStageId <= 5 && GameManager::Get().IsBattleStageUnlocked(nextStageId))
                {
                    GameManager::Get().SetCurrentBattleStageId(nextStageId);
                    return BATTLE_STAGE;
                }

                return MAP_STAGE;
            }

            if (IsPointInRect(mouseX, mouseY, GetContinueRect()) || !IsPointInRect(mouseX, mouseY, GetMainPanelRect()))
            {
                ApplyRewardOnce();
                BattleRewardManager::Get().Clear();
                return MAP_STAGE;
            }

            return -2;
        }

        if (IsPointInRect(mouseX, mouseY, GetContinueRect()))
        {
            ApplyRewardOnce();
            BattleRewardManager::Get().Clear();
            return MAP_STAGE;
        }

        HandleMouseDown(mouseX, mouseY);
        return -2;
    }

    if (uMsg == WM_KEYDOWN)
    {
        if (wParam == VK_RETURN || wParam == VK_SPACE || wParam == VK_ESCAPE)
        {
            ApplyRewardOnce();
            BattleRewardManager::Get().Clear();
            return MAP_STAGE;
        }

        if (IsRewardMode())
        {
            return -2;
        }

        if (wParam == '1')
        {
            SelectTab(EMaintenanceTab::Reward);
        }
        else if (wParam == '2')
        {
            SelectTab(EMaintenanceTab::Party);
        }
        else if (wParam == '3')
        {
            SelectTab(EMaintenanceTab::Inventory);
        }
        else if (wParam == '4')
        {
            SelectTab(EMaintenanceTab::Equipment);
        }
        else if (wParam == '5')
        {
            SelectTab(EMaintenanceTab::Skill);
        }
        else if (wParam == '6')
        {
            SelectTab(EMaintenanceTab::Shop);
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
    return StarlightUI::Centered(rects, 1120, 720);
}

RECT Maintenance_Stage::GetBodyRect() const
{
    const RECT p = GetMainPanelRect();
    return { p.left + 28, p.top + (IsRewardMode() ? 108 : 158), p.right - 28, p.bottom - 20 };
}

RECT Maintenance_Stage::GetTabRect(EMaintenanceTab tab) const
{
    const RECT p = GetMainPanelRect();
    const int width = (p.right - p.left - 56 - 5 * 8) / 6;
    const int x = p.left + 28 + static_cast<int>(tab) * (width + 8);
    return { x, p.top + 104, x + width, p.top + 144 };
}

RECT Maintenance_Stage::GetContinueRect() const
{
    const RECT p = GetMainPanelRect();
    return { p.right - 156, p.top + 29, p.right - 28, p.top + 71 };
}

RECT Maintenance_Stage::GetNextStageRect() const
{
    const RECT p = GetMainPanelRect();
    return { p.right - 340, p.top + 29, p.right - 170, p.top + 71 };
}

bool Maintenance_Stage::IsRewardMode() const
{
    return BattleRewardManager::Get().HasReward();
}

RECT Maintenance_Stage::GetPartySelectRect(int index) const
{
    const RECT body = GetBodyRect();
    const int width = (body.right - body.left - 4 * 8) / 5;
    const int x = body.left + index * (width + 8);
    return { x, body.top, x + width, body.top + 38 };
}

RECT Maintenance_Stage::GetInventorySlotRect(int index) const
{
    const RECT p = GetMainPanelRect();
    const int columns = GameManager::Get().GetInventory().size() > 10 ? 4 : 2;
    const int width = (p.right - p.left - 56 - (columns - 1) * 10) / columns;
    const int height = (std::min<int>)(58, (p.bottom - p.top - 310 - 32) / 5);
    const int x = p.left + 28 + (index % columns) * (width + 10);
    const int y = p.top + 246 + (index / columns) * (height + 8);
    return { x, y, x + width, y + height };
}

RECT Maintenance_Stage::GetEquipmentSlotRect(int index) const
{
    const RECT p = GetMainPanelRect();
    return { p.left + (p.right - p.left) / 2 + 20, p.top + 246 + index * 82,
        p.right - 28, p.top + 316 + index * 82 };
}

RECT Maintenance_Stage::GetShopSlotRect(int index) const
{
    const RECT p = GetMainPanelRect();
    const int width = (p.right - p.left - 56 - 12) / 2;
    const int height = (std::min<int>)(58, (p.bottom - p.top - 282 - 32) / 5);
    const int x = p.left + 28 + (index % 2) * (width + 12);
    const int y = p.top + 206 + (index / 2) * (height + 8);
    return { x, y, x + width, y + height };
}

RECT Maintenance_Stage::GetRosterSlotRect(int index) const
{
    const RECT p = GetMainPanelRect();
    const int startX = p.left + 328;
    const int width = (p.right - 28 - startX - 2 * 10) / 3;
    const int height = (std::min<int>)(64, (p.bottom - p.top - 256 - 32) / 5);
    const int x = startX + (index % 3) * (width + 10);
    const int y = p.top + 190 + (index / 3) * (height + 8);
    return { x, y, x + width, y + height };
}

RECT Maintenance_Stage::GetSkillSlotRect(int index) const
{
    const RECT p = GetMainPanelRect();
    const int width = (p.right - p.left - 56 - 24) / 3;
    const int height = (std::min<int>)(86, (std::max<int>)(64, (p.bottom - p.top - 420) / 2));
    const int x = p.left + 28 + index * (width + 12);
    return { x, p.top + 304, x + width, p.top + 304 + height };
}

RECT Maintenance_Stage::GetLearnedSkillRect(int index) const
{
    const RECT slot = GetSkillSlotRect(index % 3);
    const int height = slot.bottom - slot.top;
    const int top = slot.bottom + 46 + (index / 3) * (height + 10);
    return { slot.left, top, slot.right, top + height };
}

bool Maintenance_Stage::IsPointInRect(int x, int y, const RECT& rc) const
{
    return x >= rc.left && x < rc.right && y >= rc.top && y < rc.bottom;
}

void Maintenance_Stage::DrawPanel(HDC dc, const RECT& panel)
{
    StarlightUI::Panel(dc, panel, IsRewardMode() ? L"전투 보상" : L"정비", L"PREPARATION CAMP");
    const int right = IsRewardMode() ? panel.right - 358 : panel.right - 176;
    StarlightUI::Text(dc, L"GOLD " + std::to_wstring(GameManager::Get().GetGold()),
        { right - 180, panel.top + 31, right, panel.top + 69 }, 19, StarlightUI::Gold,
        DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
}

void Maintenance_Stage::DrawTabs(HDC dc)
{
    const wchar_t* labels[] = { L"1. 보상", L"2. 파티", L"3. 인벤토리", L"4. 장비", L"5. 스킬", L"6. 상점" };
    for (int i = 0; i < 6; ++i)
    {
        const auto tab = static_cast<EMaintenanceTab>(i);
        const bool selected = ActiveTab == tab;
        DrawBoxText(dc, GetTabRect(tab), labels[i], 18, FW_NORMAL, StarlightUI::Ink,
            selected ? RGB(52, 48, 71) : RGB(27, 32, 54),
            selected ? StarlightUI::Gold : RGB(101, 88, 112), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
}

void Maintenance_Stage::DrawContinueButton(HDC dc)
{
    DrawBoxText(dc, GetContinueRect(), L"맵으로", 18, FW_NORMAL, StarlightUI::Ink,
        RGB(34, 38, 61), RGB(128, 107, 72), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void Maintenance_Stage::DrawNextStageButton(HDC dc)
{
    const int next = BattleRewardManager::Get().GetNextStageId();
    if (!IsRewardMode() || next < 1 || next > 5 || !GameManager::Get().IsBattleStageUnlocked(next)) return;
    DrawBoxText(dc, GetNextStageRect(), L"다음 스테이지", 18, FW_NORMAL, StarlightUI::Ink,
        RGB(83, 67, 45), StarlightUI::Gold, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void Maintenance_Stage::DrawPortrait(HDC dc, Character* character, const RECT& bounds)
{
    if (!character) return;
    HBITMAP bitmap = character->GetIllustration();
    const bool fallback = !bitmap || !character->GetIllustrationInfo();
    if (fallback) bitmap = character->GetSpriteIdle();
    if (!bitmap) return;
    BITMAP info{}; GetObject(bitmap, sizeof(info), &info);
    int width = info.bmWidth, height = info.bmHeight;
    if (fallback && !CharacterSpriteLayout::IdlePose::HasDimensions(width, height))
    { width /= CHARACTER_SHEET_COL; height /= CHARACTER_SHEET_ROW; }
    if (width <= 0 || height <= 0) return;
    const double scale = (std::min)(double(bounds.right - bounds.left) / width, double(bounds.bottom - bounds.top) / height);
    const int w = (std::max)(1, int(width * scale)), h = (std::max)(1, int(height * scale));
    HDC source = CreateCompatibleDC(dc); HGDIOBJ old = SelectObject(source, bitmap);
    const int mode = SetStretchBltMode(dc, COLORONCOLOR);
    TransparentBlt(dc, (bounds.left + bounds.right - w) / 2, (bounds.top + bounds.bottom - h) / 2,
        w, h, source, 0, 0, width, height, fallback ? CHARACTER_TRANSPARENT_COLOR : character->GetIllustrationTransparentColor());
    SetStretchBltMode(dc, mode); SelectObject(source, old); DeleteDC(source);
}

void Maintenance_Stage::DrawRewardTab(HDC dc, const RECT& panel)
{
    BattleRewardManager& reward = BattleRewardManager::Get();
    const RECT body = GetBodyRect();
    const int leftWidth = (body.right - body.left) / 2 - 28;
    int y = body.top;
    DrawLeftText(dc, L"REWARD SUMMARY", body.left, y, leftWidth, 28, 20, FW_NORMAL, StarlightUI::Gold);
    y += 36;
    if (!reward.HasReward())
    {
        DrawLeftText(dc, L"Town mode: no recent battle reward.", body.left, y, body.right - body.left, 36, 19, FW_NORMAL, StarlightUI::Muted);
        return;
    }
    DrawLeftText(dc, reward.IsVictory() ? L"Battle Result: Victory" : L"Battle Result: Defeat", body.left, y, leftWidth, 26, 19, FW_NORMAL,
        reward.IsVictory() ? StarlightUI::Ink : RGB(230, 166, 166)); y += 28;
    DrawLeftText(dc, L"Cleared Stage: " + std::to_wstring(reward.GetClearedStageId()), body.left, y, leftWidth, 24, 17, FW_NORMAL, StarlightUI::Muted); y += 28;
    DrawLeftText(dc, L"Gold +" + std::to_wstring(reward.GetRewardGold()), body.left, y, leftWidth, 26, 19, FW_NORMAL, StarlightUI::Gold); y += 28;
    DrawLeftText(dc, L"EXP +" + std::to_wstring(reward.GetRewardExp()), body.left, y, leftWidth, 26, 19, FW_NORMAL, RGB(188, 167, 203)); y += 32;
    DrawLeftText(dc, L"Items", body.left, y, leftWidth, 24, 18, FW_NORMAL, StarlightUI::Ink); y += 26;
    const auto& items = reward.GetRewardItems();
    if (items.empty())
    { DrawLeftText(dc, L"- None", body.left + 12, y, leftWidth - 12, 24, 17, FW_NORMAL, StarlightUI::Muted); y += 24; }
    else
    {
        const int rowHeight = (std::min<int>)(24, (std::max<int>)(18, (body.bottom - y - 36) / int(items.size())));
        for (const auto& item : items)
        {
            DrawLeftText(dc, L"- " + ItemData::GetName(item.ItemID) + L" x" + std::to_wstring(item.Count), body.left + 12, y, leftWidth - 12,
                rowHeight, (std::min)(17, rowHeight - 2), FW_NORMAL, StarlightUI::Ink); y += rowHeight;
        }
    }
    DrawLeftText(dc, L"Party Gold: " + std::to_wstring(GameManager::Get().GetGold()), body.left, y + 7, leftWidth, 28, 19, FW_NORMAL, StarlightUI::Gold);
    const int levelX = body.left + leftWidth + 42, levelWidth = body.right - levelX;
    DrawLeftText(dc, L"LEVEL / EXP RESULT", levelX, body.top, levelWidth, 28, 20, FW_NORMAL, RGB(188, 167, 203));
    if (LevelResults.empty())
    { DrawLeftText(dc, L"No EXP was applied.", levelX, body.top + 36, levelWidth, 28, 18, FW_NORMAL, StarlightUI::Muted); return; }
    const int rowHeight = (std::min<int>)(94, (body.bottom - body.top - 34) / int(LevelResults.size()));
    for (size_t i = 0; i < LevelResults.size(); ++i)
    {
        const auto& r = LevelResults[i]; const int top = body.top + 34 + int(i) * rowHeight;
        const bool compact = rowHeight < 86;
        DrawLeftText(dc, r.CharacterName, levelX, top, levelWidth, 22, compact ? 17 : 18, FW_NORMAL, r.LeveledUp ? StarlightUI::Gold : StarlightUI::Ink);
        DrawLeftText(dc, L"Lv " + std::to_wstring(r.OldLevel) + L" -> " + std::to_wstring(r.NewLevel) + L"    EXP " + std::to_wstring(r.OldExp) + L" -> " + std::to_wstring(r.NewExp),
            levelX, top + 22, levelWidth, compact ? 20 : 22, compact ? 15 : 16, FW_NORMAL, StarlightUI::Muted);
        if (r.LeveledUp)
        {
            const std::wstring first = L"HP +" + std::to_wstring(r.NewStat.MaxHP - r.OldStat.MaxHP) + L"   MP +" + std::to_wstring(r.NewStat.MaxMP - r.OldStat.MaxMP) + L"   STR +" + std::to_wstring(r.NewStat.STR - r.OldStat.STR);
            const std::wstring second = L"INT +" + std::to_wstring(r.NewStat.INT - r.OldStat.INT) + L"   DEF +" + std::to_wstring(r.NewStat.DEF - r.OldStat.DEF) + L"   SPD +" + std::to_wstring(r.NewStat.SPD - r.OldStat.SPD);
            DrawLeftText(dc, first, levelX, top + (compact ? 42 : 44), levelWidth, compact ? 18 : 22, compact ? 13 : 15, FW_NORMAL, RGB(188, 167, 203));
            DrawLeftText(dc, second, levelX, top + (compact ? 60 : 66), levelWidth, compact ? 18 : 22, compact ? 13 : 15, FW_NORMAL, RGB(188, 167, 203));
        }
        else DrawLeftText(dc, L"No Level Up", levelX, top + 45, levelWidth, 24, 16, FW_NORMAL, StarlightUI::Muted);
    }
}

void Maintenance_Stage::DrawPartyTab(HDC dc, const RECT& panel)
{
    const auto& party = GameManager::Get().GetParty(); const auto& roster = GameManager::Get().GetRoster();
    const RECT first = GetRosterSlotRect(0); const int rowHeight = first.bottom - first.top;
    DrawLeftText(dc, L"ACTIVE PARTY  " + std::to_wstring(party.size()) + L" / " + std::to_wstring(GameManager::Get().GetMaxActivePartySize()),
        panel.left + 28, panel.top + 158, 212, 26, 18, FW_NORMAL, StarlightUI::Gold);
    const double pulse = GetPartyPulseStrength();
    const COLORREF accent = !PartyFeedback.WasSuccessful() ? RGB(245, 153, 152) : PartyFeedback.CountDelta() > 0 ? RGB(128, 220, 173) : StarlightUI::Gold;
    if (GetPartyCountDelta() != 0 && pulse > 0.0)
        StarlightUI::Text(dc, SignedValue(GetPartyCountDelta()),
            { panel.left + 244, panel.top + 158, panel.left + 304, panel.top + 184 }, 18,
            MixColor(StarlightUI::Navy, accent, pulse), DT_RIGHT | DT_SINGLELINE);
    DrawLeftText(dc, L"ROSTER", first.left, panel.top + 158, 400, 26, 18, FW_NORMAL, StarlightUI::Gold);
    for (int i = 0; i < int(party.size()) && i < 5; ++i)
    {
        Character* c = party[i]; const int y = first.top + i * (rowHeight + 8);
        DrawPortrait(dc, c, { panel.left + 28, y + 2, panel.left + 64, y + rowHeight - 2 });
        DrawLeftText(dc, std::to_wstring(i + 1) + L". " + (c ? c->GetName() : L"Empty"), panel.left + 74, y + 4, 226, 25, 17, FW_NORMAL, StarlightUI::Ink);
        if (c) DrawLeftText(dc, L"/ " + c->GetElementName(), panel.left + 74, y + 29, 226, 21, 15, FW_NORMAL, RGB(188, 167, 203));
    }
    for (int i = 0; i < int(roster.size()); ++i)
    {
        Character* c = roster[i]; if (!c) continue;
        const RECT rc = GetRosterSlotRect(i); const bool active = GameManager::Get().IsInActiveParty(c);
        const bool hovered = IsPointInRect(PointerPosition.x, PointerPosition.y, rc);
        const double emphasis = c->GetCharacterID() == GetPartyFeedbackCharacterID() ? pulse : 0.0;
        const COLORREF fill = hovered ? RGB(58, 52, 80) : active ? RGB(52, 48, 71) : RGB(27, 32, 54);
        const COLORREF border = hovered || active ? StarlightUI::Gold : RGB(101, 88, 112);
        StatSurface(dc, rc, MixColor(fill, accent, emphasis * 0.14), MixColor(border, accent, emphasis * 0.85));
        DrawPortrait(dc, c, { rc.left + 7, rc.top + 5, rc.left + 43, rc.bottom - 5 });
        DrawLeftText(dc, c->GetName(), rc.left + 52, rc.top + 7, rc.right - rc.left - 60, 25, 16, FW_NORMAL, StarlightUI::Ink);
        if (active) DrawLeftText(dc, L"ACTIVE", rc.left + 52, rc.top + 31, rc.right - rc.left - 60, 20, 13, FW_NORMAL, RGB(188, 167, 203));
    }
    DrawLeftText(dc, L"Click a roster character to add/remove. At least 1 and up to 5 characters can be active.",
        panel.left + 28, panel.bottom - 57, panel.right - panel.left - 56, 18, 14, FW_NORMAL, StarlightUI::Muted);
    if (IsPartyFeedbackActive())
    {
        DrawLeftText(dc, PartyFeedback.Message(), panel.left + 28, panel.bottom - 34, panel.right - panel.left - 56, 24, 17, FW_NORMAL,
            MixColor(StarlightUI::Navy, accent, PartyFeedback.DrawOpacity(dc, GetTabPresentationRect())));
    }
}

void Maintenance_Stage::DrawInventoryTab(HDC dc, const RECT& panel)
{
    DrawPartySelectButtons(dc, panel); Character* selected = GetSelectedCharacter();
    const int statusWidth = (std::min<int>)(220, (panel.right - panel.left - 56) / 4);
    const int hpLeft = panel.right - 28 - 2 * statusWidth - 16;
    DrawLeftText(dc, L"INVENTORY     GOLD: " + std::to_wstring(GameManager::Get().GetGold()) + L"     Target: " + (selected ? selected->GetName() : L"None"),
        panel.left + 28, panel.top + 207, hpLeft - panel.left - 44, 30, 17, FW_NORMAL, StarlightUI::Gold);
    const double pulse = GetInventoryPulseStrength();
    const bool noRecovery = InventoryFeedback.HadNoRecovery();
    const COLORREF accent = InventoryFeedback.WasSuccessful() ? (noRecovery ? StarlightUI::Gold : RGB(128, 220, 173)) : RGB(245, 153, 152);
    if (selected)
    {
        const CharacterStat stat = selected->GetStat();
        const int current[] = { stat.CurHP, stat.CurMP }, maximum[] = { stat.MaxHP, stat.MaxMP };
        const auto displayed = InventoryFeedback.BarFractions(selected, stat);
        for (int i = 0; i < 2; ++i)
        {
            const int left = hpLeft + i * (statusWidth + 16);
            const COLORREF resourceColor = i == 0 ? RGB(128, 220, 173) : RGB(139, 181, 247);
            const std::wstring label = (i == 0 ? L"HP " : L"MP ") + std::to_wstring(current[i]) + L" / " + std::to_wstring(maximum[i]);
            StarlightUI::Text(dc, label, { left, panel.top + 207, left + statusWidth - 50, panel.top + 229 },
                17, StarlightUI::Ink, DT_LEFT | DT_SINGLELINE);
            const bool relevant = IsInventoryFeedbackActive() && InventoryFeedback.WasSuccessful() &&
                (i == 0 ? ItemData::GetHealHP(InventoryFeedback.FeedbackItemID()) > 0 : ItemData::GetHealMP(InventoryFeedback.FeedbackItemID()) > 0);
            if (relevant && pulse > 0.0)
                StarlightUI::Text(dc, SignedValue(i == 0 ? GetInventoryHPDelta() : GetInventoryMPDelta()),
                    { left + statusWidth - 48, panel.top + 207, left + statusWidth, panel.top + 229 }, 15,
                    MixColor(StarlightUI::Navy, noRecovery ? StarlightUI::Gold : resourceColor, pulse), DT_RIGHT | DT_SINGLELINE);
            RECT track{ left, panel.top + 233, left + statusWidth, panel.top + 237 };
            HBRUSH trackBrush = CreateSolidBrush(RGB(42, 43, 65)); FillRect(dc, &track, trackBrush); DeleteObject(trackBrush);
            track.right = left + int(statusWidth * displayed[i] + 0.5);
            HBRUSH valueBrush = CreateSolidBrush(MixColor(resourceColor, StarlightUI::Ink, relevant ? pulse * 0.25 : 0.0));
            FillRect(dc, &track, valueBrush); DeleteObject(valueBrush);
        }
        if (GetObjectType(dc) != OBJ_ENHMETADC && GetObjectType(dc) != OBJ_METADC)
        {
            InventoryFeedback.RecordDisplayedBars(selected, displayed);
        }
    }
    const auto& inventory = GameManager::Get().GetInventory();
    if (inventory.empty())
        DrawLeftText(dc, L"No Items Found.", panel.left + 28, panel.top + 270, panel.right - panel.left - 56, 40, 20, FW_NORMAL, StarlightUI::Muted);
    const int count = int((std::min)(size_t(20), inventory.size()));
    for (int i = 0; i < count; ++i)
    {
        const auto& inv = inventory[i]; const RECT card = GetInventorySlotRect(i);
        const bool compact = card.bottom - card.top < 50;
        const bool hovered = IsPointInRect(PointerPosition.x, PointerPosition.y, card);
        // Match the item identity, never the vacated slot after a stack runs out.
        const double emphasis = inv.ItemID == GetInventoryFeedbackItemID() ? pulse : 0.0;
        const COLORREF fill = MixColor(hovered ? RGB(58, 52, 80) : RGB(27, 32, 54), accent, emphasis * 0.14);
        StatSurface(dc, card, fill, MixColor(hovered ? StarlightUI::Gold : RGB(101, 88, 112), accent, emphasis * 0.85));
        StarlightUI::Text(dc, ItemData::GetName(inv.ItemID), { card.left + 12, card.top + 3, card.right - 12, card.top + 23 },
            inventory.size() > 10 ? 15 : compact ? 16 : 17, StarlightUI::Ink, DT_LEFT | DT_SINGLELINE);
        std::wstring details = L"x" + std::to_wstring(inv.Count);
        if (ItemData::IsEquipment(inv.ItemID)) details += L"  [Equip]";
        else if (ItemData::IsConsumable(inv.ItemID)) details += L"  [Use]";
        const int detailTop = compact ? card.top + 24 : card.bottom - 22;
        StarlightUI::Text(dc, details, { card.left + 12, detailTop, card.right - 54, card.bottom - 3 },
            compact ? 12 : 14, StarlightUI::Muted, DT_LEFT | DT_SINGLELINE);
        if (emphasis > 0.0 && GetInventoryQuantityDelta() < 0)
            StarlightUI::Text(dc, std::to_wstring(GetInventoryQuantityDelta()),
                { card.right - 50, detailTop, card.right - 12, card.bottom - 3 }, compact ? 12 : 14,
                MixColor(fill, accent, emphasis), DT_RIGHT | DT_SINGLELINE);
    }
    DrawLeftText(dc, L"Click equipment to equip it to the selected character. Click potion to use it.", panel.left + 28, panel.bottom - 62,
        panel.right - panel.left - 56, 18, 14, FW_NORMAL, StarlightUI::Muted);
    if (IsInventoryFeedbackActive())
    {
        DrawLeftText(dc, InventoryFeedback.Message(), panel.left + 28, panel.bottom - 39, panel.right - panel.left - 56, 27, 16, FW_NORMAL,
            MixColor(StarlightUI::Navy, accent, InventoryFeedback.DrawOpacity(dc, GetTabPresentationRect())));
    }
}

void Maintenance_Stage::DrawEquipmentTab(HDC dc, const RECT& panel)
{
    DrawPartySelectButtons(dc, panel); Character* selected = GetSelectedCharacter();
    if (!selected) { DrawLeftText(dc, L"No party member.", panel.left + 28, panel.top + 215, 650, 40, 20, FW_NORMAL, StarlightUI::Muted); return; }
    const CharacterStat stat = selected->GetStat(); const CharacterEquipment equip = selected->GetEquipment();
    DrawPortrait(dc, selected, { panel.left + 28, panel.top + 210, panel.left + 106, panel.top + 305 });
    DrawLeftText(dc, selected->GetName(), panel.left + 124, panel.top + 221, 380, 32, 23, FW_NORMAL, StarlightUI::Ink);
    DrawLeftText(dc, L"Current Stats", panel.left + 124, panel.top + 264, 380, 26, 18, FW_NORMAL, StarlightUI::Gold);
    const int cellW = ((panel.right - panel.left) / 2 - 68) / 2;
    const int cellH = (std::min<int>)(54, (panel.bottom - panel.top - 390 - 16) / 3);
    const wchar_t* labels[] = { L"HP", L"MP", L"STR", L"INT", L"DEF", L"SPD" };
    const std::wstring values[] = { std::to_wstring(stat.CurHP) + L" / " + std::to_wstring(stat.MaxHP), std::to_wstring(stat.CurMP) + L" / " + std::to_wstring(stat.MaxMP), std::to_wstring(stat.STR), std::to_wstring(stat.INT), std::to_wstring(stat.DEF), std::to_wstring(stat.SPD) };
    const double feedback = IsEquipmentFeedbackActive() ? EquipmentFeedback.Strength() : 0.0;
    for (int i = 0; i < 6; ++i)
    {
        const int x = panel.left + 28 + (i % 2) * (cellW + 10), y = panel.top + 320 + (i / 2) * (cellH + 8);
        const RECT rc{ x, y, x + cellW, y + cellH };
        int delta = GetEquipmentStatDelta(i);
        std::wstring suffix = i < 2 ? L" MAX" : L"";
        if (IsEquipmentFeedbackActive() && delta == 0 && i < 2)
        {
            delta = EquipmentFeedback.CurrentDelta(i);
            suffix = L" NOW";
        }
        const COLORREF accent = delta > 0 ? RGB(128, 220, 173) : RGB(245, 153, 152);
        const COLORREF fill = MixColor(RGB(26, 28, 52), accent, delta ? feedback * 0.13 : 0.0);
        const COLORREF border = MixColor(RGB(100, 84, 70), accent, delta ? feedback * 0.8 : 0.0);
        StatSurface(dc, rc, fill, border);
        DrawLeftText(dc, labels[i], x + 10, y + 3, cellW - 20, 18, 13, FW_NORMAL, StarlightUI::Muted);
        DrawLeftText(dc, values[i], x + 10, y + 20, cellW - 20, cellH - 21, 18, FW_NORMAL, StarlightUI::Gold);
        if (delta != 0 && feedback > 0.0)
        {
            const std::wstring badge = (delta > 0 ? L"+" : L"") + std::to_wstring(delta) + suffix;
            StarlightUI::Text(dc, badge, { x + 45, y + 3, x + cellW - 10, y + 21 }, 13,
                MixColor(fill, accent, feedback), DT_RIGHT | DT_SINGLELINE);
        }
    }
    const int itemIDs[] = { equip.WeaponID, equip.ArmorID, equip.AccessoryID };
    const EEquipPart parts[] = { EEquipPart::Weapon, EEquipPart::Armor, EEquipPart::Accessory };
    const RECT first = GetEquipmentSlotRect(0);
    DrawLeftText(dc, L"Equipped Items", first.left, panel.top + 210, first.right - first.left, 28, 20, FW_NORMAL, StarlightUI::Gold);
    for (int i = 0; i < 3; ++i)
    {
        std::wstring label = GetEquipPartName(parts[i]) + L": " + GetEquippedName(itemIDs[i]);
        if (itemIDs[i] >= 0) label += L"\n[Click to Unequip]";
        const double emphasis = IsEquipmentFeedbackActive() && EquipmentFeedback.ChangedPart() == i ? feedback : 0.0;
        DrawBoxText(dc, GetEquipmentSlotRect(i), label, 18, FW_NORMAL, StarlightUI::Ink,
            MixColor(RGB(27, 32, 54), RGB(70, 59, 65), emphasis),
            MixColor(itemIDs[i] >= 0 ? StarlightUI::Gold : RGB(101, 88, 112), RGB(246, 215, 157), emphasis), DT_LEFT | DT_WORDBREAK);
    }
    DrawLeftText(dc, L"Use the Inventory tab to equip new items. Click an equipped slot here to unequip it.", panel.left + 28, panel.bottom - 52,
        panel.right - panel.left - 56, 36, 17, FW_NORMAL, StarlightUI::Muted);
}

void Maintenance_Stage::DrawSkillTab(HDC dc, const RECT& panel)
{
    DrawPartySelectButtons(dc, panel); Character* selected = GetSelectedCharacter();
    if (!selected) { DrawLeftText(dc, L"No active party member.", panel.left + 28, panel.top + 215, 800, 40, 20, FW_NORMAL, StarlightUI::Muted); return; }
    DrawPortrait(dc, selected, { panel.left + 28, panel.top + 207, panel.left + 82, panel.top + 275 });
    DrawLeftText(dc, L"SKILL SETTING", panel.left + 96, panel.top + 210, 440, 24, 15, FW_NORMAL, StarlightUI::Gold);
    DrawLeftText(dc, selected->GetName() + L" / " + selected->GetElementName(), panel.left + 96, panel.top + 239, 800, 31, 21, FW_NORMAL, StarlightUI::Ink);
    DrawLeftText(dc, L"Equipped Skills", panel.left + 28, panel.top + 276, 500, 25, 17, FW_NORMAL, StarlightUI::Gold);
    for (int i = 0; i < 3; ++i)
    {
        const SSkillData* skill = SkillData::Find(selected->GetEquippedSkillID(i));
        const std::wstring label = L"Slot " + std::to_wstring(i + 1) + L": " + (skill ? skill->Name + L"\nMP " + std::to_wstring(skill->MPCost) : L"Empty");
        const double emphasis = SkillFeedback.PulseForSlot(i);
        DrawBoxText(dc, GetSkillSlotRect(i), label, 17, FW_NORMAL, StarlightUI::Ink,
            MixColor(i == SelectedSkillSlot ? RGB(52, 48, 71) : RGB(27, 32, 54), RGB(85, 69, 69), emphasis),
            MixColor(i == SelectedSkillSlot ? StarlightUI::Gold : RGB(101, 88, 112), RGB(255, 229, 171), emphasis), DT_LEFT | DT_WORDBREAK);
    }
    const RECT first = GetLearnedSkillRect(0);
    DrawLeftText(dc, L"Learned Skills - click a skill to equip it into the selected slot.", panel.left + 28, first.top - 35,
        panel.right - panel.left - 56, 29, 17, FW_NORMAL, StarlightUI::Muted);
    const auto learned = selected->GetLearnedSkillIDs();
    for (int i = 0; i < int(learned.size()); ++i)
    {
        const SSkillData* skill = SkillData::Find(learned[i]); if (!skill) continue;
        const bool inSelectedSlot = learned[i] == selected->GetEquippedSkillID(SelectedSkillSlot);
        const double emphasis = SkillFeedback.PulseForSkill(learned[i]);
        DrawBoxText(dc, GetLearnedSkillRect(i), skill->Name + L"\nMP " + std::to_wstring(skill->MPCost) + L" / R" + std::to_wstring(skill->Range),
            18, FW_NORMAL, StarlightUI::Ink,
            MixColor(inSelectedSlot ? RGB(52, 48, 71) : RGB(27, 32, 54), RGB(85, 69, 69), emphasis),
            MixColor(inSelectedSlot ? StarlightUI::Gold : skill->Color, RGB(255, 229, 171), emphasis), DT_LEFT | DT_WORDBREAK);
    }
    if (SkillFeedback.HasMessage())
        DrawLeftText(dc, SkillFeedback.Message(), panel.left + 28, panel.bottom - 51, panel.right - panel.left - 56, 34, 18, FW_NORMAL, StarlightUI::Gold);
}

void Maintenance_Stage::DrawShopTab(HDC dc, const RECT& panel)
{
    DrawLeftText(dc, L"SHOP     GOLD: " + std::to_wstring(GameManager::Get().GetGold()) + L"     Shop Tier: " + std::to_wstring(GameManager::Get().GetMaxUnlockedBattleStageId()),
        panel.left + 28, panel.top + 158, panel.right - panel.left - 306, 32, 20, FW_NORMAL, StarlightUI::Gold);
    const double pulse = GetShopPulseStrength();
    const COLORREF accent = ShopFeedback.WasSuccessful() ? RGB(128, 220, 173) : RGB(245, 153, 152);
    if (GetShopGoldDelta() != 0 && pulse > 0.0)
        StarlightUI::Text(dc, std::to_wstring(GetShopGoldDelta()) + L"G",
            { panel.right - 268, panel.top + 158, panel.right - 28, panel.top + 190 }, 20,
            MixColor(StarlightUI::Navy, RGB(245, 153, 152), pulse), DT_RIGHT | DT_SINGLELINE);
    const auto items = GetShopItemIDs();
    for (int i = 0; i < int(items.size()); ++i)
    {
        const int id = items[i], price = ItemData::GetPrice(id); const bool affordable = GameManager::Get().GetGold() >= price;
        const RECT card = GetShopSlotRect(i);
        const bool hovered = IsPointInRect(PointerPosition.x, PointerPosition.y, card);
        const double emphasis = id == GetShopFeedbackItemID() ? pulse : 0.0;
        const COLORREF baseFill = hovered ? RGB(58, 52, 80) : affordable ? RGB(27, 32, 54) : RGB(23, 27, 48);
        const COLORREF baseBorder = hovered ? StarlightUI::Gold : affordable ? RGB(128, 107, 72) : RGB(81, 73, 91);
        const COLORREF fill = MixColor(baseFill, accent, emphasis * 0.14);
        StatSurface(dc, card, fill, MixColor(baseBorder, accent, emphasis * 0.85));
        const COLORREF ink = affordable ? StarlightUI::Ink : RGB(178, 168, 192);
        StarlightUI::Text(dc, ItemData::GetName(id), { card.left + 12, card.top + 4, card.right - 112, card.top + 26 },
            18, ink, DT_LEFT | DT_SINGLELINE);
        StarlightUI::Text(dc, std::to_wstring(price) + L"G", { card.right - 108, card.top + 4, card.right - 12, card.top + 26 },
            18, ink, DT_RIGHT | DT_SINGLELINE);
        std::wstring details = ItemData::IsConsumable(id) ? L"[Use]" : ItemData::IsEquipment(id) ? L"[Equip]" : L"";
        details += L"    Bag: " + std::to_wstring(GameManager::Get().GetItemCount(id));
        StarlightUI::Text(dc, details, { card.left + 12, card.bottom - 22, card.right - 80, card.bottom - 4 },
            15, StarlightUI::Muted, DT_LEFT | DT_SINGLELINE);
        if (emphasis > 0.0 && GetShopQuantityDelta() > 0)
            StarlightUI::Text(dc, L"+" + std::to_wstring(GetShopQuantityDelta()),
                { card.right - 76, card.bottom - 22, card.right - 12, card.bottom - 4 }, 15,
                MixColor(fill, accent, emphasis), DT_RIGHT | DT_SINGLELINE);
    }
    DrawLeftText(dc, L"Click an item to buy one. More items appear as later stages are unlocked. Bag excludes equipped items.", panel.left + 28, panel.bottom - 76,
        panel.right - panel.left - 56, 26, 16, FW_NORMAL, StarlightUI::Muted);
    if (IsShopFeedbackActive())
    {
        const COLORREF color = MixColor(StarlightUI::Navy, accent, ShopFeedback.DrawOpacity(dc, GetTabPresentationRect()));
        DrawLeftText(dc, ShopFeedback.Message(), panel.left + 28, panel.bottom - 46, panel.right - panel.left - 316, 30, 17, FW_NORMAL, color);
        if (GetShopShortfall() > 0)
            StarlightUI::Text(dc, L"Need " + std::to_wstring(GetShopShortfall()) + L"G more.",
                { panel.right - 268, panel.bottom - 46, panel.right - 28, panel.bottom - 16 }, 17,
                color, DT_RIGHT | DT_SINGLELINE);
    }
}

void Maintenance_Stage::DrawPartySelectButtons(HDC dc, const RECT&)
{
    const auto& party = GameManager::Get().GetParty();
    for (int i = 0; i < int(party.size()) && i < 5; ++i)
        DrawBoxText(dc, GetPartySelectRect(i), party[i] ? party[i]->GetName() : L"None", 17, FW_NORMAL, StarlightUI::Ink,
            i == SelectedPartyIndex ? RGB(52, 48, 71) : RGB(27, 32, 54), i == SelectedPartyIndex ? StarlightUI::Gold : RGB(101, 88, 112), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void Maintenance_Stage::DrawBoxText(HDC dc, const RECT& rc, const std::wstring& text, int fontSize, int,
    COLORREF textColor, COLORREF fillColor, COLORREF borderColor, UINT format)
{
    const bool hovered = IsPointInRect(PointerPosition.x, PointerPosition.y, rc);
    const int saved = SaveDC(dc);
    HBRUSH brush = CreateSolidBrush(hovered ? RGB(58, 52, 80) : fillColor);
    HPEN pen = CreatePen(PS_SOLID, 1, hovered ? StarlightUI::Gold : borderColor);
    SelectObject(dc, brush); SelectObject(dc, pen); RoundRect(dc, rc.left, rc.top, rc.right, rc.bottom, 6, 6);
    RestoreDC(dc, saved); DeleteObject(pen); DeleteObject(brush);
    RECT tr{ rc.left + 12, rc.top + 5, rc.right - 12, rc.bottom - 5 };
    if (!(format & DT_SINGLELINE))
    {
        // Measure multiline labels before centering; DrawText ignores VCENTER for wrapped text.
        const int textSaved = SaveDC(dc);
        HFONT font = CreateFontW(-fontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Malgun Gothic");
        SelectObject(dc, font); RECT measured = tr;
        DrawTextW(dc, text.c_str(), int(text.size()), &measured, (format & ~DT_VCENTER) | DT_CALCRECT | DT_NOPREFIX);
        const int height = measured.bottom - measured.top;
        tr.top += (std::max)(0L, (tr.bottom - tr.top - height) / 2);
        RestoreDC(dc, textSaved); DeleteObject(font);
    }
    StarlightUI::Text(dc, text, tr, fontSize, textColor, format & ~DT_END_ELLIPSIS);
}

void Maintenance_Stage::DrawCenteredText(HDC dc, const std::wstring& text, int top, int height, int fontSize, int, COLORREF color)
{
    StarlightUI::Text(dc, text, { rects.left, top, rects.right, top + height }, fontSize, color, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void Maintenance_Stage::DrawLeftText(HDC dc, const std::wstring& text, int left, int top, int width, int height, int fontSize, int, COLORREF color)
{
    StarlightUI::Text(dc, text, { left, top, left + width, top + height }, fontSize, color, DT_LEFT | DT_WORDBREAK);
}


void Maintenance_Stage::HandleMouseDown(int mouseX, int mouseY)
{
    if (IsRewardMode())
    {
        return;
    }

    if (IsPointInRect(mouseX, mouseY, GetTabRect(EMaintenanceTab::Reward)))
    {
        SelectTab(EMaintenanceTab::Reward);
        return;
    }

    if (IsPointInRect(mouseX, mouseY, GetTabRect(EMaintenanceTab::Party)))
    {
        SelectTab(EMaintenanceTab::Party);
        return;
    }

    if (IsPointInRect(mouseX, mouseY, GetTabRect(EMaintenanceTab::Inventory)))
    {
        SelectTab(EMaintenanceTab::Inventory);
        return;
    }

    if (IsPointInRect(mouseX, mouseY, GetTabRect(EMaintenanceTab::Equipment)))
    {
        SelectTab(EMaintenanceTab::Equipment);
        return;
    }

    if (IsPointInRect(mouseX, mouseY, GetTabRect(EMaintenanceTab::Skill)))
    {
        SelectTab(EMaintenanceTab::Skill);
        return;
    }

    if (IsPointInRect(mouseX, mouseY, GetTabRect(EMaintenanceTab::Shop)))
    {
        SelectTab(EMaintenanceTab::Shop);
        return;
    }

    // Tabs and the map button remain responsive; blended body controls do not
    // mutate party, equipment, skills or purchases until they are fully visible.
    if (IsTabTransitioning()) return;

    std::vector<Character*>& party = GameManager::Get().GetParty();

    for (int i = 0; i < (int)party.size() && i < 5; ++i)
    {
        if ((ActiveTab == EMaintenanceTab::Inventory || ActiveTab == EMaintenanceTab::Equipment || ActiveTab == EMaintenanceTab::Skill) &&
            IsPointInRect(mouseX, mouseY, GetPartySelectRect(i)))
        {
            SelectCharacter(i);
            return;
        }
    }

    // A different character may be selected during its fade, but detail actions
    // wait until the selected character is unambiguous on screen.
    if (IsCharacterTransitioning()) return;

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
        const int oldCount = int(GameManager::Get().GetParty().size());

        if (!GameManager::Get().ToggleActivePartyMember(c))
        {
            BeginPartyFeedback(c, oldCount, wasActive, false);
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

        BeginPartyFeedback(c, oldCount, wasActive, true);

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
            if (i != SelectedSkillSlot) BeginSkillFeedback(SkillSelectionDuration);
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
        const bool changed = selected->GetEquippedSkillID(SelectedSkillSlot) != skillID;
        selected->SetEquippedSkillID(SelectedSkillSlot, skillID);
        if (changed && selected->GetEquippedSkillID(SelectedSkillSlot) == skillID)
        {
            BeginSkillFeedback(SkillReplacementDuration);
            SkillFeedback.RecordSuccess(SelectedSkillSlot, skillID);
        }
        SkillFeedback.RecordMessage(L"Equipped " + SkillData::GetName(skillID) + L" to slot " + std::to_wstring(SelectedSkillSlot + 1) + L".", 180);
        return;
    }
}

void Maintenance_Stage::HandleInventoryClick(int mouseX, int mouseY)
{
    Character* target = GetSelectedCharacter();

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
            if (!target) return;
            const CharacterStat before = target->GetStat();
            const CharacterEquipment equipment = target->GetEquipment();
            const bool equipped = GameManager::Get().EquipItem(target, itemID);
            SelectTab(EMaintenanceTab::Equipment);
            if (equipped)
            {
                const CharacterEquipment after = target->GetEquipment();
                const bool changed = equipment.WeaponID != after.WeaponID || equipment.ArmorID != after.ArmorID ||
                    equipment.AccessoryID != after.AccessoryID;
                BeginEquipmentFeedback(target, before, static_cast<int>(ItemData::GetEquipPart(itemID)), changed);
            }
        }
        else
        {
            UseInventoryConsumable(target, itemID);
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

        const CharacterStat before = target->GetStat();
        if (GameManager::Get().UnequipItem(target, parts[i]))
            BeginEquipmentFeedback(target, before, i, true);
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
        const int oldGold = GameManager::Get().GetGold();
        const int oldCount = GameManager::Get().GetItemCount(itemID);

        const bool purchased = GameManager::Get().BuyItem(itemID, 1);
        BeginShopFeedback(itemID, purchased, oldGold, oldCount);

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
