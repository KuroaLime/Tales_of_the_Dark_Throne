#include "Battle_Stage.h"
#include "resource.h"
#include "GameManager.h"
#include "ItemData.h"
#include "UButton.h"
#include "BattleRewardManager.h"
#include "SkillData.h"

#include <cstdlib>
#include <algorithm>
#include <functional>
#include <cmath>
#include <string>
#include <queue>
#include <vector>

static void DrawBattleStageText(
    HDC hMemDC,
    const std::wstring& text,
    const RECT& rc,
    int fontSize,
    int fontWeight,
    COLORREF color,
    UINT format
);

static void DrawBattleStageButton(
    HDC hMemDC,
    const RECT& rc,
    const std::wstring& text,
    COLORREF border,
    COLORREF textColor
);

static const SSkillData* FindBattleSkillInfo(int skillID)
{
    return SkillData::Find(skillID);
}

static int BattleSkillDistance(Character* user, int targetX, int targetY)
{
    if (user == nullptr)
    {
        return 9999;
    }

    Location pos = user->GetGridPos();
    return std::abs(targetX - pos.x) + std::abs(targetY - pos.y);
}

static bool IsBattleSkillStraightLine(Character* user, int targetX, int targetY)
{
    if (user == nullptr)
    {
        return false;
    }

    Location pos = user->GetGridPos();
    return pos.x == targetX || pos.y == targetY;
}

static bool IsBattleSkillTargetValid(Character* user, int skillID, int targetX, int targetY)
{
    const SSkillData* info = FindBattleSkillInfo(skillID);

    if (user == nullptr || info == nullptr)
    {
        return false;
    }

    if (!SkillData::CanCharacterUse(user, skillID))
    {
        return false;
    }

    int distance = BattleSkillDistance(user, targetX, targetY);

    if (distance > info->Range)
    {
        return false;
    }

    if (info->AreaType == ESkillAreaType::Line)
    {
        return IsBattleSkillStraightLine(user, targetX, targetY);
    }

    return true;
}

static int ClampBattleDamage(int damage)
{
    if (damage < 1)
    {
        damage = 1;
    }

    return damage;
}

static int CalcBattleSkillSingleDamage(Character* user, Character* target, int skillID)
{
    if (user == nullptr || target == nullptr)
    {
        return 1;
    }

    const SSkillData* skill = SkillData::Find(skillID);

    if (skill == nullptr)
    {
        return 1;
    }

    CharacterStat us = user->GetStat();
    CharacterStat ts = target->GetStat();
    int level = user->GetLevel();

    int basePower = 0;

    switch (skill->StatType)
    {
    case ESkillStatType::STR:
        basePower = us.STR * 2 + level * 2;
        break;

    case ESkillStatType::INT:
        basePower = us.INT * 2 + level * 2;
        break;

    case ESkillStatType::STR_INT:
        basePower = us.STR + us.INT + level * 2;
        break;

    case ESkillStatType::SPD_STR:
        basePower = us.STR + us.SPD + level * 2;
        break;

    default:
        basePower = us.STR + us.INT + level;
        break;
    }

    int appliedDefense = ts.DEF * skill->DefenseRatePercent / 100;
    int damage = basePower * skill->Power / 100 - appliedDefense;

    float elementRate = SkillData::GetElementRate(skill->Element, target->GetElement());
    damage = (int)(damage * elementRate);

    return ClampBattleDamage(damage);
}

Battle_Stage::Battle_Stage()
    : turnSystem(nullptr),
    turnUI(nullptr),
    actionMenu(nullptr),
    controlMode(EBattleControlMode::WAIT_COMMAND),
    TILE_SIZE(64),
    W(0),
    H(0),
    MovingUnit(nullptr),
    ActingUnit(nullptr),
    BattleEnded(false),
    BattleWon(false),
    HoverGridX(-1),
    HoverGridY(-1),
    LastMouseX(-1),
    LastMouseY(-1),
    ActiveMoveIndex(0),
    SelectedBattleItemID(-1),
    bSelectingBattleItemTarget(false),
    SelectedBattleSkillID(-1),
    bSelectingBattleSkillTarget(false),
    camPosX(0.0f),
    camPosY(0.0f)
{
    CurStageID = 2;
}

Battle_Stage::~Battle_Stage()
{
    for (auto c : enemyTeam)
    {
        delete c;
    }

    enemyTeam.clear();

    if (turnSystem)
    {
        delete turnSystem;
        turnSystem = nullptr;
    }

    if (turnUI)
    {
        delete turnUI;
        turnUI = nullptr;
    }

    if (actionMenu)
    {
        delete actionMenu;
        actionMenu = nullptr;
    }
}

void Battle_Stage::Init(HWND hWnd, HINSTANCE const g_hInst)
{
    GetClientRect(hWnd, &rects);

    BattleEnded = false;
    BattleWon = false;

    BattleRewardManager::Get().Clear();

    int battleStageId = GameManager::Get().GetCurrentBattleStageId();
    EBattleMapId battleMapId = BattleMapData::FromStageId(battleStageId);

    LoadBattleMap(battleMapId);

    hBitBackground = nullptr;

    // 배경 파일은 절대 경로를 쓰지 않는다.
    // Visual Studio 실행 위치가 솔루션 루트일 때와 프로젝트 폴더일 때를 모두 처리한다.
    if (!CurrentMap.backgroundFilePath.empty())
    {
        std::vector<std::wstring> backgroundPathCandidates;
        backgroundPathCandidates.push_back(CurrentMap.backgroundFilePath);
        backgroundPathCandidates.push_back(L"Tower_Triumph\\" + CurrentMap.backgroundFilePath);
        backgroundPathCandidates.push_back(L"..\\Tower_Triumph\\" + CurrentMap.backgroundFilePath);

        for (const std::wstring& path : backgroundPathCandidates)
        {
            hBitBackground = (HBITMAP)LoadImageW(
                NULL,
                path.c_str(),
                IMAGE_BITMAP,
                0,
                0,
                LR_LOADFROMFILE | LR_CREATEDIBSECTION
            );

            if (hBitBackground != nullptr)
            {
                break;
            }
        }
    }

    if (hBitBackground == nullptr && CurrentMap.backgroundResourceId != 0)
    {
        hBitBackground = LoadBitmap(g_hInst, MAKEINTRESOURCE(CurrentMap.backgroundResourceId));
    }

    if (hBitBackground == nullptr)
    {
        hBitBackground = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP3));
    }

    if (hBitBackground)
    {
        GetObject(hBitBackground, sizeof(BITMAP), &BackgroundInfo);
    }
    else
    {
        BackgroundInfo.bmWidth = rects.right;
        BackgroundInfo.bmHeight = rects.bottom;
    }

    TILE_SIZE = CurrentMap.tileSize;

    if (TILE_SIZE <= 0)
    {
        TILE_SIZE = 64;
    }

    W = CurrentMap.width;
    H = CurrentMap.height;

    if (W <= 0)
    {
        W = BackgroundInfo.bmWidth / TILE_SIZE;
    }

    if (H <= 0)
    {
        H = BackgroundInfo.bmHeight / TILE_SIZE;
    }

    if ((int)TileMap.size() != H)
    {
        TileMap.clear();
        TileMap.resize(H);

        for (int y = 0; y < H; ++y)
        {
            TileMap[y].resize(W, ETileType::Normal);
        }
    }

    Loc.first.x = rects.right / 2;
    Loc.first.y = rects.bottom / 2;
    Loc.second.sX = BackgroundInfo.bmWidth / 2;
    Loc.second.sY = BackgroundInfo.bmHeight / 2;

    GameManager::Get().InitDefaultParty();
    playerTeam = GameManager::Get().GetParty();

    for (size_t i = 0; i < playerTeam.size() && i < CurrentMap.playerStarts.size(); ++i)
    {
        Character* c = playerTeam[i];

        if (c == nullptr)
        {
            continue;
        }

        const auto& start = CurrentMap.playerStarts[i];

        c->SetGridPos(start.x, start.y);
        c->LoadSprites(L"Resource");
        c->SyncWorldPixelToGrid(TILE_SIZE);
        c->ResetTurnState();
    }

    for (auto c : enemyTeam)
    {
        delete c;
    }

    enemyTeam.clear();

    for (const auto& start : CurrentMap.enemyStarts)
    {
        Character* e = new Character();
        e->Init(start.classType, start.x, start.y);
        e->SetName(start.name);
        e->LoadSprites(L"Resource");
        e->SyncWorldPixelToGrid(TILE_SIZE);

        enemyTeam.push_back(e);
    }

    turnSystem = new STurnTimeline();
    turnSystem->Init(playerTeam, enemyTeam);

    turnUI = new UTurnOrderPanel();
    turnUI->Setup(rects);
    turnUI->Bind(turnSystem);

    actionMenu = new UBattleActionMenu();
    actionMenu->Init();
    actionMenu->SetScreenRect(rects);
    actionMenu->SetOnMoveClick(std::bind(&Battle_Stage::OnMenuMove, this));
    actionMenu->SetOnAttackClick(std::bind(&Battle_Stage::OnMenuAttack, this));
    actionMenu->SetOnSkillClick(std::bind(&Battle_Stage::OnMenuSkill, this));
    actionMenu->SetOnDefendClick(std::bind(&Battle_Stage::OnMenuDefend, this));
    actionMenu->SetOnItemClick(std::bind(&Battle_Stage::OnMenuItem, this));
    actionMenu->SetOnEndTurnClick(std::bind(&Battle_Stage::OnMenuEndTurn, this));

    ResetTurnActions();

    // 첫 진입 카메라는 보간 없이 즉시 시작 위치로 맞춘다.
    // 기존 FocusCameraOnGrid는 camPosX/Y가 0인 상태에서 0.1 보간을 해서
    // 실제 캐릭터 시작 위치가 화면 밖으로 밀려 보일 수 있었다.
    {
        int halfW = rects.right / 2;
        int halfH = rects.bottom / 2;

        int minX = halfW;
        int minY = halfH;
        int maxX = BackgroundInfo.bmWidth - halfW;
        int maxY = BackgroundInfo.bmHeight - halfH;

        if (maxX < minX) maxX = minX;
        if (maxY < minY) maxY = minY;

        int targetX = CurrentMap.cameraStartX * TILE_SIZE + TILE_SIZE / 2;
        int targetY = CurrentMap.cameraStartY * TILE_SIZE + TILE_SIZE / 2;

        if (targetX < minX) targetX = minX;
        if (targetX > maxX) targetX = maxX;
        if (targetY < minY) targetY = minY;
        if (targetY > maxY) targetY = maxY;

        Loc.second.sX = targetX;
        Loc.second.sY = targetY;
        camPosX = (float)targetX;
        camPosY = (float)targetY;
    }
}

void Battle_Stage::LoadBattleMap(EBattleMapId mapId)
{
    CurrentMap = BattleMapData::Load(mapId);
    TileMap = CurrentMap.tiles;
}

void Battle_Stage::Update(HWND hWnd)
{
    for (auto c : playerTeam)
    {
        if (c)
        {
            c->Update();
        }
    }

    for (auto c : enemyTeam)
    {
        if (c)
        {
            c->Update();
        }
    }

    UpdateDamageTexts();

    CheckBattleEnd();

    if (BattleEnded)
    {
        if (actionMenu)
        {
            actionMenu->SetVisible(false);
        }

        ClearMovePath();

        if (turnUI)
        {
            turnUI->Update();
        }

        return;
    }

    Character* focusTarget = nullptr;

    const bool bSkillTargetCameraMode =
        (controlMode == EBattleControlMode::SELECT_SKILL && bSelectingBattleSkillTarget);

    if (bSkillTargetCameraMode)
    {
        UpdateSkillTargetCameraByMouse(hWnd);
    }
    else
    {
        if (MovingUnit != nullptr)
        {
            focusTarget = MovingUnit;
        }
        else if (ActingUnit != nullptr)
        {
            focusTarget = ActingUnit;
        }
        else if (turnSystem != nullptr)
        {
            focusTarget = turnSystem->GetCurrentTurnCharacter();
        }

        if (focusTarget != nullptr)
        {
            Location p = focusTarget->GetWorldPixelPos(TILE_SIZE);
            FocusCameraOnWorldPos(p.x + TILE_SIZE / 2, p.y + TILE_SIZE / 2);
        }
    }

    if (MovingUnit != nullptr)
    {
        if (!MovingUnit->GetIsMoving())
        {
            bool isEnemyTurn = false;

            if (turnSystem)
            {
                isEnemyTurn = turnSystem->IsCurrentTurnEnemy();
            }

            ActiveMoveIndex++;

            if (ActiveMoveIndex < (int)ActiveMovePath.size())
            {
                Location next = ActiveMovePath[ActiveMoveIndex];

                if (MovingUnit->StartMoveToGrid(next.x, next.y, TILE_SIZE))
                {
                    if (turnUI)
                    {
                        turnUI->Update();
                    }

                    return;
                }
            }

            ApplyTileEffect(MovingUnit);

            bool movingUnitAlive = MovingUnit->IsAlive();

            MovingUnit = nullptr;
            ClearMovePath();

            controlMode = EBattleControlMode::WAIT_COMMAND;

            CheckBattleEnd();

            if (BattleEnded)
            {
                return;
            }

            if (turnSystem)
            {
                if (movingUnitAlive)
                {
                    if (isEnemyTurn)
                    {
                        turnSystem->NextTurn();
                    }
                }
                else
                {
                    // The current moving unit can be removed by terrain damage.
                    // In that case, do not call NextTurn(), because the next unit is
                    // already at the front of the turn queue after RemoveDeadUnits().
                    turnSystem->RemoveDeadUnits();
                }
            }

            if (isEnemyTurn || !movingUnitAlive)
            {
                ResetTurnActions();
            }
        }

        if (turnUI)
        {
            turnUI->Update();
        }

        return;
    }

    if (ActingUnit != nullptr)
    {
        if (ActingUnit->GetIsActionEnd())
        {
            if (ActingUnit->IsAlive())
            {
                ActingUnit->SetState(ECharacterState::Idle);
            }

            ActingUnit->SetIsActionEnd(false);

            bool isEnemyTurn = false;

            if (turnSystem)
            {
                isEnemyTurn = turnSystem->IsCurrentTurnEnemy();
            }

            ActingUnit = nullptr;

            for (auto c : playerTeam)
            {
                if (c && c->IsAlive() && c->GetState() == ECharacterState::Hit)
                {
                    c->SetState(ECharacterState::Idle);
                    c->SetIsActionEnd(false);
                }
            }

            for (auto c : enemyTeam)
            {
                if (c && c->IsAlive() && c->GetState() == ECharacterState::Hit)
                {
                    c->SetState(ECharacterState::Idle);
                    c->SetIsActionEnd(false);
                }
            }

            CheckBattleEnd();

            if (BattleEnded)
            {
                return;
            }

            if (isEnemyTurn)
            {
                if (turnSystem)
                {
                    turnSystem->NextTurn();
                }

                ResetTurnActions();
            }

            controlMode = EBattleControlMode::WAIT_COMMAND;
        }

        if (turnUI)
        {
            turnUI->Update();
        }

        return;
    }

    if (turnSystem != nullptr && turnSystem->IsCurrentTurnEnemy())
    {
        ClearMovePath();

        if (actionMenu)
        {
            actionMenu->SetVisible(false);
        }

        ProcessEnemyTurn();

        if (turnUI)
        {
            turnUI->Update();
        }

        return;
    }

    if (turnUI)
    {
        turnUI->Update();
    }

    if (turnSystem != nullptr && !turnSystem->IsCurrentTurnEnemy())
    {
        Character* current = turnSystem->GetCurrentTurnCharacter();

        if (current != nullptr && current->IsAlive())
        {
            if (controlMode == EBattleControlMode::WAIT_COMMAND)
            {
                if (actionMenu)
                {
                    actionMenu->SetVisible(true);
                    actionMenu->UpdateButtonStates(current);

                    Location worldPos = current->GetWorldPixelPos(TILE_SIZE);
                    int offsetX = GetOffsetX();
                    int offsetY = GetOffsetY();

                    actionMenu->UpdatePosition(offsetX + worldPos.x, offsetY + worldPos.y);
                }
            }
            else
            {
                if (actionMenu)
                {
                    actionMenu->SetVisible(false);
                }
            }
        }
    }
    else
    {
        if (actionMenu)
        {
            actionMenu->SetVisible(false);
        }
    }

    if (actionMenu)
    {
        actionMenu->Update();
    }
}

void Battle_Stage::Draw(HDC hMemDC, HDC hMemDC1)
{
    DrawBackground(hMemDC, hMemDC1);
    DrawGrid(hMemDC);
    DrawTileOverlay(hMemDC);

    if (controlMode == EBattleControlMode::SELECT_MOVE && !BattleEnded)
    {
        DrawMoveRange(hMemDC);
        DrawBlockedMoveRange(hMemDC);
    }

    if (!BattleEnded && controlMode == EBattleControlMode::SELECT_ATTACK)
    {
        DrawAttackRange(hMemDC);
    }

    if (!BattleEnded && controlMode == EBattleControlMode::SELECT_SKILL && bSelectingBattleSkillTarget)
    {
        DrawSkillRange(hMemDC);
    }

    DrawMoveArrow(hMemDC);

    for (auto c : playerTeam)
    {
        DrawUnit(hMemDC, hMemDC1, c, false);
    }

    for (auto c : enemyTeam)
    {
        DrawUnit(hMemDC, hMemDC1, c, true);
    }

    DrawDamageTexts(hMemDC);

    if (turnUI)
    {
        turnUI->Draw(hMemDC, hMemDC1);
    }

    if (actionMenu && actionMenu->GetVisible())
    {
        actionMenu->Draw(hMemDC, hMemDC1);
    }

    if (!BattleEnded && controlMode == EBattleControlMode::SELECT_SKILL)
    {
        DrawBattleSkillUI(hMemDC);
    }

    if (!BattleEnded && controlMode == EBattleControlMode::SELECT_ITEM)
    {
        DrawBattleItemUI(hMemDC);
    }

    if (BattleEnded)
    {
        DrawBattleEndMessage(hMemDC);
    }
}

int Battle_Stage::Input(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (BattleEnded)
    {
        if (uMsg == WM_KEYDOWN)
        {
            if (wParam == VK_RETURN || wParam == VK_SPACE || wParam == VK_ESCAPE)
            {
                if (BattleWon)
                {
                    return MAINTENANCE_STAGE;
                }

                return MAP_STAGE;
            }
        }

        if (uMsg == WM_LBUTTONDOWN)
        {
            if (BattleWon)
            {
                return MAINTENANCE_STAGE;
            }

            return MAP_STAGE;
        }

        return -2;
    }

    if (uMsg == WM_KEYDOWN)
    {
        if (wParam == VK_ESCAPE)
        {
            if (controlMode == EBattleControlMode::SELECT_ITEM)
            {
                ClearBattleItemSelection();
                controlMode = EBattleControlMode::WAIT_COMMAND;
                return -2;
            }

            if (controlMode == EBattleControlMode::SELECT_SKILL)
            {
                ClearBattleSkillSelection();
                controlMode = EBattleControlMode::WAIT_COMMAND;
                return -2;
            }

            return MAP_STAGE;
        }

        if (wParam == VK_SPACE)
        {
            if (turnSystem && MovingUnit == nullptr && ActingUnit == nullptr)
            {
                ClearMovePath();
                controlMode = EBattleControlMode::WAIT_COMMAND;
                turnSystem->NextTurn();
                ResetTurnActions();
            }

            return -2;
        }

        if (wParam == VK_LEFT || wParam == 'A')
        {
            BackgroundMove(-1, 0);
            return -2;
        }

        if (wParam == VK_RIGHT || wParam == 'D')
        {
            BackgroundMove(1, 0);
            return -2;
        }

        if (wParam == VK_UP || wParam == 'W')
        {
            BackgroundMove(0, -1);
            return -2;
        }

        if (wParam == VK_DOWN || wParam == 'S')
        {
            BackgroundMove(0, 1);
            return -2;
        }
    }

    if (uMsg == WM_MOUSEMOVE)
    {
        int mouseX = LOWORD(lParam);
        int mouseY = HIWORD(lParam);
        LastMouseX = mouseX;
        LastMouseY = mouseY;

        if (controlMode == EBattleControlMode::SELECT_MOVE)
        {
            UpdatePreviewMovePathByMouse(mouseX, mouseY);
        }
        else if (controlMode == EBattleControlMode::SELECT_SKILL && bSelectingBattleSkillTarget)
        {
            UpdateHoverGridByMouse(mouseX, mouseY);
        }
        else
        {
            HoverGridX = -1;
            HoverGridY = -1;
            PreviewMovePath.clear();
        }

        return -2;
    }

    if (uMsg == WM_RBUTTONDOWN)
    {
        if (MovingUnit == nullptr && ActingUnit == nullptr)
        {
            ClearMovePath();
            ClearBattleItemSelection();
            ClearBattleSkillSelection();
            controlMode = EBattleControlMode::WAIT_COMMAND;
        }

        return -2;
    }

    if (uMsg == WM_LBUTTONDOWN)
    {
        if (turnSystem == nullptr)
        {
            return -2;
        }

        if (MovingUnit != nullptr || ActingUnit != nullptr)
        {
            return -2;
        }

        if (turnSystem->IsCurrentTurnEnemy())
        {
            return -2;
        }

        Character* current = turnSystem->GetCurrentTurnCharacter();

        if (current == nullptr || !current->IsAlive())
        {
            return -2;
        }

        int mouseX = LOWORD(lParam);
        int mouseY = HIWORD(lParam);

        if (controlMode == EBattleControlMode::SELECT_SKILL)
        {
            HandleBattleSkillClick(mouseX, mouseY, current);
            return -2;
        }

        if (controlMode == EBattleControlMode::SELECT_ITEM)
        {
            HandleBattleItemClick(mouseX, mouseY, current);
            return -2;
        }

        if (controlMode == EBattleControlMode::WAIT_COMMAND && actionMenu && actionMenu->GetVisible())
        {
            UWidget* clicked = actionMenu->FindWidgetUnderMouse(mouseX, mouseY);

            if (clicked)
            {
                UButton* btn = dynamic_cast<UButton*>(clicked);

                if (btn && btn->GetEnabled())
                {
                    btn->Click();
                    return -2;
                }
            }
        }

        int offsetX = GetOffsetX();
        int offsetY = GetOffsetY();

        int worldX = mouseX - offsetX;
        int worldY = mouseY - offsetY;

        if (worldX < 0 || worldY < 0)
        {
            return -2;
        }

        int gridX = worldX / TILE_SIZE;
        int gridY = worldY / TILE_SIZE;

        if (!IsInsideGrid(gridX, gridY))
        {
            return -2;
        }

        if (controlMode == EBattleControlMode::SELECT_MOVE)
        {
            UpdatePreviewMovePathByMouse(mouseX, mouseY);

            if (!PreviewMovePath.empty())
            {
                if (StartActiveMovePath(current, PreviewMovePath))
                {
                    current->SetHasMoved(true);
                    controlMode = EBattleControlMode::WAIT_COMMAND;
                }
            }

            return -2;
        }

        if (controlMode == EBattleControlMode::SELECT_ATTACK)
        {
            Character* target = GetEnemyAt(gridX, gridY);

            if (target && current->IsInAttackRange(*target))
            {
                current->LookAtGrid(gridX, gridY);

                int damage = current->CalcAttackDamage(*target);

                current->SetState(ECharacterState::Attack);
                target->GetDamage(damage);
                AddDamageText(target, damage, RGB(255, 70, 70));

                if (turnSystem)
                {
                    turnSystem->RemoveDeadUnits();
                }

                current->SetHasActed(true);
                ActingUnit = current;
                ClearMovePath();
                controlMode = EBattleControlMode::WAIT_COMMAND;

                CheckBattleEnd();
            }

            return -2;
        }

        return -2;
    }

    return -2;
}

void Battle_Stage::OnMenuMove()
{
    ClearMovePath();
    ClearBattleSkillSelection();
    controlMode = EBattleControlMode::SELECT_MOVE;
}

void Battle_Stage::OnMenuAttack()
{
    ClearMovePath();
    ClearBattleSkillSelection();
    controlMode = EBattleControlMode::SELECT_ATTACK;
}

void Battle_Stage::OnMenuSkill()
{
    ClearMovePath();

    if (turnSystem != nullptr)
    {
        RefreshBattleSkillList(turnSystem->GetCurrentTurnCharacter());
    }

    SelectedBattleSkillID = -1;
    bSelectingBattleSkillTarget = false;
    controlMode = EBattleControlMode::SELECT_SKILL;
}

void Battle_Stage::OnMenuDefend()
{
    ClearMovePath();
    ClearBattleSkillSelection();

    if (turnSystem == nullptr)
    {
        return;
    }

    Character* current = turnSystem->GetCurrentTurnCharacter();

    if (current)
    {
        current->SetState(ECharacterState::Defend);
        current->SetHasActed(true);
        controlMode = EBattleControlMode::WAIT_COMMAND;
    }
}

void Battle_Stage::OnMenuItem()
{
    ClearMovePath();
    ClearBattleSkillSelection();
    RefreshBattleConsumableItems();
    SelectedBattleItemID = -1;
    bSelectingBattleItemTarget = false;
    controlMode = EBattleControlMode::SELECT_ITEM;
}

void Battle_Stage::OnMenuEndTurn()
{
    ClearMovePath();
    ClearBattleSkillSelection();

    if (actionMenu)
    {
        actionMenu->SetVisible(false);
    }

    if (turnSystem)
    {
        Character* current = turnSystem->GetCurrentTurnCharacter();

        if (current != nullptr && current->IsAlive())
        {
            ApplyTileEffect(current);
            CheckBattleEnd();

            if (BattleEnded)
            {
                return;
            }
        }

        turnSystem->NextTurn();
    }

    ResetTurnActions();
    controlMode = EBattleControlMode::WAIT_COMMAND;
}

void Battle_Stage::ResetTurnActions()
{
    if (turnSystem == nullptr)
    {
        return;
    }

    Character* current = turnSystem->GetCurrentTurnCharacter();

    if (current)
    {
        current->ResetTurnState();
    }
}

bool Battle_Stage::IsTeamAllDead(const std::vector<Character*>& team) const
{
    if (team.empty())
    {
        return false;
    }

    for (auto c : team)
    {
        if (c != nullptr && c->IsAlive())
        {
            return false;
        }
    }

    return true;
}

void Battle_Stage::CheckBattleEnd()
{
    if (BattleEnded)
    {
        return;
    }

    if (IsTeamAllDead(enemyTeam))
    {
        BattleEnded = true;
        BattleWon = true;

        int clearedStageId = GameManager::Get().GetCurrentBattleStageId();
        int nextStageId = clearedStageId + 1;

        if (nextStageId > 5)
        {
            nextStageId = 5;
        }

        BattleRewardManager::Get().SetDefaultVictoryReward(clearedStageId, nextStageId);

        ClearMovePath();

        if (actionMenu)
        {
            actionMenu->SetVisible(false);
        }

        return;
    }

    if (IsTeamAllDead(playerTeam))
    {
        BattleEnded = true;
        BattleWon = false;

        std::vector<SBattleRewardItem> noItems;

        int currentStageId = GameManager::Get().GetCurrentBattleStageId();

        BattleRewardManager::Get().SetReward(
            false,
            currentStageId,
            currentStageId,
            0,
            0,
            noItems
        );

        ClearMovePath();

        if (actionMenu)
        {
            actionMenu->SetVisible(false);
        }

        return;
    }
}

void Battle_Stage::DrawBattleEndMessage(HDC hMemDC)
{
    RECT box;

    box.left = rects.right / 2 - 300;
    box.top = rects.bottom / 2 - 110;
    box.right = rects.right / 2 + 300;
    box.bottom = rects.bottom / 2 + 110;

    HBRUSH hBrush = CreateSolidBrush(RGB(20, 20, 20));
    FillRect(hMemDC, &box, hBrush);
    DeleteObject(hBrush);

    HPEN hPen = CreatePen(PS_SOLID, 3, BattleWon ? RGB(255, 220, 80) : RGB(255, 80, 80));
    HPEN hOldPen = (HPEN)SelectObject(hMemDC, hPen);

    HBRUSH hOldBrush = (HBRUSH)SelectObject(hMemDC, GetStockObject(NULL_BRUSH));

    Rectangle(hMemDC, box.left, box.top, box.right, box.bottom);

    SelectObject(hMemDC, hOldBrush);
    SelectObject(hMemDC, hOldPen);
    DeleteObject(hPen);

    int oldBkMode = SetBkMode(hMemDC, TRANSPARENT);

    HFONT hTitleFont = CreateFontW(
        54,
        0,
        0,
        0,
        FW_HEAVY,
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

    HFONT hOldFont = (HFONT)SelectObject(hMemDC, hTitleFont);

    std::wstring title = BattleWon ? L"VICTORY" : L"DEFEAT";

    RECT titleRect = box;
    titleRect.top += 35;
    titleRect.bottom = titleRect.top + 70;

    SetTextColor(hMemDC, BattleWon ? RGB(255, 220, 80) : RGB(255, 100, 100));

    DrawTextW(
        hMemDC,
        title.c_str(),
        (int)title.length(),
        &titleRect,
        DT_CENTER | DT_VCENTER | DT_SINGLELINE
    );

    SelectObject(hMemDC, hOldFont);
    DeleteObject(hTitleFont);

    HFONT hSubFont = CreateFontW(
        24,
        0,
        0,
        0,
        FW_NORMAL,
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

    hOldFont = (HFONT)SelectObject(hMemDC, hSubFont);

    std::wstring sub;

    if (BattleWon)
    {
        sub = L"Click or press Enter / Space to Preparation";
    }
    else
    {
        sub = L"Click or press Enter / Space to Map";
    }

    RECT subRect = box;
    subRect.top += 125;
    subRect.bottom = subRect.top + 40;

    SetTextColor(hMemDC, RGB(230, 230, 230));

    DrawTextW(
        hMemDC,
        sub.c_str(),
        (int)sub.length(),
        &subRect,
        DT_CENTER | DT_VCENTER | DT_SINGLELINE
    );

    SelectObject(hMemDC, hOldFont);
    DeleteObject(hSubFont);

    SetBkMode(hMemDC, oldBkMode);
}

ETileType Battle_Stage::GetTileType(int gx, int gy) const
{
    if (!IsInsideGrid(gx, gy))
    {
        return ETileType::Wall;
    }

    if (gy < 0 || gy >= (int)TileMap.size())
    {
        return ETileType::Wall;
    }

    if (gx < 0 || gx >= (int)TileMap[gy].size())
    {
        return ETileType::Wall;
    }

    return TileMap[gy][gx];
}

bool Battle_Stage::IsBlockedTile(int gx, int gy, Character* unit) const
{
    ETileType type = GetTileType(gx, gy);

    switch (type)
    {
    case ETileType::Water:
        // 바다/물 타일은 수속성 유닛만 이동 가능.
        return !(unit != nullptr && unit->GetElement() == EElement::Water);

    case ETileType::SkyVoid:
        // 천상/공중 빈 공간은 빛속성 유닛만 이동 가능.
        return !(unit != nullptr && unit->GetElement() == EElement::Light);

    case ETileType::Building:
    case ETileType::Wall:
    case ETileType::Fence:
    case ETileType::Tree:
    case ETileType::Grave:
        return true;

    case ETileType::Normal:
    case ETileType::Road:
    case ETileType::MagicHazard:
    case ETileType::CursedGround:
    case ETileType::Lava:
    default:
        return false;
    }
}

bool Battle_Stage::IsDamageTile(int gx, int gy) const
{
    ETileType type = GetTileType(gx, gy);

    return type == ETileType::MagicHazard ||
        type == ETileType::CursedGround ||
        type == ETileType::Lava;
}

int Battle_Stage::GetTileDamage(int gx, int gy) const
{
    ETileType type = GetTileType(gx, gy);

    switch (type)
    {
    case ETileType::MagicHazard:
        return 8;

    case ETileType::CursedGround:
        return 4;

    case ETileType::Lava:
        return 10;

    default:
        return 0;
    }
}

void Battle_Stage::ApplyTileEffect(Character* unit)
{
    if (unit == nullptr || !unit->IsAlive())
    {
        return;
    }

    Location pos = unit->GetGridPos();

    if (!IsDamageTile(pos.x, pos.y))
    {
        return;
    }

    ETileType type = GetTileType(pos.x, pos.y);

    // 용암은 누구나 밟을 수 있지만, 불속성은 피해를 받지 않는다.
    if (type == ETileType::Lava && unit->GetElement() == EElement::Fire)
    {
        return;
    }

    int damage = GetTileDamage(pos.x, pos.y);

    if (damage <= 0)
    {
        return;
    }

    unit->GetDamage(damage);

    if (type == ETileType::MagicHazard)
    {
        AddDamageText(unit, damage, RGB(80, 180, 255));
    }
    else if (type == ETileType::CursedGround)
    {
        AddDamageText(unit, damage, RGB(180, 80, 255));
    }
    else if (type == ETileType::Lava)
    {
        AddDamageText(unit, damage, RGB(255, 80, 20));
    }
    else
    {
        AddDamageText(unit, damage, RGB(255, 255, 255));
    }

    if (turnSystem)
    {
        turnSystem->RemoveDeadUnits();
    }
}

COLORREF Battle_Stage::GetTileOverlayColor(ETileType type) const
{
    switch (type)
    {
    case ETileType::Water:
        return RGB(40, 120, 255);

    case ETileType::Building:
        return RGB(70, 70, 70);

    case ETileType::Wall:
        return RGB(110, 110, 110);

    case ETileType::Fence:
        return RGB(150, 120, 80);

    case ETileType::Tree:
        return RGB(30, 130, 60);

    case ETileType::Grave:
        return RGB(170, 170, 170);

    case ETileType::MagicHazard:
        return RGB(80, 180, 255);

    case ETileType::CursedGround:
        return RGB(180, 80, 255);

    case ETileType::Lava:
        return RGB(255, 80, 20);

    case ETileType::SkyVoid:
        return RGB(120, 200, 255);

    case ETileType::Road:
    case ETileType::Normal:
    default:
        return RGB(0, 0, 0);
    }
}

void Battle_Stage::DrawTileOverlay(HDC hMemDC)
{
    if (!CurrentMap.drawTileDebug)
    {
        return;
    }

    int offsetX = GetOffsetX();
    int offsetY = GetOffsetY();

    for (int y = 0; y < H; ++y)
    {
        for (int x = 0; x < W; ++x)
        {
            ETileType type = GetTileType(x, y);

            if (type == ETileType::Normal || type == ETileType::Road)
            {
                continue;
            }

            COLORREF color = GetTileOverlayColor(type);

            int sx = offsetX + x * TILE_SIZE;
            int sy = offsetY + y * TILE_SIZE;

            HPEN hPen = CreatePen(PS_SOLID, 3, color);
            HPEN hOldPen = (HPEN)SelectObject(hMemDC, hPen);

            HBRUSH hOldBrush = (HBRUSH)SelectObject(hMemDC, GetStockObject(NULL_BRUSH));

            Rectangle(
                hMemDC,
                sx + 3,
                sy + 3,
                sx + TILE_SIZE - 3,
                sy + TILE_SIZE - 3
            );

            if (type == ETileType::MagicHazard ||
                type == ETileType::CursedGround ||
                type == ETileType::Water ||
                type == ETileType::Lava ||
                type == ETileType::SkyVoid)
            {
                MoveToEx(hMemDC, sx + 8, sy + 8, NULL);
                LineTo(hMemDC, sx + TILE_SIZE - 8, sy + TILE_SIZE - 8);

                MoveToEx(hMemDC, sx + TILE_SIZE - 8, sy + 8, NULL);
                LineTo(hMemDC, sx + 8, sy + TILE_SIZE - 8);
            }

            SelectObject(hMemDC, hOldBrush);
            SelectObject(hMemDC, hOldPen);
            DeleteObject(hPen);
        }
    }
}

void Battle_Stage::AddDamageText(Character* target, int damage, COLORREF color)
{
    if (target == nullptr)
    {
        return;
    }

    Location gridPos = target->GetGridPos();

    DamageText dt;
    dt.x = gridPos.x * TILE_SIZE + TILE_SIZE / 2;
    dt.y = gridPos.y * TILE_SIZE - 12;
    dt.value = damage;
    dt.life = 60;
    dt.maxLife = 60;
    dt.color = color;

    DamageTexts.push_back(dt);
}

void Battle_Stage::UpdateDamageTexts()
{
    for (auto& dt : DamageTexts)
    {
        dt.life--;
    }

    DamageTexts.erase(
        std::remove_if(
            DamageTexts.begin(),
            DamageTexts.end(),
            [](const DamageText& dt)
            {
                return dt.life <= 0;
            }
        ),
        DamageTexts.end()
    );
}

void Battle_Stage::DrawDamageTexts(HDC hMemDC)
{
    if (DamageTexts.empty())
    {
        return;
    }

    int offsetX = GetOffsetX();
    int offsetY = GetOffsetY();

    int oldBkMode = SetBkMode(hMemDC, TRANSPARENT);

    HFONT hFont = CreateFontW(
        34,
        0,
        0,
        0,
        FW_HEAVY,
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

    for (const auto& dt : DamageTexts)
    {
        int elapsed = dt.maxLife - dt.life;

        int sx = offsetX + dt.x;
        int sy = offsetY + dt.y - elapsed / 2;

        std::wstring text;

        if (dt.value >= 0)
        {
            text = L"-" + std::to_wstring(dt.value);
        }
        else
        {
            text = L"+" + std::to_wstring(-dt.value);
        }

        SIZE textSize;
        GetTextExtentPoint32W(hMemDC, text.c_str(), (int)text.length(), &textSize);

        int drawX = sx - textSize.cx / 2;
        int drawY = sy - textSize.cy / 2;

        SetTextColor(hMemDC, RGB(0, 0, 0));
        TextOutW(hMemDC, drawX - 2, drawY, text.c_str(), (int)text.length());
        TextOutW(hMemDC, drawX + 2, drawY, text.c_str(), (int)text.length());
        TextOutW(hMemDC, drawX, drawY - 2, text.c_str(), (int)text.length());
        TextOutW(hMemDC, drawX, drawY + 2, text.c_str(), (int)text.length());

        SetTextColor(hMemDC, dt.color);
        TextOutW(hMemDC, drawX, drawY, text.c_str(), (int)text.length());
    }

    SelectObject(hMemDC, hOldFont);
    DeleteObject(hFont);

    SetBkMode(hMemDC, oldBkMode);
}



void Battle_Stage::RefreshBattleSkillList(Character* current)
{
    BattleSkillIDs.clear();

    if (current == nullptr)
    {
        return;
    }

    std::vector<int> equipped = current->GetEquippedSkillIDs();

    for (int skillID : equipped)
    {
        if (SkillData::CanCharacterUse(current, skillID))
        {
            BattleSkillIDs.push_back(skillID);
        }
    }

    if (!BattleSkillIDs.empty())
    {
        return;
    }

    std::vector<int> learned = current->GetLearnedSkillIDs();

    for (int skillID : learned)
    {
        if (SkillData::CanCharacterUse(current, skillID))
        {
            BattleSkillIDs.push_back(skillID);
        }
    }
}

void Battle_Stage::ClearBattleSkillSelection()
{
    SelectedBattleSkillID = -1;
    bSelectingBattleSkillTarget = false;
    BattleSkillIDs.clear();
    HoverGridX = -1;
    HoverGridY = -1;
}

RECT Battle_Stage::GetBattleSkillRect(int index) const
{
    return GetBattleItemRect(index);
}

void Battle_Stage::DrawBattleSkillUI(HDC hMemDC)
{
    if (bSelectingBattleSkillTarget)
    {
        DrawBattleSkillTargetSelect(hMemDC);
    }
    else
    {
        DrawBattleSkillList(hMemDC);
    }
}

void Battle_Stage::DrawBattleSkillList(HDC hMemDC)
{
    RECT panel;
    panel.left = rects.right - 390;
    panel.top = 80;
    panel.right = rects.right - 25;
    panel.bottom = 610;

    HBRUSH hBrush = CreateSolidBrush(RGB(19, 23, 31));
    FillRect(hMemDC, &panel, hBrush);
    DeleteObject(hBrush);

    HPEN hPen = CreatePen(PS_SOLID, 3, RGB(176, 146, 78));
    HPEN hOldPen = (HPEN)SelectObject(hMemDC, hPen);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hMemDC, GetStockObject(NULL_BRUSH));
    Rectangle(hMemDC, panel.left, panel.top, panel.right, panel.bottom);
    SelectObject(hMemDC, hOldBrush);
    SelectObject(hMemDC, hOldPen);
    DeleteObject(hPen);

    RECT titleRect = panel;
    titleRect.top += 18;
    titleRect.bottom = titleRect.top + 40;

    DrawBattleStageText(
        hMemDC,
        L"마법",
        titleRect,
        28,
        FW_HEAVY,
        RGB(245, 220, 150),
        DT_CENTER | DT_VCENTER | DT_SINGLELINE
    );

    RECT guideRect = panel;
    guideRect.left += 20;
    guideRect.right -= 20;
    guideRect.top += 60;
    guideRect.bottom = guideRect.top + 26;

    DrawBattleStageText(
        hMemDC,
        L"사용할 마법을 선택하세요.",
        guideRect,
        16,
        FW_NORMAL,
        RGB(220, 220, 220),
        DT_CENTER | DT_VCENTER | DT_SINGLELINE
    );

    Character* current = nullptr;

    if (turnSystem != nullptr)
    {
        current = turnSystem->GetCurrentTurnCharacter();
    }

    RefreshBattleSkillList(current);

    if (BattleSkillIDs.empty())
    {
        RECT emptyRect = panel;
        emptyRect.left += 25;
        emptyRect.right -= 25;
        emptyRect.top += 150;
        emptyRect.bottom = emptyRect.top + 70;

        DrawBattleStageText(
            hMemDC,
            L"사용 가능한 마법이 없습니다.",
            emptyRect,
            22,
            FW_BOLD,
            RGB(230, 230, 230),
            DT_CENTER | DT_VCENTER | DT_SINGLELINE
        );
    }
    else
    {
        int visibleCount = (int)BattleSkillIDs.size();

        if (visibleCount > 6)
        {
            visibleCount = 6;
        }

        CharacterStat stat;
        int curMP = 0;

        if (current != nullptr)
        {
            stat = current->GetStat();
            curMP = stat.CurMP;
        }

        for (int i = 0; i < visibleCount; ++i)
        {
            int skillID = BattleSkillIDs[i];
            const SSkillData* info = FindBattleSkillInfo(skillID);

            if (info == nullptr)
            {
                continue;
            }

            std::wstring text = std::wstring(info->Name)
                + L"  MP " + std::to_wstring(info->MPCost)
                + L"  R" + std::to_wstring(info->Range);

            COLORREF border = info->Color;
            COLORREF textColor = RGB(240, 240, 240);

            if (curMP < info->MPCost)
            {
                border = RGB(90, 90, 90);
                textColor = RGB(150, 150, 150);
            }

            DrawBattleStageButton(
                hMemDC,
                GetBattleSkillRect(i),
                text,
                border,
                textColor
            );

            RECT descRect = GetBattleSkillRect(i);
            descRect.left += 8;
            descRect.right -= 8;
            descRect.top += 28;
            descRect.bottom -= 2;

            DrawBattleStageText(
                hMemDC,
                info->Description,
                descRect,
                13,
                FW_NORMAL,
                textColor,
                DT_CENTER | DT_VCENTER | DT_SINGLELINE
            );
        }
    }

    DrawBattleStageButton(
        hMemDC,
        GetBattleCancelRect(),
        L"취소",
        RGB(170, 170, 170),
        RGB(230, 230, 230)
    );
}

void Battle_Stage::DrawBattleSkillTargetSelect(HDC hMemDC)
{
    RECT panel;
    panel.left = rects.right - 390;
    panel.top = 80;
    panel.right = rects.right - 25;
    panel.bottom = 610;

    HBRUSH hBrush = CreateSolidBrush(RGB(19, 23, 31));
    FillRect(hMemDC, &panel, hBrush);
    DeleteObject(hBrush);

    const SSkillData* info = FindBattleSkillInfo(SelectedBattleSkillID);
    COLORREF lineColor = RGB(210, 120, 255);

    if (info != nullptr)
    {
        lineColor = info->Color;
    }

    HPEN hPen = CreatePen(PS_SOLID, 3, lineColor);
    HPEN hOldPen = (HPEN)SelectObject(hMemDC, hPen);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hMemDC, GetStockObject(NULL_BRUSH));
    Rectangle(hMemDC, panel.left, panel.top, panel.right, panel.bottom);
    SelectObject(hMemDC, hOldBrush);
    SelectObject(hMemDC, hOldPen);
    DeleteObject(hPen);

    RECT titleRect = panel;
    titleRect.top += 18;
    titleRect.bottom = titleRect.top + 40;

    DrawBattleStageText(
        hMemDC,
        L"대상 선택",
        titleRect,
        28,
        FW_HEAVY,
        lineColor,
        DT_CENTER | DT_VCENTER | DT_SINGLELINE
    );

    RECT infoRect = panel;
    infoRect.left += 20;
    infoRect.right -= 20;
    infoRect.top += 72;
    infoRect.bottom = infoRect.top + 88;

    std::wstring skillText = L"Unknown Skill";

    if (info != nullptr)
    {
        skillText = std::wstring(info->Name)
            + L"\nMP " + std::to_wstring(info->MPCost)
            + L" / Range " + std::to_wstring(info->Range)
            + L"\n" + info->Description;
    }

    DrawBattleStageText(
        hMemDC,
        skillText,
        infoRect,
        17,
        FW_BOLD,
        RGB(240, 240, 240),
        DT_CENTER | DT_VCENTER | DT_WORDBREAK
    );

    RECT guideRect = panel;
    guideRect.left += 24;
    guideRect.right -= 24;
    guideRect.top += 180;
    guideRect.bottom = guideRect.top + 90;

    DrawBattleStageText(
        hMemDC,
        L"Click an enemy on the map.\nColored tiles show the target range.",
        guideRect,
        16,
        FW_NORMAL,
        RGB(220, 220, 220),
        DT_CENTER | DT_VCENTER | DT_WORDBREAK
    );

    DrawBattleStageButton(
        hMemDC,
        GetBattleCancelRect(),
        L"Back",
        RGB(170, 170, 170),
        RGB(230, 230, 230)
    );
}

void Battle_Stage::DrawSkillRange(HDC hMemDC)
{
    if (turnSystem == nullptr)
    {
        return;
    }

    Character* current = turnSystem->GetCurrentTurnCharacter();
    const SSkillData* info = FindBattleSkillInfo(SelectedBattleSkillID);

    if (current == nullptr || info == nullptr)
    {
        return;
    }

    int offsetX = GetOffsetX();
    int offsetY = GetOffsetY();

    HPEN hRangePen = CreatePen(PS_SOLID, 2, info->Color);
    HPEN hOldPen = (HPEN)SelectObject(hMemDC, hRangePen);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hMemDC, GetStockObject(NULL_BRUSH));

    for (int y = 0; y < H; ++y)
    {
        for (int x = 0; x < W; ++x)
        {
            if (!IsBattleSkillTargetValid(current, SelectedBattleSkillID, x, y))
            {
                continue;
            }

            int sx = offsetX + x * TILE_SIZE;
            int sy = offsetY + y * TILE_SIZE;

            Rectangle(
                hMemDC,
                sx + 4,
                sy + 4,
                sx + TILE_SIZE - 4,
                sy + TILE_SIZE - 4
            );
        }
    }

    SelectObject(hMemDC, hOldBrush);
    SelectObject(hMemDC, hOldPen);
    DeleteObject(hRangePen);

    DrawSkillAreaPreview(hMemDC);
}

void Battle_Stage::UpdateHoverGridByMouse(int mouseX, int mouseY)
{
    if (controlMode != EBattleControlMode::SELECT_SKILL || !bSelectingBattleSkillTarget)
    {
        HoverGridX = -1;
        HoverGridY = -1;
        return;
    }

    int offsetX = GetOffsetX();
    int offsetY = GetOffsetY();

    int worldX = mouseX - offsetX;
    int worldY = mouseY - offsetY;

    if (worldX < 0 || worldY < 0)
    {
        HoverGridX = -1;
        HoverGridY = -1;
        return;
    }

    int gridX = worldX / TILE_SIZE;
    int gridY = worldY / TILE_SIZE;

    if (!IsInsideGrid(gridX, gridY))
    {
        HoverGridX = -1;
        HoverGridY = -1;
        return;
    }

    HoverGridX = gridX;
    HoverGridY = gridY;
}

void Battle_Stage::UpdateSkillTargetCameraByMouse(HWND hWnd)
{
    if (controlMode != EBattleControlMode::SELECT_SKILL || !bSelectingBattleSkillTarget)
    {
        return;
    }

    POINT mousePos;
    GetCursorPos(&mousePos);
    ScreenToClient(hWnd, &mousePos);

    if (mousePos.x < 0 || mousePos.y < 0 || mousePos.x > rects.right || mousePos.y > rects.bottom)
    {
        return;
    }

    LastMouseX = mousePos.x;
    LastMouseY = mousePos.y;

    float moveX = 0.0f;
    float moveY = 0.0f;

    const int edgeMargin = 42;
    const int skillPanelLeft = rects.right - 420;
    const float cameraSpeed = 34.0f;

    if (mousePos.x < edgeMargin)
    {
        moveX -= 1.0f;
    }

    if (mousePos.x > rects.right - edgeMargin || mousePos.x >= skillPanelLeft)
    {
        moveX += 1.0f;
    }

    if (mousePos.y < edgeMargin)
    {
        moveY -= 1.0f;
    }

    if (mousePos.y > rects.bottom - edgeMargin)
    {
        moveY += 1.0f;
    }

    if (moveX != 0.0f || moveY != 0.0f)
    {
        FocusCameraOnWorldPos(
            (int)(camPosX + moveX * cameraSpeed),
            (int)(camPosY + moveY * cameraSpeed)
        );

        UpdateHoverGridByMouse(LastMouseX, LastMouseY);
    }
}

std::vector<Location> Battle_Stage::BuildSkillAreaCells(Character* user, int skillID, int centerX, int centerY) const
{
    std::vector<Location> cells;

    const SSkillData* info = FindBattleSkillInfo(skillID);

    if (user == nullptr || info == nullptr)
    {
        return cells;
    }

    if (!IsInsideGrid(centerX, centerY))
    {
        return cells;
    }

    Location userPos = user->GetGridPos();

    if (info->AreaType == ESkillAreaType::Single)
    {
        cells.push_back({ centerX, centerY });
        return cells;
    }

    if (info->AreaType == ESkillAreaType::Splash1 || info->AreaType == ESkillAreaType::Splash2)
    {
        int radius = (info->AreaType == ESkillAreaType::Splash2) ? 2 : 1;

        for (int y = centerY - radius; y <= centerY + radius; ++y)
        {
            for (int x = centerX - radius; x <= centerX + radius; ++x)
            {
                if (!IsInsideGrid(x, y))
                {
                    continue;
                }

                int dist = std::abs(x - centerX) + std::abs(y - centerY);

                if (dist <= radius)
                {
                    cells.push_back({ x, y });
                }
            }
        }

        return cells;
    }

    if (info->AreaType == ESkillAreaType::Line)
    {
        int dirX = 0;
        int dirY = 0;

        if (centerX == userPos.x)
        {
            dirY = (centerY > userPos.y) ? 1 : -1;
        }
        else if (centerY == userPos.y)
        {
            dirX = (centerX > userPos.x) ? 1 : -1;
        }
        else
        {
            return cells;
        }

        for (int i = 1; i <= info->Range; ++i)
        {
            int x = userPos.x + dirX * i;
            int y = userPos.y + dirY * i;

            if (!IsInsideGrid(x, y))
            {
                break;
            }

            cells.push_back({ x, y });
        }
    }

    return cells;
}

void Battle_Stage::DrawSkillAreaPreview(HDC hMemDC)
{
    if (turnSystem == nullptr)
    {
        return;
    }

    if (controlMode != EBattleControlMode::SELECT_SKILL || !bSelectingBattleSkillTarget)
    {
        return;
    }

    if (HoverGridX < 0 || HoverGridY < 0)
    {
        return;
    }

    Character* current = turnSystem->GetCurrentTurnCharacter();
    const SSkillData* info = FindBattleSkillInfo(SelectedBattleSkillID);

    if (current == nullptr || info == nullptr)
    {
        return;
    }

    int offsetX = GetOffsetX();
    int offsetY = GetOffsetY();

    bool validTarget = IsBattleSkillTargetValid(current, SelectedBattleSkillID, HoverGridX, HoverGridY);
    COLORREF previewColor = validTarget ? info->Color : RGB(255, 60, 60);

    if (!validTarget)
    {
        int sx = offsetX + HoverGridX * TILE_SIZE;
        int sy = offsetY + HoverGridY * TILE_SIZE;

        HPEN hInvalidPen = CreatePen(PS_SOLID, 4, previewColor);
        HPEN hOldPen = (HPEN)SelectObject(hMemDC, hInvalidPen);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hMemDC, GetStockObject(NULL_BRUSH));

        Rectangle(hMemDC, sx + 6, sy + 6, sx + TILE_SIZE - 6, sy + TILE_SIZE - 6);
        MoveToEx(hMemDC, sx + 12, sy + 12, NULL);
        LineTo(hMemDC, sx + TILE_SIZE - 12, sy + TILE_SIZE - 12);
        MoveToEx(hMemDC, sx + TILE_SIZE - 12, sy + 12, NULL);
        LineTo(hMemDC, sx + 12, sy + TILE_SIZE - 12);

        SelectObject(hMemDC, hOldBrush);
        SelectObject(hMemDC, hOldPen);
        DeleteObject(hInvalidPen);
        return;
    }

    std::vector<Location> cells = BuildSkillAreaCells(current, SelectedBattleSkillID, HoverGridX, HoverGridY);

    if (cells.empty())
    {
        return;
    }

    HBRUSH hAreaBrush = CreateHatchBrush(HS_DIAGCROSS, previewColor);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hMemDC, hAreaBrush);
    HPEN hAreaPen = CreatePen(PS_SOLID, 4, previewColor);
    HPEN hOldPen = (HPEN)SelectObject(hMemDC, hAreaPen);

    for (const Location& cell : cells)
    {
        int sx = offsetX + cell.x * TILE_SIZE;
        int sy = offsetY + cell.y * TILE_SIZE;
        Rectangle(hMemDC, sx + 8, sy + 8, sx + TILE_SIZE - 8, sy + TILE_SIZE - 8);
    }

    SelectObject(hMemDC, hOldBrush);
    SelectObject(hMemDC, hOldPen);
    DeleteObject(hAreaBrush);
    DeleteObject(hAreaPen);

    int centerX = offsetX + HoverGridX * TILE_SIZE;
    int centerY = offsetY + HoverGridY * TILE_SIZE;
    HPEN hCenterPen = CreatePen(PS_SOLID, 5, RGB(255, 255, 255));
    hOldPen = (HPEN)SelectObject(hMemDC, hCenterPen);
    hOldBrush = (HBRUSH)SelectObject(hMemDC, GetStockObject(NULL_BRUSH));
    Rectangle(hMemDC, centerX + 3, centerY + 3, centerX + TILE_SIZE - 3, centerY + TILE_SIZE - 3);
    SelectObject(hMemDC, hOldBrush);
    SelectObject(hMemDC, hOldPen);
    DeleteObject(hCenterPen);
}

bool Battle_Stage::HandleBattleSkillClick(int mouseX, int mouseY, Character* current)
{
    if (current == nullptr || !current->IsAlive())
    {
        return false;
    }

    RECT cancelRect = GetBattleCancelRect();

    if (IsPointInRect(cancelRect, mouseX, mouseY))
    {
        if (bSelectingBattleSkillTarget)
        {
            SelectedBattleSkillID = -1;
            bSelectingBattleSkillTarget = false;
        }
        else
        {
            ClearBattleSkillSelection();
            controlMode = EBattleControlMode::WAIT_COMMAND;
        }

        return true;
    }

    if (!bSelectingBattleSkillTarget)
    {
        RefreshBattleSkillList(current);

        int visibleCount = (int)BattleSkillIDs.size();

        if (visibleCount > 6)
        {
            visibleCount = 6;
        }

        CharacterStat stat = current->GetStat();

        for (int i = 0; i < visibleCount; ++i)
        {
            RECT rc = GetBattleSkillRect(i);

            if (!IsPointInRect(rc, mouseX, mouseY))
            {
                continue;
            }

            int skillID = BattleSkillIDs[i];
            const SSkillData* info = FindBattleSkillInfo(skillID);

            if (info == nullptr)
            {
                return true;
            }

            if (stat.CurMP < info->MPCost)
            {
                AddDamageText(current, 0, RGB(120, 180, 255));
                return true;
            }

            SelectedBattleSkillID = skillID;
            bSelectingBattleSkillTarget = true;
            return true;
        }

        return true;
    }

    int offsetX = GetOffsetX();
    int offsetY = GetOffsetY();

    int worldX = mouseX - offsetX;
    int worldY = mouseY - offsetY;

    if (worldX < 0 || worldY < 0)
    {
        return true;
    }

    int gridX = worldX / TILE_SIZE;
    int gridY = worldY / TILE_SIZE;

    if (!IsInsideGrid(gridX, gridY))
    {
        return true;
    }

    Character* target = GetEnemyAt(gridX, gridY);

    if (target == nullptr)
    {
        return true;
    }

    if (!IsBattleSkillTargetValid(current, SelectedBattleSkillID, gridX, gridY))
    {
        return true;
    }

    UseSelectedBattleSkill(current, target);
    return true;
}

bool Battle_Stage::UseSelectedBattleSkill(Character* user, Character* target)
{
    if (user == nullptr || target == nullptr)
    {
        return false;
    }

    const SSkillData* info = FindBattleSkillInfo(SelectedBattleSkillID);

    if (info == nullptr)
    {
        return false;
    }

    CharacterStat userStat = user->GetStat();

    if (userStat.CurMP < info->MPCost)
    {
        AddDamageText(user, 0, RGB(120, 180, 255));
        return false;
    }

    userStat.CurMP -= info->MPCost;
    user->SetStat(userStat);

    Location userPos = user->GetGridPos();
    Location targetPos = target->GetGridPos();

    user->LookAtGrid(targetPos.x, targetPos.y);
    user->SetState(ECharacterState::Skill);

    std::vector<Character*> hitTargets;

    ESkillAreaType areaType = info->AreaType;

    if (areaType == ESkillAreaType::Single)
    {
        hitTargets.push_back(target);
    }
    else if (areaType == ESkillAreaType::Splash1 || areaType == ESkillAreaType::Splash2)
    {
        int radius = (areaType == ESkillAreaType::Splash2) ? 2 : 1;

        for (Character* enemy : enemyTeam)
        {
            if (enemy == nullptr || !enemy->IsAlive())
            {
                continue;
            }

            Location pos = enemy->GetGridPos();
            int dist = std::abs(pos.x - targetPos.x) + std::abs(pos.y - targetPos.y);

            if (dist <= radius)
            {
                hitTargets.push_back(enemy);
            }
        }
    }
    else if (areaType == ESkillAreaType::Line)
    {
        for (Character* enemy : enemyTeam)
        {
            if (enemy == nullptr || !enemy->IsAlive())
            {
                continue;
            }

            Location pos = enemy->GetGridPos();

            if (userPos.x == targetPos.x && pos.x == userPos.x)
            {
                int dir = (targetPos.y > userPos.y) ? 1 : -1;
                int rel = pos.y - userPos.y;

                if (rel * dir > 0 && std::abs(rel) <= info->Range)
                {
                    hitTargets.push_back(enemy);
                }
            }
            else if (userPos.y == targetPos.y && pos.y == userPos.y)
            {
                int dir = (targetPos.x > userPos.x) ? 1 : -1;
                int rel = pos.x - userPos.x;

                if (rel * dir > 0 && std::abs(rel) <= info->Range)
                {
                    hitTargets.push_back(enemy);
                }
            }
        }
    }

    if (hitTargets.empty())
    {
        hitTargets.push_back(target);
    }

    for (Character* hit : hitTargets)
    {
        if (hit == nullptr || !hit->IsAlive())
        {
            continue;
        }

        int damage = CalcBattleSkillSingleDamage(user, hit, SelectedBattleSkillID);

        if (hit != target)
        {
            if (info->AreaType == ESkillAreaType::Splash1)
            {
                damage = ClampBattleDamage(damage * 65 / 100);
            }
            else if (info->AreaType == ESkillAreaType::Splash2)
            {
                damage = ClampBattleDamage(damage * 75 / 100);
            }
            else if (info->AreaType == ESkillAreaType::Line)
            {
                damage = ClampBattleDamage(damage * 85 / 100);
            }
        }

        hit->GetDamage(damage);
        AddDamageText(hit, damage, info->Color);
    }

    if (turnSystem)
    {
        turnSystem->RemoveDeadUnits();
    }

    user->SetHasActed(true);
    ActingUnit = user;

    ClearBattleSkillSelection();
    ClearMovePath();
    controlMode = EBattleControlMode::WAIT_COMMAND;

    if (actionMenu)
    {
        actionMenu->SetVisible(false);
    }

    CheckBattleEnd();
    return true;
}

void Battle_Stage::RefreshBattleConsumableItems()
{
    BattleConsumableItemIDs.clear();

    const std::vector<InventoryItem>& inventory = GameManager::Get().GetInventory();

    for (const InventoryItem& item : inventory)
    {
        if (item.Count <= 0)
        {
            continue;
        }

        if (!ItemData::IsConsumable(item.ItemID))
        {
            continue;
        }

        BattleConsumableItemIDs.push_back(item.ItemID);
    }
}

void Battle_Stage::ClearBattleItemSelection()
{
    SelectedBattleItemID = -1;
    bSelectingBattleItemTarget = false;
    BattleConsumableItemIDs.clear();
}

RECT Battle_Stage::GetBattleItemRect(int index) const
{
    RECT rc;
    int panelLeft = rects.right - 370;
    int panelTop = 100;
    int itemTop = panelTop + 90 + index * 58;

    rc.left = panelLeft + 20;
    rc.top = itemTop;
    rc.right = panelLeft + 330;
    rc.bottom = itemTop + 48;

    return rc;
}

RECT Battle_Stage::GetBattleTargetRect(int index) const
{
    RECT rc;
    int panelLeft = rects.right - 370;
    int panelTop = 100;
    int targetTop = panelTop + 150 + index * 58;

    rc.left = panelLeft + 20;
    rc.top = targetTop;
    rc.right = panelLeft + 330;
    rc.bottom = targetTop + 48;

    return rc;
}

RECT Battle_Stage::GetBattleCancelRect() const
{
    RECT rc;
    int panelLeft = rects.right - 370;
    int panelTop = 100;

    rc.left = panelLeft + 20;
    rc.top = panelTop + 430;
    rc.right = panelLeft + 330;
    rc.bottom = panelTop + 478;

    return rc;
}

bool Battle_Stage::IsPointInRect(const RECT& rc, int x, int y) const
{
    return x >= rc.left && x <= rc.right && y >= rc.top && y <= rc.bottom;
}

static void DrawBattleStageText(
    HDC hMemDC,
    const std::wstring& text,
    const RECT& rc,
    int fontSize,
    int fontWeight,
    COLORREF color,
    UINT format
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

    SetTextColor(hMemDC, color);
    DrawTextW(hMemDC, text.c_str(), (int)text.length(), (RECT*)&rc, format);

    SelectObject(hMemDC, hOldFont);
    DeleteObject(hFont);

    SetBkMode(hMemDC, oldBkMode);
}

static void DrawBattleStageButton(HDC hMemDC, const RECT& rc, const std::wstring& text, COLORREF border, COLORREF textColor)
{
    HBRUSH hBrush = CreateSolidBrush(RGB(38, 44, 58));
    FillRect(hMemDC, &rc, hBrush);
    DeleteObject(hBrush);

    HPEN hPen = CreatePen(PS_SOLID, 2, border);
    HPEN hOldPen = (HPEN)SelectObject(hMemDC, hPen);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hMemDC, GetStockObject(NULL_BRUSH));

    Rectangle(hMemDC, rc.left, rc.top, rc.right, rc.bottom);

    SelectObject(hMemDC, hOldBrush);
    SelectObject(hMemDC, hOldPen);
    DeleteObject(hPen);

    DrawBattleStageText(
        hMemDC,
        text,
        rc,
        18,
        FW_BOLD,
        textColor,
        DT_CENTER | DT_VCENTER | DT_SINGLELINE
    );
}

void Battle_Stage::DrawBattleItemUI(HDC hMemDC)
{
    if (bSelectingBattleItemTarget)
    {
        DrawBattleItemTargetSelect(hMemDC);
    }
    else
    {
        DrawBattleItemList(hMemDC);
    }
}

void Battle_Stage::DrawBattleItemList(HDC hMemDC)
{
    RECT panel;
    panel.left = rects.right - 390;
    panel.top = 80;
    panel.right = rects.right - 25;
    panel.bottom = 610;

    HBRUSH hBrush = CreateSolidBrush(RGB(19, 23, 31));
    FillRect(hMemDC, &panel, hBrush);
    DeleteObject(hBrush);

    HPEN hPen = CreatePen(PS_SOLID, 3, RGB(176, 146, 78));
    HPEN hOldPen = (HPEN)SelectObject(hMemDC, hPen);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hMemDC, GetStockObject(NULL_BRUSH));
    Rectangle(hMemDC, panel.left, panel.top, panel.right, panel.bottom);
    SelectObject(hMemDC, hOldBrush);
    SelectObject(hMemDC, hOldPen);
    DeleteObject(hPen);

    RECT titleRect = panel;
    titleRect.top += 18;
    titleRect.bottom = titleRect.top + 40;

    DrawBattleStageText(
        hMemDC,
        L"아이템",
        titleRect,
        28,
        FW_HEAVY,
        RGB(245, 220, 150),
        DT_CENTER | DT_VCENTER | DT_SINGLELINE
    );

    RECT guideRect = panel;
    guideRect.left += 20;
    guideRect.right -= 20;
    guideRect.top += 60;
    guideRect.bottom = guideRect.top + 26;

    DrawBattleStageText(
        hMemDC,
        L"사용할 아이템을 선택하세요.",
        guideRect,
        16,
        FW_NORMAL,
        RGB(220, 220, 220),
        DT_CENTER | DT_VCENTER | DT_SINGLELINE
    );

    RefreshBattleConsumableItems();

    if (BattleConsumableItemIDs.empty())
    {
        RECT emptyRect = panel;
        emptyRect.left += 25;
        emptyRect.right -= 25;
        emptyRect.top += 150;
        emptyRect.bottom = emptyRect.top + 70;

        DrawBattleStageText(
            hMemDC,
            L"사용 가능한 아이템이 없습니다.",
            emptyRect,
            22,
            FW_BOLD,
            RGB(230, 230, 230),
            DT_CENTER | DT_VCENTER | DT_SINGLELINE
        );
    }
    else
    {
        int visibleCount = (int)BattleConsumableItemIDs.size();

        if (visibleCount > 6)
        {
            visibleCount = 6;
        }

        for (int i = 0; i < visibleCount; ++i)
        {
            int itemID = BattleConsumableItemIDs[i];
            int count = GameManager::Get().GetItemCount(itemID);

            std::wstring itemText = ItemData::GetName(itemID) + L" x" + std::to_wstring(count);

            int hp = ItemData::GetHealHP(itemID);
            int mp = ItemData::GetHealMP(itemID);

            if (hp > 0)
            {
                itemText += L"  HP+" + std::to_wstring(hp);
            }

            if (mp > 0)
            {
                itemText += L"  MP+" + std::to_wstring(mp);
            }

            DrawBattleStageButton(
                hMemDC,
                GetBattleItemRect(i),
                itemText,
                RGB(100, 180, 255),
                RGB(240, 240, 240)
            );
        }
    }

    DrawBattleStageButton(
        hMemDC,
        GetBattleCancelRect(),
        L"취소",
        RGB(170, 170, 170),
        RGB(230, 230, 230)
    );
}

void Battle_Stage::DrawBattleItemTargetSelect(HDC hMemDC)
{
    RECT panel;
    panel.left = rects.right - 390;
    panel.top = 80;
    panel.right = rects.right - 25;
    panel.bottom = 610;

    HBRUSH hBrush = CreateSolidBrush(RGB(19, 23, 31));
    FillRect(hMemDC, &panel, hBrush);
    DeleteObject(hBrush);

    HPEN hPen = CreatePen(PS_SOLID, 3, RGB(176, 146, 78));
    HPEN hOldPen = (HPEN)SelectObject(hMemDC, hPen);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hMemDC, GetStockObject(NULL_BRUSH));
    Rectangle(hMemDC, panel.left, panel.top, panel.right, panel.bottom);
    SelectObject(hMemDC, hOldBrush);
    SelectObject(hMemDC, hOldPen);
    DeleteObject(hPen);

    RECT titleRect = panel;
    titleRect.top += 18;
    titleRect.bottom = titleRect.top + 40;

    DrawBattleStageText(
        hMemDC,
        L"대상 선택",
        titleRect,
        28,
        FW_HEAVY,
        RGB(245, 220, 150),
        DT_CENTER | DT_VCENTER | DT_SINGLELINE
    );

    RECT itemRect = panel;
    itemRect.left += 20;
    itemRect.right -= 20;
    itemRect.top += 68;
    itemRect.bottom = itemRect.top + 58;

    std::wstring itemText = ItemData::GetName(SelectedBattleItemID);

    int hp = ItemData::GetHealHP(SelectedBattleItemID);
    int mp = ItemData::GetHealMP(SelectedBattleItemID);

    if (hp > 0)
    {
        itemText += L"  HP+" + std::to_wstring(hp);
    }

    if (mp > 0)
    {
        itemText += L"  MP+" + std::to_wstring(mp);
    }

    DrawBattleStageText(
        hMemDC,
        itemText,
        itemRect,
        18,
        FW_BOLD,
        RGB(240, 240, 240),
        DT_CENTER | DT_VCENTER | DT_WORDBREAK
    );

    int targetIndex = 0;

    for (Character* c : playerTeam)
    {
        if (c == nullptr || !c->IsAlive())
        {
            continue;
        }

        CharacterStat stat = c->GetStat();
        std::wstring targetText = c->GetName()
            + L"  HP " + std::to_wstring(stat.CurHP) + L"/" + std::to_wstring(stat.MaxHP)
            + L"  MP " + std::to_wstring(stat.CurMP) + L"/" + std::to_wstring(stat.MaxMP);

        DrawBattleStageButton(
            hMemDC,
            GetBattleTargetRect(targetIndex),
            targetText,
            RGB(120, 230, 140),
            RGB(240, 240, 240)
        );

        targetIndex++;
    }

    DrawBattleStageButton(
        hMemDC,
        GetBattleCancelRect(),
        L"Back",
        RGB(170, 170, 170),
        RGB(230, 230, 230)
    );
}

bool Battle_Stage::HandleBattleItemClick(int mouseX, int mouseY, Character* current)
{
    if (current == nullptr || !current->IsAlive())
    {
        return false;
    }

    RECT cancelRect = GetBattleCancelRect();

    if (IsPointInRect(cancelRect, mouseX, mouseY))
    {
        if (bSelectingBattleItemTarget)
        {
            SelectedBattleItemID = -1;
            bSelectingBattleItemTarget = false;
        }
        else
        {
            ClearBattleItemSelection();
            controlMode = EBattleControlMode::WAIT_COMMAND;
        }

        return true;
    }

    if (!bSelectingBattleItemTarget)
    {
        RefreshBattleConsumableItems();

        int visibleCount = (int)BattleConsumableItemIDs.size();

        if (visibleCount > 6)
        {
            visibleCount = 6;
        }

        for (int i = 0; i < visibleCount; ++i)
        {
            RECT rc = GetBattleItemRect(i);

            if (!IsPointInRect(rc, mouseX, mouseY))
            {
                continue;
            }

            SelectedBattleItemID = BattleConsumableItemIDs[i];
            bSelectingBattleItemTarget = true;
            return true;
        }

        return true;
    }

    int targetIndex = 0;

    for (Character* target : playerTeam)
    {
        if (target == nullptr || !target->IsAlive())
        {
            continue;
        }

        RECT rc = GetBattleTargetRect(targetIndex);

        if (IsPointInRect(rc, mouseX, mouseY))
        {
            UseSelectedBattleItem(current, target);
            return true;
        }

        targetIndex++;
    }

    return true;
}

bool Battle_Stage::UseSelectedBattleItem(Character* user, Character* target)
{
    if (user == nullptr || target == nullptr)
    {
        return false;
    }

    if (SelectedBattleItemID < 0)
    {
        return false;
    }

    int healHP = ItemData::GetHealHP(SelectedBattleItemID);
    int healMP = ItemData::GetHealMP(SelectedBattleItemID);

    if (!GameManager::Get().UseConsumable(target, SelectedBattleItemID))
    {
        RefreshBattleConsumableItems();
        return false;
    }

    if (healHP > 0)
    {
        AddDamageText(target, -healHP, RGB(80, 255, 120));
    }
    else if (healMP > 0)
    {
        AddDamageText(target, -healMP, RGB(80, 180, 255));
    }

    user->SetHasActed(true);

    ClearBattleItemSelection();
    ClearMovePath();
    controlMode = EBattleControlMode::WAIT_COMMAND;

    if (actionMenu)
    {
        actionMenu->SetVisible(false);
    }

    return true;
}

bool Battle_Stage::BuildMovePath(Character* unit, int targetX, int targetY, std::vector<Location>& outPath) const
{
    outPath.clear();

    if (unit == nullptr || !unit->IsAlive())
    {
        return false;
    }

    if (!IsInsideGrid(targetX, targetY))
    {
        return false;
    }

    Location start = unit->GetGridPos();

    if (start.x == targetX && start.y == targetY)
    {
        return false;
    }

    if (!unit->IsInMoveRange(targetX, targetY))
    {
        return false;
    }

    if (IsOccupied(targetX, targetY, unit))
    {
        return false;
    }

    if (IsBlockedTile(targetX, targetY, unit))
    {
        return false;
    }

    std::vector<std::vector<int>> dist;
    std::vector<std::vector<Location>> parent;

    dist.resize(H);
    parent.resize(H);

    for (int y = 0; y < H; ++y)
    {
        dist[y].resize(W, -1);
        parent[y].resize(W);

        for (int x = 0; x < W; ++x)
        {
            parent[y][x].x = -1;
            parent[y][x].y = -1;
        }
    }

    std::queue<Location> q;

    dist[start.y][start.x] = 0;
    q.push(start);

    const int dx[4] = { 1, -1, 0, 0 };
    const int dy[4] = { 0, 0, 1, -1 };

    while (!q.empty())
    {
        Location cur = q.front();
        q.pop();

        if (cur.x == targetX && cur.y == targetY)
        {
            break;
        }

        for (int i = 0; i < 4; ++i)
        {
            int nx = cur.x + dx[i];
            int ny = cur.y + dy[i];

            if (!IsInsideGrid(nx, ny))
            {
                continue;
            }

            if (dist[ny][nx] != -1)
            {
                continue;
            }

            if (IsOccupied(nx, ny, unit))
            {
                continue;
            }

            if (IsBlockedTile(nx, ny, unit))
            {
                continue;
            }

            int nextDist = dist[cur.y][cur.x] + 1;

            if (nextDist > unit->GetAP())
            {
                continue;
            }

            dist[ny][nx] = nextDist;
            parent[ny][nx] = cur;

            Location next;
            next.x = nx;
            next.y = ny;

            q.push(next);
        }
    }

    if (dist[targetY][targetX] == -1)
    {
        return false;
    }

    std::vector<Location> reversed;

    Location cur;
    cur.x = targetX;
    cur.y = targetY;

    while (!(cur.x == start.x && cur.y == start.y))
    {
        reversed.push_back(cur);

        Location p = parent[cur.y][cur.x];

        if (p.x == -1 || p.y == -1)
        {
            outPath.clear();
            return false;
        }

        cur = p;
    }

    std::reverse(reversed.begin(), reversed.end());

    outPath = reversed;

    return !outPath.empty();
}

bool Battle_Stage::StartActiveMovePath(Character* unit, const std::vector<Location>& path)
{
    if (unit == nullptr || path.empty())
    {
        return false;
    }

    for (const auto& step : path)
    {
        if (!IsInsideGrid(step.x, step.y))
        {
            return false;
        }

        if (IsOccupied(step.x, step.y, unit))
        {
            return false;
        }

        if (IsBlockedTile(step.x, step.y, unit))
        {
            return false;
        }
    }

    ActiveMovePath = path;
    ActiveMoveIndex = 0;

    Location first = ActiveMovePath[ActiveMoveIndex];

    if (!unit->StartMoveToGrid(first.x, first.y, TILE_SIZE))
    {
        ClearMovePath();
        return false;
    }

    MovingUnit = unit;

    PreviewMovePath.clear();
    HoverGridX = -1;
    HoverGridY = -1;

    return true;
}

void Battle_Stage::ClearMovePath()
{
    PreviewMovePath.clear();
    ActiveMovePath.clear();
    ActiveMoveIndex = 0;

    HoverGridX = -1;
    HoverGridY = -1;
}

void Battle_Stage::UpdatePreviewMovePathByMouse(int mouseX, int mouseY)
{
    PreviewMovePath.clear();

    if (controlMode != EBattleControlMode::SELECT_MOVE)
    {
        HoverGridX = -1;
        HoverGridY = -1;
        return;
    }

    if (turnSystem == nullptr || turnSystem->IsCurrentTurnEnemy())
    {
        HoverGridX = -1;
        HoverGridY = -1;
        return;
    }

    Character* current = turnSystem->GetCurrentTurnCharacter();

    if (current == nullptr || !current->IsAlive())
    {
        HoverGridX = -1;
        HoverGridY = -1;
        return;
    }

    int offsetX = GetOffsetX();
    int offsetY = GetOffsetY();

    int worldX = mouseX - offsetX;
    int worldY = mouseY - offsetY;

    if (worldX < 0 || worldY < 0)
    {
        HoverGridX = -1;
        HoverGridY = -1;
        return;
    }

    int gridX = worldX / TILE_SIZE;
    int gridY = worldY / TILE_SIZE;

    if (!IsInsideGrid(gridX, gridY))
    {
        HoverGridX = -1;
        HoverGridY = -1;
        return;
    }

    HoverGridX = gridX;
    HoverGridY = gridY;

    BuildMovePath(current, gridX, gridY, PreviewMovePath);
}

void Battle_Stage::DrawMoveArrow(HDC hMemDC)
{
    std::vector<POINT> points;

    int offsetX = GetOffsetX();
    int offsetY = GetOffsetY();

    if (MovingUnit != nullptr && !ActiveMovePath.empty())
    {
        Location movingWorld = MovingUnit->GetWorldPixelPos(TILE_SIZE);

        POINT start;
        start.x = offsetX + movingWorld.x + TILE_SIZE / 2;
        start.y = offsetY + movingWorld.y + TILE_SIZE / 2;

        points.push_back(start);

        for (int i = ActiveMoveIndex; i < (int)ActiveMovePath.size(); ++i)
        {
            POINT p;
            p.x = offsetX + ActiveMovePath[i].x * TILE_SIZE + TILE_SIZE / 2;
            p.y = offsetY + ActiveMovePath[i].y * TILE_SIZE + TILE_SIZE / 2;

            points.push_back(p);
        }

        COLORREF arrowColor = RGB(255, 230, 40);

        if (!ActiveMovePath.empty())
        {
            const Location& end = ActiveMovePath.back();

            if (IsDamageTile(end.x, end.y))
            {
                arrowColor = RGB(255, 90, 50);
            }
        }

        DrawArrowPolyline(hMemDC, points, arrowColor, 5);
        return;
    }

    if (controlMode == EBattleControlMode::SELECT_MOVE && !PreviewMovePath.empty())
    {
        if (turnSystem == nullptr)
        {
            return;
        }

        Character* current = turnSystem->GetCurrentTurnCharacter();

        if (current == nullptr)
        {
            return;
        }

        Location startGrid = current->GetGridPos();

        POINT start;
        start.x = offsetX + startGrid.x * TILE_SIZE + TILE_SIZE / 2;
        start.y = offsetY + startGrid.y * TILE_SIZE + TILE_SIZE / 2;

        points.push_back(start);

        for (const auto& step : PreviewMovePath)
        {
            POINT p;
            p.x = offsetX + step.x * TILE_SIZE + TILE_SIZE / 2;
            p.y = offsetY + step.y * TILE_SIZE + TILE_SIZE / 2;

            points.push_back(p);
        }

        COLORREF arrowColor = RGB(255, 230, 40);

        if (!PreviewMovePath.empty())
        {
            const Location& end = PreviewMovePath.back();

            if (IsDamageTile(end.x, end.y))
            {
                arrowColor = RGB(255, 90, 50);
            }
        }

        DrawArrowPolyline(hMemDC, points, arrowColor, 5);
    }
}

void Battle_Stage::DrawArrowPolyline(HDC hMemDC, const std::vector<POINT>& points, COLORREF color, int width)
{
    if (points.size() < 2)
    {
        return;
    }

    HPEN hPen = CreatePen(PS_SOLID, width, color);
    HPEN hOldPen = (HPEN)SelectObject(hMemDC, hPen);

    MoveToEx(hMemDC, points[0].x, points[0].y, NULL);

    for (size_t i = 1; i < points.size(); ++i)
    {
        LineTo(hMemDC, points[i].x, points[i].y);
    }

    SelectObject(hMemDC, hOldPen);
    DeleteObject(hPen);

    POINT tip = points[points.size() - 1];
    POINT prev = points[points.size() - 2];

    double dx = (double)(tip.x - prev.x);
    double dy = (double)(tip.y - prev.y);
    double len = std::sqrt(dx * dx + dy * dy);

    if (len <= 0.01)
    {
        return;
    }

    double ux = dx / len;
    double uy = dy / len;

    double px = -uy;
    double py = ux;

    const int arrowLen = 18;
    const int arrowWidth = 10;

    POINT tri[3];

    tri[0] = tip;

    tri[1].x = (LONG)(tip.x - ux * arrowLen + px * arrowWidth);
    tri[1].y = (LONG)(tip.y - uy * arrowLen + py * arrowWidth);

    tri[2].x = (LONG)(tip.x - ux * arrowLen - px * arrowWidth);
    tri[2].y = (LONG)(tip.y - uy * arrowLen - py * arrowWidth);

    HBRUSH hBrush = CreateSolidBrush(color);
    HPEN hTriPen = CreatePen(PS_SOLID, 1, color);

    HPEN hOldTriPen = (HPEN)SelectObject(hMemDC, hTriPen);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hMemDC, hBrush);

    Polygon(hMemDC, tri, 3);

    SelectObject(hMemDC, hOldBrush);
    SelectObject(hMemDC, hOldTriPen);

    DeleteObject(hBrush);
    DeleteObject(hTriPen);
}

void Battle_Stage::DrawAttackRange(HDC hMemDC)
{
    if (turnSystem == nullptr)
    {
        return;
    }

    Character* current = turnSystem->GetCurrentTurnCharacter();

    if (current == nullptr)
    {
        return;
    }

    int offsetX = GetOffsetX();
    int offsetY = GetOffsetY();

    HPEN hRangePen = CreatePen(PS_SOLID, 2, RGB(255, 80, 80));
    HPEN hOldPen = (HPEN)SelectObject(hMemDC, hRangePen);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hMemDC, GetStockObject(NULL_BRUSH));

    for (int y = 0; y < H; ++y)
    {
        for (int x = 0; x < W; ++x)
        {
            if (current->IsInAttackRangeByGrid(x, y))
            {
                int sx = offsetX + x * TILE_SIZE;
                int sy = offsetY + y * TILE_SIZE;

                Rectangle(
                    hMemDC,
                    sx + 4,
                    sy + 4,
                    sx + TILE_SIZE - 4,
                    sy + TILE_SIZE - 4
                );
            }
        }
    }

    SelectObject(hMemDC, hOldBrush);
    SelectObject(hMemDC, hOldPen);
    DeleteObject(hRangePen);
}

void Battle_Stage::DrawMoveRange(HDC hMemDC)
{
    if (turnSystem == nullptr)
    {
        return;
    }

    Character* current = turnSystem->GetCurrentTurnCharacter();

    if (current == nullptr || !current->IsAlive())
    {
        return;
    }

    int offsetX = GetOffsetX();
    int offsetY = GetOffsetY();

    HPEN hRangePen = CreatePen(PS_SOLID, 2, RGB(80, 180, 255));
    HPEN hOldPen = (HPEN)SelectObject(hMemDC, hRangePen);

    HBRUSH hOldBrush = (HBRUSH)SelectObject(hMemDC, GetStockObject(NULL_BRUSH));

    for (int y = 0; y < H; ++y)
    {
        for (int x = 0; x < W; ++x)
        {
            if (!current->IsInMoveRange(x, y))
            {
                continue;
            }

            if (IsOccupied(x, y, current))
            {
                continue;
            }

            if (IsBlockedTile(x, y, current))
            {
                continue;
            }

            std::vector<Location> tempPath;

            if (!BuildMovePath(current, x, y, tempPath))
            {
                continue;
            }

            int sx = offsetX + x * TILE_SIZE;
            int sy = offsetY + y * TILE_SIZE;

            Rectangle(
                hMemDC,
                sx + 4,
                sy + 4,
                sx + TILE_SIZE - 4,
                sy + TILE_SIZE - 4
            );
        }
    }

    SelectObject(hMemDC, hOldBrush);
    SelectObject(hMemDC, hOldPen);
    DeleteObject(hRangePen);
}

void Battle_Stage::DrawBlockedMoveRange(HDC hMemDC)
{
    if (turnSystem == nullptr)
    {
        return;
    }

    Character* current = turnSystem->GetCurrentTurnCharacter();

    if (current == nullptr || !current->IsAlive())
    {
        return;
    }

    int offsetX = GetOffsetX();
    int offsetY = GetOffsetY();

    Location curPos = current->GetGridPos();

    HBRUSH hOldBrush = (HBRUSH)SelectObject(hMemDC, GetStockObject(NULL_BRUSH));

    for (int y = 0; y < H; ++y)
    {
        for (int x = 0; x < W; ++x)
        {
            int dist = std::abs(x - curPos.x) + std::abs(y - curPos.y);

            if (dist > current->GetAP())
            {
                continue;
            }

            bool blocked = IsBlockedTile(x, y, current);
            bool damageTile = IsDamageTile(x, y);

            ETileType type = GetTileType(x, y);

            if (type == ETileType::Lava && current->GetElement() == EElement::Fire)
            {
                damageTile = false;
            }

            if (!blocked && !damageTile)
            {
                continue;
            }

            COLORREF color = blocked ? RGB(255, 60, 60) : GetTileOverlayColor(type);

            HPEN hPen = CreatePen(PS_SOLID, 3, color);
            HPEN hOldPen = (HPEN)SelectObject(hMemDC, hPen);

            int sx = offsetX + x * TILE_SIZE;
            int sy = offsetY + y * TILE_SIZE;

            Rectangle(
                hMemDC,
                sx + 6,
                sy + 6,
                sx + TILE_SIZE - 6,
                sy + TILE_SIZE - 6
            );

            if (blocked)
            {
                MoveToEx(hMemDC, sx + 12, sy + 12, NULL);
                LineTo(hMemDC, sx + TILE_SIZE - 12, sy + TILE_SIZE - 12);

                MoveToEx(hMemDC, sx + TILE_SIZE - 12, sy + 12, NULL);
                LineTo(hMemDC, sx + 12, sy + TILE_SIZE - 12);
            }
            else if (damageTile)
            {
                // 이동은 가능하지만 피해가 들어가는 타일은 X 대신 마름모로 표시한다.
                MoveToEx(hMemDC, sx + TILE_SIZE / 2, sy + 10, NULL);
                LineTo(hMemDC, sx + TILE_SIZE - 10, sy + TILE_SIZE / 2);
                LineTo(hMemDC, sx + TILE_SIZE / 2, sy + TILE_SIZE - 10);
                LineTo(hMemDC, sx + 10, sy + TILE_SIZE / 2);
                LineTo(hMemDC, sx + TILE_SIZE / 2, sy + 10);
            }

            SelectObject(hMemDC, hOldPen);
            DeleteObject(hPen);
        }
    }

    SelectObject(hMemDC, hOldBrush);
}

void Battle_Stage::Comp_NextStage(int ID)
{
}

void Battle_Stage::BackgroundMove(int x, int y)
{
    const int CAMERA_SPEED = 32;

    camPosX += x * CAMERA_SPEED;
    camPosY += y * CAMERA_SPEED;

    FocusCameraOnWorldPos((int)camPosX, (int)camPosY);
}

void Battle_Stage::FocusCameraOnGrid(int gx, int gy)
{
    int focusX = gx * TILE_SIZE + TILE_SIZE / 2;
    int focusY = gy * TILE_SIZE + TILE_SIZE / 2;

    FocusCameraOnWorldPos(focusX, focusY);
}

void Battle_Stage::FocusCameraOnWorldPos(int wx, int wy)
{
    int halfW = rects.right / 2;
    int halfH = rects.bottom / 2;

    int minX = halfW;
    int minY = halfH;

    int maxX = BackgroundInfo.bmWidth - halfW;
    int maxY = BackgroundInfo.bmHeight - halfH;

    if (maxX < minX)
    {
        maxX = minX;
    }

    if (maxY < minY)
    {
        maxY = minY;
    }

    float targetX = (float)wx;
    float targetY = (float)wy;

    if (targetX < minX)
    {
        targetX = (float)minX;
    }

    if (targetX > maxX)
    {
        targetX = (float)maxX;
    }

    if (targetY < minY)
    {
        targetY = (float)minY;
    }

    if (targetY > maxY)
    {
        targetY = (float)maxY;
    }

    camPosX += (targetX - camPosX) * 0.1f;
    camPosY += (targetY - camPosY) * 0.1f;

    Loc.first.x = halfW;
    Loc.first.y = halfH;

    Loc.second.sX = (int)camPosX;
    Loc.second.sY = (int)camPosY;
}

int Battle_Stage::GetOffsetX() const
{
    return Loc.first.x - Loc.second.sX;
}

int Battle_Stage::GetOffsetY() const
{
    return Loc.first.y - Loc.second.sY;
}

bool Battle_Stage::IsInsideGrid(int gx, int gy) const
{
    return gx >= 0 && gy >= 0 && gx < W && gy < H;
}

bool Battle_Stage::IsOccupied(int gx, int gy, Character* except) const
{
    for (auto c : playerTeam)
    {
        if (c == nullptr)
        {
            continue;
        }

        if (c == except)
        {
            continue;
        }

        Location p = c->GetGridPos();

        if (p.x == gx && p.y == gy)
        {
            return true;
        }
    }

    for (auto c : enemyTeam)
    {
        if (c == nullptr)
        {
            continue;
        }

        if (c == except)
        {
            continue;
        }

        Location p = c->GetGridPos();

        if (p.x == gx && p.y == gy)
        {
            return true;
        }
    }

    return false;
}

Character* Battle_Stage::GetEnemyAt(int gx, int gy) const
{
    for (auto c : enemyTeam)
    {
        if (c == nullptr || !c->IsAlive())
        {
            continue;
        }

        Location p = c->GetGridPos();

        if (p.x == gx && p.y == gy)
        {
            return c;
        }
    }

    return nullptr;
}

Character* Battle_Stage::GetPlayerAt(int gx, int gy) const
{
    for (auto c : playerTeam)
    {
        if (c == nullptr || !c->IsAlive())
        {
            continue;
        }

        Location p = c->GetGridPos();

        if (p.x == gx && p.y == gy)
        {
            return c;
        }
    }

    return nullptr;
}

Character* Battle_Stage::FindNearestAlivePlayer(Character* enemy) const
{
    if (enemy == nullptr)
    {
        return nullptr;
    }

    Location ep = enemy->GetGridPos();

    Character* bestTarget = nullptr;
    int bestDist = 999999;

    for (auto p : playerTeam)
    {
        if (p == nullptr || !p->IsAlive())
        {
            continue;
        }

        Location pp = p->GetGridPos();

        int dist = std::abs(pp.x - ep.x) + std::abs(pp.y - ep.y);

        if (dist < bestDist)
        {
            bestDist = dist;
            bestTarget = p;
        }
    }

    return bestTarget;
}

int Battle_Stage::GetAttackRangeForUnit(Character* unit) const
{
    if (unit == nullptr)
    {
        return 1;
    }

    switch (unit->GetClassType())
    {
    case ECharacterClass::Warrior:
        return 1;

    case ECharacterClass::Archer:
        return 3;

    case ECharacterClass::Mage:
        return 2;

    default:
        return 1;
    }
}

bool Battle_Stage::CanAttackFromTile(Character* unit, int fromX, int fromY, Character* target) const
{
    if (unit == nullptr || target == nullptr || !target->IsAlive())
    {
        return false;
    }

    Location tp = target->GetGridPos();
    int dist = std::abs(tp.x - fromX) + std::abs(tp.y - fromY);

    return dist <= GetAttackRangeForUnit(unit);
}

int Battle_Stage::GetMoveTileDangerPenalty(int gx, int gy) const
{
    if (!IsDamageTile(gx, gy))
    {
        return 0;
    }

    int damage = GetTileDamage(gx, gy);

    if (damage <= 0)
    {
        return 0;
    }

    return damage * 30;
}

bool Battle_Stage::FindBestMoveTileToward(Character* unit, Character* target, int& outX, int& outY) const
{
    if (unit == nullptr || target == nullptr)
    {
        return false;
    }

    Location up = unit->GetGridPos();
    Location tp = target->GetGridPos();

    bool found = false;
    int bestScore = 999999;
    int bestDist = 999999;
    int bestDanger = 999999;

    outX = up.x;
    outY = up.y;

    for (int y = 0; y < H; ++y)
    {
        for (int x = 0; x < W; ++x)
        {
            if (x == up.x && y == up.y)
            {
                continue;
            }

            if (!unit->IsInMoveRange(x, y))
            {
                continue;
            }

            if (IsOccupied(x, y, unit))
            {
                continue;
            }

            if (IsBlockedTile(x, y, unit))
            {
                continue;
            }

            std::vector<Location> tempPath;

            if (!BuildMovePath(unit, x, y, tempPath))
            {
                continue;
            }

            int distToTarget = std::abs(tp.x - x) + std::abs(tp.y - y);
            int pathCost = (int)tempPath.size();
            int dangerPenalty = GetMoveTileDangerPenalty(x, y);
            int attackReadyBonus = CanAttackFromTile(unit, x, y, target) ? -500 : 0;

            int score = distToTarget * 100 + pathCost * 3 + dangerPenalty + attackReadyBonus;

            if (!found ||
                score < bestScore ||
                (score == bestScore && dangerPenalty < bestDanger) ||
                (score == bestScore && dangerPenalty == bestDanger && distToTarget < bestDist))
            {
                bestScore = score;
                bestDist = distToTarget;
                bestDanger = dangerPenalty;
                outX = x;
                outY = y;
                found = true;
            }
        }
    }

    return found;
}

void Battle_Stage::ProcessEnemyTurn()
{
    if (turnSystem == nullptr)
    {
        return;
    }

    if (BattleEnded)
    {
        return;
    }

    Character* enemy = turnSystem->GetCurrentTurnCharacter();

    if (enemy == nullptr || !enemy->IsAlive())
    {
        turnSystem->NextTurn();
        ResetTurnActions();
        return;
    }

    Character* target = FindNearestAlivePlayer(enemy);

    if (target == nullptr)
    {
        turnSystem->NextTurn();
        ResetTurnActions();
        return;
    }

    Location targetPos = target->GetGridPos();

    enemy->LookAtGrid(targetPos.x, targetPos.y);

    if (enemy->IsInAttackRange(*target))
    {
        int damage = enemy->CalcAttackDamage(*target);

        enemy->SetState(ECharacterState::Attack);
        target->GetDamage(damage);
        AddDamageText(target, damage, RGB(255, 160, 40));

        if (turnSystem)
        {
            turnSystem->RemoveDeadUnits();
        }

        enemy->SetHasActed(true);
        ActingUnit = enemy;

        CheckBattleEnd();

        return;
    }

    int moveX = 0;
    int moveY = 0;

    if (FindBestMoveTileToward(enemy, target, moveX, moveY))
    {
        std::vector<Location> enemyPath;

        if (BuildMovePath(enemy, moveX, moveY, enemyPath))
        {
            if (StartActiveMovePath(enemy, enemyPath))
            {
                enemy->SetHasMoved(true);
                return;
            }
        }
    }

    turnSystem->NextTurn();
    ResetTurnActions();
}

void Battle_Stage::DrawBackground(HDC hMemDC, HDC hMemDC1)
{
    if (hBitBackground == NULL)
    {
        return;
    }

    int offsetX = GetOffsetX();
    int offsetY = GetOffsetY();

    HBITMAP hOld = (HBITMAP)SelectObject(hMemDC1, hBitBackground);

    BitBlt(
        hMemDC,
        offsetX,
        offsetY,
        BackgroundInfo.bmWidth,
        BackgroundInfo.bmHeight,
        hMemDC1,
        0,
        0,
        SRCCOPY
    );

    SelectObject(hMemDC1, hOld);
}

void Battle_Stage::DrawGrid(HDC hMemDC)
{
    int offsetX = GetOffsetX();
    int offsetY = GetOffsetY();

    HPEN hPen = CreatePen(PS_SOLID, 1, RGB(230, 230, 230));
    HPEN hOldPen = (HPEN)SelectObject(hMemDC, hPen);

    for (int x = 0; x <= W; ++x)
    {
        int sx = offsetX + x * TILE_SIZE;

        MoveToEx(hMemDC, sx, offsetY, NULL);
        LineTo(hMemDC, sx, offsetY + H * TILE_SIZE);
    }

    for (int y = 0; y <= H; ++y)
    {
        int sy = offsetY + y * TILE_SIZE;

        MoveToEx(hMemDC, offsetX, sy, NULL);
        LineTo(hMemDC, offsetX + W * TILE_SIZE, sy);
    }

    SelectObject(hMemDC, hOldPen);
    DeleteObject(hPen);
}

void Battle_Stage::DrawUnit(HDC hMemDC, HDC hMemDC1, Character* unit, bool isEnemy)
{
    if (unit == nullptr)
    {
        return;
    }

    Location worldPixel = unit->GetWorldPixelPos(TILE_SIZE);

    int offsetX = GetOffsetX();
    int offsetY = GetOffsetY();

    int tileScreenX = offsetX + worldPixel.x;
    int tileScreenY = offsetY + worldPixel.y;

    int drawW = TILE_SIZE;
    int drawH = TILE_SIZE + 16;

    int drawX = tileScreenX + (TILE_SIZE - drawW) / 2;
    int drawY = tileScreenY + TILE_SIZE - drawH;

    unit->SetPixelPos(drawX, drawY);
    unit->Draw(hMemDC, hMemDC1, drawW, drawH);

    HPEN hPen = CreatePen(PS_SOLID, 2, isEnemy ? RGB(255, 80, 80) : RGB(80, 120, 255));
    HPEN hOldPen = (HPEN)SelectObject(hMemDC, hPen);

    HBRUSH hOldBrush = (HBRUSH)SelectObject(hMemDC, GetStockObject(NULL_BRUSH));

    Rectangle(
        hMemDC,
        tileScreenX + 6,
        tileScreenY + 6,
        tileScreenX + TILE_SIZE - 6,
        tileScreenY + TILE_SIZE - 6
    );

    SelectObject(hMemDC, hOldBrush);
    SelectObject(hMemDC, hOldPen);
    DeleteObject(hPen);
}