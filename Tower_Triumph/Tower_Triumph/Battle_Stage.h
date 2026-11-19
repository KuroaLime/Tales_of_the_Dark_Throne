#pragma once

#include "CStage.h"
#include "Character.h"
#include "STurnTimeline.h"
#include "UTurnOrderPanel.h"
#include "UBattleActionMenu.h"
#include "BattleMapData.h"
#include "SkillData.h"
#include "BattleActionTiming.h"
#include "BattleSkillArea.h"
#include "SkillPresentation.h"
#include "StandardSkillPresentation.h"

#include <vector>
#include <map>
#include <string>
#include <chrono>

enum class EBattleControlMode
{
    WAIT_COMMAND,
    SELECT_MOVE,
    SELECT_ATTACK,
    SELECT_SKILL,
    SELECT_ITEM
};

class Battle_Stage : public CStage
{
private:
    std::vector<Character*> playerTeam;
    std::vector<Character*> enemyTeam;
    std::vector<class AItemBase*> stageItems;

    STurnTimeline* turnSystem;
    UTurnOrderPanel* turnUI;
    UBattleActionMenu* actionMenu;

    EBattleControlMode controlMode;

    int TILE_SIZE;
    int W;
    int H;

    Character* MovingUnit;
    Character* ActingUnit;

    struct PendingHit
    {
        Character* target = nullptr;
        int damage = 0;
        COLORREF color = RGB(255, 70, 70);
        bool reacted = false;
        int areaCellIndex = -1;
        bool resolved = false;
    };
    struct PendingBattleAction
    {
        Character* actor = nullptr;
        ECharacterState state = ECharacterState::Attack;
        std::uint64_t sequence = 0;
        BattleActionTiming::ActionTimeline timeline;
        BattleActionTiming::ProjectileKind projectile = BattleActionTiming::ProjectileKind::None;
        BattleActionTiming::ElementVisual element = BattleActionTiming::ElementVisual::Neutral;
        SkillPresentation::Kind presentation = SkillPresentation::Kind::Normal;
        StandardSkillPresentation::Profile standard;
        bool standardStarted = false;
        std::uint64_t presentationID = 0;
        bool presentationStarted = false;
        float releaseSeconds = 0.0f;
        float casterFootX = 0.0f, casterFootY = 0.0f;
        float facing = 1.0f;
        float startX = 0.0f;
        float startY = 0.0f;
        float targetX = 0.0f;
        float targetY = 0.0f;
        BattleSkillArea::LinePath linePath;
        std::vector<PendingHit> hits;
        bool enemyTurn = false;
        bool cancelled = false;
        bool actorFinished = false;
        int skillID = -1;
        Location effectGrid{};
        ESkillAreaType areaType = ESkillAreaType::Single;
        std::vector<BattleSkillArea::Cell> areaCells;
        std::size_t nextAreaCell = 0;
        float elapsedSeconds = 0.0f;
        bool fieldStarted = false;
        Location casterGrid{};
    };
    PendingBattleAction PendingAction;
    std::uint64_t NextPresentationID = 0;

    bool BattleEnded;
    bool BattleWon;

    int HoverGridX;
    int HoverGridY;
    int LastMouseX;
    int LastMouseY;

    std::vector<Location> PreviewMovePath;
    std::vector<Location> ActiveMovePath;
    int ActiveMoveIndex;

    std::chrono::steady_clock::time_point LastCharacterUpdate;

    SBattleMapData CurrentMap;
    std::vector<std::vector<ETileType>> TileMap;

    struct DamageText
    {
        int x;
        int y;
        int value;
        int life;
        int maxLife;
        COLORREF color;
        float elapsedSeconds = 0.0f;
        bool areaDamage = false;
    };

    std::vector<DamageText> DamageTexts;

    struct SkillEffectAnim
    {
        int skillID;
        int gridX;
        int gridY;
        ESkillAreaType areaType;
        int frameIndex;
        int frameTick;
        int frameDelay;
        int maxFrame;
        float elapsedSeconds = 0.0f;
        bool enhancedContact = false;
    };

    std::vector<SkillEffectAnim> ActiveSkillEffects;
    struct AreaEffectAnim
    {
        std::vector<BattleSkillArea::Cell> cells;
        BattleSkillArea::Kind kind = BattleSkillArea::Kind::Single;
        BattleActionTiming::ElementVisual element = BattleActionTiming::ElementVisual::Neutral;
        SkillPresentation::Kind presentation = SkillPresentation::Kind::Normal;
        bool standard = false;
        Location origin{};
        COLORREF color = RGB(255, 255, 255);
        float elapsedSeconds = 0.0f;
    };
    std::vector<AreaEffectAnim> ActiveAreaEffects;
    struct UltimateEffectAnim
    {
        SkillPresentation::Kind presentation = SkillPresentation::Kind::Normal;
        BattleActionTiming::ElementVisual element = BattleActionTiming::ElementVisual::Neutral;
        std::vector<BattleSkillArea::Cell> cells;
        Location center{};
        float elapsedSeconds = 0.0f;
        float releaseSeconds = 0.0f;
        float deliverySeconds = 0.0f;
        float casterFootX = 0.0f, casterFootY = 0.0f;
        float startX = 0.0f, startY = 0.0f, targetX = 0.0f, targetY = 0.0f;
        float facing = 1.0f;
        std::uint64_t presentationID = 0;
    };
    std::vector<UltimateEffectAnim> ActiveUltimateEffects;
    std::vector<StandardSkillPresentation::Action> ActiveStandardEffects;
    std::map<int, HBITMAP> SkillEffectBitmaps;
    std::map<int, BITMAP> SkillEffectBitmapInfos;
    std::map<int, COLORREF> SkillEffectColorKeys;

    int SelectedBattleSkillID;
    bool bSelectingBattleSkillTarget;
    std::vector<int> BattleSkillIDs;

    int SelectedBattleItemID;
    bool bSelectingBattleItemTarget;
    std::vector<int> BattleConsumableItemIDs;

private:
    bool IsInsideGrid(int gx, int gy) const;
    bool IsOccupied(int gx, int gy, Character* except = nullptr) const;

    Character* FindNearestAlivePlayer(Character* enemy) const;
    Character* GetEnemyAt(int gx, int gy) const;
    Character* GetPlayerAt(int gx, int gy) const;

    bool FindBestMoveTileToward(Character* unit, Character* target, int& outX, int& outY) const;
    int GetAttackRangeForUnit(Character* unit) const;
    bool CanAttackFromTile(Character* unit, int fromX, int fromY, Character* target) const;
    int GetMoveTileDangerPenalty(int gx, int gy) const;
    void ProcessEnemyTurn();

    bool BuildMovePath(Character* unit, int targetX, int targetY, std::vector<Location>& outPath) const;
    bool StartActiveMovePath(Character* unit, const std::vector<Location>& path);
    void ClearMovePath();
    void UpdatePreviewMovePathByMouse(int mouseX, int mouseY);

    void LoadBattleMap(EBattleMapId mapId);
    Location FindNearestValidStartTile(Character* unit, int startX, int startY, bool avoidDamage) const;

    ETileType GetTileType(int gx, int gy) const;
    bool IsBlockedTile(int gx, int gy, Character* unit = nullptr) const;
    bool IsDamageTile(int gx, int gy) const;
    int GetTileDamage(int gx, int gy) const;
    COLORREF GetTileOverlayColor(ETileType type) const;

    void ApplyTileEffect(Character* unit);
    void DrawTileOverlay(HDC hMemDC);
    void DrawBlockedMoveRange(HDC hMemDC);

    void AddDamageText(Character* target, int damage, COLORREF color, bool areaDamage = false);
    void UpdateDamageTexts(float elapsedSeconds);
    void DrawDamageTexts(HDC hMemDC);

    std::wstring GetSkillEffectRelativePath(int skillID) const;
    HBITMAP LoadSkillEffectBitmap(int skillID, BITMAP* outInfo);
    void ReleaseSkillEffectBitmaps();
    int GetSkillEffectDrawSize(ESkillAreaType areaType) const;
    void PlaySkillEffect(int skillID, int gridX, int gridY, ESkillAreaType areaType, bool enhancedContact = false);
    void UpdateSkillEffects(float elapsedSeconds);
    void DrawSkillEffects(HDC hMemDC, HDC hMemDC1);
    void DrawAreaSkillEffects(HDC hMemDC, HDC hMemDC1, bool front = false);
    void DrawUltimateSkillEffects(HDC hMemDC, HDC hMemDC1, bool front = false);

    void RefreshBattleSkillList(Character* current);
    void ClearBattleSkillSelection();
    void DrawBattleSkillUI(HDC hMemDC);
    void DrawBattleSkillList(HDC hMemDC);
    void DrawBattleSkillTargetSelect(HDC hMemDC);
    void DrawSkillRange(HDC hMemDC);
    void UpdateHoverGridByMouse(int mouseX, int mouseY);
    void UpdateSkillTargetCameraByMouse(HWND hWnd);
    std::vector<BattleSkillArea::Cell> BuildSkillAreaCells(Character* user, int skillID, int centerX, int centerY) const;
    void DrawSkillAreaPreview(HDC hMemDC);
    bool HandleBattleSkillClick(int mouseX, int mouseY, Character* current);
    bool UseSelectedBattleSkill(Character* user, Character* target);
    RECT GetBattleSkillPanelRect() const;
    RECT GetBattleSkillRect(int index) const;

    void RefreshBattleConsumableItems();
    void ClearBattleItemSelection();
    void DrawBattleItemUI(HDC hMemDC);
    void DrawBattleItemList(HDC hMemDC);
    void DrawBattleItemTargetSelect(HDC hMemDC);
    bool HandleBattleItemClick(int mouseX, int mouseY, Character* current);
    bool UseSelectedBattleItem(Character* user, Character* target);
    RECT GetBattleItemRect(int index) const;
    RECT GetBattleTargetRect(int index) const;
    RECT GetBattleCancelRect() const;
    bool IsPointInRect(const RECT& rc, int x, int y) const;

    void DrawBackground(HDC hMemDC, HDC hMemDC1);
    void DrawGrid(HDC hMemDC);
    void DrawMoveRange(HDC hMemDC);
    void DrawAttackRange(HDC hMemDC);
    void DrawMoveArrow(HDC hMemDC);
    void DrawUnit(HDC hMemDC, HDC hMemDC1, Character* unit, bool isEnemy);

    void DrawArrowPolyline(HDC hMemDC, const std::vector<POINT>& points, COLORREF color, int width);

    bool IsTeamAllDead(const std::vector<Character*>& team) const;
    void CheckBattleEnd();
    void DrawBattleEndMessage(HDC hMemDC);

    void FocusCameraOnGrid(int gx, int gy);

    int GetOffsetX() const;
    int GetOffsetY() const;

    void OnMenuMove();
    void OnMenuAttack();
    void OnMenuSkill();
    void OnMenuDefend();
    void OnMenuItem();
    void OnMenuEndTurn();

    void ResetTurnActions();
    bool ContainsBattleUnit(const Character* unit) const;
    void BeginPendingAction(Character* actor, ECharacterState state, std::vector<PendingHit> hits,
        int skillID = -1, Location effectGrid = {}, ESkillAreaType areaType = ESkillAreaType::Single,
        std::vector<BattleSkillArea::Cell> areaCells = {});
    void CommitPendingImpact();
    void CommitPendingHit(PendingHit& hit);
    void CommitPendingAreaCells();
    void StartPendingPresentation();
    void StartPendingStandardPresentation();
    void DrawStandardSkillEffects(HDC hMemDC, HDC hMemDC1, bool front = false);
    void CancelPendingPresentation();
    void ClearPendingAction(bool preserveAreaEffects = false);
    float AdvanceBattleVisuals(float elapsedSeconds);
    bool UpdateActionResolution();
    float AdvancePendingAction(float elapsedSeconds);
    void DrawPendingProjectile(HDC hMemDC, HDC hMemDC1);

public:
    Battle_Stage();
    virtual ~Battle_Stage();

    virtual void Init(HWND hWnd, HINSTANCE const g_hInst);
    virtual void Update(HWND hWnd);
    virtual void Draw(HDC hMemDC, HDC hMemDC1);
    virtual int Input(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    virtual void Comp_NextStage(int ID);
    virtual void BackgroundMove(int x, int y) override;

    void FocusCameraOnWorldPos(int wx, int wy);

private:
    float camPosX;
    float camPosY;
};
