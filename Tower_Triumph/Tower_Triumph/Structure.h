#pragma once
//#include <windows.h>

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