#pragma once

#include "UContainerWidget.h"
#include "UTextLabel.h"
#include "Character.h"

class UProfilePanel : public UContainerWidget
{
private:
    Character* CurrentCharacter;
    UTextLabel* NameLabel;
    UTextLabel* LevelLabel;
    UTextLabel* ElementLabel;

public:
    UProfilePanel();
    virtual ~UProfilePanel();

    void Setup(int x, int y);
    void SetCharacter(Character* character);
    virtual void Draw(HDC hMemDC, HDC hMemDC1) override;
};
