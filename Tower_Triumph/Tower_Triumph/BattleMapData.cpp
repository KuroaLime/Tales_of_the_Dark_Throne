#include "BattleMapData.h"
#include "resource.h"

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
    InitBaseMap(map);

    map.mapId = EBattleMapId::Stage01;
    map.mapName = L"Stage 01 - 묘지";
    map.backgroundFilePath = L"Source\\SMap\\background\\Test01.bmp";
    map.cameraStartX = 15;
    map.cameraStartY = 10;

    // 첫 번째 묘지맵은 기존 하드코딩 장애물이 그림과 어긋나던 문제가 있어
    // 전투 확인용으로 일단 전체를 이동 가능 지형으로 초기화한다.
    FillMapTiles(map, ETileType::Normal);
    map.drawTileDebug = false;

    // 아군은 화면 시작 시 바로 보이도록 중앙-서쪽 개활지에 배치.
    AddPlayerStarts(map, {
        { 8, 11 },
        { 10, 11 },
        { 9, 12 },
        { 11, 12 },
        { 12, 11 }
    });

    // 묘지 기믹: 이동은 가능하지만 피해를 주는 저주지대/마법진만 배치.
    SetMapRect(map, 13, 10, 15, 12, ETileType::CursedGround);
    SetMapRect(map, 20, 12, 22, 14, ETileType::MagicHazard);
    SetMapRect(map, 3, 16, 5, 18, ETileType::CursedGround);

    map.enemyStarts.push_back(MakeMonsterStart(ECharacterClass::Warrior, L"Skeleton", 21, 10, 12, L"S1_Skeleton.bmp", true, EElement::Dark));
    map.enemyStarts.push_back(MakeMonsterStart(ECharacterClass::Mage,    L"Lich",     24, 11, 18, L"S1_Leach.bmp",   true, EElement::Dark));
    map.enemyStarts.push_back(MakeMonsterStart(ECharacterClass::Warrior, L"Skeleton", 26, 10, 12, L"S1_Skeleton.bmp", true, EElement::Dark));
    map.enemyStarts.push_back(MakeMonsterStart(ECharacterClass::Mage,    L"Lich",     23, 13, 18, L"S1_Leach.bmp",   true, EElement::Dark));

    return map;
}

SBattleMapData BattleMapData::BuildStage02()
{
    SBattleMapData map;
    InitBaseMap(map);

    map.mapId = EBattleMapId::Stage02;
    map.mapName = L"Stage 02 - 천상 성전";
    map.backgroundFilePath = L"Source\\SMap\\background\\Test02.bmp";
    map.cameraStartX = 15;
    map.cameraStartY = 10;

    // 천상맵은 기본을 SkyVoid로 깔고, 실제 발판만 Normal로 복구한다.
    // 따라서 빛속성 외 캐릭터는 발판 밖으로 이동할 수 없다.
    FillMapTiles(map, ETileType::SkyVoid);

    // 서쪽 시작 발판
    SetMapRect(map, 5, 8, 13, 12, ETileType::Normal);
    SetMapRect(map, 6, 13, 11, 15, ETileType::Normal);

    // 중앙 성전 발판
    SetMapRect(map, 15, 5, 26, 14, ETileType::Normal);
    SetMapRect(map, 18, 15, 24, 18, ETileType::Normal);

    // 동쪽 적 발판
    SetMapRect(map, 30, 8, 39, 14, ETileType::Normal);
    SetMapRect(map, 31, 15, 37, 17, ETileType::Normal);

    // 다리/연결로
    SetMapRect(map, 13, 9, 16, 11, ETileType::Normal);
    SetMapRect(map, 26, 9, 31, 11, ETileType::Normal);
    SetMapRect(map, 9, 16, 17, 18, ETileType::Normal);
    SetMapRect(map, 24, 16, 31, 18, ETileType::Normal);

    // 하단 보조 발판
    SetMapRect(map, 8, 18, 18, 22, ETileType::Normal);
    SetMapRect(map, 21, 18, 28, 22, ETileType::Normal);
    SetMapRect(map, 31, 18, 36, 22, ETileType::Normal);

    AddPlayerStarts(map, {
        { 7, 10 },
        { 8, 11 },
        { 9, 10 },
        { 10, 11 },
        { 11, 10 }
    });

    map.enemyStarts.push_back(MakeMonsterStart(ECharacterClass::Warrior, L"Angel Guard", 21, 10, 16, L"S2_Angel.bmp",  true,  EElement::Light));
    map.enemyStarts.push_back(MakeMonsterStart(ECharacterClass::Archer,  L"Baby Angel",  23, 11, 22, L"S2_BAngel.bmp", true,  EElement::Light));
    map.enemyStarts.push_back(MakeMonsterStart(ECharacterClass::Warrior, L"Sky Falcon",  25, 10, 20, L"S2_Falcon.bmp", false, EElement::Light));
    map.enemyStarts.push_back(MakeMonsterStart(ECharacterClass::Mage,    L"Wing Angel",  27, 11, 18, L"S2_WAngel.bmp", true,  EElement::Light));

    return map;
}

SBattleMapData BattleMapData::BuildStage03()
{
    SBattleMapData map;
    InitBaseMap(map);

    map.mapId = EBattleMapId::Stage03;
    map.mapName = L"Stage 03 - 기본 방";
    map.backgroundFilePath = L"Source\\SMap\\background\\Test05.bmp";
    map.cameraStartX = 15;
    map.cameraStartY = 10;

    // 기본 방: 기믹 없는 전투 확인용.
    FillMapTiles(map, ETileType::Normal);

    AddPlayerStarts(map, {
        { 7, 10 },
        { 8, 11 },
        { 9, 10 },
        { 10, 11 },
        { 11, 10 }
    });

    // Stage3은 전용 몬스터 없이 다른 스테이지 몬스터를 랜덤으로 사용한다.
    map.enemyStarts.push_back(MakeRandomStage03Monster(22, 10, 0));
    map.enemyStarts.push_back(MakeRandomStage03Monster(24, 11, 1));
    map.enemyStarts.push_back(MakeRandomStage03Monster(26, 10, 2));
    map.enemyStarts.push_back(MakeRandomStage03Monster(23, 13, 3));

    return map;
}

SBattleMapData BattleMapData::BuildStage04()
{
    SBattleMapData map;
    InitBaseMap(map);

    map.mapId = EBattleMapId::Stage04;
    map.mapName = L"Stage 04 - 바다 폐허";
    map.backgroundFilePath = L"Source\\SMap\\background\\Test04.bmp";
    map.cameraStartX = 15;
    map.cameraStartY = 10;

    FillMapTiles(map, ETileType::Normal);

    // 바다맵: 실제 이미지의 물길/침수 구역을 좌표로 지정.
    // 물속성 외 캐릭터는 Water에 진입할 수 없다.
    SetMapRect(map, 0, 0, 6, 8, ETileType::Water);
    SetMapRect(map, 11, 0, 14, 4, ETileType::Water);
    SetMapRect(map, 20, 0, 26, 7, ETileType::Water);
    SetMapRect(map, 35, 0, 43, 7, ETileType::Water);

    // 시작 지점 근처 물길: Move 테스트 시 바로 보이게 배치
    SetMapRect(map, 12, 10, 14, 12, ETileType::Water);

    SetMapRect(map, 4, 13, 8, 17, ETileType::Water);
    SetMapRect(map, 14, 13, 20, 18, ETileType::Water);
    SetMapRect(map, 24, 13, 31, 18, ETileType::Water);
    SetMapRect(map, 37, 12, 43, 20, ETileType::Water);

    SetMapRect(map, 0, 20, 7, 23, ETileType::Water);
    SetMapRect(map, 18, 20, 24, 23, ETileType::Water);
    SetMapRect(map, 33, 20, 43, 23, ETileType::Water);

    AddPlayerStarts(map, {
        { 7, 10 },
        { 8, 11 },
        { 9, 10 },
        { 10, 11 },
        { 11, 10 }
    });

    map.enemyStarts.push_back(MakeMonsterStart(ECharacterClass::Warrior, L"Sea Slime",  22, 10, 14, L"S4_Slime.bmp", true, EElement::Water));
    map.enemyStarts.push_back(MakeMonsterStart(ECharacterClass::Mage,    L"Mermaid",    24, 11, 18, L"S4_Snale.bmp", true, EElement::Water));
    map.enemyStarts.push_back(MakeMonsterStart(ECharacterClass::Warrior, L"Spear Crab", 26, 10, 16, L"S4_Spear.bmp", true, EElement::Water));
    map.enemyStarts.push_back(MakeMonsterStart(ECharacterClass::Mage,    L"Jellyfish",  25, 12, 17, L"S4_WFly.bmp",  true, EElement::Water));

    return map;
}

SBattleMapData BattleMapData::BuildStage05()
{
    SBattleMapData map;
    InitBaseMap(map);

    map.mapId = EBattleMapId::Stage05;
    map.mapName = L"Stage 05 - 용암지대";
    map.backgroundFilePath = L"Source\\SMap\\background\\Test03.bmp";
    map.cameraStartX = 15;
    map.cameraStartY = 10;

    FillMapTiles(map, ETileType::Normal);

    // 용암맵: 용암은 이동 가능하지만 불속성 외 유닛에게 피해를 준다.
    // 시작 지점 바로 동쪽에 용암을 둬서 Move/피해 테스트가 바로 되게 한다.
    SetMapRect(map, 12, 10, 15, 12, ETileType::Lava);

    SetMapRect(map, 2, 3, 6, 7, ETileType::Lava);
    SetMapRect(map, 10, 4, 14, 7, ETileType::Lava);
    SetMapRect(map, 20, 3, 24, 6, ETileType::Lava);
    SetMapRect(map, 34, 3, 39, 7, ETileType::Lava);

    SetMapRect(map, 5, 14, 10, 17, ETileType::Lava);
    SetMapRect(map, 17, 14, 22, 17, ETileType::Lava);
    SetMapRect(map, 27, 13, 31, 18, ETileType::Lava);
    SetMapRect(map, 37, 14, 42, 19, ETileType::Lava);

    AddPlayerStarts(map, {
        { 7, 10 },
        { 8, 11 },
        { 9, 10 },
        { 10, 11 },
        { 11, 10 }
    });

    map.enemyStarts.push_back(MakeMonsterStart(ECharacterClass::Mage,    L"Fire Demon", 22, 10, 18, L"S5_Dragon.bmp", true,  EElement::Fire));
    map.enemyStarts.push_back(MakeMonsterStart(ECharacterClass::Warrior, L"Lava Golem", 24, 11, 10, L"S5_Golem.bmp",  true,  EElement::Fire));
    map.enemyStarts.push_back(MakeMonsterStart(ECharacterClass::Warrior, L"Lava Hound", 26, 10, 20, L"S5_Hound.bmp",  false, EElement::Fire));
    map.enemyStarts.push_back(MakeMonsterStart(ECharacterClass::Warrior, L"Lava Larva", 25, 12, 12, L"S5_Larva.bmp",  true,  EElement::Fire));

    return map;
}
