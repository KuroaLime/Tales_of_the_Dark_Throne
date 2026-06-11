#pragma once

#include "Widget.h"
#include "Character.h"

class UTurnCharacterInfo : public UWidget
{
private:
    Character* owner;
    bool isEnemy;
    bool isCurrentTurn;

private:
    void DrawPortrait(HDC hMemDC, const RECT& portraitRect);

public:
    UTurnCharacterInfo(Character* character, bool enemy);
    virtual void Draw(HDC hMemDC, HDC hMemDC1) override;

    void SetIsCurrentTurn(bool current) { isCurrentTurn = current; }
    Character* GetOwner() const { return owner; }
    void SetSize(int sx, int sy) { Size.sX = sx; Size.sY = sy; }
};