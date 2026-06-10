#pragma once

#include "CStage.h"
#include "Character.h"
#include "STurnTimeline.h"
#include "UTurnOrderPanel.h"
#include "UBattleActionMenu.h"

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

    // ̵ ִϸ̼    ѱ ٸ  
    Character* MovingUnit;

    // /ǰ  ׼ ִϸ̼   ٸ  
    Character* ActingUnit;

private:
    bool IsInsideGrid(int gx, int gy) const;
    bool IsOccupied(int gx, int gy, Character* except = nullptr) const;

    Character* FindNearestAlivePlayer(Character* enemy) const;
    Character* GetEnemyAt(int gx, int gy) const;
    Character* GetPlayerAt(int gx, int gy) const;

    bool FindBestMoveTileToward(Character* unit, Character* target, int& outX, int& outY) const;
    void ProcessEnemyTurn();

    void DrawBackground(HDC hMemDC, HDC hMemDC1);
    void DrawGrid(HDC hMemDC);
    void DrawMoveRange(HDC hMemDC);
    void DrawAttackRange(HDC hMemDC);
    void DrawUnit(HDC hMemDC, HDC hMemDC1, Character* unit, bool isEnemy);

    void FocusCameraOnGrid(int gx, int gy);

    int GetOffsetX() const;
    int GetOffsetY() const;

    // 메뉴 콜백
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

    // 카메라 추적 관련
    void FocusCameraOnWorldPos(int wx, int wy);

private:
    // 부드러운 카메라 이동을 위한 내부 좌표
    float camPosX;
    float camPosY;
};