#pragma once

#include <vector>

struct SBattleRewardItem
{
    int ItemID;
    int Count;

    SBattleRewardItem()
        : ItemID(-1), Count(0)
    {
    }

    SBattleRewardItem(int itemID, int count)
        : ItemID(itemID), Count(count)
    {
    }
};

class BattleRewardManager
{
private:
    bool HasRewardData;
    bool Victory;

    int ClearedStageId;
    int NextStageId;

    int RewardGold;
    int RewardExp;

    std::vector<SBattleRewardItem> RewardItems;

private:
    BattleRewardManager();

    BattleRewardManager(const BattleRewardManager&) = delete;
    BattleRewardManager& operator=(const BattleRewardManager&) = delete;

public:
    static BattleRewardManager& Get();

    void Clear();

    void SetReward(
        bool victory,
        int clearedStageId,
        int nextStageId,
        int gold,
        int exp,
        const std::vector<SBattleRewardItem>& items
    );

    void SetDefaultVictoryReward(int clearedStageId, int nextStageId);

    bool HasReward() const;
    bool IsVictory() const;

    int GetClearedStageId() const;
    int GetNextStageId() const;

    int GetRewardGold() const;
    int GetRewardExp() const;

    const std::vector<SBattleRewardItem>& GetRewardItems() const;
};
