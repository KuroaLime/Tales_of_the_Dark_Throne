#pragma once

#include "Character.h"

#include <vector>

struct InventoryItem
{
    int ItemID;
    int Count;

    InventoryItem()
        : ItemID(-1),
        Count(0)
    {
    }

    InventoryItem(int itemID, int count)
        : ItemID(itemID),
        Count(count)
    {
    }
};

class GameManager
{
private:
    std::vector<Character*> PlayerParty;
    std::vector<InventoryItem> Inventory;

    int PartyGold;
    int CurrentBattleStageId;
    int MaxUnlockedBattleStageId;

private:
    GameManager();

    GameManager(const GameManager&) = delete;
    GameManager& operator=(const GameManager&) = delete;

public:
    ~GameManager();

    static GameManager& Get();

    void Clear();

    void InitDefaultParty();
    bool HasParty() const;

    std::vector<Character*>& GetParty();
    const std::vector<Character*>& GetParty() const;

    int GetGold() const;
    void SetGold(int gold);
    void AddGold(int amount);
    bool SpendGold(int amount);
    bool BuyItem(int itemID, int count = 1);

    int GetCurrentBattleStageId() const;
    void SetCurrentBattleStageId(int stageId);

    int GetMaxUnlockedBattleStageId() const;
    bool IsBattleStageUnlocked(int stageId) const;
    void UnlockBattleStage(int stageId);

    void AddItem(int itemID, int count);
    bool RemoveItem(int itemID, int count);
    int GetItemCount(int itemID) const;
    bool HasItem(int itemID, int count = 1) const;

    const std::vector<InventoryItem>& GetInventory() const;
    void ClearInventory();

    bool EquipItem(Character* target, int itemID);
    bool EquipItem(Character* target, int itemID, EEquipPart part);
    bool UnequipItem(Character* target, EEquipPart part);

    bool UseConsumable(Character* target, int itemID);

    int GetEquippedItemID(Character* target, EEquipPart part) const;
};