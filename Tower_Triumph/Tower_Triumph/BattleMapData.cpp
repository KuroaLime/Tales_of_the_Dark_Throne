#include "BattleMapData.h"
#include "resource.h"

#include <algorithm>

static void InitBaseMap(SBattleMapData& map)
{
    map.backgroundResourceId = IDB_BITMAP3;
    map.tileSize = 64;
    map.width = 44;
    map.height = 24;
    map.cameraStartX = 12;
    map.cameraStartY = 18;
    map.drawTileDebug = true;

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
    SetRect(24, 5, 27, 10, ETileType::Tree);
    SetRect(31, 7, 35, 14, ETileType::Tree);
    SetRect(19, 12, 23, 16, ETileType::Tree);
    SetRect(16, 14, 19, 18, ETileType::Tree);
    SetRect(0, 20, 3, 23, ETileType::Tree);
    SetRect(23, 20, 25, 23, ETileType::Tree);
    SetRect(34, 20, 37, 23, ETileType::Tree);

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

    default:
        return BuildStage01();
    }
}

SBattleMapData BattleMapData::BuildTest01()
{
    return BuildStage01();
}

SBattleMapData BattleMapData::BuildStage01()
{
    SBattleMapData map;
    InitBaseMap(map);

    map.mapId = EBattleMapId::Stage01;
    map.mapName = L"Stage 01 - Graveyard Entrance";

    map.playerStarts.push_back({ ECharacterClass::Warrior, L"Warrior", 6, 19, 15 });
    map.playerStarts.push_back({ ECharacterClass::Archer,  L"Archer",  8, 19, 25 });
    map.playerStarts.push_back({ ECharacterClass::Mage,    L"Mage",    7, 18, 18 });
    map.playerStarts.push_back({ ECharacterClass::Warrior, L"Warrior", 5, 19, 15 });
    map.playerStarts.push_back({ ECharacterClass::Mage,    L"Mage",    9, 18, 18 });

    map.enemyStarts.push_back({ ECharacterClass::Warrior, L"Enemy Warrior", 36, 18, 10 });
    map.enemyStarts.push_back({ ECharacterClass::Mage,    L"Enemy Mage",    34, 19, 20 });
    map.enemyStarts.push_back({ ECharacterClass::Archer,  L"Enemy Archer",  38, 18, 18 });

    return map;
}

SBattleMapData BattleMapData::BuildStage02()
{
    SBattleMapData map;
    InitBaseMap(map);

    map.mapId = EBattleMapId::Stage02;
    map.mapName = L"Stage 02 - Cursed Library";

    map.playerStarts.push_back({ ECharacterClass::Warrior, L"Warrior", 6, 19, 15 });
    map.playerStarts.push_back({ ECharacterClass::Archer,  L"Archer",  8, 19, 25 });
    map.playerStarts.push_back({ ECharacterClass::Mage,    L"Mage",    7, 18, 18 });
    map.playerStarts.push_back({ ECharacterClass::Warrior, L"Warrior", 5, 19, 15 });
    map.playerStarts.push_back({ ECharacterClass::Mage,    L"Mage",    9, 18, 18 });

    map.enemyStarts.push_back({ ECharacterClass::Warrior, L"Stage2 Guard", 35, 18, 12 });
    map.enemyStarts.push_back({ ECharacterClass::Warrior, L"Stage2 Guard", 37, 19, 12 });
    map.enemyStarts.push_back({ ECharacterClass::Mage,    L"Stage2 Mage",  34, 16, 22 });
    map.enemyStarts.push_back({ ECharacterClass::Archer,  L"Stage2 Archer", 39, 18, 20 });

    return map;
}

SBattleMapData BattleMapData::BuildStage03()
{
    SBattleMapData map;
    InitBaseMap(map);

    map.mapId = EBattleMapId::Stage03;
    map.mapName = L"Stage 03 - Final Graveyard";

    map.playerStarts.push_back({ ECharacterClass::Warrior, L"Warrior", 6, 19, 15 });
    map.playerStarts.push_back({ ECharacterClass::Archer,  L"Archer",  8, 19, 25 });
    map.playerStarts.push_back({ ECharacterClass::Mage,    L"Mage",    7, 18, 18 });
    map.playerStarts.push_back({ ECharacterClass::Warrior, L"Warrior", 5, 19, 15 });
    map.playerStarts.push_back({ ECharacterClass::Mage,    L"Mage",    9, 18, 18 });

    map.enemyStarts.push_back({ ECharacterClass::Warrior, L"Final Knight", 35, 18, 14 });
    map.enemyStarts.push_back({ ECharacterClass::Warrior, L"Final Knight", 37, 19, 14 });
    map.enemyStarts.push_back({ ECharacterClass::Mage,    L"Final Mage",   34, 16, 24 });
    map.enemyStarts.push_back({ ECharacterClass::Archer,  L"Final Archer", 39, 18, 22 });
    map.enemyStarts.push_back({ ECharacterClass::Mage,    L"Final Boss",   36, 16, 26 });

    return map;
}
