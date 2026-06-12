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
    std::vector<Character*> Roster;
    std::vector<Character*> PlayerParty;
    std::vector<InventoryItem> Inventory;

    static const int MAX_ACTIVE_PARTY = 5;

    int PartyGold;
    int CurrentBattleStageId;
    int MaxUnlockedBattleStageId;
    std::vector<int> CollectedMapItemIDs;

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

    std::vector<Character*>& GetRoster();
    const std::vector<Character*>& GetRoster() const;
    int GetMaxActivePartySize() const;
    bool IsInActiveParty(Character* character) const;
    bool AddActivePartyMember(Character* character);
    bool RemoveActivePartyMember(Character* character);
    bool ToggleActivePartyMember(Character* character);

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

    bool IsMapItemCollected(int mapItemId) const;
    void MarkMapItemCollected(int mapItemId);
    void ClearCollectedMapItems();

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