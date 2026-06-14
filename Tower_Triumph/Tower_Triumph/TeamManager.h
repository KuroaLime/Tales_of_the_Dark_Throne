#pragma once
#include <vector>
#include "Character.h"
#include "AItemBase.h"
#include "AArmor.h"

class TeamManager
{
private:
    TeamManager();
    ~TeamManager();

    std::vector<Character*> Players;
    std::vector<AItemBase*> Inventory;

public:
    static TeamManager& Get()
    {
        static TeamManager instance;
        return instance;
    }

    void AddPlayer(Character* character) { Players.push_back(character); }
    const std::vector<Character*>& GetPlayers() const { return Players; }

    void AddItem(AItemBase* item) { Inventory.push_back(item); }
    void RemoveItem(AItemBase* item);
    const std::vector<AItemBase*>& GetInventory() const { return Inventory; }

    void EquipArmor(Character* target, class AArmor* newArmor);
    void UnequipArmor(Character* target, EArmorPart part);
};
