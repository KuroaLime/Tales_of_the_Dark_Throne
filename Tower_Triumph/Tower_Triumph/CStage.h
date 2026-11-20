#pragma once

#include <windows.h>
#include <vector>
#include <utility>

#include "Structure.h"

#define GAME_END -1
#define TITLE_STAGE 0
#define MAP_STAGE 1
#define BATTLE_STAGE 2
#define MAINTENANCE_STAGE 3

class CStage
{
protected:
    std::vector<std::vector<Tile>> Baord;

    int TILE_SIZE = 64;
    int W = 0;
    int H = 0;

    HBITMAP hBitBackground = NULL;
    BITMAP BackgroundInfo{};

    std::pair<Location, BuSIZE> Loc;
    RECT rects{};

    int CurStageID = -1;
    int NextStageID = -2;

public:
    virtual ~CStage() {}

    virtual void Init(HWND hWnd, HINSTANCE const g_hInst) = 0;
    virtual void Update(HWND hWnd) = 0;
    virtual void Draw(HDC hMemDC, HDC hMemDC1) = 0;
    // Kept outside scene snapshots so the pointer stays live during transitions.
    virtual void DrawCursor(HDC hMemDC, HDC hMemDC1) {}
    virtual int Input(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;
    virtual bool HandleSetCursor(HWND hWnd, WPARAM wParam, LPARAM lParam) { return false; }

    virtual void Comp_NextStage(int ID) = 0;
    virtual void BackgroundMove(int x, int y) = 0;
};
