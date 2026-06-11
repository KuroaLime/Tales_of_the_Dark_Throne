#pragma once

#include "CStage.h"
#include "Character.h"
#include "STurnTimeline.h"
#include "UTurnOrderPanel.h"
#include "UBattleActionMenu.h"
#include "BattleMapData.h"

#include <vector>

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

    int HoverGridX;
    int HoverGridY;

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

private:
    bool IsInsideGrid(int gx, int gy) const;
    bool IsOccupied(int gx, int gy, Character* except = nullptr) const;

    Character* FindNearestAlivePlayer(Character* enemy) const;
    Character* GetEnemyAt(int gx, int gy) const;
    Character* GetPlayerAt(int gx, int gy) const;

    bool FindBestMoveTileToward(Character* unit, Character* target, int& outX, int& outY) const;
    void ProcessEnemyTurn();

    bool BuildMovePath(Character* unit, int targetX, int targetY, std::vector<Location>& outPath) const;
    bool StartActiveMovePath(Character* unit, const std::vector<Location>& path);
    void ClearMovePath();
    void UpdatePreviewMovePathByMouse(int mouseX, int mouseY);

    void LoadBattleMap(EBattleMapId mapId);

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

    void DrawBackground(HDC hMemDC, HDC hMemDC1);
    void DrawGrid(HDC hMemDC);
    void DrawMoveRange(HDC hMemDC);
    void DrawAttackRange(HDC hMemDC);
    void DrawMoveArrow(HDC hMemDC);
    void DrawUnit(HDC hMemDC, HDC hMemDC1, Character* unit, bool isEnemy);

    void DrawArrowPolyline(HDC hMemDC, const std::vector<POINT>& points, COLORREF color, int width);

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
