#include "TeamManager.h"
#include "AArmor.h"
#include <algorithm>

TeamManager::TeamManager()
{
}

TeamManager::~TeamManager()
{
    for (auto c : Players) {
        delete c;
    }

    for (auto i : Inventory) {
        delete i;
    }
}

void TeamManager::RemoveItem(AItemBase* item)
{
    auto it = std::find(Inventory.begin(), Inventory.end(), item);
    if (it != Inventory.end()) {
        Inventory.erase(it);
    }
}

void TeamManager::EquipArmor(Character* target, AArmor* newArmor)
{
    if (!target || !newArmor) {
        return;
    }

    CharacterEquipment equip = target->GetEquipment();
    int itemID = newArmor->GetItemID();

    switch (newArmor->GetPart()) {
    case EArmorPart::Weapon:
        equip.WeaponID = itemID;
        break;

    case EArmorPart::Accessory:
        equip.AccessoryID = itemID;
        break;

    case EArmorPart::Helmet:
    case EArmorPart::Armor:
    case EArmorPart::Pants:
    default:
        equip.ArmorID = itemID;
        break;
    }

    target->SetEquipment(equip);
    RemoveItem(newArmor);
}

void TeamManager::UnequipArmor(Character* target, EArmorPart part)
{
    if (!target) {
        return;
    }

    CharacterEquipment equip = target->GetEquipment();

    switch (part) {
    case EArmorPart::Weapon:
        equip.WeaponID = -1;
        break;

    case EArmorPart::Accessory:
        equip.AccessoryID = -1;
        break;

    case EArmorPart::Helmet:
    case EArmorPart::Armor:
    case EArmorPart::Pants:
    default:
        equip.ArmorID = -1;
        break;
    }

    target->SetEquipment(equip);
}
