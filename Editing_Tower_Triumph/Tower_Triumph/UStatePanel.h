#pragma once
#include "UContainerWidget.h"
#include "UImage.h"
#include "UTextLabel.h"
#include "UButton.h"
#include "Character.h"
#include <vector>
#include <functional>

class UStatePanel : public UContainerWidget
{
private:
    UImage* bgImage;
    UTextLabel* strLabel;
    UTextLabel* intLabel;
    UTextLabel* defLabel;
    UTextLabel* spdLabel;
    
    std::vector<UButton*> equipSlotButtons;
    Character* currentCharacter;

public:
    UStatePanel();
    void Setup(int x, int y);
    void SetCharacter(Character* character);
    
    std::function<void(int)> OnEquipSlotClicked;
};
