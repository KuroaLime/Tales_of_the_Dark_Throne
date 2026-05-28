#pragma once
#include <windows.h>
#include <tchar.h>
#include"resource.h"
#include<vector>
#include"CStructure.h"
#pragma comment(lib, "msimg32.lib")


class MapManager {
private:
	std::vector<std::vector<Tile>> m_mapData;
	int TILE_W = 128;
	int TILE_H = 64;
	int m_offsetX = 100;
	int m_offsetY = 50;

	HBITMAP Background;
	BITMAP BackgroundInfo;

	int totalPixelW;
	int totalPixelH;
	int W;
	int H;
public:
	void Reset(TCHAR* szFilePath/*DWORD Image*/);
	void RebuildMap(int w, int h, int tileW, int tileH);
	void SetBackground(TCHAR* szFilePath/*DWORD Image*/);
	POINT GetScreenPos(int x, int y);

	void Render(HDC hDC);
	void AddLoc(int x, int y);
	void ScreenToMap(int mouseX, int mouseY, HWND hWnd, int g_SelectedTileType);

	void Scrolling(bool bAdd);

	bool SaveMap(TCHAR* szFilePath);
	bool LoadMap(TCHAR* szFilePath);
};

