#pragma once
#include "UContainerWidget.h"
#include "UImage.h"
#include "UTextLabel.h"
#include "Character.h"
#include "ItemData.h"

class UStatePanel : public UContainerWidget
{
private:
    UImage* bgImage;
    UTextLabel* weaponLabel;
    UTextLabel* armorLabel;
    UTextLabel* accessoryLabel;
    UTextLabel* strLabel;
    UTextLabel* intLabel;
    UTextLabel* defLabel;
    UTextLabel* spdLabel;

    Character* currentCharacter;

private:
    void SetupLabel(UTextLabel*& label, int x, int y, int w, int h, int fontSize, int weight, int align);

public:
    UStatePanel();
    virtual ~UStatePanel() {}

    void Setup(int x, int y);
    void SetCharacter(Character* character);
};
