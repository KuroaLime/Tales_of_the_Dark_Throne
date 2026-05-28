#pragma once
#include <windows.h>

struct Tile {
	int type;
	int height;
};
const COLORREF colorPalette[6] = {
	RGB(255, 255, 255), // EMPTY
	RGB(200, 200, 200), // MOVABLE_NORMAL
	RGB(255, 100, 100), // MOVABLE_LAVA
	RGB(50, 50, 50),    // MOVABLE_DARK
	RGB(255, 255, 150), // MOVABLE_LIGHT
	RGB(100, 100, 255)  // UNMOVABLE
};

struct MapConfig {
	int X;
	int Y;
	int TILE_W;
	int TILE_H;

	TCHAR mapName[256];
};