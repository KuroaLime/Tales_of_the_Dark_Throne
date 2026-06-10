#include "Battle_Stage.h"
#include "resource.h"
#include "TeamManager.h"

#include <cstdlib>
#include <algorithm>
#include <functional>

Battle_Stage::Battle_Stage()
    : turnSystem(nullptr),
    turnUI(nullptr),
    actionMenu(nullptr),
    controlMode(EBattleControlMode::WAIT_COMMAND),
    TILE_SIZE(64),
    W(0),
    H(0),
    MovingUnit(nullptr),
    ActingUnit(nullptr)
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

    hBitBackground = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP3));
    GetObject(hBitBackground, sizeof(BITMAP), &BackgroundInfo);

    TILE_SIZE = 64;
    W = BackgroundInfo.bmWidth / TILE_SIZE;
    H = BackgroundInfo.bmHeight / TILE_SIZE;

    Loc.first.x = rects.right / 2;
    Loc.first.y = rects.bottom / 2;
    Loc.second.sX = BackgroundInfo.bmWidth / 2;
    Loc.second.sY = BackgroundInfo.bmHeight / 2;

    int centerX = W / 2;
    int centerY = H / 2;

    playerTeam = TeamManager::Get().GetPlayers();

    if (playerTeam.empty())
    {
        Character* p1 = new Character();
        p1->Init(ECharacterClass::Warrior, centerX - 4, centerY);
        p1->SetName(L"Warrior");
        p1->LoadSprites(L"Resource");
        TeamManager::Get().AddPlayer(p1);

        Character* p2 = new Character();
        p2->Init(ECharacterClass::Archer, centerX - 4, centerY + 2);
        p2->SetName(L"Archer");
        p2->LoadSprites(L"Resource");
        TeamManager::Get().AddPlayer(p2);

        Character* p3 = new Character();
        p3->Init(ECharacterClass::Mage, centerX - 4, centerY - 2);
        p3->SetName(L"Mage");
        p3->LoadSprites(L"Resource");
        TeamManager::Get().AddPlayer(p3);

        playerTeam = TeamManager::Get().GetPlayers();
    }

    if (playerTeam.size() >= 1) playerTeam[0]->SetGridPos(centerX - 4, centerY);
    if (playerTeam.size() >= 2) playerTeam[1]->SetGridPos(centerX - 4, centerY + 2);
    if (playerTeam.size() >= 3) playerTeam[2]->SetGridPos(centerX - 4, centerY - 2);

    for (auto c : playerTeam)
    {
        if (c) {
            c->LoadSprites(L"Resource");
            c->SyncWorldPixelToGrid(TILE_SIZE);
        }
    }

    Character* e1 = new Character();
    e1->Init(ECharacterClass::Warrior, centerX + 4, centerY);
    e1->SetName(L"Enemy Warrior");
    e1->LoadSprites(L"Resource");
    {
        CharacterStat st = e1->GetStat();
        st.SPD = 10;
        e1->SetStat(st);
    }
    e1->SyncWorldPixelToGrid(TILE_SIZE);
    enemyTeam.push_back(e1);

    Character* e2 = new Character();
    e2->Init(ECharacterClass::Mage, centerX + 4, centerY + 2);
    e2->SetName(L"Enemy Mage");
    e2->LoadSprites(L"Resource");
    {
        CharacterStat st = e2->GetStat();
        st.SPD = 20;
        e2->SetStat(st);
    }
    e2->SyncWorldPixelToGrid(TILE_SIZE);
    enemyTeam.push_back(e2);

    Character* e3 = new Character();
    e3->Init(ECharacterClass::Archer, centerX + 5, centerY - 1);
    e3->SetName(L"Enemy Archer");
    e3->LoadSprites(L"Resource");
    {
        CharacterStat st = e3->GetStat();
        st.SPD = 18;
        e3->SetStat(st);
    }
    e3->SyncWorldPixelToGrid(TILE_SIZE);
    enemyTeam.push_back(e3);

    turnSystem = new STurnTimeline();
    turnSystem->Init(playerTeam, enemyTeam);

    turnUI = new UTurnOrderPanel();
    turnUI->Setup(rects);
    turnUI->Bind(turnSystem);

    actionMenu = new UBattleActionMenu();
    actionMenu->Init();
    actionMenu->SetOnMoveClick(std::bind(&Battle_Stage::OnMenuMove, this));
    actionMenu->SetOnAttackClick(std::bind(&Battle_Stage::OnMenuAttack, this));
    actionMenu->SetOnSkillClick(std::bind(&Battle_Stage::OnMenuSkill, this));
    actionMenu->SetOnDefendClick(std::bind(&Battle_Stage::OnMenuDefend, this));
    actionMenu->SetOnItemClick(std::bind(&Battle_Stage::OnMenuItem, this));
    actionMenu->SetOnEndTurnClick(std::bind(&Battle_Stage::OnMenuEndTurn, this));

    ResetTurnActions();

    FocusCameraOnGrid(centerX, centerY);
    camPosX = (float)Loc.second.sX;
    camPosY = (float)Loc.second.sY;
}

void Battle_Stage::Update(HWND hWnd)
{
    for (auto c : playerTeam) if (c) c->Update();
    for (auto c : enemyTeam) if (c) c->Update();

    Character* focusTarget = nullptr;
    if (MovingUnit != nullptr) focusTarget = MovingUnit;
    else if (ActingUnit != nullptr) focusTarget = ActingUnit;
    else if (turnSystem != nullptr) focusTarget = turnSystem->GetCurrentTurnCharacter();

    if (focusTarget != nullptr)
    {
        Location p = focusTarget->GetWorldPixelPos(TILE_SIZE);
        FocusCameraOnWorldPos(p.x + TILE_SIZE / 2, p.y + TILE_SIZE / 2);
    }

    if (MovingUnit != nullptr)
    {
        if (!MovingUnit->GetIsMoving())
        {
            bool isEnemyTurn = turnSystem->IsCurrentTurnEnemy();
            MovingUnit = nullptr;
            
            if (isEnemyTurn)
            {
                // 적은 이동 후 바로 공격 시도 로직이 ProcessEnemyTurn에 있으므로
                // 여기서는 일단 WAIT_COMMAND로 돌려보내서 다시 ProcessEnemyTurn이 돌게 하거나
                // 혹은 이동만 하고 끝낼지 결정해야 함.
                // 현재 ProcessEnemyTurn 구조상 이동 후 바로 종료하는 것이 안전함.
                turnSystem->NextTurn();
                ResetTurnActions();
            }

            controlMode = EBattleControlMode::WAIT_COMMAND;
        }
        if (turnUI) turnUI->Update();
        return;
    }

    if (ActingUnit != nullptr)
    {
        if (ActingUnit->GetIsActionEnd())
        {
            if (ActingUnit->IsAlive()) ActingUnit->SetState(ECharacterState::Idle);
            ActingUnit->SetIsActionEnd(false);

            bool isEnemyTurn = turnSystem->IsCurrentTurnEnemy();
            ActingUnit = nullptr;

            for (auto c : playerTeam) if (c && c->IsAlive() && c->GetState() == ECharacterState::Hit) c->SetState(ECharacterState::Idle);
            for (auto c : enemyTeam) if (c && c->IsAlive() && c->GetState() == ECharacterState::Hit) c->SetState(ECharacterState::Idle);

            if (isEnemyTurn)
            {
                turnSystem->NextTurn();
                ResetTurnActions();
            }

            controlMode = EBattleControlMode::WAIT_COMMAND;
        }
        if (turnUI) turnUI->Update();
        return;
    }

    if (turnSystem != nullptr && turnSystem->IsCurrentTurnEnemy())
    {
        ProcessEnemyTurn();
        if (turnUI) turnUI->Update();
        return;
    }

    if (turnUI) turnUI->Update();

    if (turnSystem != nullptr && !turnSystem->IsCurrentTurnEnemy())
    {
        Character* current = turnSystem->GetCurrentTurnCharacter();
        if (current != nullptr && current->IsAlive())
        {
            if (controlMode == EBattleControlMode::WAIT_COMMAND)
            {
                actionMenu->SetVisible(true);
                actionMenu->UpdateButtonStates(current);

                Location worldPos = current->GetWorldPixelPos(TILE_SIZE);
                int offsetX = GetOffsetX();
                int offsetY = GetOffsetY();
                actionMenu->UpdatePosition(offsetX + worldPos.x, offsetY + worldPos.y);
            }
            else
            {
                actionMenu->SetVisible(false);
            }
        }
    }
    else
    {
        actionMenu->SetVisible(false);
    }

    if (actionMenu) actionMenu->Update();
}

void Battle_Stage::Draw(HDC hMemDC, HDC hMemDC1)
{
    DrawBackground(hMemDC, hMemDC1);
    DrawGrid(hMemDC);

    if (controlMode == EBattleControlMode::SELECT_MOVE) DrawMoveRange(hMemDC);
    else if (controlMode == EBattleControlMode::SELECT_ATTACK || controlMode == EBattleControlMode::SELECT_SKILL) DrawAttackRange(hMemDC);

    for (auto c : playerTeam) DrawUnit(hMemDC, hMemDC1, c, false);
    for (auto c : enemyTeam) DrawUnit(hMemDC, hMemDC1, c, true);

    if (turnUI) turnUI->Draw(hMemDC, hMemDC1);
    if (actionMenu) actionMenu->Draw(hMemDC, hMemDC1);
}

int Battle_Stage::Input(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_KEYDOWN)
    {
        if (wParam == VK_ESCAPE)
        {
            if (controlMode != EBattleControlMode::WAIT_COMMAND) {
                controlMode = EBattleControlMode::WAIT_COMMAND;
                return -2;
            }
            return MAP_STAGE;
        }

        if (wParam == VK_LEFT || wParam == 'A') { BackgroundMove(-1, 0); return -2; }
        if (wParam == VK_RIGHT || wParam == 'D') { BackgroundMove(1, 0); return -2; }
        if (wParam == VK_UP || wParam == 'W') { BackgroundMove(0, -1); return -2; }
        if (wParam == VK_DOWN || wParam == 'S') { BackgroundMove(0, 1); return -2; }
    }

    if (uMsg == WM_RBUTTONDOWN)
    {
        if (controlMode != EBattleControlMode::WAIT_COMMAND)
        {
            controlMode = EBattleControlMode::WAIT_COMMAND;
            return -2;
        }
    }

    if (uMsg == WM_LBUTTONDOWN)
    {
        if (turnSystem == nullptr || MovingUnit != nullptr || ActingUnit != nullptr || turnSystem->IsCurrentTurnEnemy()) return -2;

        Character* current = turnSystem->GetCurrentTurnCharacter();
        if (current == nullptr || !current->IsAlive()) return -2;

        int mouseX = LOWORD(lParam);
        int mouseY = HIWORD(lParam);

        if (controlMode == EBattleControlMode::WAIT_COMMAND && actionMenu->GetVisible())
        {
            UWidget* clicked = actionMenu->FindWidgetUnderMouse(mouseX, mouseY);
            if (clicked)
            {
                UButton* btn = dynamic_cast<UButton*>(clicked);
                if (btn && btn->GetEnabled()) {
                    btn->Click();
                    return -2;
                }
            }
        }

        int offsetX = GetOffsetX();
        int offsetY = GetOffsetY();
        int worldX = mouseX - offsetX;
        int worldY = mouseY - offsetY;
        if (worldX < 0 || worldY < 0) return -2;
        int gridX = worldX / TILE_SIZE;
        int gridY = worldY / TILE_SIZE;
        if (!IsInsideGrid(gridX, gridY)) return -2;

        if (controlMode == EBattleControlMode::SELECT_MOVE)
        {
            if (current->IsInMoveRange(gridX, gridY) && !IsOccupied(gridX, gridY, current))
            {
                if (current->StartMoveToGrid(gridX, gridY, TILE_SIZE))
                {
                    current->SetHasMoved(true);
                    MovingUnit = current;
                }
            }
        }
        else if (controlMode == EBattleControlMode::SELECT_ATTACK)
        {
            Character* target = GetEnemyAt(gridX, gridY);
            if (target && current->IsInAttackRange(*target))
            {
                current->LookAtGrid(gridX, gridY);
                int damage = current->CalcAttackDamage(*target);
                current->SetState(ECharacterState::Attack);
                target->GetDamage(damage);
                current->SetHasActed(true);
                ActingUnit = current;
            }
        }
        else if (controlMode == EBattleControlMode::SELECT_SKILL)
        {
            Character* target = GetEnemyAt(gridX, gridY);
            if (target && current->IsInAttackRange(*target))
            {
                current->LookAtGrid(gridX, gridY);
                int damage = current->CalcSkillDamage(*target);
                current->SetState(ECharacterState::Skill);
                target->GetDamage(damage);
                current->SetHasActed(true);
                ActingUnit = current;
            }
        }

        return -2;
    }

    return -2;
}

void Battle_Stage::OnMenuMove() { controlMode = EBattleControlMode::SELECT_MOVE; }
void Battle_Stage::OnMenuAttack() { controlMode = EBattleControlMode::SELECT_ATTACK; }
void Battle_Stage::OnMenuSkill() { controlMode = EBattleControlMode::SELECT_SKILL; }
void Battle_Stage::OnMenuDefend() 
{ 
    Character* current = turnSystem->GetCurrentTurnCharacter();
    if (current) {
        current->SetState(ECharacterState::Defend);
        current->SetHasActed(true);
        controlMode = EBattleControlMode::WAIT_COMMAND;
    }
}
void Battle_Stage::OnMenuItem() { controlMode = EBattleControlMode::SELECT_ITEM; }
void Battle_Stage::OnMenuEndTurn() 
{ 
    if (actionMenu) actionMenu->SetVisible(false);
    turnSystem->NextTurn(); 
    ResetTurnActions();
    controlMode = EBattleControlMode::WAIT_COMMAND;
}

void Battle_Stage::ResetTurnActions()
{
    Character* current = turnSystem->GetCurrentTurnCharacter();
    if (current) current->ResetTurnState();
}

void Battle_Stage::DrawAttackRange(HDC hMemDC)
{
    Character* current = turnSystem->GetCurrentTurnCharacter();
    if (current == nullptr) return;

    int offsetX = GetOffsetX();
    int offsetY = GetOffsetY();
    HPEN hRangePen = CreatePen(PS_SOLID, 2, RGB(255, 80, 80));
    HPEN hOldPen = (HPEN)SelectObject(hMemDC, hRangePen);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hMemDC, GetStockObject(NULL_BRUSH));

    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            if (current->IsInAttackRangeByGrid(x, y)) {
                int sx = offsetX + x * TILE_SIZE;
                int sy = offsetY + y * TILE_SIZE;
                Rectangle(hMemDC, sx + 4, sy + 4, sx + TILE_SIZE - 4, sy + TILE_SIZE - 4);
            }
        }
    }
    SelectObject(hMemDC, hOldBrush);
    SelectObject(hMemDC, hOldPen);
    DeleteObject(hRangePen);
}

void Battle_Stage::Comp_NextStage(int ID) {}
void Battle_Stage::BackgroundMove(int x, int y) { const int CAMERA_SPEED = 32; camPosX += x * CAMERA_SPEED; camPosY += y * CAMERA_SPEED; FocusCameraOnWorldPos((int)camPosX, (int)camPosY); }
void Battle_Stage::FocusCameraOnGrid(int gx, int gy) { int focusX = gx * TILE_SIZE + TILE_SIZE / 2; int focusY = gy * TILE_SIZE + TILE_SIZE / 2; FocusCameraOnWorldPos(focusX, focusY); }
void Battle_Stage::FocusCameraOnWorldPos(int wx, int wy) {
    int halfW = rects.right / 2; int halfH = rects.bottom / 2;
    int minX = halfW; int minY = halfH;
    int maxX = BackgroundInfo.bmWidth - halfW; int maxY = BackgroundInfo.bmHeight - halfH;
    if (maxX < minX) maxX = minX; if (maxY < minY) maxY = minY;
    float targetX = (float)wx; float targetY = (float)wy;
    if (targetX < minX) targetX = (float)minX; if (targetX > maxX) targetX = (float)maxX;
    if (targetY < minY) targetY = (float)minY; if (targetY > maxY) targetY = (float)maxY;
    camPosX += (targetX - camPosX) * 0.1f; camPosY += (targetY - camPosY) * 0.1f;
    Loc.first.x = halfW; Loc.first.y = halfH; Loc.second.sX = (int)camPosX; Loc.second.sY = (int)camPosY;
}
int Battle_Stage::GetOffsetX() const { return Loc.first.x - Loc.second.sX; }
int Battle_Stage::GetOffsetY() const { return Loc.first.y - Loc.second.sY; }
bool Battle_Stage::IsInsideGrid(int gx, int gy) const { return gx >= 0 && gy >= 0 && gx < W && gy < H; }
bool Battle_Stage::IsOccupied(int gx, int gy, Character* except) const {
    for (auto c : playerTeam) { if (c == nullptr || c == except || !c->IsAlive()) continue; Location p = c->GetGridPos(); if (p.x == gx && p.y == gy) return true; }
    for (auto c : enemyTeam) { if (c == nullptr || c == except || !c->IsAlive()) continue; Location p = c->GetGridPos(); if (p.x == gx && p.y == gy) return true; }
    return false;
}
Character* Battle_Stage::GetEnemyAt(int gx, int gy) const { for (auto c : enemyTeam) { if (c == nullptr || !c->IsAlive()) continue; Location p = c->GetGridPos(); if (p.x == gx && p.y == gy) return c; } return nullptr; }
Character* Battle_Stage::GetPlayerAt(int gx, int gy) const { for (auto c : playerTeam) { if (c == nullptr || !c->IsAlive()) continue; Location p = c->GetGridPos(); if (p.x == gx && p.y == gy) return c; } return nullptr; }
Character* Battle_Stage::FindNearestAlivePlayer(Character* enemy) const {
    if (enemy == nullptr) return nullptr;
    Location ep = enemy->GetGridPos(); Character* bestTarget = nullptr; int bestDist = 999999;
    for (auto p : playerTeam) { if (p == nullptr || !p->IsAlive()) continue; Location pp = p->GetGridPos(); int dist = std::abs(pp.x - ep.x) + std::abs(pp.y - ep.y); if (dist < bestDist) { bestDist = dist; bestTarget = p; } }
    return bestTarget;
}
bool Battle_Stage::FindBestMoveTileToward(Character* unit, Character* target, int& outX, int& outY) const {
    if (unit == nullptr || target == nullptr) return false;
    Location up = unit->GetGridPos(); Location tp = target->GetGridPos();
    bool found = false; int bestDist = 999999; outX = up.x; outY = up.y;
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            if (x == up.x && y == up.y) continue;
            if (!unit->IsInMoveRange(x, y)) continue;
            if (IsOccupied(x, y, unit)) continue;
            int distToTarget = std::abs(tp.x - x) + std::abs(tp.y - y);
            if (distToTarget < bestDist) { bestDist = distToTarget; outX = x; outY = y; found = true; }
        }
    }
    return found;
}
void Battle_Stage::ProcessEnemyTurn() {
    if (turnSystem == nullptr) return;
    Character* enemy = turnSystem->GetCurrentTurnCharacter();
    if (enemy == nullptr || !enemy->IsAlive()) { turnSystem->NextTurn(); return; }
    Character* target = FindNearestAlivePlayer(enemy);
    if (target == nullptr) { turnSystem->NextTurn(); return; }
    Location targetPos = target->GetGridPos();
    enemy->LookAtGrid(targetPos.x, targetPos.y);
    if (enemy->IsInAttackRange(*target)) {
        int damage = enemy->CalcAttackDamage(*target);
        enemy->SetState(ECharacterState::Attack);
        target->GetDamage(damage);
        enemy->SetHasActed(true);
        ActingUnit = enemy;
        return;
    }
    int moveX = 0; int moveY = 0;
    if (FindBestMoveTileToward(enemy, target, moveX, moveY)) {
        if (enemy->StartMoveToGrid(moveX, moveY, TILE_SIZE)) { MovingUnit = enemy; return; }
    }
    turnSystem->NextTurn();
}
void Battle_Stage::DrawBackground(HDC hMemDC, HDC hMemDC1) {
    if (hBitBackground == NULL) return;
    int offsetX = GetOffsetX(); int offsetY = GetOffsetY();
    HBITMAP hOld = (HBITMAP)SelectObject(hMemDC1, hBitBackground);
    BitBlt(hMemDC, offsetX, offsetY, BackgroundInfo.bmWidth, BackgroundInfo.bmHeight, hMemDC1, 0, 0, SRCCOPY);
    SelectObject(hMemDC1, hOld);
}
void Battle_Stage::DrawGrid(HDC hMemDC) {
    int offsetX = GetOffsetX(); int offsetY = GetOffsetY();
    HPEN hPen = CreatePen(PS_SOLID, 1, RGB(230, 230, 230)); HPEN hOldPen = (HPEN)SelectObject(hMemDC, hPen);
    for (int x = 0; x <= W; ++x) { int sx = offsetX + x * TILE_SIZE; MoveToEx(hMemDC, sx, offsetY, NULL); LineTo(hMemDC, sx, offsetY + H * TILE_SIZE); }
    for (int y = 0; y <= H; ++y) { int sy = offsetY + y * TILE_SIZE; MoveToEx(hMemDC, offsetX, sy, NULL); LineTo(hMemDC, offsetX + W * TILE_SIZE, sy); }
    SelectObject(hMemDC, hOldPen); DeleteObject(hPen);
}
void Battle_Stage::DrawMoveRange(HDC hMemDC) {
    Character* current = turnSystem->GetCurrentTurnCharacter();
    if (current == nullptr) return;
    int offsetX = GetOffsetX(); int offsetY = GetOffsetY();
    HPEN hRangePen = CreatePen(PS_SOLID, 2, RGB(80, 180, 255)); HPEN hOldPen = (HPEN)SelectObject(hMemDC, hRangePen);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hMemDC, GetStockObject(NULL_BRUSH));
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            if (!current->IsInMoveRange(x, y) || IsOccupied(x, y, current)) continue;
            int sx = offsetX + x * TILE_SIZE; int sy = offsetY + y * TILE_SIZE;
            Rectangle(hMemDC, sx + 4, sy + 4, sx + TILE_SIZE - 4, sy + TILE_SIZE - 4);
        }
    }
    SelectObject(hMemDC, hOldBrush); SelectObject(hMemDC, hOldPen); DeleteObject(hRangePen);
}
void Battle_Stage::DrawUnit(HDC hMemDC, HDC hMemDC1, Character* unit, bool isEnemy) {
    if (unit == nullptr) return;
    Location worldPixel = unit->GetWorldPixelPos(TILE_SIZE);
    int offsetX = GetOffsetX(); int offsetY = GetOffsetY();
    int tileScreenX = offsetX + worldPixel.x; int tileScreenY = offsetY + worldPixel.y;
    int drawW = TILE_SIZE; int drawH = TILE_SIZE + 16;
    int drawX = tileScreenX + (TILE_SIZE - drawW) / 2; int drawY = tileScreenY + TILE_SIZE - drawH;
    unit->SetPixelPos(drawX, drawY); unit->Draw(hMemDC, hMemDC1, drawW, drawH);
    HPEN hPen = CreatePen(PS_SOLID, 2, isEnemy ? RGB(255, 80, 80) : RGB(80, 120, 255));
    HPEN hOldPen = (HPEN)SelectObject(hMemDC, hPen); HBRUSH hOldBrush = (HBRUSH)SelectObject(hMemDC, GetStockObject(NULL_BRUSH));
    Rectangle(hMemDC, tileScreenX + 6, tileScreenY + 6, tileScreenX + TILE_SIZE - 6, tileScreenY + TILE_SIZE - 6);
    SelectObject(hMemDC, hOldBrush); SelectObject(hMemDC, hOldPen); DeleteObject(hPen);
}


