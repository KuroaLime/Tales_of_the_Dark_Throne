#pragma once
#include "Structure.h"
#include <Windows.h>

class AActor
{
protected:
    Location GridPos;
    bool bIsActive;

public:
    AActor() : GridPos{ 0, 0 }, bIsActive(true) {}
    virtual ~AActor() {}

    virtual void Update(HWND hWnd) {}
    virtual void Draw(HDC hMemDC, HDC hBackDC) {}

    void SetGridPos(int x, int y) { GridPos.x = x; GridPos.y = y; }
    Location GetGridPos() const { return GridPos; }

    void SetActive(bool active) { bIsActive = active; }
    bool IsActive() const { return bIsActive; }
};

