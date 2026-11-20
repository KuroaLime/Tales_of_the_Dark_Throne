#include "Battle_Stage.h"
#include "resource.h"
#include "SoundManager.h"
#include "GameManager.h"
#include "ItemData.h"
#include "UButton.h"
#include "BattleRewardManager.h"
#include "SkillData.h"
#include "GraveyardRenderer.h"
#include "CharacterMotion.h"
#include "AreaSkillEffectRenderer.h"
#include "UltimateSkillRenderer.h"
#include "StandardSkillRenderer.h"

#include <cstdlib>
#include <algorithm>
#include <functional>
#include <cmath>
#include <string>
#include <queue>
#include <vector>

static bool UsesGridAlignedBackground(const SBattleMapData& map)
{
    return map.gridAlignedBackground || !map.tileAtlasFilePath.empty();
}

static BattleActionTiming::ElementVisual ToElementVisual(EElement element)
{
    switch (element)
    {
    case EElement::Fire: return BattleActionTiming::ElementVisual::Fire;
    case EElement::Water: return BattleActionTiming::ElementVisual::Water;
    case EElement::Grass: return BattleActionTiming::ElementVisual::Grass;
    case EElement::Light: return BattleActionTiming::ElementVisual::Light;
    case EElement::Dark: return BattleActionTiming::ElementVisual::Dark;
    default: return BattleActionTiming::ElementVisual::Neutral;
    }
}

static BattleSkillArea::Kind ToAreaKind(ESkillAreaType area)
{
    switch (area)
    {
    case ESkillAreaType::Splash1: return BattleSkillArea::Kind::Splash1;
    case ESkillAreaType::Splash2: return BattleSkillArea::Kind::Splash2;
    case ESkillAreaType::Line: return BattleSkillArea::Kind::Line;
    default: return BattleSkillArea::Kind::Single;
    }
}

static HBITMAP LoadStageBitmap(const std::wstring& relativePath, HINSTANCE instance,
    int resourceId, int expectedWidth = 0, int expectedHeight = 0)
{
    std::vector<std::wstring> candidates{ relativePath,
        L"Tower_Triumph\\Tower_Triumph\\" + relativePath,
        L"Tower_Triumph\\" + relativePath,
        L"..\\Tower_Triumph\\" + relativePath };
    wchar_t executable[MAX_PATH]{};
    if (GetModuleFileNameW(nullptr, executable, MAX_PATH))
    {
        std::wstring directory(executable);
        for (int level = 0; level < 4; ++level)
        {
            const size_t slash = directory.find_last_of(L"\\/");
            if (slash == std::wstring::npos) break;
            directory.resize(slash);
            candidates.push_back(directory + L"\\" + relativePath);
            candidates.push_back(directory + L"\\Tower_Triumph\\" + relativePath);
        }
    }
    auto Accept = [&](HBITMAP bitmap)
    {
        if (!bitmap) return false;
        BITMAP info{};
        if (GetObject(bitmap, sizeof(info), &info) &&
            (expectedWidth == 0 || info.bmWidth == expectedWidth) &&
            (expectedHeight == 0 || info.bmHeight == expectedHeight)) return true;
        DeleteObject(bitmap);
        return false;
    };
    for (const auto& path : candidates)
    {
        if (relativePath.empty()) break;
        HBITMAP bitmap = static_cast<HBITMAP>(LoadImageW(nullptr, path.c_str(),
            IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION));
        if (Accept(bitmap)) return bitmap;
    }
    // Both scene and fallback atlas travel with the executable. Reject a scene
    // with mismatched dimensions instead of stretching it away from its grid.
    HBITMAP embedded = resourceId != 0 ?
        LoadBitmapW(instance, MAKEINTRESOURCEW(resourceId)) : nullptr;
    return Accept(embedded) ? embedded : nullptr;
}

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
    ClearPendingAction();
    ActiveAreaEffects.clear();
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

    ReleaseSkillEffectBitmaps();
    if (hBitBackground) DeleteObject(hBitBackground);
}

void Battle_Stage::Init(HWND hWnd, HINSTANCE const g_hInst)
{
    ClearPendingAction();
    ActiveAreaEffects.clear();
    DamageTexts.clear();
    GetClientRect(hWnd, &rects);

    SoundManager::PlayBGM(L"Resource\\Sound\\TowerTriumph_Fantasy_Battle_Loop_64s.wav", true);

    BattleEnded = false;
    BattleWon = false;
    ActiveSkillEffects.clear();

    BattleRewardManager::Get().Clear();

    int battleStageId = GameManager::Get().GetCurrentBattleStageId();
    EBattleMapId battleMapId = BattleMapData::FromStageId(battleStageId);

    LoadBattleMap(battleMapId);

    if (hBitBackground) DeleteObject(hBitBackground);
    hBitBackground = nullptr;

    if (CurrentMap.gridAlignedBackground)
    {
        hBitBackground = LoadStageBitmap(CurrentMap.backgroundFilePath, g_hInst,
            CurrentMap.backgroundResourceId, CurrentMap.width * CurrentMap.tileSize,
            CurrentMap.height * CurrentMap.tileSize);
    }

    if (hBitBackground == nullptr && !CurrentMap.tileAtlasFilePath.empty())
    {
        HBITMAP atlas = LoadStageBitmap(CurrentMap.tileAtlasFilePath, g_hInst, IDB_GRAVEYARD_ATLAS);
        hBitBackground = GraveyardRenderer::CreateBackground(CurrentMap, atlas);
        if (atlas) DeleteObject(atlas);
    }

    if (hBitBackground == nullptr && CurrentMap.gridAlignedBackground)
    {
        // Preserve the collision geometry even if both authored copies fail.
        // Maps without an atlas use distinct terrain colors at world resolution.
        hBitBackground = GraveyardRenderer::CreateBackground(CurrentMap, nullptr);
    }

    if (!UsesGridAlignedBackground(CurrentMap) && !CurrentMap.backgroundFilePath.empty())
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

    if (hBitBackground == nullptr && !UsesGridAlignedBackground(CurrentMap) && CurrentMap.backgroundResourceId != 0)
    {
        hBitBackground = LoadBitmap(g_hInst, MAKEINTRESOURCE(CurrentMap.backgroundResourceId));
    }

    if (hBitBackground == nullptr && !UsesGridAlignedBackground(CurrentMap))
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
    Loc.second.sX = W * TILE_SIZE / 2;
    Loc.second.sY = H * TILE_SIZE / 2;

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
        Location safeStart = FindNearestValidStartTile(c, start.x, start.y, true);

        c->SetGridPos(safeStart.x, safeStart.y);
        c->LoadSprites(L"Resource");
        c->SyncWorldPixelToGrid(TILE_SIZE);
        c->ResetBattleAnimation();
        c->ResetTurnState();
        c->SetDirection(EDirection::Right);
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

        if (start.element != EElement::None)
        {
            e->SetElement(start.element);
        }

        if (start.spd > 0)
        {
            CharacterStat stat = e->GetStat();
            stat.SPD = start.spd;
            e->SetStat(stat);
        }

        Location safeStart = FindNearestValidStartTile(e, start.x, start.y, true);
        e->SetGridPos(safeStart.x, safeStart.y);

        bool loadedMonsterSheet = false;

        if (!start.monsterSheetFile.empty())
        {
            std::vector<std::wstring> monsterPathCandidates;

            monsterPathCandidates.push_back(L"Resource\\Monster");

            monsterPathCandidates.push_back(L"Tower_Triumph\\Tower_Triumph\\Resource\\Monster");
            monsterPathCandidates.push_back(L"Tower_Triumph\\Resource\\Monster");
            monsterPathCandidates.push_back(L"..\\Tower_Triumph\\Tower_Triumph\\Resource\\Monster");
            monsterPathCandidates.push_back(L"..\\Tower_Triumph\\Resource\\Monster");

            for (const std::wstring& folderPath : monsterPathCandidates)
            {
                if (e->LoadMonsterSheet(folderPath, start.monsterSheetFile, start.monsterSourceFacesRight))
                {
                    loadedMonsterSheet = true;
                    break;
                }
            }

            if (!loadedMonsterSheet)
            {
                std::wstring debugText = L"[MonsterLoadFail] " + start.monsterSheetFile + L"\n";
                OutputDebugStringW(debugText.c_str());
            }
        }

        if (!loadedMonsterSheet)
        {
            e->LoadSprites(L"Resource");
        }

        e->SyncWorldPixelToGrid(TILE_SIZE);
        e->SetDirection(EDirection::Left);

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

    {
        int halfW = rects.right / 2;
        int halfH = rects.bottom / 2;

        int minX = halfW;
        int minY = halfH;
        int maxX = W * TILE_SIZE - halfW;
        int maxY = H * TILE_SIZE - halfH;

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
    LastCharacterUpdate = std::chrono::steady_clock::now();
}

Location Battle_Stage::FindNearestValidStartTile(Character* unit, int startX, int startY, bool avoidDamage) const
{
    Location fallback;
    fallback.x = startX;
    fallback.y = startY;

    if (unit == nullptr || W <= 0 || H <= 0)
    {
        return fallback;
    }

    if (IsInsideGrid(startX, startY) &&
        !IsBlockedTile(startX, startY, unit) &&
        (!avoidDamage || !IsDamageTile(startX, startY)))
    {
        return fallback;
    }

    std::vector<std::vector<bool>> visited;
    visited.resize(H);

    for (int y = 0; y < H; ++y)
    {
        visited[y].resize(W, false);
    }

    std::queue<Location> q;

    if (IsInsideGrid(startX, startY))
    {
        q.push({ startX, startY });
        visited[startY][startX] = true;
    }
    else
    {
        int clampedX = startX;
        int clampedY = startY;

        if (clampedX < 0) clampedX = 0;
        if (clampedX >= W) clampedX = W - 1;
        if (clampedY < 0) clampedY = 0;
        if (clampedY >= H) clampedY = H - 1;

        q.push({ clampedX, clampedY });
        visited[clampedY][clampedX] = true;
    }

    const int dx[4] = { 1, -1, 0, 0 };
    const int dy[4] = { 0, 0, 1, -1 };

    while (!q.empty())
    {
        Location cur = q.front();
        q.pop();

        if (!IsBlockedTile(cur.x, cur.y, unit) &&
            (!avoidDamage || !IsDamageTile(cur.x, cur.y)))
        {
            return cur;
        }

        for (int i = 0; i < 4; ++i)
        {
            int nx = cur.x + dx[i];
            int ny = cur.y + dy[i];

            if (!IsInsideGrid(nx, ny))
            {
                continue;
            }

            if (visited[ny][nx])
            {
                continue;
            }

            visited[ny][nx] = true;
            q.push({ nx, ny });
        }
    }

    return fallback;
}

void Battle_Stage::LoadBattleMap(EBattleMapId mapId)
{
    ClearPendingAction();
    ActiveAreaEffects.clear();
    CurrentMap = BattleMapData::Load(mapId);
    TileMap = CurrentMap.tiles;
}

bool Battle_Stage::ContainsBattleUnit(const Character* unit) const
{
    return unit && (std::find(playerTeam.begin(), playerTeam.end(), unit) != playerTeam.end() ||
        std::find(enemyTeam.begin(), enemyTeam.end(), unit) != enemyTeam.end());
}

void Battle_Stage::ClearPendingAction(bool preserveAreaEffects)
{
    PendingAction = {};
    ActingUnit = nullptr;
    if (!preserveAreaEffects)
    {
        ActiveAreaEffects.clear();
        ActiveUltimateEffects.clear();
        ActiveStandardEffects.clear();
    }
}

void Battle_Stage::BeginPendingAction(Character* actor, ECharacterState state, std::vector<PendingHit> hits,
    int skillID, Location effectGrid, ESkillAreaType areaType, std::vector<BattleSkillArea::Cell> areaCells)
{
    if (PendingAction.actor || ActingUnit || !ContainsBattleUnit(actor) || !actor->IsAlive() ||
        (hits.empty() && areaCells.empty())) return;
    actor->SetState(state, true);
    actor->SetHasActed(true);
    ActingUnit = actor;
    PendingAction.actor = actor;
    PendingAction.state = state;
    PendingAction.sequence = actor->GetActionSequence();
    PendingAction.timeline.clock.impactSeconds = actor->GetCurrentActionImpactSeconds();
    PendingAction.releaseSeconds = PendingAction.timeline.clock.impactSeconds;
    PendingAction.hits = std::move(hits);
    PendingAction.enemyTurn = std::find(enemyTeam.begin(), enemyTeam.end(), actor) != enemyTeam.end();
    PendingAction.skillID = skillID;
    PendingAction.element = state == ECharacterState::Skill ?
        ToElementVisual(SkillData::GetElement(skillID)) : BattleActionTiming::ElementVisual::Neutral;
    PendingAction.standard = state == ECharacterState::Skill ? StandardSkillPresentation::ProfileFor(skillID) :
        StandardSkillPresentation::Profile{};
    if (StandardSkillPresentation::Valid(PendingAction.standard))
    {
        // SkillData's captured element remains authoritative even if actor/UI state changes later.
        PendingAction.standard.element = PendingAction.element;
        PendingAction.presentationID = ++NextPresentationID;
    }
    PendingAction.effectGrid = effectGrid;
    PendingAction.areaType = areaType;
    PendingAction.casterGrid = actor->GetGridPos();
    const Location casterWorld = actor->GetWorldPixelPos(TILE_SIZE);
    PendingAction.casterFootX = casterWorld.x + TILE_SIZE * 0.5f;
    PendingAction.casterFootY = casterWorld.y + (UsesGridAlignedBackground(CurrentMap) ? TILE_SIZE * 0.75f : static_cast<float>(TILE_SIZE));
    PendingAction.facing = actor->GetDirection() == EDirection::Left ? -1.0f : 1.0f;
    if (areaType != ESkillAreaType::Single) PendingAction.areaCells = std::move(areaCells);
    PendingAction.projectile = actor->GetCurrentProjectileKind();
    const auto requestedPresentation = state == ECharacterState::Skill ?
        SkillPresentation::Requested(skillID) : SkillPresentation::Kind::Normal;
    if (requestedPresentation != SkillPresentation::Kind::Normal)
    {
        BITMAP info{};
        const bool available = LoadSkillEffectBitmap(SkillPresentation::BitmapID(requestedPresentation), &info) != nullptr;
        const bool flightAvailable = !SkillPresentation::IsSnipe(requestedPresentation) ||
            LoadSkillEffectBitmap(SkillPresentation::FlightBitmapID(requestedPresentation), &info) != nullptr;
        PendingAction.presentation = SkillPresentation::Resolve(skillID, available, flightAvailable);
        if (PendingAction.presentation != SkillPresentation::Kind::Normal)
        {
            PendingAction.projectile = BattleActionTiming::ProjectileKind::None;
            PendingAction.presentationID = ++NextPresentationID;
        }
    }
    if (PendingAction.projectile != BattleActionTiming::ProjectileKind::None ||
        SkillPresentation::IsSnipe(PendingAction.presentation) || SkillPresentation::IsRain(PendingAction.presentation))
    {
        const bool arrow = PendingAction.projectile == BattleActionTiming::ProjectileKind::Arrow ||
            SkillPresentation::IsSnipe(PendingAction.presentation) || SkillPresentation::IsRain(PendingAction.presentation);
        const float referenceScale = (TILE_SIZE + 16) / 64.0f;
        const float footHeight = UsesGridAlignedBackground(CurrentMap) ? TILE_SIZE * 0.75f : static_cast<float>(TILE_SIZE);
        const Location origin = actor->GetWorldPixelPos(TILE_SIZE);
        Location goal = effectGrid;
        // A piercing shot travels forward to the clipped range endpoint, never
        // to the selected enemy followed by a backwards area expansion.
        if (areaType == ESkillAreaType::Line && !PendingAction.areaCells.empty())
            goal = { PendingAction.areaCells.back().x, PendingAction.areaCells.back().y };
        if (skillID < 0 && ContainsBattleUnit(PendingAction.hits.front().target))
            goal = PendingAction.hits.front().target->GetGridPos();
        const float facing = actor->GetDirection() == EDirection::Left ? -1.0f : 1.0f;
        PendingAction.startX = origin.x + TILE_SIZE * 0.5f + facing * referenceScale *
            (arrow ? BattleActionTiming::ArrowLaunchX : BattleActionTiming::MagicLaunchX);
        PendingAction.startY = origin.y + footHeight + referenceScale *
            (arrow ? BattleActionTiming::ArrowLaunchY : BattleActionTiming::MagicLaunchY);
        PendingAction.targetX = goal.x * TILE_SIZE + TILE_SIZE * 0.5f;
        PendingAction.targetY = goal.y * TILE_SIZE + footHeight - referenceScale * BattleActionTiming::ProjectileTargetHeight;
        const float dx = PendingAction.targetX - PendingAction.startX;
        const float dy = PendingAction.targetY - PendingAction.startY;
        float pathLength = sqrtf(dx * dx + dy * dy);
        if (areaType == ESkillAreaType::Line && !PendingAction.areaCells.empty())
        {
            const auto& first = PendingAction.areaCells.front();
            PendingAction.linePath = BattleSkillArea::MakeLinePath(PendingAction.startX, PendingAction.startY,
                first.x * TILE_SIZE + TILE_SIZE * 0.5f,
                first.y * TILE_SIZE + footHeight - referenceScale * BattleActionTiming::ProjectileTargetHeight,
                PendingAction.targetX, PendingAction.targetY);
            pathLength = BattleSkillArea::TotalLength(PendingAction.linePath);
        }
        PendingAction.timeline.hasProjectile = true;
        PendingAction.timeline.flightSeconds = BattleActionTiming::FlightSeconds(PendingAction.projectile, pathLength);
    }
    if (PendingAction.presentation != SkillPresentation::Kind::Normal)
    {
        // Unique profiles use blade contact, focused-arrow travel or sky delivery;
        // each retains the common release-persistence and impact-resolution rules.
        PendingAction.timeline.hasProjectile = !SkillPresentation::IsKnight(PendingAction.presentation);
        const float dx = PendingAction.targetX - PendingAction.startX;
        const float dy = PendingAction.targetY - PendingAction.startY;
        PendingAction.timeline.flightSeconds = SkillPresentation::IsSnipe(PendingAction.presentation) ?
            SkillPresentation::SnipeFlightSeconds(sqrtf(dx * dx + dy * dy)) : SkillPresentation::DeliverySeconds(PendingAction.presentation);
    }
    const float release = PendingAction.timeline.clock.impactSeconds;
    const float flight = PendingAction.timeline.flightSeconds;
    for (std::size_t i = 0; i < PendingAction.areaCells.size(); ++i)
    {
        auto& cell = PendingAction.areaCells[i];
        if (areaType == ESkillAreaType::Line)
        {
            cell.arrivalSeconds = PendingAction.timeline.hasProjectile ?
                BattleSkillArea::LineArrival(release, flight, PendingAction.linePath, static_cast<int>(i),
                    static_cast<float>(TILE_SIZE)) :
                BattleSkillArea::MeleeLineArrival(release, static_cast<int>(i));
        }
        else
            cell.arrivalSeconds = BattleSkillArea::SplashArrival(release, flight,
                BattleSkillArea::Distance({ cell.x, cell.y }, { effectGrid.x, effectGrid.y }));
    }
    if (SkillPresentation::StartsOnCommand(PendingAction.presentation)) StartPendingPresentation();
    StartPendingStandardPresentation();
}

void Battle_Stage::StartPendingStandardPresentation()
{
    if (!StandardSkillPresentation::Valid(PendingAction.standard) || PendingAction.standardStarted) return;
    PendingAction.standardStarted = true;
    StandardSkillPresentation::Action effect;
    effect.profile = PendingAction.standard;
    effect.area = ToAreaKind(PendingAction.areaType);
    effect.cells = PendingAction.areaCells;
    effect.center = { PendingAction.effectGrid.x, PendingAction.effectGrid.y };
    effect.elapsed = PendingAction.elapsedSeconds;
    effect.release = PendingAction.releaseSeconds;
    effect.flight = PendingAction.timeline.flightSeconds;
    effect.arrival = effect.release + effect.flight;
    if (effect.cells.empty()) effect.cells.push_back({ effect.center.x, effect.center.y, effect.arrival });
    effect.startX = PendingAction.startX; effect.startY = PendingAction.startY;
    effect.targetX = PendingAction.targetX; effect.targetY = PendingAction.targetY;
    effect.linePath = PendingAction.linePath;
    effect.facing = PendingAction.facing;
    effect.id = PendingAction.presentationID;
    ActiveStandardEffects.push_back(std::move(effect));
}

void Battle_Stage::StartPendingPresentation()
{
    if (PendingAction.presentation == SkillPresentation::Kind::Normal || PendingAction.presentationStarted) return;
    PendingAction.presentationStarted = true;
    UltimateEffectAnim effect;
    effect.presentation = PendingAction.presentation;
    effect.element = PendingAction.element;
    effect.cells = PendingAction.areaCells;
    effect.center = PendingAction.effectGrid;
    effect.elapsedSeconds = PendingAction.elapsedSeconds;
    effect.releaseSeconds = PendingAction.releaseSeconds;
    effect.deliverySeconds = PendingAction.timeline.flightSeconds;
    effect.casterFootX = PendingAction.casterFootX;
    effect.casterFootY = PendingAction.casterFootY;
    effect.startX = PendingAction.startX; effect.startY = PendingAction.startY;
    effect.targetX = PendingAction.targetX; effect.targetY = PendingAction.targetY;
    effect.facing = PendingAction.facing;
    effect.presentationID = PendingAction.presentationID;
    ActiveUltimateEffects.push_back(std::move(effect));
}

void Battle_Stage::CancelPendingPresentation()
{
    ActiveStandardEffects.erase(std::remove_if(ActiveStandardEffects.begin(), ActiveStandardEffects.end(),
        [this](const StandardSkillPresentation::Action& effect) { return effect.id == PendingAction.presentationID; }), ActiveStandardEffects.end());
    ActiveUltimateEffects.erase(std::remove_if(ActiveUltimateEffects.begin(), ActiveUltimateEffects.end(),
        [this](const UltimateEffectAnim& effect) { return effect.presentationID == PendingAction.presentationID; }), ActiveUltimateEffects.end());
}

void Battle_Stage::CommitPendingImpact()
{
    Character* actor = PendingAction.actor;
    if (!PendingAction.timeline.launched && (!ContainsBattleUnit(actor) || !actor->IsAlive() ||
        actor->GetState() != PendingAction.state || actor->GetActionSequence() != PendingAction.sequence))
    {
        PendingAction.cancelled = true;
        CancelPendingPresentation();
        return;
    }
    // Area cells own their individual arrivals. Their floor wave must also
    // finish through empty cells and cells whose original target has died.
    if (!PendingAction.areaCells.empty()) return;
    const bool anyTarget = std::any_of(PendingAction.hits.begin(), PendingAction.hits.end(),
        [this](const PendingHit& hit) { return ContainsBattleUnit(hit.target) && hit.target->IsAlive(); });
    if (!anyTarget) return;
    if (PendingAction.skillID >= 0 && PendingAction.presentation == SkillPresentation::Kind::Normal &&
        !StandardSkillPresentation::Valid(PendingAction.standard))
        PlaySkillEffect(PendingAction.skillID, PendingAction.effectGrid.x, PendingAction.effectGrid.y, PendingAction.areaType);
    else if (PendingAction.skillID >= 0)
        SoundManager::PlaySFX(L"Resource\\Sound\\Skill_Impact.wav");
    for (auto& hit : PendingAction.hits)
    {
        CommitPendingHit(hit);
    }
    if (turnSystem) turnSystem->RemoveDeadUnits();
}

void Battle_Stage::CommitPendingHit(PendingHit& hit)
{
    if (hit.resolved) return;
    hit.resolved = true; // Latch before callbacks, including removed/dead targets.
    if (!ContainsBattleUnit(hit.target) || !hit.target->IsAlive()) return;
    const Location pos = hit.target->GetGridPos();
    if (hit.areaCellIndex >= 0)
    {
        const auto& cell = PendingAction.areaCells[static_cast<std::size_t>(hit.areaCellIndex)];
        if (pos.x != cell.x || pos.y != cell.y) return;
    }
    const int hpBefore = hit.target->GetHP();
    // Skills share one impact sound; per-target reactions must not replace it.
    hit.target->GetDamage(hit.damage, PendingAction.skillID < 0);
    hit.reacted = true;
    PlaySkillEffect(BattleActionTiming::ContactEffectID(PendingAction.element), pos.x, pos.y, ESkillAreaType::Single,
        !PendingAction.areaCells.empty() || PendingAction.presentation != SkillPresentation::Kind::Normal ||
        (StandardSkillPresentation::Valid(PendingAction.standard) && StandardSkillPresentation::Middle(PendingAction.standard)));
    AddDamageText(hit.target, hpBefore - hit.target->GetHP(), hit.color, !PendingAction.areaCells.empty());
}

void Battle_Stage::CommitPendingAreaCells()
{
    if (PendingAction.areaCells.empty() || PendingAction.cancelled ||
        (!PendingAction.timeline.launched && !PendingAction.timeline.impactCommitted)) return;
    while (PendingAction.nextAreaCell < PendingAction.areaCells.size() &&
        BattleSkillArea::IsDue(PendingAction.elapsedSeconds,
            PendingAction.areaCells[PendingAction.nextAreaCell].arrivalSeconds))
    {
        if (!PendingAction.fieldStarted)
        {
            PendingAction.fieldStarted = true;
            AreaEffectAnim effect;
            effect.cells = PendingAction.areaCells;
            effect.kind = ToAreaKind(PendingAction.areaType);
            effect.element = PendingAction.element;
            effect.presentation = PendingAction.presentation;
            effect.standard = StandardSkillPresentation::Valid(PendingAction.standard);
            effect.origin = PendingAction.areaType == ESkillAreaType::Line ? PendingAction.casterGrid : PendingAction.effectGrid;
            const SSkillData* skill = SkillData::Find(PendingAction.skillID);
            effect.color = skill ? skill->Color : RGB(255, 255, 255);
            effect.elapsedSeconds = PendingAction.elapsedSeconds;
            ActiveAreaEffects.push_back(std::move(effect));
            SoundManager::PlaySFX(L"Resource\\Sound\\Skill_Impact.wav");
        }
        const int cellIndex = static_cast<int>(PendingAction.nextAreaCell++);
        for (auto& hit : PendingAction.hits)
            if (hit.areaCellIndex == cellIndex) CommitPendingHit(hit);
    }
    if (turnSystem) turnSystem->RemoveDeadUnits();
}

float Battle_Stage::AdvanceBattleVisuals(float elapsedSeconds)
{
    if (!(elapsedSeconds > 0.0f)) return 0.0f;
    float remainingMovementSeconds = 0.0f;
    for (const auto* team : { &playerTeam, &enemyTeam })
    {
        for (auto c : *team)
        {
            if (!c) continue;
            const float remaining = c->Update(elapsedSeconds);
            if (c == MovingUnit) remainingMovementSeconds = remaining;
            // Each reaction owns its full duration; the attacker cannot cut it short.
            if (c->IsAlive() && c->GetState() == ECharacterState::Hit && c->GetIsActionEnd())
                c->SetState(ECharacterState::Idle);
        }
    }
    UpdateDamageTexts(elapsedSeconds);
    UpdateSkillEffects(elapsedSeconds);
    for (auto& effect : ActiveAreaEffects) effect.elapsedSeconds += elapsedSeconds;
    ActiveAreaEffects.erase(std::remove_if(ActiveAreaEffects.begin(), ActiveAreaEffects.end(),
        [](const AreaEffectAnim& effect) { return effect.cells.empty() || effect.elapsedSeconds >=
            effect.cells.back().arrivalSeconds + AreaSkillEffectRenderer::TailSeconds; }), ActiveAreaEffects.end());
    for (auto& effect : ActiveUltimateEffects) effect.elapsedSeconds += elapsedSeconds;
    ActiveUltimateEffects.erase(std::remove_if(ActiveUltimateEffects.begin(), ActiveUltimateEffects.end(),
        [](const UltimateEffectAnim& effect) { return effect.elapsedSeconds - effect.releaseSeconds >=
            SkillPresentation::Lifetime(effect.presentation, effect.deliverySeconds); }), ActiveUltimateEffects.end());
    for (auto& effect : ActiveStandardEffects) effect.elapsed += elapsedSeconds;
    ActiveStandardEffects.erase(std::remove_if(ActiveStandardEffects.begin(), ActiveStandardEffects.end(),
        [](const StandardSkillPresentation::Action& effect) { return effect.elapsed >= StandardSkillPresentation::EndSeconds(effect); }),
        ActiveStandardEffects.end());
    return remainingMovementSeconds;
}

float Battle_Stage::AdvancePendingAction(float elapsedSeconds)
{
    if (!PendingAction.actor) return AdvanceBattleVisuals(elapsedSeconds);
    if (!PendingAction.timeline.launched && !PendingAction.timeline.impactCommitted && !PendingAction.cancelled)
    {
        Character* actor = PendingAction.actor;
        if (!ContainsBattleUnit(actor) || !actor->IsAlive() || actor->GetState() != PendingAction.state ||
            actor->GetActionSequence() != PendingAction.sequence)
        {
            PendingAction.cancelled = true;
            CancelPendingPresentation();
        }
    }
    float remaining = elapsedSeconds;
    float remainingMovementSeconds = 0.0f;
    // Split at release, projectile end and every cell arrival. Equal-time rings
    // commit together, before any remaining frame time advances their reactions.
    while (remaining > 0.0f)
    {
        if (PendingAction.cancelled) return AdvanceBattleVisuals(remaining);
        CommitPendingAreaCells();
        const float nextArrival = PendingAction.nextAreaCell < PendingAction.areaCells.size() ?
            PendingAction.areaCells[PendingAction.nextAreaCell].arrivalSeconds : -1.0f;
        const auto step = BattleSkillArea::AdvanceTimeline(PendingAction.timeline,
            PendingAction.elapsedSeconds, remaining, nextArrival);
        remainingMovementSeconds = AdvanceBattleVisuals(step.beforeEvent);
        if (step.event == BattleActionTiming::ActionEvent::Launch) StartPendingPresentation();
        if (step.event == BattleActionTiming::ActionEvent::Impact) CommitPendingImpact();
        CommitPendingAreaCells();
        remaining = step.afterEvent;
        if (step.beforeEvent == 0.0f && step.event == BattleActionTiming::ActionEvent::None) break;
    }
    return remainingMovementSeconds;
}

bool Battle_Stage::UpdateActionResolution()
{
    Character* actor = PendingAction.actor;
    if (!actor) return false;
    const bool actorPresent = ContainsBattleUnit(actor);
    if (!actorPresent && !PendingAction.timeline.launched && !PendingAction.timeline.impactCommitted)
    {
        ClearPendingAction();
        controlMode = EBattleControlMode::WAIT_COMMAND;
        return true;
    }
    if (!actorPresent) PendingAction.actorFinished = true;
    if (actorPresent && !PendingAction.timeline.launched && !PendingAction.timeline.impactCommitted &&
        (actor->GetActionSequence() != PendingAction.sequence ||
        actor->GetState() != PendingAction.state || !actor->IsAlive()))
    {
        PendingAction.cancelled = true;
        CancelPendingPresentation();
    }

    if (actorPresent && !PendingAction.actorFinished)
    {
        const ECharacterState state = actor->GetState();
        if (actor->GetActionSequence() != PendingAction.sequence &&
            state != ECharacterState::Hit && state != ECharacterState::Dead)
            PendingAction.actorFinished = true;
        else if (actor->GetIsActionEnd())
        {
            PendingAction.actorFinished = true;
            if (actor->IsAlive()) actor->SetState(ECharacterState::Idle);
        }
        else if (state != PendingAction.state && state != ECharacterState::Hit && state != ECharacterState::Dead)
            PendingAction.actorFinished = true;
    }
    if ((!PendingAction.timeline.impactCommitted && !PendingAction.cancelled) || !PendingAction.actorFinished) return false;
    if (!PendingAction.cancelled && PendingAction.nextAreaCell < PendingAction.areaCells.size()) return false;
    for (const auto& hit : PendingAction.hits)
    {
        if (!hit.reacted || !ContainsBattleUnit(hit.target)) continue;
        const auto state = hit.target->GetState();
        if ((state == ECharacterState::Hit || state == ECharacterState::Dead) && !hit.target->GetIsActionEnd()) return false;
    }

    const bool enemyTurn = PendingAction.enemyTurn;
    const bool actorStillCurrent = turnSystem && turnSystem->GetCurrentTurnCharacter() == actor;
    ClearPendingAction(true);
    CheckBattleEnd();
    if (!BattleEnded && enemyTurn && turnSystem)
    {
        if (actorStillCurrent) turnSystem->NextTurn();
        ResetTurnActions();
    }
    controlMode = EBattleControlMode::WAIT_COMMAND;
    return true;
}

void Battle_Stage::Update(HWND hWnd)
{
    const auto now = std::chrono::steady_clock::now();
    const float elapsedSeconds = CharacterMotion::ClampDelta(
        std::chrono::duration<float>(now - LastCharacterUpdate).count());
    LastCharacterUpdate = now;
    float remainingMovementSeconds = AdvancePendingAction(elapsedSeconds);
    UpdateActionResolution();

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
        else if (ContainsBattleUnit(ActingUnit))
        {
            focusTarget = ActingUnit;
        }
        else if (turnSystem != nullptr)
        {
            focusTarget = turnSystem->GetCurrentTurnCharacter();
        }

        if (ContainsBattleUnit(focusTarget))
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

            // Carry time across cardinal tile boundaries. The index increases
            // on each pass, so even a delayed update is bounded by path length.
            while (MovingUnit->IsAlive() && ActiveMoveIndex + 1 < (int)ActiveMovePath.size())
            {
                ++ActiveMoveIndex;
                Location next = ActiveMovePath[ActiveMoveIndex];
                const bool hasAnotherStep = ActiveMoveIndex + 1 < (int)ActiveMovePath.size();
                if (!MovingUnit->StartMoveToGrid(next.x, next.y, TILE_SIZE, hasAnotherStep)) break;
                remainingMovementSeconds = MovingUnit->AdvanceMovement(remainingMovementSeconds);
                if (MovingUnit->GetIsMoving())
                {
                    if (turnUI)
                    {
                        turnUI->Update();
                    }

                    return;
                }
            }

            // An aborted continuation also stops its walk cycle. Dead/hit
            // states remain intact, and destination hazards still apply once.
            if (MovingUnit->IsAlive() && MovingUnit->GetState() == ECharacterState::Walk)
                MovingUnit->SetState(ECharacterState::Idle);
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
        if (turnUI) turnUI->Update();
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
    DrawAreaSkillEffects(hMemDC, hMemDC1);
    DrawUltimateSkillEffects(hMemDC, hMemDC1);
    DrawStandardSkillEffects(hMemDC, hMemDC1);

    for (auto c : playerTeam)
    {
        DrawUnit(hMemDC, hMemDC1, c, false);
    }

    for (auto c : enemyTeam)
    {
        DrawUnit(hMemDC, hMemDC1, c, true);
    }

    DrawPendingProjectile(hMemDC, hMemDC1);
    DrawUltimateSkillEffects(hMemDC, hMemDC1, true);
    DrawStandardSkillEffects(hMemDC, hMemDC1, true);
    DrawAreaSkillEffects(hMemDC, hMemDC1, true);
    DrawSkillEffects(hMemDC, hMemDC1);
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

            ClearPendingAction();
            ActiveAreaEffects.clear();
            return MAP_STAGE;
        }

        if (wParam == VK_SPACE)
        {
            OnMenuEndTurn();
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

                BeginPendingAction(current, ECharacterState::Attack,
                    { { target, damage, RGB(255, 70, 70), false } });
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
    SoundManager::PlaySFX(L"Resource\\Sound\\UI_Click.wav");
    ClearMovePath();
    ClearBattleSkillSelection();
    controlMode = EBattleControlMode::SELECT_ATTACK;
}

void Battle_Stage::OnMenuSkill()
{
    SoundManager::PlaySFX(L"Resource\\Sound\\UI_Click.wav");
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
    SoundManager::PlaySFX(L"Resource\\Sound\\UI_Click.wav");
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
    // Keyboard and menu completion share the same busy/enemy guards and hazards.
    if (BattleEnded || turnSystem == nullptr || MovingUnit != nullptr || ActingUnit != nullptr ||
        PendingAction.actor != nullptr || turnSystem->IsCurrentTurnEnemy()) return;
    Character* current = turnSystem->GetCurrentTurnCharacter();
    if (!ContainsBattleUnit(current) || !current->IsAlive()) return;

    SoundManager::PlaySFX(L"Resource\\Sound\\UI_Click.wav");
    ClearMovePath();
    ClearBattleSkillSelection();

    if (actionMenu)
    {
        actionMenu->SetVisible(false);
    }

    ApplyTileEffect(current);
    CheckBattleEnd();
    if (BattleEnded) return;

    // A lethal hazard already removes this unit from the queue. Rotating again
    // would skip the next living unit, so only advance the turn we actually ended.
    if (turnSystem->GetCurrentTurnCharacter() == current) turnSystem->NextTurn();

    ResetTurnActions();
    controlMode = EBattleControlMode::WAIT_COMMAND;
}

void Battle_Stage::ResetTurnActions()
{
    // Forced reset cancels unfinished propagation. A normal completed action
    // has no pending actor and may leave only its independent visual tail.
    if (PendingAction.actor) ClearPendingAction();
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
    // A lethal impact must show its reaction before the result covers the map.
    if (PendingAction.actor != nullptr) return;
    if (BattleEnded)
    {
        return;
    }

    if (IsTeamAllDead(enemyTeam))
    {
        BattleEnded = true;
        BattleWon = true;

        SoundManager::StopBGM();
        SoundManager::PlaySFX(L"Resource\\Sound\\TowerTriumph_Victory_Jingle_7s.wav");

        int clearedStageId = GameManager::Get().GetCurrentBattleStageId();
        int nextStageId = clearedStageId + 1;

        if (clearedStageId >= 1 && clearedStageId <= 3)
        {
            nextStageId = 5;
        }
        else if (nextStageId > 5)
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
        return !CurrentMap.allowElementalWaterTraversal ||
            !(unit != nullptr && unit->GetElement() == EElement::Water);

    case ETileType::SkyVoid:
        return !CurrentMap.allowElementalSkyTraversal ||
            !(unit != nullptr && unit->GetElement() == EElement::Light);

    case ETileType::Lava:
        return !CurrentMap.allowLavaTraversal;

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

void Battle_Stage::AddDamageText(Character* target, int damage, COLORREF color, bool areaDamage)
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
    dt.life = static_cast<int>(BattleActionTiming::DamageTextSeconds * 60.0f);
    dt.maxLife = dt.life;
    dt.color = color;
    dt.areaDamage = areaDamage;

    DamageTexts.push_back(dt);
}

void Battle_Stage::UpdateDamageTexts(float elapsedSeconds)
{
    for (auto& dt : DamageTexts)
    {
        dt.elapsedSeconds += elapsedSeconds;
        dt.life = (std::max)(0, dt.maxLife - static_cast<int>(dt.elapsedSeconds * 60.0f + 0.000001f));
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
        BattleSkillArea::SingleDamageFontSize,
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
    LOGFONTW areaFontInfo{};
    GetObjectW(hFont, sizeof(areaFontInfo), &areaFontInfo);
    areaFontInfo.lfHeight = BattleSkillArea::AreaDamageFontSize;
    HFONT areaFont = CreateFontIndirectW(&areaFontInfo);

    for (const auto& dt : DamageTexts)
    {
        SelectObject(hMemDC, dt.areaDamage ? areaFont : hFont);
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
        HFONT compactFont = nullptr;
        if (dt.areaDamage && textSize.cx > TILE_SIZE - 6)
        {
            TEXTMETRICW metrics{};
            GetTextMetricsW(hMemDC, &metrics);
            LOGFONTW compactInfo = areaFontInfo;
            compactInfo.lfWidth = (std::max)(1L, metrics.tmAveCharWidth * (TILE_SIZE - 6) / textSize.cx);
            do
            {
                if (compactFont) { SelectObject(hMemDC, areaFont); DeleteObject(compactFont); }
                compactFont = CreateFontIndirectW(&compactInfo);
                SelectObject(hMemDC, compactFont);
                GetTextExtentPoint32W(hMemDC, text.c_str(), (int)text.length(), &textSize);
            } while (textSize.cx > TILE_SIZE - 6 && --compactInfo.lfWidth > 0);
        }

        int drawX = sx - textSize.cx / 2;
        int drawY = sy - textSize.cy / 2;

        SetTextColor(hMemDC, RGB(0, 0, 0));
        TextOutW(hMemDC, drawX - 2, drawY, text.c_str(), (int)text.length());
        TextOutW(hMemDC, drawX + 2, drawY, text.c_str(), (int)text.length());
        TextOutW(hMemDC, drawX, drawY - 2, text.c_str(), (int)text.length());
        TextOutW(hMemDC, drawX, drawY + 2, text.c_str(), (int)text.length());

        SetTextColor(hMemDC, dt.color);
        TextOutW(hMemDC, drawX, drawY, text.c_str(), (int)text.length());
        if (compactFont) { SelectObject(hMemDC, areaFont); DeleteObject(compactFont); }
    }

    SelectObject(hMemDC, hOldFont);
    DeleteObject(hFont);
    DeleteObject(areaFont);

    SetBkMode(hMemDC, oldBkMode);
}




std::wstring Battle_Stage::GetSkillEffectRelativePath(int skillID) const
{
    if (StandardSkillPresentation::IsBitmap(skillID)) return StandardSkillPresentation::Resource(skillID);
    if (SkillPresentation::IsUltimateBitmap(skillID) || SkillPresentation::IsFlightBitmap(skillID))
        return SkillPresentation::Resource(SkillPresentation::KindForBitmap(skillID), SkillPresentation::IsFlightBitmap(skillID));
    const auto element = BattleActionTiming::EffectElement(skillID);
    if (element != BattleActionTiming::ElementVisual::Neutral)
    {
        const wchar_t* prefix = BattleActionTiming::IsElementContact(skillID) ? L"Resource\\Element_Impact_" :
            BattleActionTiming::IsElementArrow(skillID) ? L"Resource\\Archer_Projectile_" : L"Resource\\Mage_Projectile_";
        return std::wstring(prefix) + BattleActionTiming::ElementSuffix(element) + L".bmp";
    }
    switch (skillID)
    {
    case BattleActionTiming::BasicImpactEffectID: return L"Resource\\Basic_Impact.bmp";
    case BattleActionTiming::ArrowProjectileEffectID: return L"Resource\\Archer_Projectile.bmp";
    case BattleActionTiming::MagicProjectileEffectID: return L"Resource\\Mage_Projectile.bmp";
    case SkillPresentation::MeteorBitmapID: return L"Resource\\Skill_Mage_Meteor_Ultimate.bmp";
    case SkillPresentation::SanctuaryBitmapID: return L"Resource\\Skill_Mage_Sanctuary_Ultimate.bmp";
    case 1101: return L"Skill\\Warrior\\Fire\\1101_Flame_Slash.bmp";
    case 1102: return L"Skill\\Warrior\\Fire\\1102_Burning_Cleave.bmp";
    case 1103: return L"Skill\\Warrior\\Fire\\1103_Blaze_Break.bmp";
    case 1201: return L"Skill\\Warrior\\Water\\1201_Aqua_Slash.bmp";
    case 1202: return L"Skill\\Warrior\\Water\\1202_Wave_Cutter.bmp";
    case 1203: return L"Skill\\Warrior\\Water\\1203_Tide_Breaker.bmp";
    case 1301: return L"Skill\\Warrior\\Grass\\1301_Vine_Slash.bmp";
    case 1302: return L"Skill\\Warrior\\Grass\\1302_Thorn_Cleave.bmp";
    case 1303: return L"Skill\\Warrior\\Grass\\1303_Root_Crusher.bmp";
    case 1401: return L"Skill\\Warrior\\Light\\1401_Holy_Slash.bmp";
    case 1402: return L"Skill\\Warrior\\Light\\1401_Holy_Slash.bmp";
    case 1403: return L"Skill\\Warrior\\Light\\1403_Judgment_Cleave.bmp";
    case 1501: return L"Skill\\Warrior\\Dark\\1502_Shadow_Break.bmp";
    case 1502: return L"Skill\\Warrior\\Dark\\1502_Shadow_Break.bmp";
    case 1503: return L"Skill\\Warrior\\Dark\\1503_Abyss_Cleave.bmp";
    case 2101: return L"Skill\\Warrior\\Fire\\1103_Blaze_Break.bmp";
    case 2102: return L"Skill\\Archer\\Fire\\2102_Explosive_Arrow.bmp";
    case 2103: return L"Skill\\Archer\\Fire\\2103_Blazing_Snipe.bmp";
    case 2201: return L"Skill\\Archer\\Water\\2201_Aqua_Arrow.bmp";
    case 2202: return L"Skill\\Warrior\\Water\\1202_Wave_Cutter.bmp";
    case 2203: return L"Skill\\Archer\\Water\\2203_Tidal_Snipe.bmp";
    case 2301: return L"Skill\\Archer\\Grass\\2301_Thorn_Arrow.bmp";
    case 2302: return L"Skill\\Archer\\Grass\\2302_Vine_Volley.bmp";
    case 2303: return L"Skill\\Archer\\Grass\\2303_Root_Snipe.bmp";
    case 2401: return L"Skill\\Warrior\\Light\\1401_Holy_Slash.bmp";
    case 2402: return L"Skill\\Archer\\Light\\2402_Radiant_Shot.bmp";
    case 2403: return L"Skill\\Archer\\Light\\2403_Judgment_Rain.bmp";
    case 2501: return L"Skill\\Archer\\Dark\\2501_Dark_Arrow.bmp";
    case 2502: return L"Skill\\Warrior\\Dark\\1502_Shadow_Break.bmp";
    case 2503: return L"Skill\\Archer\\Dark\\2503_Abyss_Snipe.bmp";
    case 3101: return L"Skill\\Warrior\\Fire\\1103_Blaze_Break.bmp";
    case 3102: return L"Skill\\Archer\\Fire\\2102_Explosive_Arrow.bmp";
    case 3103: return L"Skill\\Archer\\Fire\\2103_Blazing_Snipe.bmp";
    case 3201: return L"Skill\\Archer\\Water\\2201_Aqua_Arrow.bmp";
    case 3202: return L"Skill\\Warrior\\Water\\1202_Wave_Cutter.bmp";
    case 3203: return L"Skill\\Warrior\\Water\\1203_Tide_Breaker.bmp";
    case 3301: return L"Skill\\Mage\\Grass\\3301_Vine_Whip.bmp";
    case 3302: return L"Skill\\Mage\\Grass\\3302_Thorn_Field.bmp";
    case 3303: return L"Skill\\Warrior\\Grass\\1303_Root_Crusher.bmp";
    case 3401: return L"Skill\\Mage\\Light\\3401_Holy_Ray.bmp";
    case 3402: return L"Skill\\Archer\\Light\\2402_Radiant_Shot.bmp";
    case 3403: return L"Skill\\Archer\\Light\\2403_Judgment_Rain.bmp";
    case 3501: return L"Skill\\Warrior\\Dark\\1502_Shadow_Break.bmp";
    case 3502: return L"Skill\\Warrior\\Dark\\1502_Shadow_Break.bmp";
    case 3503: return L"Skill\\Archer\\Dark\\2503_Abyss_Snipe.bmp";
    default:
        return L"";
    }
}
HBITMAP Battle_Stage::LoadSkillEffectBitmap(int skillID, BITMAP* outInfo)
{
    auto found = SkillEffectBitmaps.find(skillID);

    if (found != SkillEffectBitmaps.end())
    {
        if (!found->second && BattleActionTiming::NeutralEffectID(skillID) != skillID)
            return LoadSkillEffectBitmap(BattleActionTiming::NeutralEffectID(skillID), outInfo);
        auto infoFound = SkillEffectBitmapInfos.find(skillID);

        if (outInfo != nullptr && infoFound != SkillEffectBitmapInfos.end())
        {
            *outInfo = infoFound->second;
        }

        return found->second;
    }

    std::wstring relativePath = GetSkillEffectRelativePath(skillID);

    if (relativePath.empty())
    {
        return nullptr;
    }

    std::vector<std::wstring> candidates;
    candidates.push_back(relativePath);
    candidates.push_back(L"Tower_Triumph\\" + relativePath);
    candidates.push_back(L"..\\Tower_Triumph\\" + relativePath);
    candidates.push_back(L"..\\..\\Tower_Triumph\\" + relativePath);

    for (const std::wstring& path : candidates)
    {
        HBITMAP hBitmap = (HBITMAP)LoadImageW(
            NULL,
            path.c_str(),
            IMAGE_BITMAP,
            0,
            0,
            LR_LOADFROMFILE | LR_CREATEDIBSECTION
        );

        if (hBitmap != nullptr)
        {
            BITMAP info = {};
            GetObject(hBitmap, sizeof(BITMAP), &info);
            if (BattleActionTiming::IsContactEffect(skillID) && (info.bmWidth != 256 || info.bmHeight != 128))
            {
                DeleteObject(hBitmap);
                continue;
            }
            const bool projectile = BattleActionTiming::IsProjectileEffect(skillID);
            const bool ultimate = SkillPresentation::IsUltimateBitmap(skillID);
            const bool ultimateFlight = SkillPresentation::IsFlightBitmap(skillID);
            const bool standard = StandardSkillPresentation::IsBitmap(skillID);
            if (standard && (info.bmWidth != StandardSkillPresentation::Columns * StandardSkillPresentation::CellSize ||
                info.bmHeight != StandardSkillPresentation::Rows * StandardSkillPresentation::CellSize))
            {
                DeleteObject(hBitmap);
                continue;
            }
            if (ultimate && (info.bmWidth != SkillPresentation::Columns * SkillPresentation::CellSize ||
                info.bmHeight != SkillPresentation::Rows * SkillPresentation::CellSize))
            {
                DeleteObject(hBitmap);
                continue;
            }
            if (ultimateFlight && (info.bmWidth != SkillPresentation::FlightDirections * SkillPresentation::CellSize ||
                info.bmHeight != SkillPresentation::FlightFrames * SkillPresentation::CellSize))
            {
                DeleteObject(hBitmap);
                continue;
            }
            if (projectile && (info.bmWidth != BattleActionTiming::ProjectileDirections * BattleActionTiming::ProjectileCellSize ||
                info.bmHeight != BattleActionTiming::ProjectileFrames * BattleActionTiming::ProjectileCellSize))
            {
                DeleteObject(hBitmap);
                continue;
            }

            COLORREF colorKey = RGB(0, 0, 0);
            HDC hScreenDC = GetDC(nullptr);
            HDC hTempDC = CreateCompatibleDC(hScreenDC);

            if (hTempDC != nullptr)
            {
                HBITMAP hOldBitmap = (HBITMAP)SelectObject(hTempDC, hBitmap);
                COLORREF sampledColor = GetPixel(hTempDC, 0, 0);

                if (sampledColor != CLR_INVALID)
                {
                    colorKey = sampledColor;
                }

                SelectObject(hTempDC, hOldBitmap);
                DeleteDC(hTempDC);
            }

            if (hScreenDC != nullptr)
            {
                ReleaseDC(nullptr, hScreenDC);
            }
            if (BattleActionTiming::IsContactEffect(skillID) || projectile || ultimate || ultimateFlight || standard) colorKey = CHARACTER_TRANSPARENT_COLOR;

            SkillEffectBitmaps[skillID] = hBitmap;
            SkillEffectBitmapInfos[skillID] = info;
            SkillEffectColorKeys[skillID] = colorKey;

            if (outInfo != nullptr)
            {
                *outInfo = info;
            }

            return hBitmap;
        }
    }

    if (BattleActionTiming::NeutralEffectID(skillID) != skillID)
    {
        // Cache only the failed lookup, not an alias to an owned fallback bitmap.
        SkillEffectBitmaps[skillID] = nullptr;
        SkillEffectColorKeys[skillID] = CHARACTER_TRANSPARENT_COLOR;
        return LoadSkillEffectBitmap(BattleActionTiming::NeutralEffectID(skillID), outInfo);
    }
    if (StandardSkillPresentation::IsBitmap(skillID)) SkillEffectBitmaps[skillID] = nullptr;
    return nullptr;
}

void Battle_Stage::ReleaseSkillEffectBitmaps()
{
    for (auto& pair : SkillEffectBitmaps)
    {
        if (pair.second != nullptr)
        {
            DeleteObject(pair.second);
        }
    }

    SkillEffectBitmaps.clear();
    SkillEffectBitmapInfos.clear();
    SkillEffectColorKeys.clear();
    ActiveSkillEffects.clear();
    ActiveUltimateEffects.clear();
    ActiveStandardEffects.clear();
}

int Battle_Stage::GetSkillEffectDrawSize(ESkillAreaType areaType) const
{
    switch (areaType)
    {
    case ESkillAreaType::Splash2:
        return TILE_SIZE * 5 / 2;
    case ESkillAreaType::Splash1:
        return TILE_SIZE * 7 / 4; 
    case ESkillAreaType::Line:
        return TILE_SIZE * 2;        
    case ESkillAreaType::Single:
    default:
        return TILE_SIZE * 3 / 2;     
    }
}

void Battle_Stage::PlaySkillEffect(int skillID, int gridX, int gridY, ESkillAreaType areaType, bool enhancedContact)
{
    if (!BattleActionTiming::IsContactEffect(skillID))
        SoundManager::PlaySFX(L"Resource\\Sound\\Skill_Impact.wav");
    if (!IsInsideGrid(gridX, gridY))
    {
        return;
    }

    BITMAP info = {};
    HBITMAP hBitmap = LoadSkillEffectBitmap(skillID, &info);

    if (hBitmap == nullptr || info.bmWidth <= 0 || info.bmHeight <= 0)
    {
        return;
    }

    SkillEffectAnim effect = {};
    effect.skillID = skillID;
    effect.gridX = gridX;
    effect.gridY = gridY;
    effect.areaType = areaType;
    effect.frameIndex = 0;
    effect.frameTick = 0;
    effect.frameDelay = 4;
    effect.maxFrame = 8;
    effect.enhancedContact = enhancedContact && BattleActionTiming::IsContactEffect(skillID);

    ActiveSkillEffects.push_back(effect);
}

void Battle_Stage::UpdateSkillEffects(float elapsedSeconds)
{
    for (SkillEffectAnim& effect : ActiveSkillEffects)
    {
        effect.elapsedSeconds += elapsedSeconds;
        if (effect.enhancedContact)
        {
            effect.frameIndex = AreaSkillVisuals::BodyFrame(effect.elapsedSeconds);
            continue;
        }
        const float frameSeconds = BattleActionTiming::IsContactEffect(effect.skillID) ?
            BattleActionTiming::BasicImpactFrameSeconds : BattleActionTiming::SkillEffectFrameSeconds;
        effect.frameIndex = static_cast<int>((effect.elapsedSeconds + 0.000001f) /
            frameSeconds);
    }

    ActiveSkillEffects.erase(
        std::remove_if(
            ActiveSkillEffects.begin(),
            ActiveSkillEffects.end(),
            [](const SkillEffectAnim& effect)
            {
                return effect.enhancedContact ? effect.elapsedSeconds >= AreaSkillVisuals::BodySeconds :
                    effect.frameIndex >= effect.maxFrame;
            }
        ),
        ActiveSkillEffects.end()
    );
}

void Battle_Stage::DrawAreaSkillEffects(HDC hMemDC, HDC hMemDC1, bool front)
{
    for (const auto& effect : ActiveAreaEffects)
    {
        AreaSkillEffectRenderer::Options options;
        options.kind = effect.kind;
        options.element = effect.element;
        options.groundOnly = effect.presentation != SkillPresentation::Kind::Normal || effect.standard;
        options.originX = effect.origin.x;
        options.originY = effect.origin.y;
        options.tileSize = TILE_SIZE;
        options.offsetX = GetOffsetX();
        options.offsetY = GetOffsetY();
        options.footRatio = UsesGridAlignedBackground(CurrentMap) ? 0.75f : 1.0f;
        options.color = effect.color;
        BITMAP info{};
        options.contactBitmap = LoadSkillEffectBitmap(BattleActionTiming::ContactEffectID(effect.element), &info);
        options.bitmapWidth = info.bmWidth;
        options.bitmapHeight = info.bmHeight;
        if (front) AreaSkillEffectRenderer::DrawFront(hMemDC, hMemDC1, effect.cells, effect.elapsedSeconds, options);
        else AreaSkillEffectRenderer::Draw(hMemDC, hMemDC1, effect.cells, effect.elapsedSeconds, options);
    }
}

void Battle_Stage::DrawUltimateSkillEffects(HDC hMemDC, HDC hMemDC1, bool front)
{
    for (const auto& effect : ActiveUltimateEffects)
    {
        UltimateSkillRenderer::Options options;
        options.kind = effect.presentation;
        options.element = effect.element;
        options.front = front;
        BITMAP info{};
        options.bitmap = LoadSkillEffectBitmap(SkillPresentation::BitmapID(effect.presentation), &info);
        if (SkillPresentation::IsSnipe(effect.presentation))
            options.flightBitmap = LoadSkillEffectBitmap(SkillPresentation::FlightBitmapID(effect.presentation), &info);
        if (SkillPresentation::IsRain(effect.presentation))
            options.rainLaunchBitmap = LoadSkillEffectBitmap(BattleActionTiming::ProjectileEffectID(
                BattleActionTiming::ProjectileKind::Arrow, BattleActionTiming::ElementVisual::Light), &info);
        if (SkillPresentation::IsCellSummon(effect.presentation))
            options.contactBitmap = LoadSkillEffectBitmap(BattleActionTiming::ContactEffectID(effect.element), &info);
        options.deliverySeconds = effect.deliverySeconds;
        options.facing = effect.facing;
        options.casterFootX = GetOffsetX() + effect.casterFootX;
        options.casterFootY = GetOffsetY() + effect.casterFootY;
        options.startX = GetOffsetX() + effect.startX; options.startY = GetOffsetY() + effect.startY;
        options.targetX = GetOffsetX() + effect.targetX; options.targetY = GetOffsetY() + effect.targetY;
        options.tileSize = TILE_SIZE;
        options.offsetX = GetOffsetX();
        options.offsetY = GetOffsetY();
        options.footRatio = UsesGridAlignedBackground(CurrentMap) ? 0.75f : 1.0f;
        options.centerX = options.offsetX + effect.center.x * TILE_SIZE + TILE_SIZE / 2;
        options.centerY = options.offsetY + effect.center.y * TILE_SIZE +
            (UsesGridAlignedBackground(CurrentMap) ? TILE_SIZE * 3 / 4 : TILE_SIZE);
        UltimateSkillRenderer::Draw(hMemDC, hMemDC1, effect.cells, effect.elapsedSeconds, effect.releaseSeconds, options);
    }
}

void Battle_Stage::DrawStandardSkillEffects(HDC hMemDC, HDC hMemDC1, bool front)
{
    for (const auto& effect : ActiveStandardEffects)
    {
        StandardSkillRenderer::Options options;
        options.front = front;
        options.tileSize = TILE_SIZE;
        options.offsetX = GetOffsetX(); options.offsetY = GetOffsetY();
        options.footRatio = UsesGridAlignedBackground(CurrentMap) ? 0.75f : 1.0f;
        const int bitmapID = StandardSkillPresentation::BitmapID(effect.profile);
        if (bitmapID != 0) options.bitmap = LoadSkillEffectBitmap(bitmapID, nullptr);
        options.contact = LoadSkillEffectBitmap(BattleActionTiming::ContactEffectID(effect.profile.element), nullptr);
        if (!StandardSkillPresentation::Knight(effect.profile))
            options.projectile = LoadSkillEffectBitmap(BattleActionTiming::ProjectileEffectID(
                effect.profile.owner == StandardSkillPresentation::Class::Archer ?
                BattleActionTiming::ProjectileKind::Arrow : BattleActionTiming::ProjectileKind::Magic, effect.profile.element), nullptr);
        StandardSkillRenderer::Draw(hMemDC, hMemDC1, effect, options);
    }
}

void Battle_Stage::DrawPendingProjectile(HDC hMemDC, HDC hMemDC1)
{
    if (StandardSkillPresentation::Valid(PendingAction.standard)) return;
    const auto& flight = PendingAction.timeline;
    if (!flight.launched || flight.impactCommitted || PendingAction.cancelled ||
        PendingAction.projectile == BattleActionTiming::ProjectileKind::None || !(flight.flightSeconds > 0.0f)) return;
    const bool arrow = PendingAction.projectile == BattleActionTiming::ProjectileKind::Arrow;
    const int bitmapID = BattleActionTiming::ProjectileEffectID(PendingAction.projectile, PendingAction.element);
    BITMAP info{};
    HBITMAP bitmap = LoadSkillEffectBitmap(bitmapID, &info);
    if (!bitmap) return;

    float dx = PendingAction.targetX - PendingAction.startX;
    float dy = PendingAction.targetY - PendingAction.startY;
    const float progress = (std::min)(1.0f, flight.clock.elapsed / flight.flightSeconds);
    float pointX = PendingAction.startX + dx * progress;
    float pointY = PendingAction.startY + dy * progress;
    if (PendingAction.areaType == ESkillAreaType::Line && !PendingAction.areaCells.empty())
    {
        const auto sample = BattleSkillArea::SampleLinePath(PendingAction.linePath, progress);
        pointX = sample.x;
        pointY = sample.y;
        dx = sample.dx;
        dy = sample.dy;
    }
    const int direction = BattleActionTiming::DirectionFromRadians(atan2f(dy, dx));
    const float cosine = BattleActionTiming::DirectionCosines[direction];
    const float sine = BattleActionTiming::DirectionCosines[(direction + 12) % BattleActionTiming::ProjectileDirections];
    const float anchorX = arrow ? BattleActionTiming::ArrowTipOffsetX : BattleActionTiming::MagicCoreOffsetX;
    const float anchorY = arrow ? BattleActionTiming::ArrowTipOffsetY : BattleActionTiming::MagicCoreOffsetY;
    const float scale = static_cast<float>(BattleActionTiming::ProjectileDrawSize) / BattleActionTiming::ProjectileCellSize;
    // The path follows the arrow tip / bright core, not the bitmap's center.
    // Quantizing atlas direction never changes the continuous flight vector.
    const int drawX = GetOffsetX() + static_cast<int>(std::lround(pointX - (anchorX * cosine - anchorY * sine) * scale -
        BattleActionTiming::ProjectileDrawSize * 0.5f));
    const int drawY = GetOffsetY() + static_cast<int>(std::lround(pointY - (anchorX * sine + anchorY * cosine) * scale -
        BattleActionTiming::ProjectileDrawSize * 0.5f));
    const int frame = static_cast<int>((flight.clock.elapsed + 0.000001f) / BattleActionTiming::ProjectileFrameSeconds) %
        BattleActionTiming::ProjectileFrames;
    HBITMAP oldBitmap = static_cast<HBITMAP>(SelectObject(hMemDC1, bitmap));
    TransparentBlt(hMemDC, drawX, drawY, BattleActionTiming::ProjectileDrawSize, BattleActionTiming::ProjectileDrawSize,
        hMemDC1, direction * BattleActionTiming::ProjectileCellSize, frame * BattleActionTiming::ProjectileCellSize,
        BattleActionTiming::ProjectileCellSize, BattleActionTiming::ProjectileCellSize, CHARACTER_TRANSPARENT_COLOR);
    SelectObject(hMemDC1, oldBitmap);
}

void Battle_Stage::DrawSkillEffects(HDC hMemDC, HDC hMemDC1)
{
    if (ActiveSkillEffects.empty())
    {
        return;
    }

    const int frameColumnCount = 4;
    const int frameRowCount = 2;

    int offsetX = GetOffsetX();
    int offsetY = GetOffsetY();

    for (const SkillEffectAnim& effect : ActiveSkillEffects)
    {
        BITMAP info = {};
        HBITMAP hBitmap = LoadSkillEffectBitmap(effect.skillID, &info);

        if (hBitmap == nullptr || info.bmWidth <= 0 || info.bmHeight <= 0)
        {
            continue;
        }

        int frameWidth = info.bmWidth / frameColumnCount;
        int frameHeight = info.bmHeight / frameRowCount;

        if (frameWidth <= 0 || frameHeight <= 0)
        {
            continue;
        }

        int frame = effect.frameIndex;

        if (frame < 0)
        {
            frame = 0;
        }

        if (frame >= effect.maxFrame)
        {
            frame = effect.maxFrame - 1;
        }

        int srcX = (frame % frameColumnCount) * frameWidth;
        int srcY = (frame / frameColumnCount) * frameHeight;

        const bool contactImpact = BattleActionTiming::IsContactEffect(effect.skillID);
        int drawWidth = contactImpact ? TILE_SIZE : GetSkillEffectDrawSize(effect.areaType);
        int drawHeight = drawWidth * frameHeight / frameWidth;

        int centerX = offsetX + effect.gridX * TILE_SIZE + TILE_SIZE / 2;
        int centerY = offsetY + effect.gridY * TILE_SIZE + TILE_SIZE / 2;
        if (contactImpact) centerY -= TILE_SIZE / 4;

        int drawX = centerX - drawWidth / 2;
        int drawY = centerY - drawHeight / 2;

        COLORREF colorKey = RGB(0, 0, 0);
        auto colorKeyFound = SkillEffectColorKeys.find(effect.skillID);

        if (colorKeyFound != SkillEffectColorKeys.end())
        {
            colorKey = colorKeyFound->second;
        }

        if (effect.enhancedContact)
        {
            AreaSkillEffectRenderer::DrawBodyContact(hMemDC, hMemDC1, hBitmap, centerX, centerY,
                effect.elapsedSeconds, colorKey);
            continue;
        }

        HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC1, hBitmap);

        TransparentBlt(
            hMemDC,
            drawX,
            drawY,
            drawWidth,
            drawHeight,
            hMemDC1,
            srcX,
            srcY,
            frameWidth,
            frameHeight,
            colorKey
        );

        SelectObject(hMemDC1, hOldBitmap);
    }
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

RECT Battle_Stage::GetBattleSkillPanelRect() const
{
    RECT panel;
    panel.left = rects.right - 390;
    panel.top = 80;
    panel.right = rects.right - 25;
    panel.bottom = 610;
    return panel;
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
    RECT panel = GetBattleSkillPanelRect();

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
    RECT panel = GetBattleSkillPanelRect();

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
    if (controlMode != EBattleControlMode::SELECT_SKILL || !bSelectingBattleSkillTarget ||
        mouseX < 0 || mouseY < 0 || mouseX >= rects.right || mouseY >= rects.bottom ||
        IsPointInRect(GetBattleSkillPanelRect(), mouseX, mouseY))
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

    POINT mousePos{};
    if (!GetCursorPos(&mousePos) || !ScreenToClient(hWnd, &mousePos))
    {
        LastMouseX = -1;
        LastMouseY = -1;
        HoverGridX = -1;
        HoverGridY = -1;
        return;
    }

    LastMouseX = mousePos.x;
    LastMouseY = mousePos.y;

    // The opaque skill panel consumes pointer input, including edge scrolling.
    if (mousePos.x < 0 || mousePos.y < 0 || mousePos.x >= rects.right || mousePos.y >= rects.bottom ||
        IsPointInRect(GetBattleSkillPanelRect(), mousePos.x, mousePos.y))
    {
        UpdateHoverGridByMouse(LastMouseX, LastMouseY);
        return;
    }

    float moveX = 0.0f;
    float moveY = 0.0f;

    const int edgeMargin = 42;
    const float cameraSpeed = 34.0f;

    if (mousePos.x < edgeMargin)
    {
        moveX -= 1.0f;
    }

    if (mousePos.x > rects.right - edgeMargin)
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

    }

    // Recompute even without edge movement: keyboard panning and a new
    // selection can change the cell beneath a stationary pointer.
    UpdateHoverGridByMouse(LastMouseX, LastMouseY);
}

std::vector<BattleSkillArea::Cell> Battle_Stage::BuildSkillAreaCells(Character* user, int skillID, int centerX, int centerY) const
{
    const SSkillData* info = FindBattleSkillInfo(skillID);
    if (!user || !info) return {};
    const Location caster = user->GetGridPos();
    return BattleSkillArea::BuildCells(ToAreaKind(info->AreaType), { caster.x, caster.y },
        { centerX, centerY }, info->Range, W, H);
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

    const auto cells = BuildSkillAreaCells(current, SelectedBattleSkillID, HoverGridX, HoverGridY);

    if (cells.empty())
    {
        return;
    }

    HBRUSH hAreaBrush = CreateHatchBrush(HS_DIAGCROSS, previewColor);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hMemDC, hAreaBrush);
    HPEN hAreaPen = CreatePen(PS_SOLID, 4, previewColor);
    HPEN hOldPen = (HPEN)SelectObject(hMemDC, hAreaPen);

    for (const auto& cell : cells)
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

    LastMouseX = mouseX;
    LastMouseY = mouseY;
    RECT cancelRect = GetBattleCancelRect();

    if (IsPointInRect(cancelRect, mouseX, mouseY))
    {
        if (bSelectingBattleSkillTarget)
        {
            SelectedBattleSkillID = -1;
            bSelectingBattleSkillTarget = false;
            UpdateHoverGridByMouse(mouseX, mouseY);
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
            UpdateHoverGridByMouse(mouseX, mouseY);
            return true;
        }

        return true;
    }

    UpdateHoverGridByMouse(mouseX, mouseY);
    if (HoverGridX < 0 || HoverGridY < 0) return true;
    const int gridX = HoverGridX;
    const int gridY = HoverGridY;
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
    if (ActingUnit || PendingAction.actor || !ContainsBattleUnit(user) || !ContainsBattleUnit(target) ||
        !user->IsAlive() || !target->IsAlive())
    {
        return false;
    }

    const SSkillData* info = FindBattleSkillInfo(SelectedBattleSkillID);

    if (info == nullptr)
    {
        return false;
    }

    const Location requested = target->GetGridPos();
    const auto cells = BuildSkillAreaCells(user, SelectedBattleSkillID, requested.x, requested.y);
    if (!IsBattleSkillTargetValid(user, SelectedBattleSkillID, requested.x, requested.y) ||
        cells.empty()) return false;
    CharacterStat userStat = user->GetStat();

    if (userStat.CurMP < info->MPCost)
    {
        AddDamageText(user, 0, RGB(120, 180, 255));
        return false;
    }

    userStat.CurMP -= info->MPCost;
    user->SetStat(userStat);

    const Location targetPos = target->GetGridPos();
    user->LookAtGrid(targetPos.x, targetPos.y);
    std::vector<PendingHit> pendingHits;
    for (std::size_t i = 0; i < cells.size(); ++i)
    {
        for (Character* hit : enemyTeam)
        {
            if (!hit || !hit->IsAlive()) continue;
            const Location pos = hit->GetGridPos();
            if (pos.x != cells[i].x || pos.y != cells[i].y) continue;
            int damage = CalcBattleSkillSingleDamage(user, hit, SelectedBattleSkillID);
            if (hit != target)
                damage = ClampBattleDamage(damage * BattleSkillArea::SecondaryDamagePercent(ToAreaKind(info->AreaType)) / 100);
            PendingHit pendingHit{ hit, damage, info->Color, false };
            if (info->AreaType != ESkillAreaType::Single) pendingHit.areaCellIndex = static_cast<int>(i);
            pendingHits.push_back(pendingHit);
        }
    }

    BeginPendingAction(user, ECharacterState::Skill, std::move(pendingHits),
        SelectedBattleSkillID, targetPos, info->AreaType, cells);

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

    SoundManager::PlaySFX(L"Resource\\Sound\\Item_Use.wav");

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
    if (unit == nullptr || path.empty() || MovingUnit != nullptr)
    {
        return false;
    }

    Location previous = unit->GetGridPos();
    for (const auto& step : path)
    {
        if (std::abs(step.x - previous.x) + std::abs(step.y - previous.y) != 1)
        {
            return false;
        }
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
        previous = step;
    }

    ActiveMovePath = path;
    ActiveMoveIndex = 0;

    Location first = ActiveMovePath[ActiveMoveIndex];

    if (!unit->StartMoveToGrid(first.x, first.y, TILE_SIZE, ActiveMovePath.size() > 1))
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
    if (MovingUnit != nullptr)
    {
        MovingUnit->SyncWorldPixelToGrid(TILE_SIZE);
        if (MovingUnit->IsAlive() && MovingUnit->GetState() == ECharacterState::Walk)
            MovingUnit->SetState(ECharacterState::Idle);
        MovingUnit = nullptr;
    }
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
    if (controlMode == EBattleControlMode::SELECT_SKILL && bSelectingBattleSkillTarget)
    {
        UpdateHoverGridByMouse(LastMouseX, LastMouseY);
    }
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

    int maxX = W * TILE_SIZE - halfW;
    int maxY = H * TILE_SIZE - halfH;

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

        BeginPendingAction(enemy, ECharacterState::Attack,
            { { target, damage, RGB(255, 160, 40), false } });

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

    if (UsesGridAlignedBackground(CurrentMap))
    {
        // Authored or composed at world resolution: no independent rescaling.
        BitBlt(hMemDC, offsetX, offsetY, W * TILE_SIZE, H * TILE_SIZE,
            hMemDC1, 0, 0, SRCCOPY);
        SelectObject(hMemDC1, hOld);
        return;
    }

    StretchBlt(
        hMemDC,
        offsetX,
        offsetY,
        W * TILE_SIZE,
        H * TILE_SIZE,
        hMemDC1,
        0,
        0,
        BackgroundInfo.bmWidth,
        BackgroundInfo.bmHeight,
        SRCCOPY
    );

    SelectObject(hMemDC1, hOld);
}

void Battle_Stage::DrawGrid(HDC hMemDC)
{
    if (UsesGridAlignedBackground(CurrentMap) && !CurrentMap.drawTileDebug &&
        controlMode == EBattleControlMode::WAIT_COMMAND)
    {
        return;
    }
    int offsetX = GetOffsetX();
    int offsetY = GetOffsetY();

    const COLORREF gridColor = !UsesGridAlignedBackground(CurrentMap) ?
        RGB(230, 230, 230) : RGB(63, 78, 91);
    HPEN hPen = CreatePen(PS_SOLID, 1, gridColor);
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

    // Preserve the old vertical scale, but never squeeze the source horizontally.
    const int referenceFrameHeight = TILE_SIZE + 16;
    int footX = tileScreenX + TILE_SIZE / 2;
    int footY = tileScreenY + TILE_SIZE;

    if (UsesGridAlignedBackground(CurrentMap))
    {
        footX = tileScreenX + TILE_SIZE *
            GraveyardRenderer::FootXNumerator / GraveyardRenderer::FootXDenominator;
        footY = tileScreenY + TILE_SIZE *
            GraveyardRenderer::FootYNumerator / GraveyardRenderer::FootYDenominator;
        HBRUSH shadow = CreateSolidBrush(RGB(21, 27, 31));
        HGDIOBJ oldBrush = SelectObject(hMemDC, shadow);
        HGDIOBJ oldPen = SelectObject(hMemDC, GetStockObject(NULL_PEN));
        Ellipse(hMemDC, footX - TILE_SIZE / 6, footY - TILE_SIZE / 16,
            footX + TILE_SIZE / 6, footY + TILE_SIZE / 16);
        SelectObject(hMemDC, oldPen);
        SelectObject(hMemDC, oldBrush);
        DeleteObject(shadow);
    }

    unit->DrawAtFoot(hMemDC, hMemDC1, footX, footY, referenceFrameHeight);

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
