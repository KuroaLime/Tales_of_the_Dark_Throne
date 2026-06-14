#pragma once
#include "AItemBase.h"

class Character;
struct CharacterStat;

enum class EArmorPart
{
    Helmet,
    Armor,
    Pants,
    Weapon,
    Accessory
};

class AArmor : public AItemBase
{
private:
    EArmorPart Part;
    CharacterStat StatBonus;

public:
    AArmor();
    virtual ~AArmor() {}

    virtual void Use(Character* user) override;

    void Equip(Character* target);
    void Unequip(Character* target);

    void SetArmorData(EArmorPart part, const CharacterStat& stat)
    {
        Part = part;
        StatBonus = stat;
        ItemType = EItemType::Armor;
    }

    EArmorPart GetPart() const { return Part; }
    CharacterStat GetStatBonus() const { return StatBonus; }
};

