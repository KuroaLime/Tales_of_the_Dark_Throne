#pragma once
#include "AItemBase.h"

class Character;

class AUsable : public AItemBase
{
private:
    int Amount;
    bool bIsHPPotion; // true: HP, false: MP

public:
    AUsable();
    virtual ~AUsable() {}

    virtual void Use(Character* user) override;

    void SetUsableData(int amount, bool isHP)
    {
        Amount = amount;
        bIsHPPotion = isHP;
        ItemType = EItemType::Usable;
    }
};

