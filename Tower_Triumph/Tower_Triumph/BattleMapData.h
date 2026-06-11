#pragma once

#include "Character.h"

#include <vector>
#include <string>

enum class EBattleMapId
{
    Test01,
};

enum class ETileType
{
    Normal,
    Road,

    Water,
    Building,
    Wall,
    Fence,
    Tree,
    Grave,

    MagicHazard,
    CursedGround,
    Lava
};

struct SBattleStartInfo
{
    ECharacterClass classType;
    std::wstring name;

    int x;
    int y;
};

struct SBattleMapData
{
    EBattleMapId mapId = EBattleMapId::Test01;
    std::wstring mapName;

    int backgroundResourceId = 0;

    int width = 0;
    int height = 0;
    int tileSize = 64;

    int cameraStartX = 0;
    int cameraStartY = 0;

    bool drawTileDebug = false;

    std::vector<SBattleStartInfo> playerStarts;
    std::vector<SBattleStartInfo> enemyStarts;

    std::vector<std::vector<ETileType>> tiles;
};

class BattleMapData
{
public:
    static SBattleMapData Load(EBattleMapId mapId);

private:
    static SBattleMapData BuildTest01();
};
