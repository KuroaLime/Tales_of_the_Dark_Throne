#pragma once
#include "UContainerWidget.h"
#include "Character.h"
#include "ItemData.h"
class UStatePanel : public UContainerWidget
{
private:
    Character* currentCharacter;
    RECT panelBounds;
public:
    UStatePanel();
    void Setup(int x, int y);
    void SetPanelBounds(const RECT& bounds);
    void SetCharacter(Character* character);
    void Draw(HDC hMemDC, HDC hMemDC1) override;
};
