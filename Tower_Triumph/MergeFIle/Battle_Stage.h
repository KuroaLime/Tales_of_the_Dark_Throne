#pragma once

#include "CStage.h"
#include "Character.h"
#include "STurnTimeline.h"
#include "UTurnOrderPanel.h"
#include "UBattleActionMenu.h"
#include "BattleMapData.h"
#include "SkillData.h"

#include <vector>
#include <map>
#include <string>

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

    bool BattleEnded;
    bool BattleWon;

    int HoverGridX;
    int HoverGridY;
    int LastMouseX;
    int LastMouseY;

    std::vector<Location> PreviewMovePath;
    std::vector<Location> ActiveMovePath;
    int ActiveMoveIndex;

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
    };

    std::vector<SkillEffectAnim> ActiveSkillEffects;
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

    void AddDamageText(Character* target, int damage, COLORREF color);
    void UpdateDamageTexts();
    void DrawDamageTexts(HDC hMemDC);

    std::wstring GetSkillEffectRelativePath(int skillID) const;
    HBITMAP LoadSkillEffectBitmap(int skillID, BITMAP* outInfo);
    void ReleaseSkillEffectBitmaps();
    int GetSkillEffectDrawSize(ESkillAreaType areaType) const;
    void PlaySkillEffect(int skillID, int gridX, int gridY, ESkillAreaType areaType);
    void UpdateSkillEffects();
    void DrawSkillEffects(HDC hMemDC, HDC hMemDC1);

    void RefreshBattleSkillList(Character* current);
    void ClearBattleSkillSelection();
    void DrawBattleSkillUI(HDC hMemDC);
    void DrawBattleSkillList(HDC hMemDC);
    void DrawBattleSkillTargetSelect(HDC hMemDC);
    void DrawSkillRange(HDC hMemDC);
    void UpdateHoverGridByMouse(int mouseX, int mouseY);
    void UpdateSkillTargetCameraByMouse(HWND hWnd);
    std::vector<Location> BuildSkillAreaCells(Character* user, int skillID, int centerX, int centerY) const;
    void DrawSkillAreaPreview(HDC hMemDC);
    bool HandleBattleSkillClick(int mouseX, int mouseY, Character* current);
    bool UseSelectedBattleSkill(Character* user, Character* target);
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