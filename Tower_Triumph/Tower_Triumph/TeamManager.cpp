#include "TeamManager.h"
#include "AArmor.h"
#include <algorithm>

TeamManager::TeamManager()
{
}

TeamManager::~TeamManager()
{
    for (auto c : Players) delete c;
    for (auto i : Inventory) delete i;
}

void TeamManager::RemoveItem(AItemBase* item)
{
    auto it = std::find(Inventory.begin(), Inventory.end(), item);
    if (it != Inventory.end())
    {
        Inventory.erase(it);
    }
}

void TeamManager::EquipArmor(Character* target, AArmor* newArmor)
{
    if (!target || !newArmor) return;

    CharacterEquipment equip = target->GetEquipment();
    AArmor* oldArmor = nullptr;

    // 부위에 맞는 기존 장비 확인 및 해제
    switch (newArmor->GetPart())
    {
    case EArmorPart::Helmet:
        oldArmor = equip.Helmet;
        equip.Helmet = newArmor;
        break;
    case EArmorPart::Armor:
        oldArmor = equip.Armor;
        equip.Armor = newArmor;
        break;
    case EArmorPart::Pants:
        oldArmor = equip.Pants;
        equip.Pants = newArmor;
        break;
    case EArmorPart::Weapon:
        oldArmor = equip.Weapon;
        equip.Weapon = newArmor;
        break;
    case EArmorPart::Accessory:
        oldArmor = equip.Accessory;
        equip.Accessory = newArmor;
        break;
    }

    // 기존 장비가 있었다면 해제하고 인벤토리에 추가
    if (oldArmor)
    {
        oldArmor->Unequip(target);
        AddItem(oldArmor);
    }

    // 새 장비 장착하고 인벤토리에서 제거
    newArmor->Equip(target);
    RemoveItem(newArmor);

    target->SetEquipment(equip);
}

void TeamManager::UnequipArmor(Character* target, EArmorPart part)
{
    if (!target) return;

    CharacterEquipment equip = target->GetEquipment();
    AArmor* oldArmor = nullptr;

    switch (part)
    {
    case EArmorPart::Helmet:
        oldArmor = equip.Helmet;
        equip.Helmet = nullptr;
        break;
    case EArmorPart::Armor:
        oldArmor = equip.Armor;
        equip.Armor = nullptr;
        break;
    case EArmorPart::Pants:
        oldArmor = equip.Pants;
        equip.Pants = nullptr;
        break;
    case EArmorPart::Weapon:
        oldArmor = equip.Weapon;
        equip.Weapon = nullptr;
        break;
    case EArmorPart::Accessory:
        oldArmor = equip.Accessory;
        equip.Accessory = nullptr;
        break;
    }

    if (oldArmor)
    {
        oldArmor->Unequip(target);
        AddItem(oldArmor);
        target->SetEquipment(equip);
    }
}
