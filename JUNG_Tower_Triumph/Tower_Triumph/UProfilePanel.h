#pragma once
#include "UContainerWidget.h"
#include "UImage.h"
#include "UTextLabel.h"
#include "Character.h"

class UProfilePanel : public UContainerWidget
{
private:
    UImage* bgImage;
    UTextLabel* nameLabel;
    UTextLabel* levelLabel;
    
    Character* currentCharacter;

public:
    UProfilePanel();
    void Setup(int x, int y);
    void SetCharacter(Character* character);
    virtual void Draw(HDC hMemDC, HDC hMemDC1) override;
};
