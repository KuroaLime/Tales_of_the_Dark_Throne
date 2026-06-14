#pragma once
#include <windows.h>
#include <tchar.h>

struct Location {
	int x;
	int y;
};

struct BuSIZE {
	int sX;
	int sY;
};

struct Tile {
	int type;
	int height;
};

struct MapConfig {
	int X;
	int Y;
	int TILE_W;
	int TILE_H;

	TCHAR mapName[256];
};

struct CharacterStat
{
    int MaxHP;
    int CurHP;

    int MaxMP;
    int CurMP;

    int STR;
    int INT;
    int DEF;
    int SPD;

    CharacterStat()
    {
        MaxHP = 100;
        CurHP = 100;

        MaxMP = 30;
        CurMP = 30;

        STR = 10;
        INT = 10;
        DEF = 5;
        SPD = 10;
    }
};
