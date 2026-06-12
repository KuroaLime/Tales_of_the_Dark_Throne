#pragma once

#include "Character.h"

#include <string>
#include <vector>

enum class EBattleMapId
{
    Test01,
    Stage01,
    Stage02,
    Stage03
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
    int spd;
};

struct SBattleMapData
{
    EBattleMapId mapId;
    std::wstring mapName;

    int backgroundResourceId;

    int width;
    int height;
    int tileSize;

    int cameraStartX;
    int cameraStartY;

    bool drawTileDebug;

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
    static SBattleMapData BuildStage01();
    static SBattleMapData BuildStage02();
    static SBattleMapData BuildStage03();
};
