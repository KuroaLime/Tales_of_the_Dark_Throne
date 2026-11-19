#include "BattleMapData.h"
#include "resource.h"
#include "GraveyardMapLayout.h"
#include "CelestialMapLayout.h"
#include "SeaRuinsMapLayout.h"
#include "LavaMapLayout.h"
#include "HallMapLayout.h"

#include <algorithm>
#include <utility>
#include <cstdlib>
#include <ctime>
#include <vector>

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
    map.tileSize = 48;
    map.width = 33;
    map.height = 18;
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


static SBattleStartInfo MakeMonsterStart(
    ECharacterClass classType,
    const std::wstring& name,
    int x,
    int y,
    int spd,
    const std::wstring& sheetFile,
    bool sourceFacesRight,
    EElement element)
{
    SBattleStartInfo info;
    info.classType = classType;
    info.name = name;
    info.x = x;
    info.y = y;
    info.spd = spd;
    info.monsterSheetFile = sheetFile;
    info.monsterSourceFacesRight = sourceFacesRight;
    info.element = element;
    return info;
}

static SBattleStartInfo MakeRandomStage03Monster(int x, int y, int order)
{
    static bool seeded = false;
    if (!seeded)
    {
        std::srand((unsigned int)::time(nullptr));
        seeded = true;
    }

    std::vector<SBattleStartInfo> pool;

    pool.push_back(MakeMonsterStart(ECharacterClass::Warrior, L"Random Skeleton", 0, 0, 12, L"S1_Skeleton.bmp", true, EElement::Dark));
    pool.push_back(MakeMonsterStart(ECharacterClass::Mage,    L"Random Lich",     0, 0, 18, L"S1_Leach.bmp",   true, EElement::Dark));

    pool.push_back(MakeMonsterStart(ECharacterClass::Warrior, L"Random Angel Guard", 0, 0, 16, L"S2_Angel.bmp",  true,  EElement::Light));
    pool.push_back(MakeMonsterStart(ECharacterClass::Archer,  L"Random Baby Angel",  0, 0, 22, L"S2_BAngel.bmp", true,  EElement::Light));
    pool.push_back(MakeMonsterStart(ECharacterClass::Warrior, L"Random Falcon",      0, 0, 20, L"S2_Falcon.bmp", false, EElement::Light));
    pool.push_back(MakeMonsterStart(ECharacterClass::Mage,    L"Random Wing Angel",  0, 0, 18, L"S2_WAngel.bmp", true,  EElement::Light));

    pool.push_back(MakeMonsterStart(ECharacterClass::Warrior, L"Random Sea Slime",  0, 0, 14, L"S4_Slime.bmp", true, EElement::Water));
    pool.push_back(MakeMonsterStart(ECharacterClass::Mage,    L"Random Mermaid",    0, 0, 18, L"S4_Snale.bmp", true, EElement::Water));
    pool.push_back(MakeMonsterStart(ECharacterClass::Warrior, L"Random Spear Crab", 0, 0, 16, L"S4_Spear.bmp", true, EElement::Water));
    pool.push_back(MakeMonsterStart(ECharacterClass::Mage,    L"Random Jellyfish",  0, 0, 17, L"S4_WFly.bmp",  true, EElement::Water));

    pool.push_back(MakeMonsterStart(ECharacterClass::Mage,    L"Random Fire Demon", 0, 0, 18, L"S5_Dragon.bmp", true,  EElement::Fire));
    pool.push_back(MakeMonsterStart(ECharacterClass::Warrior, L"Random Lava Golem", 0, 0, 10, L"S5_Golem.bmp",  true,  EElement::Fire));
    pool.push_back(MakeMonsterStart(ECharacterClass::Warrior, L"Random Lava Hound", 0, 0, 20, L"S5_Hound.bmp",  false, EElement::Fire));
    pool.push_back(MakeMonsterStart(ECharacterClass::Warrior, L"Random Lava Larva", 0, 0, 12, L"S5_Larva.bmp",  true,  EElement::Fire));

    int index = 0;
    if (!pool.empty())
    {
        index = (std::rand() + order) % (int)pool.size();
    }

    SBattleStartInfo picked = pool[index];
    picked.x = x;
    picked.y = y;
    return picked;
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

    map.mapId = EBattleMapId::Stage01;
    map.mapName = L"Stage 01 - 묘지";
    map.gridAlignedBackground = true;
    map.backgroundFilePath = L"Source\\SMap\\background\\GraveyardScene.bmp";
    map.backgroundResourceId = IDB_GRAVEYARD_SCENE;
    map.tileAtlasFilePath = L"Source\\SMap\\background\\GraveyardAtlas.bmp";
    map.width = GraveyardMapLayout::Width;
    map.height = GraveyardMapLayout::Height;
    map.tileSize = GraveyardMapLayout::TileSize;
    map.cameraStartX = 8;
    map.cameraStartY = 8;
    map.drawTileDebug = false;
    // This river is a physical boundary: every element must use the bridges.
    map.allowElementalWaterTraversal = false;

    FillMapTiles(map, ETileType::Wall);
    for (int y = 0; y < map.height; ++y)
    {
        for (int x = 0; x < map.width; ++x)
        {
            ETileType type = ETileType::Wall;
            switch (GraveyardMapLayout::Rows[y][x])
            {
            case '.': type = ETileType::Normal; break;
            case '+': type = ETileType::Road; break;
            case '=': type = ETileType::Bridge; break;
            case '~': type = ETileType::Water; break;
            case 'B': type = ETileType::Building; break;
            case 'T': type = ETileType::Tree; break;
            case 'G': type = ETileType::Grave; break;
            case 'C': type = ETileType::CursedGround; break;
            case 'M': type = ETileType::MagicHazard; break;
            }
            SetMapPoint(map, x, y, type);
        }
    }

    // The unified scene follows this terrain mask. Its burial gardens and
    // architecture share the same blocked footprints; no repeated prop cards.
    AddPlayerStarts(map, {
        { 2, 8 },
        { 4, 7 },
        { 4, 9 },
        { 6, 7 },
        { 6, 9 }
    });

    map.enemyStarts.push_back(MakeMonsterStart(ECharacterClass::Warrior, L"Skeleton", 24, 8, 12, L"S1_Skeleton.bmp", true, EElement::Dark));
    map.enemyStarts.push_back(MakeMonsterStart(ECharacterClass::Mage,    L"Lich",     26, 9, 18, L"S1_Leach.bmp",   true, EElement::Dark));
    map.enemyStarts.push_back(MakeMonsterStart(ECharacterClass::Warrior, L"Skeleton", 28, 8, 12, L"S1_Skeleton.bmp", true, EElement::Dark));
    map.enemyStarts.push_back(MakeMonsterStart(ECharacterClass::Mage,    L"Lich",     23, 9, 18, L"S1_Leach.bmp",   true, EElement::Dark));

    return map;
}

SBattleMapData BattleMapData::BuildStage02()
{
    SBattleMapData map;

    map.mapId = EBattleMapId::Stage02;
    map.mapName = L"Stage 02 - 천상 성전";
    map.gridAlignedBackground = true;
    map.backgroundFilePath = L"Source\\SMap\\background\\CelestialScene.bmp";
    map.backgroundResourceId = IDB_CELESTIAL_SCENE;
    map.width = CelestialMapLayout::Width;
    map.height = CelestialMapLayout::Height;
    map.tileSize = CelestialMapLayout::TileSize;
    map.cameraStartX = 8;
    map.cameraStartY = 9;
    map.drawTileDebug = false;
    // All combatants use the stone decks, including Light-element angels.
    map.allowElementalSkyTraversal = false;

    FillMapTiles(map, ETileType::SkyVoid);
    for (int y = 0; y < map.height; ++y)
    {
        for (int x = 0; x < map.width; ++x)
        {
            ETileType type = ETileType::SkyVoid;
            switch (CelestialMapLayout::Rows[y][x])
            {
            case '.': type = ETileType::Normal; break;
            case '+': type = ETileType::Road; break;
            case '=': type = ETileType::Bridge; break;
            case '#': type = ETileType::Wall; break;
            case 'B': type = ETileType::Building; break;
            case 'T': type = ETileType::Tree; break;
            }
            SetMapPoint(map, x, y, type);
        }
    }

    AddPlayerStarts(map, {
        { 5, 8 },
        { 6, 7 },
        { 7, 9 },
        { 8, 7 },
        { 8, 10 }
    });

    map.enemyStarts.push_back(MakeMonsterStart(ECharacterClass::Warrior, L"Angel Guard", 26, 8, 16, L"S2_Angel.bmp",  true,  EElement::Light));
    map.enemyStarts.push_back(MakeMonsterStart(ECharacterClass::Archer,  L"Baby Angel",  29, 9, 22, L"S2_BAngel.bmp", true,  EElement::Light));
    map.enemyStarts.push_back(MakeMonsterStart(ECharacterClass::Warrior, L"Sky Falcon",  27, 11, 20, L"S2_Falcon.bmp", false, EElement::Light));
    map.enemyStarts.push_back(MakeMonsterStart(ECharacterClass::Mage,    L"Wing Angel",  28, 13, 18, L"S2_WAngel.bmp", true,  EElement::Light));

    return map;
}

SBattleMapData BattleMapData::BuildStage03()
{
    SBattleMapData map;

    map.mapId = EBattleMapId::Stage03;
    map.mapName = L"Stage 03 - 기본 방";
    map.gridAlignedBackground = true;
    map.backgroundFilePath = L"Source\\SMap\\background\\HallScene.bmp";
    map.backgroundResourceId = IDB_HALL_SCENE;
    map.width = HallMapLayout::Width;
    map.height = HallMapLayout::Height;
    map.tileSize = HallMapLayout::TileSize;
    map.cameraStartX = 7;
    map.cameraStartY = 9;
    map.drawTileDebug = false;

    FillMapTiles(map, ETileType::Wall);
    for (int y = 0; y < map.height; ++y)
    {
        for (int x = 0; x < map.width; ++x)
        {
            ETileType type = ETileType::Wall;
            switch (HallMapLayout::Rows[y][x])
            {
            case '.': type = ETileType::Normal; break;
            case '+': type = ETileType::Road; break;
            case '#': type = ETileType::Wall; break;
            case 'B': type = ETileType::Building; break;
            }
            SetMapPoint(map, x, y, type);
        }
    }

    AddPlayerStarts(map, {
        { 6, 6 },
        { 7, 5 },
        { 6, 8 },
        { 8, 6 },
        { 7, 10 }
    });

    map.enemyStarts.push_back(MakeRandomStage03Monster(25, 6, 0));
    map.enemyStarts.push_back(MakeRandomStage03Monster(26, 8, 1));
    map.enemyStarts.push_back(MakeRandomStage03Monster(25, 10, 2));
    map.enemyStarts.push_back(MakeRandomStage03Monster(23, 12, 3));

    return map;
}

SBattleMapData BattleMapData::BuildStage04()
{
    SBattleMapData map;

    map.mapId = EBattleMapId::Stage04;
    map.mapName = L"Stage 04 - 바다 폐허";
    map.gridAlignedBackground = true;
    map.backgroundFilePath = L"Source\\SMap\\background\\SeaRuinsScene.bmp";
    map.backgroundResourceId = IDB_SEA_RUINS_SCENE;
    map.width = SeaRuinsMapLayout::Width;
    map.height = SeaRuinsMapLayout::Height;
    map.tileSize = SeaRuinsMapLayout::TileSize;
    map.cameraStartX = 7;
    map.cameraStartY = 9;
    map.drawTileDebug = false;
    // Water-element enemies also follow the dry ruins and authored crossings.
    map.allowElementalWaterTraversal = false;

    FillMapTiles(map, ETileType::Water);
    for (int y = 0; y < map.height; ++y)
    {
        for (int x = 0; x < map.width; ++x)
        {
            ETileType type = ETileType::Water;
            switch (SeaRuinsMapLayout::Rows[y][x])
            {
            case '.': type = ETileType::Normal; break;
            case '+': type = ETileType::Road; break;
            case '=': type = ETileType::Bridge; break;
            case '#': type = ETileType::Wall; break;
            case 'B': type = ETileType::Building; break;
            case 'T': type = ETileType::Tree; break;
            }
            SetMapPoint(map, x, y, type);
        }
    }

    AddPlayerStarts(map, {
        { 4, 7 },
        { 5, 6 },
        { 4, 9 },
        { 7, 6 },
        { 8, 8 }
    });

    map.enemyStarts.push_back(MakeMonsterStart(ECharacterClass::Warrior, L"Sea Slime",  24, 8, 14, L"S4_Slime.bmp", true, EElement::Water));
    map.enemyStarts.push_back(MakeMonsterStart(ECharacterClass::Mage,    L"Mermaid",    28, 7, 18, L"S4_Snale.bmp", true, EElement::Water));
    map.enemyStarts.push_back(MakeMonsterStart(ECharacterClass::Warrior, L"Spear Crab", 28, 11, 16, L"S4_Spear.bmp", true, EElement::Water));
    map.enemyStarts.push_back(MakeMonsterStart(ECharacterClass::Mage,    L"Jellyfish",  25, 13, 17, L"S4_WFly.bmp",  true, EElement::Water));

    return map;
}

SBattleMapData BattleMapData::BuildStage05()
{
    SBattleMapData map;

    map.mapId = EBattleMapId::Stage05;
    map.mapName = L"Stage 05 - 용암지대";
    map.gridAlignedBackground = true;
    map.backgroundFilePath = L"Source\\SMap\\background\\LavaScene.bmp";
    map.backgroundResourceId = IDB_LAVA_SCENE;
    map.width = LavaMapLayout::Width;
    map.height = LavaMapLayout::Height;
    map.tileSize = LavaMapLayout::TileSize;
    map.cameraStartX = 7;
    map.cameraStartY = 9;
    map.drawTileDebug = false;
    // The molten caldera blocks every combatant, including Fire-element foes.
    map.allowLavaTraversal = false;

    FillMapTiles(map, ETileType::Lava);
    for (int y = 0; y < map.height; ++y)
    {
        for (int x = 0; x < map.width; ++x)
        {
            ETileType type = ETileType::Lava;
            switch (LavaMapLayout::Rows[y][x])
            {
            case '.': type = ETileType::Normal; break;
            case '+': type = ETileType::Road; break;
            case '=': type = ETileType::Bridge; break;
            case '#': type = ETileType::Wall; break;
            case 'B': type = ETileType::Building; break;
            }
            SetMapPoint(map, x, y, type);
        }
    }

    AddPlayerStarts(map, {
        { 4, 7 },
        { 5, 6 },
        { 4, 9 },
        { 7, 6 },
        { 8, 8 }
    });

    map.enemyStarts.push_back(MakeMonsterStart(ECharacterClass::Mage,    L"Fire Demon", 23, 7, 18, L"S5_Dragon.bmp", true,  EElement::Fire));
    map.enemyStarts.push_back(MakeMonsterStart(ECharacterClass::Warrior, L"Lava Golem", 27, 7, 10, L"S5_Golem.bmp",  true,  EElement::Fire));
    map.enemyStarts.push_back(MakeMonsterStart(ECharacterClass::Warrior, L"Lava Hound", 28, 11, 20, L"S5_Hound.bmp",  false, EElement::Fire));
    map.enemyStarts.push_back(MakeMonsterStart(ECharacterClass::Warrior, L"Lava Larva", 23, 12, 12, L"S5_Larva.bmp",  true,  EElement::Fire));

    return map;
}
