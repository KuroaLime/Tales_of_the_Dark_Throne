#pragma once
#include "UContainerWidget.h"
#include "Character.h"
class UProfilePanel : public UContainerWidget
{
private:
    Character* currentCharacter;
    RECT panelBounds;
public:
    UProfilePanel();
    void Setup(int x, int y);
    void SetPanelBounds(const RECT& bounds);
    void SetCharacter(Character* character);
    void Draw(HDC hMemDC, HDC hMemDC1) override;
};
