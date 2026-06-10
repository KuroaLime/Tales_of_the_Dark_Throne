#pragma once
#include "Widget.h"

class Character; // Forward declaration

class UTurnCharacterInfo : public UWidget
{
private:
    Character* owner;
    bool isEnemy;
    bool isCurrentTurn;

public:
    UTurnCharacterInfo(Character* character, bool enemy);
    virtual void Draw(HDC hMemDC, HDC hMemDC1) override;
    
    void SetIsCurrentTurn(bool current) { isCurrentTurn = current; }
    Character* GetOwner() const { return owner; }
    void SetSize(int sx, int sy) { Size.sX = sx; Size.sY = sy; }
};
