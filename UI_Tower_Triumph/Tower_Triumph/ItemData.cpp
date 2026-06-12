#include "ItemData.h"

#include <vector>

static std::vector<SItemData> BuildItemTable()
{
    std::vector<SItemData> items;

    items.push_back(
        SItemData(
            101,
            L"Small Potion",
            L"Recover 50 HP.",
            EGameItemType::Consumable,
            EEquipPart::Weapon,
            50,
            50,
            0,
            true,
            SItemStatBonus()
        )
    );

    items.push_back(
        SItemData(
            102,
            L"Mana Potion",
            L"Recover 30 MP.",
            EGameItemType::Consumable,
            EEquipPart::Weapon,
            70,
            0,
            30,
            true,
            SItemStatBonus()
        )
    );

    items.push_back(
        SItemData(
            103,
            L"Large Potion",
            L"Recover 120 HP.",
            EGameItemType::Consumable,
            EEquipPart::Weapon,
            130,
            120,
            0,
            true,
            SItemStatBonus()
        )
    );

    items.push_back(
        SItemData(
            201,
            L"Iron Sword",
            L"A basic sword for warriors.",
            EGameItemType::Weapon,
            EEquipPart::Weapon,
            200,
            0,
            0,
            false,
            SItemStatBonus(0, 0, 5, 0, 0, 0)
        )
    );

    items.push_back(
        SItemData(
            202,
            L"Hunter Bow",
            L"A bow for archers.",
            EGameItemType::Weapon,
            EEquipPart::Weapon,
            220,
            0,
            0,
            false,
            SItemStatBonus(0, 0, 3, 0, 0, 3)
        )
    );

    items.push_back(
        SItemData(
            203,
            L"Mage Staff",
            L"A staff that increases magic power.",
            EGameItemType::Weapon,
            EEquipPart::Weapon,
            240,
            0,
            0,
            false,
            SItemStatBonus(0, 10, 0, 6, 0, 0)
        )
    );

    items.push_back(
        SItemData(
            301,
            L"Leather Armor",
            L"Light armor with basic defense.",
            EGameItemType::Armor,
            EEquipPart::Armor,
            180,
            0,
            0,
            false,
            SItemStatBonus(10, 0, 0, 0, 3, 0)
        )
    );

    items.push_back(
        SItemData(
            302,
            L"Iron Armor",
            L"Heavy armor with strong defense.",
            EGameItemType::Armor,
            EEquipPart::Armor,
            300,
            0,
            0,
            false,
            SItemStatBonus(20, 0, 0, 0, 6, -1)
        )
    );

    items.push_back(
        SItemData(
            401,
            L"Power Ring",
            L"A ring that increases strength.",
            EGameItemType::Accessory,
            EEquipPart::Accessory,
            250,
            0,
            0,
            false,
            SItemStatBonus(0, 0, 3, 0, 0, 0)
        )
    );

    items.push_back(
        SItemData(
            402,
            L"Speed Ring",
            L"A ring that increases speed.",
            EGameItemType::Accessory,
            EEquipPart::Accessory,
            250,
            0,
            0,
            false,
            SItemStatBonus(0, 0, 0, 0, 0, 3)
        )
    );

    return items;
}

const std::vector<SItemData>& ItemData::GetAllItems()
{
    static std::vector<SItemData> itemTable = BuildItemTable();
    return itemTable;
}

const SItemData* ItemData::Find(int itemID)
{
    const std::vector<SItemData>& items = GetAllItems();

    for (const SItemData& item : items)
    {
        if (item.ItemID == itemID)
        {
            return &item;
        }
    }

    return nullptr;
}

std::wstring ItemData::GetName(int itemID)
{
    const SItemData* data = Find(itemID);

    if (data == nullptr)
    {
        return L"Unknown Item";
    }

    return data->Name;
}

std::wstring ItemData::GetDescription(int itemID)
{
    const SItemData* data = Find(itemID);

    if (data == nullptr)
    {
        return L"";
    }

    return data->Description;
}

EGameItemType ItemData::GetType(int itemID)
{
    const SItemData* data = Find(itemID);

    if (data == nullptr)
    {
        return EGameItemType::None;
    }

    return data->Type;
}

EEquipPart ItemData::GetEquipPart(int itemID)
{
    const SItemData* data = Find(itemID);

    if (data == nullptr)
    {
        return EEquipPart::Weapon;
    }

    return data->EquipPart;
}

int ItemData::GetPrice(int itemID)
{
    const SItemData* data = Find(itemID);

    if (data == nullptr)
    {
        return 0;
    }

    return data->Price;
}

int ItemData::GetHealHP(int itemID)
{
    const SItemData* data = Find(itemID);

    if (data == nullptr)
    {
        return 0;
    }

    return data->HealHP;
}

int ItemData::GetHealMP(int itemID)
{
    const SItemData* data = Find(itemID);

    if (data == nullptr)
    {
        return 0;
    }

    return data->HealMP;
}

bool ItemData::IsValidItem(int itemID)
{
    return Find(itemID) != nullptr;
}

bool ItemData::IsConsumable(int itemID)
{
    return GetType(itemID) == EGameItemType::Consumable;
}

bool ItemData::IsEquipment(int itemID)
{
    EGameItemType type = GetType(itemID);

    return type == EGameItemType::Weapon ||
        type == EGameItemType::Armor ||
        type == EGameItemType::Accessory;
}

bool ItemData::IsStackable(int itemID)
{
    const SItemData* data = Find(itemID);

    if (data == nullptr)
    {
        return false;
    }

    return data->Stackable;
}

SItemStatBonus ItemData::GetBonus(int itemID)
{
    const SItemData* data = Find(itemID);

    if (data == nullptr)
    {
        return SItemStatBonus();
    }

    return data->Bonus;
}