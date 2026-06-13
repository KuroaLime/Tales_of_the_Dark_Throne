#include "BattleMapData.h"
#include "resource.h"

#include <algorithm>
#include <utility>

static void SetMapPoint(SBattleMapData& map, int x, int y, ETileType type)
{
    if (x < 0 || y < 0 || x >= map.width || y >= map.height)
    {
        return;
    }

    map.tiles[y][x] = type;
}

static void SetMapRect(SBattleMapData& map, int x1, int y1, int x2, int y2, ETileType type)
{
    if (x1 > x2)
    {
        std::swap(x1, x2);
    }

    if (y1 > y2)
    {
        std::swap(y1, y2);
    }

    for (int y = y1; y <= y2; ++y)
    {
        for (int x = x1; x <= x2; ++x)
        {
            SetMapPoint(map, x, y, type);
        }
    }
}

static void FillMapTiles(SBattleMapData& map, ETileType type)
{
    map.tiles.clear();
    map.tiles.resize(map.height);

    for (int y = 0; y < map.height; ++y)
    {
        map.tiles[y].resize(map.width, type);
    }
}

static void InitBaseMap(SBattleMapData& map)
{
    map.backgroundResourceId = IDB_BITMAP3;
    map.tileSize = 64;
    map.width = 44;
    map.height = 24;
    map.cameraStartX = 12;
    map.cameraStartY = 18;
    map.drawTileDebug = false;

    map.tiles.clear();
    map.tiles.resize(map.height);

    for (int y = 0; y < map.height; ++y)
    {
        map.tiles[y].resize(map.width, ETileType::Normal);
    }

    auto SetPoint = [&](int x, int y, ETileType type)
    {
        if (x < 0 || y < 0 || x >= map.width || y >= map.height)
        {
            return;
        }

        map.tiles[y][x] = type;
    };

    auto SetRect = [&](int x1, int y1, int x2, int y2, ETileType type)
    {
        if (x1 > x2)
        {
            std::swap(x1, x2);
        }

        if (y1 > y2)
        {
            std::swap(y1, y2);
        }

        for (int y = y1; y <= y2; ++y)
        {
            for (int x = x1; x <= x2; ++x)
            {
                SetPoint(x, y, type);
            }
        }
    };

    auto SetRoad = [&](int x1, int y1, int x2, int y2)
    {
        SetRect(x1, y1, x2, y2, ETileType::Road);
    };

    SetRect(0, 4, 5, 6, ETileType::Water);
    SetRect(0, 5, 2, 8, ETileType::Water);
    SetRect(7, 0, 11, 3, ETileType::Water);

    SetRect(13, 0, 19, 7, ETileType::Building);
    SetRect(20, 0, 23, 8, ETileType::Building);
    SetRect(37, 0, 40, 5, ETileType::Building);
    SetRect(40, 8, 43, 13, ETileType::Building);
    SetRect(27, 2, 30, 9, ETileType::Building);
    SetRect(31, 3, 34, 6, ETileType::Building);
    SetRect(12, 12, 16, 18, ETileType::Building);
    SetRect(13, 9, 16, 11, ETileType::Building);
    SetRect(0, 15, 2, 18, ETileType::Building);
    SetRect(15, 20, 20, 23, ETileType::Building);
    SetRect(25, 20, 31, 23, ETileType::Building);
    SetRect(32, 20, 35, 23, ETileType::Building);
    SetRect(40, 14, 43, 19, ETileType::Building);
    SetRect(30, 14, 35, 18, ETileType::Building);

    SetRect(0, 0, 2, 3, ETileType::Tree);
    SetRect(9, 2, 11, 5, ETileType::Tree);
    SetRect(14, 9, 17, 12, ETileType::Tree);
    SetRect(24, 5, 27, 10, ETileType::Tree);
    SetRect(31, 7, 35, 14, ETileType::Tree);
    SetRect(19, 12, 23, 16, ETileType::Tree);
    SetRect(16, 14, 19, 18, ETileType::Tree);
    SetRect(0, 20, 3, 23, ETileType::Tree);
    SetRect(23, 20, 25, 23, ETileType::Tree);
    SetRect(34, 20, 37, 23, ETileType::Tree);
    SetRect(35, 0, 37, 3, ETileType::Tree);
    SetRect(41, 0, 43, 3, ETileType::Tree);

    SetRect(5, 8, 11, 8, ETileType::Fence);
    SetRect(10, 7, 11, 10, ETileType::Fence);
    SetRect(12, 2, 12, 9, ETileType::Wall);
    SetRect(18, 18, 28, 18, ETileType::Fence);
    SetRect(35, 8, 40, 8, ETileType::Fence);
    SetRect(36, 14, 40, 14, ETileType::Fence);
    SetRect(38, 10, 39, 12, ETileType::Wall);
    SetRect(39, 15, 40, 17, ETileType::Wall);

    SetPoint(3, 2, ETileType::Grave);
    SetPoint(5, 0, ETileType::Grave);
    SetPoint(6, 5, ETileType::Grave);
    SetPoint(10, 5, ETileType::Grave);
    SetPoint(12, 9, ETileType::Grave);
    SetPoint(18, 8, ETileType::Grave);
    SetPoint(24, 11, ETileType::Grave);
    SetPoint(26, 12, ETileType::Grave);
    SetPoint(28, 10, ETileType::Grave);
    SetPoint(30, 12, ETileType::Grave);
    SetPoint(32, 13, ETileType::Grave);
    SetPoint(34, 12, ETileType::Grave);
    SetPoint(36, 10, ETileType::Grave);
    SetPoint(38, 20, ETileType::Grave);
    SetPoint(39, 6, ETileType::Grave);
    SetPoint(41, 6, ETileType::Grave);
    SetPoint(42, 12, ETileType::Grave);

    SetRoad(5, 18, 10, 20);
    SetRoad(6, 17, 9, 19);
    SetRoad(17, 8, 22, 20);
    SetRoad(14, 18, 28, 20);
    SetRoad(4, 17, 16, 20);
    SetRoad(8, 14, 12, 18);
    SetRoad(28, 11, 39, 13);
    SetRoad(34, 15, 39, 19);
    SetRoad(34, 18, 39, 19);
    SetRoad(36, 16, 39, 18);
    SetRoad(5, 3, 8, 4);
    SetRoad(3, 7, 5, 8);
    SetRoad(15, 7, 18, 9);

    SetRect(6, 14, 10, 16, ETileType::MagicHazard);
    SetPoint(8, 17, ETileType::MagicHazard);
    SetRect(21, 4, 22, 5, ETileType::MagicHazard);
    SetRect(32, 19, 33, 19, ETileType::MagicHazard);

    SetRect(3, 12, 5, 14, ETileType::CursedGround);
    SetRect(11, 5, 12, 7, ETileType::CursedGround);
    SetRect(29, 0, 34, 2, ETileType::CursedGround);
    SetRect(0, 22, 8, 23, ETileType::CursedGround);

    SetRoad(6, 19, 6, 19);
    SetRoad(8, 19, 8, 19);
    SetRoad(7, 18, 7, 18);
    SetRoad(36, 18, 36, 18);
    SetRoad(34, 19, 34, 19);
    SetRoad(38, 18, 38, 18);
}

EBattleMapId BattleMapData::FromStageId(int stageId)
{
    switch (stageId)
    {
    case 1:
        return EBattleMapId::Stage01;

    case 2:
        return EBattleMapId::Stage02;

    case 3:
        return EBattleMapId::Stage03;

    case 4:
        return EBattleMapId::Stage04;

    case 5:
        return EBattleMapId::Stage05;

    default:
        return EBattleMapId::Stage01;
    }
}

SBattleMapData BattleMapData::LoadByStageId(int stageId)
{
    return Load(FromStageId(stageId));
}

SBattleMapData BattleMapData::Load(EBattleMapId mapId)
{
    switch (mapId)
    {
    case EBattleMapId::Test01:
    case EBattleMapId::Stage01:
        return BuildStage01();

    case EBattleMapId::Stage02:
        return BuildStage02();

    case EBattleMapId::Stage03:
        return BuildStage03();

    case EBattleMapId::Stage04:
        return BuildStage04();

    case EBattleMapId::Stage05:
        return BuildStage05();

    default:
        return BuildStage01();
    }
}

SBattleMapData BattleMapData::BuildTest01()
{
    return BuildStage01();
}

static void AddPlayerStarts(SBattleMapData& map, const std::vector<std::pair<int, int>>& starts)
{
    if (starts.size() < 5)
    {
        return;
    }

    map.playerStarts.push_back({ ECharacterClass::Warrior, L"Warrior", starts[0].first, starts[0].second, 15 });
    map.playerStarts.push_back({ ECharacterClass::Archer,  L"Archer",  starts[1].first, starts[1].second, 25 });
    map.playerStarts.push_back({ ECharacterClass::Mage,    L"Mage",    starts[2].first, starts[2].second, 18 });
    map.playerStarts.push_back({ ECharacterClass::Warrior, L"Warrior", starts[3].first, starts[3].second, 15 });
    map.playerStarts.push_back({ ECharacterClass::Mage,    L"Mage",    starts[4].first, starts[4].second, 18 });
}

static void AddDefaultPlayerStarts(SBattleMapData& map)
{
    AddPlayerStarts(map, {
        { 6, 19 },
        { 8, 19 },
        { 7, 18 },
        { 5, 19 },
        { 9, 18 }
    });
}

SBattleMapData BattleMapData::BuildStage01()
{
    SBattleMapData map;
    InitBaseMap(map);

    map.mapId = EBattleMapId::Stage01;
    map.mapName = L"Stage 01 - 묘지";
    map.backgroundFilePath = L"Source\\SMap\\background\\Test01.bmp";
    map.cameraStartX = 11;
    map.cameraStartY = 11;

    // 아군은 화면 시작 시 바로 보이도록 중앙-서쪽 개활지에 배치.
    AddPlayerStarts(map, {
        { 8, 11 },
        { 10, 11 },
        { 9, 12 },
        { 11, 12 },
        { 12, 11 }
    });

    // 묘지는 저주지대와 묘비 타일 중심.
    SetMapRect(map, 3, 12, 5, 14, ETileType::CursedGround);
    SetMapRect(map, 29, 0, 34, 2, ETileType::CursedGround);

    map.enemyStarts.push_back({ ECharacterClass::Warrior, L"Enemy Warrior", 32, 11, 10 });
    map.enemyStarts.push_back({ ECharacterClass::Mage,    L"Enemy Mage",    34, 12, 20 });
    map.enemyStarts.push_back({ ECharacterClass::Archer,  L"Enemy Archer",  36, 11, 18 });

    return map;
}

SBattleMapData BattleMapData::BuildStage02()
{
    SBattleMapData map;
    InitBaseMap(map);

    map.mapId = EBattleMapId::Stage02;
    map.mapName = L"Stage 02 - 천상 성전";
    map.backgroundFilePath = L"Source\\SMap\\background\\Test02.bmp";
    map.cameraStartX = 10;
    map.cameraStartY = 10;

    FillMapTiles(map, ETileType::Normal);

    // 천당/공중맵: 하늘/구름 빈 공간은 이동 불가 SkyVoid로 처리.
    SetMapRect(map, 0, 0, 4, 5, ETileType::SkyVoid);
    SetMapRect(map, 10, 1, 13, 6, ETileType::SkyVoid);
    SetMapRect(map, 21, 0, 24, 5, ETileType::SkyVoid);
    SetMapRect(map, 33, 2, 37, 7, ETileType::SkyVoid);
    SetMapRect(map, 4, 12, 7, 16, ETileType::SkyVoid);
    SetMapRect(map, 16, 15, 20, 19, ETileType::SkyVoid);
    SetMapRect(map, 29, 14, 33, 18, ETileType::SkyVoid);
    SetMapRect(map, 39, 11, 43, 16, ETileType::SkyVoid);

    AddPlayerStarts(map, {
        { 8, 10 },
        { 9, 11 },
        { 10, 10 },
        { 11, 11 },
        { 12, 10 }
    });

    map.enemyStarts.push_back({ ECharacterClass::Warrior, L"Sky Guard", 33, 10, 12 });
    map.enemyStarts.push_back({ ECharacterClass::Archer,  L"Sky Archer",34, 11, 20 });
    map.enemyStarts.push_back({ ECharacterClass::Mage,    L"Sky Mage",  35, 10, 22 });
    map.enemyStarts.push_back({ ECharacterClass::Mage,    L"Angel",     36, 11, 24 });

    return map;
}

SBattleMapData BattleMapData::BuildStage03()
{
    SBattleMapData map;
    InitBaseMap(map);

    map.mapId = EBattleMapId::Stage03;
    map.mapName = L"Stage 03 - 기본 방";
    map.backgroundFilePath = L"Source\\SMap\\background\\Test05.bmp";
    map.cameraStartX = 10;
    map.cameraStartY = 12;

    // 기본 방: 기믹 없는 전투 확인용.
    FillMapTiles(map, ETileType::Normal);

    AddPlayerStarts(map, {
        { 8, 12 },
        { 10, 12 },
        { 9, 13 },
        { 11, 13 },
        { 12, 12 }
    });

    map.enemyStarts.push_back({ ECharacterClass::Warrior, L"Training Guard", 32, 11, 10 });
    map.enemyStarts.push_back({ ECharacterClass::Mage,    L"Training Mage",  34, 12, 18 });
    map.enemyStarts.push_back({ ECharacterClass::Archer,  L"Training Archer",36, 11, 18 });

    return map;
}

SBattleMapData BattleMapData::BuildStage04()
{
    SBattleMapData map;
    InitBaseMap(map);

    map.mapId = EBattleMapId::Stage04;
    map.mapName = L"Stage 04 - 바다 폐허";
    map.backgroundFilePath = L"Source\\SMap\\background\\Test04.bmp";
    map.cameraStartX = 11;
    map.cameraStartY = 10;

    FillMapTiles(map, ETileType::Normal);

    // 바다맵: 중간중간 물 타일을 이동 불가 기믹으로 배치.
    SetMapRect(map, 0, 2, 6, 8, ETileType::Water);
    SetMapRect(map, 10, 2, 15, 6, ETileType::Water);
    SetMapRect(map, 21, 3, 26, 8, ETileType::Water);
    SetMapRect(map, 35, 1, 43, 7, ETileType::Water);
    SetMapRect(map, 4, 13, 8, 17, ETileType::Water);
    SetMapRect(map, 14, 12, 20, 18, ETileType::Water);
    SetMapRect(map, 26, 13, 32, 19, ETileType::Water);
    SetMapRect(map, 38, 13, 43, 20, ETileType::Water);

    AddPlayerStarts(map, {
        { 9, 10 },
        { 10, 11 },
        { 11, 10 },
        { 12, 11 },
        { 13, 10 }
    });

    map.enemyStarts.push_back({ ECharacterClass::Warrior, L"Coast Guard", 32, 10, 12 });
    map.enemyStarts.push_back({ ECharacterClass::Warrior, L"Coast Guard", 34, 11, 12 });
    map.enemyStarts.push_back({ ECharacterClass::Mage,    L"Tide Mage",   36, 10, 22 });
    map.enemyStarts.push_back({ ECharacterClass::Archer,  L"Coast Archer",35, 11, 20 });

    return map;
}

SBattleMapData BattleMapData::BuildStage05()
{
    SBattleMapData map;
    InitBaseMap(map);

    map.mapId = EBattleMapId::Stage05;
    map.mapName = L"Stage 05 - 용암지대";
    map.backgroundFilePath = L"Source\\SMap\\background\\Test03.bmp";
    map.cameraStartX = 12;
    map.cameraStartY = 10;

    FillMapTiles(map, ETileType::Normal);

    // 용암맵: 중간중간 용암 타일을 피해 기믹으로 배치.
    SetMapRect(map, 2, 3, 6, 6, ETileType::Lava);
    SetMapRect(map, 10, 4, 14, 7, ETileType::Lava);
    SetMapRect(map, 20, 3, 24, 6, ETileType::Lava);
    SetMapRect(map, 34, 3, 39, 7, ETileType::Lava);
    SetMapRect(map, 5, 13, 9, 16, ETileType::Lava);
    SetMapRect(map, 16, 14, 21, 17, ETileType::Lava);
    SetMapRect(map, 27, 13, 31, 18, ETileType::Lava);
    SetMapRect(map, 37, 15, 42, 19, ETileType::Lava);

    AddPlayerStarts(map, {
        { 10, 10 },
        { 11, 11 },
        { 12, 10 },
        { 13, 11 },
        { 14, 10 }
    });

    map.enemyStarts.push_back({ ECharacterClass::Warrior, L"Lava Guard", 32, 10, 14 });
    map.enemyStarts.push_back({ ECharacterClass::Warrior, L"Lava Guard", 34, 11, 14 });
    map.enemyStarts.push_back({ ECharacterClass::Mage,    L"Lava Mage",  36, 10, 24 });
    map.enemyStarts.push_back({ ECharacterClass::Archer,  L"Lava Archer",35, 11, 22 });
    map.enemyStarts.push_back({ ECharacterClass::Mage,    L"Lava Boss",  33, 12, 26 });

    return map;
}
