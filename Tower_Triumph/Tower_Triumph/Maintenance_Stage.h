#pragma once

#include "CStage.h"
#include "BattleRewardManager.h"
#include "Character.h"
#include "GameManager.h"
#include "SkillData.h"
#include "MaintenanceFeedback.h"

#include <string>
#include <vector>
#include <chrono>

enum class EMaintenanceTab
{
    Reward,
    Party,
    Inventory,
    Equipment,
    Skill,
    Shop
};

struct SMaintenanceLevelResult
{
    std::wstring CharacterName;

    int OldLevel;
    int NewLevel;

    int OldExp;
    int NewExp;

    CharacterStat OldStat;
    CharacterStat NewStat;

    bool LeveledUp;

    SMaintenanceLevelResult()
        : OldLevel(1),
          NewLevel(1),
          OldExp(0),
          NewExp(0),
          LeveledUp(false)
    {
    }
};

class Maintenance_Stage : public CStage
{
private:
    bool RewardApplied;
    EMaintenanceTab ActiveTab;
    int SelectedPartyIndex;
    int SelectedSkillSlot;


    std::vector<SMaintenanceLevelResult> LevelResults;
    class UMouseMark* MouseMark = nullptr;
    POINT PointerPosition{ -1, -1 };

    UIFrameSnapshot PresentedTabFrame;
    using BodyTransitionKind = MaintenanceFeedback::BodyTransition::Kind;
    MaintenanceFeedback::BodyTransition BodyPresentation;
    MaintenanceFeedback::Equipment EquipmentFeedback;
    MaintenanceFeedback::Skill SkillFeedback;
    MaintenanceFeedback::Shop ShopFeedback;
    MaintenanceFeedback::Inventory InventoryFeedback;
    MaintenanceFeedback::Party PartyFeedback;
    std::chrono::steady_clock::time_point LastTabPresentationUpdate = std::chrono::steady_clock::now();

private:
    void ApplyRewardOnce();
    void RefreshPointer(HWND hWnd);
    void DrawPortrait(HDC dc, Character* character, const RECT& bounds);
    RECT GetBodyRect() const;
    RECT GetTabPresentationRect() const;
    void SelectTab(EMaintenanceTab tab);
    void SelectCharacter(int index);
    void BeginTabPresentation(BodyTransitionKind kind);
    void ResetTabPresentation();
    void CompositeTabPresentation(HDC destination);
    void ResetActionFeedback();
    void BeginEquipmentFeedback(Character* target, const CharacterStat& before, int part, bool equipmentChanged);
    void BeginSkillFeedback(double duration);
    void CompositeSkillFeedback(HDC destination);
    void BeginShopFeedback(int itemID, bool purchased, int oldGold, int oldCount);
    void CompositeShopNotice(HDC destination);
    double GetShopPulseStrength() const;
    void UseInventoryConsumable(Character* target, int itemID);
    void CompositeInventoryNotice(HDC destination);
    double GetInventoryPulseStrength() const;
    void BeginPartyFeedback(Character* character, int oldCount, bool wasActive, bool changed);
    void CompositePartyPresentation(HDC destination);
    double GetPartyPulseStrength() const;

public:
    // Shared, read-only layout queries used by rendering and input routing.
    RECT GetMainPanelRect() const;
    RECT GetTabRect(EMaintenanceTab tab) const;
    RECT GetContinueRect() const;
    RECT GetNextStageRect() const;
    bool IsRewardMode() const;
    RECT GetPartySelectRect(int index) const;
    RECT GetInventorySlotRect(int index) const;
    RECT GetEquipmentSlotRect(int index) const;
    RECT GetShopSlotRect(int index) const;
    RECT GetRosterSlotRect(int index) const;
    RECT GetSkillSlotRect(int index) const;
    RECT GetLearnedSkillRect(int index) const;
    std::vector<int> GetShopItemIDs() const;

    EMaintenanceTab GetActiveTab() const { return ActiveTab; }
    int GetSelectedPartyIndex() const { return SelectedPartyIndex; }
    int GetSelectedSkillSlot() const { return SelectedSkillSlot; }
    double GetInventoryDisplayedHPFraction() const { return InventoryFeedback.DisplayedHPFraction(); }
    double GetInventoryDisplayedMPFraction() const { return InventoryFeedback.DisplayedMPFraction(); }
    double GetSkillPulseStrengthForSlot(int slot) const { return SkillFeedback.PulseForSlot(slot); }
    double GetLearnedSkillPulseStrength(int skillID) const { return SkillFeedback.PulseForSkill(skillID); }

    // Runtime and offscreen replay advance the same presentation clock.
    void AdvanceTabPresentation(double seconds);
    bool IsTabTransitioning() const;
    double GetTabTransitionProgress() const;
    bool IsCharacterTransitioning() const;
    double GetCharacterTransitionProgress() const;
    bool IsEquipmentFeedbackActive() const;
    double GetEquipmentFeedbackProgress() const;
    // HP/MP deltas refer to maximum values; current values remain live.
    int GetEquipmentStatDelta(int index) const;
    bool IsSkillFeedbackTransitioning() const;
    double GetSkillFeedbackProgress() const;
    bool IsShopFeedbackActive() const;
    double GetShopFeedbackProgress() const;
    double GetShopNoticeOpacity() const;
    int GetShopGoldDelta() const;
    int GetShopQuantityDelta() const;
    int GetShopShortfall() const;
    int GetShopFeedbackItemID() const;
    bool IsInventoryFeedbackActive() const;
    double GetInventoryFeedbackProgress() const;
    double GetInventoryNoticeOpacity() const;
    int GetInventoryHPDelta() const;
    int GetInventoryMPDelta() const;
    int GetInventoryQuantityDelta() const;
    int GetInventoryRemainingCount() const;
    int GetInventoryFeedbackItemID() const;
    bool WasInventoryUseSuccessful() const;
    bool IsPartyFeedbackActive() const;
    double GetPartyFeedbackProgress() const;
    double GetPartyNoticeOpacity() const;
    int GetPartyCountDelta() const;
    int GetPartyFeedbackCharacterID() const;
    bool WasPartyChangeSuccessful() const;
    bool IsPartyPresentationTransitioning() const;
    double GetPartyPresentationProgress() const;

private:
    bool IsPointInRect(int x, int y, const RECT& rc) const;

    void DrawPanel(HDC hMemDC, const RECT& panel);
    void DrawTabs(HDC hMemDC);
    void DrawContinueButton(HDC hMemDC);
    void DrawNextStageButton(HDC hMemDC);

    void DrawRewardTab(HDC hMemDC, const RECT& panel);
    void DrawPartyTab(HDC hMemDC, const RECT& panel);
    void DrawInventoryTab(HDC hMemDC, const RECT& panel);
    void DrawEquipmentTab(HDC hMemDC, const RECT& panel);
    void DrawShopTab(HDC hMemDC, const RECT& panel);
    void DrawSkillTab(HDC hMemDC, const RECT& panel);

    void DrawPartySelectButtons(HDC hMemDC, const RECT& panel);

    void DrawBoxText(
        HDC hMemDC,
        const RECT& rc,
        const std::wstring& text,
        int fontSize,
        int fontWeight,
        COLORREF textColor,
        COLORREF fillColor,
        COLORREF borderColor,
        UINT format
    );

    void DrawCenteredText(
        HDC hMemDC,
        const std::wstring& text,
        int top,
        int height,
        int fontSize,
        int fontWeight,
        COLORREF color
    );

    void DrawLeftText(
        HDC hMemDC,
        const std::wstring& text,
        int left,
        int top,
        int width,
        int height,
        int fontSize,
        int fontWeight,
        COLORREF color
    );

    void HandleMouseDown(int mouseX, int mouseY);
    void HandleInventoryClick(int mouseX, int mouseY);
    void HandleEquipmentClick(int mouseX, int mouseY);
    void HandleShopClick(int mouseX, int mouseY);
    void HandlePartyClick(int mouseX, int mouseY);
    void HandleSkillClick(int mouseX, int mouseY);

    Character* GetSelectedCharacter() const;
    std::wstring GetEquipPartName(EEquipPart part) const;
    std::wstring GetEquippedName(int itemID) const;
    std::wstring MakeStatLine(const CharacterStat& stat) const;

public:
    Maintenance_Stage();
    virtual ~Maintenance_Stage() override;

    virtual void Init(HWND hWnd, HINSTANCE const g_hInst) override;
    virtual void Update(HWND hWnd) override;
    virtual void Draw(HDC hMemDC, HDC hMemDC1) override;
    virtual void DrawCursor(HDC hMemDC, HDC hMemDC1) override;
    virtual int Input(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
    virtual bool HandleSetCursor(HWND hWnd, WPARAM wParam, LPARAM lParam) override;

    virtual void Comp_NextStage(int ID) override;
    virtual void BackgroundMove(int x, int y) override;
};
