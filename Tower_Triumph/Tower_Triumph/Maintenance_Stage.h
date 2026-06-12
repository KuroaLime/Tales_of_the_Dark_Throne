#pragma once

#include "CStage.h"
#include "BattleRewardManager.h"
#include "Character.h"
#include "GameManager.h"
#include "SkillData.h"

#include <string>
#include <vector>

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

    std::wstring ShopMessage;
    int ShopMessageLife;
    std::wstring SkillMessage;
    int SkillMessageLife;

    std::vector<SMaintenanceLevelResult> LevelResults;

private:
    void ApplyRewardOnce();

    RECT GetMainPanelRect() const;
    RECT GetTabRect(EMaintenanceTab tab) const;
    RECT GetContinueRect() const;
    RECT GetNextStageButtonRect() const;
    RECT GetPartySelectRect(int index) const;
    RECT GetInventorySlotRect(int index) const;
    RECT GetEquipmentSlotRect(int index) const;
    RECT GetShopSlotRect(int index) const;
    RECT GetRosterSlotRect(int index) const;
    RECT GetSkillSlotRect(int index) const;
    RECT GetLearnedSkillRect(int index) const;

    bool IsPointInRect(int x, int y, const RECT& rc) const;

    void DrawPanel(HDC hMemDC, const RECT& panel);
    void DrawTabs(HDC hMemDC);
    void DrawContinueButton(HDC hMemDC);
    bool IsRewardMode() const;
    bool CanProceedToNextStage() const;
    int ProceedToNextStage();
    int ReturnToMapFromReward();

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
    std::vector<int> GetShopItemIDs() const;

public:
    Maintenance_Stage();
    virtual ~Maintenance_Stage();

    virtual void Init(HWND hWnd, HINSTANCE const g_hInst);
    virtual void Update(HWND hWnd);
    virtual void Draw(HDC hMemDC, HDC hMemDC1);
    virtual int Input(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    virtual void Comp_NextStage(int ID);
    virtual void BackgroundMove(int x, int y) override;
};
