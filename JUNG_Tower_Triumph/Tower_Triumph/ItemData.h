#pragma once

#include "Character.h"

#include <string>
#include <vector>

enum class EGameItemType
{
    None,
    Consumable,
    Weapon,
    Armor,
    Accessory
};

struct SItemStatBonus
{
    int MaxHP;
    int MaxMP;
    int STR;
    int INT;
    int DEF;
    int SPD;

    SItemStatBonus()
        : MaxHP(0),
        MaxMP(0),
        STR(0),
        INT(0),
        DEF(0),
        SPD(0)
    {
    }

    SItemStatBonus(int maxHP, int maxMP, int str, int intel, int def, int spd)
        : MaxHP(maxHP),
        MaxMP(maxMP),
        STR(str),
        INT(intel),
        DEF(def),
        SPD(spd)
    {
    }
};

struct SItemData
{
    int ItemID;

    std::wstring Name;
    std::wstring Description;

    EGameItemType Type;
    EEquipPart EquipPart;

    int Price;

    int HealHP;
    int HealMP;

    bool Stackable;

    SItemStatBonus Bonus;

    SItemData()
        : ItemID(-1),
        Name(L"Unknown"),
        Description(L""),
        Type(EGameItemType::None),
        EquipPart(EEquipPart::Weapon),
        Price(0),
        HealHP(0),
        HealMP(0),
        Stackable(false),
        Bonus()
    {
    }

    SItemData(
        int itemID,
        const std::wstring& name,
        const std::wstring& description,
        EGameItemType type,
        EEquipPart equipPart,
        int price,
        int healHP,
        int healMP,
        bool stackable,
        const SItemStatBonus& bonus
    )
        : ItemID(itemID),
        Name(name),
        Description(description),
        Type(type),
        EquipPart(equipPart),
        Price(price),
        HealHP(healHP),
        HealMP(healMP),
        Stackable(stackable),
        Bonus(bonus)
    {
    }
};

class ItemData
{
public:
    static const SItemData* Find(int itemID);

    static std::wstring GetName(int itemID);
    static std::wstring GetDescription(int itemID);

    static EGameItemType GetType(int itemID);
    static EEquipPart GetEquipPart(int itemID);

    static int GetPrice(int itemID);
    static int GetHealHP(int itemID);
    static int GetHealMP(int itemID);

    static bool IsValidItem(int itemID);
    static bool IsConsumable(int itemID);
    static bool IsEquipment(int itemID);
    static bool IsStackable(int itemID);

    static SItemStatBonus GetBonus(int itemID);

    static const std::vector<SItemData>& GetAllItems();
};