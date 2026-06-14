#pragma once

#include "Character.h"

#include <string>
#include <vector>

enum class EBattleMapId
{
    Test01,
    Stage01,
    Stage02,
    Stage03,
    Stage04,
    Stage05
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
    Lava,
    SkyVoid
};

struct SBattleStartInfo
{
    ECharacterClass classType = ECharacterClass::Warrior;
    std::wstring name = L"";
    int x = 0;
    int y = 0;
    int spd = 0;

    std::wstring monsterSheetFile = L"";

    bool monsterSourceFacesRight = true;

    EElement element = EElement::None;
};

struct SBattleMapData
{
    EBattleMapId mapId = EBattleMapId::Test01;
    std::wstring mapName = L"";

    int backgroundResourceId = 0;
    std::wstring backgroundFilePath = L"";

    int width = 0;
    int height = 0;
    int tileSize = 0;

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
    static EBattleMapId FromStageId(int stageId);
    static SBattleMapData LoadByStageId(int stageId);
    static SBattleMapData Load(EBattleMapId mapId);

private:
    static SBattleMapData BuildTest01();
    static SBattleMapData BuildStage01();
    static SBattleMapData BuildStage02();
    static SBattleMapData BuildStage03();
    static SBattleMapData BuildStage04();
    static SBattleMapData BuildStage05();
};
