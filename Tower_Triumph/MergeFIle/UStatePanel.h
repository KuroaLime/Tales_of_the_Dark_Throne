#pragma once

#include "UContainerWidget.h"
#include "UTextLabel.h"
#include "Character.h"
#include "ItemData.h"

class UStatePanel : public UContainerWidget
{
private:
    Character* CurrentCharacter;
    UTextLabel* StatLabels[8];
    UTextLabel* EquipLabels[3];

private:
    std::wstring GetEquipLine(EEquipPart part, int itemID) const;

public:
    UStatePanel();
    virtual ~UStatePanel();

    void Setup(int x, int y);
    void SetCharacter(Character* character);
    virtual void Draw(HDC hMemDC, HDC hMemDC1) override;
};
