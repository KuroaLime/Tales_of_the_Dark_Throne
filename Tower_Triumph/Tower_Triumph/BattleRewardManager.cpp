#include "BattleRewardManager.h"

BattleRewardManager::BattleRewardManager()
{
    Clear();
}

BattleRewardManager& BattleRewardManager::Get()
{
    static BattleRewardManager instance;
    return instance;
}

void BattleRewardManager::Clear()
{
    HasRewardData = false;
    Victory = false;

    ClearedStageId = 0;
    NextStageId = 0;

    RewardGold = 0;
    RewardExp = 0;

    RewardItems.clear();
}

void BattleRewardManager::SetReward(
    bool victory,
    int clearedStageId,
    int nextStageId,
    int gold,
    int exp,
    const std::vector<SBattleRewardItem>& items
)
{
    HasRewardData = true;
    Victory = victory;

    ClearedStageId = clearedStageId;
    NextStageId = nextStageId;

    RewardGold = gold;
    RewardExp = exp;

    RewardItems = items;
}

void BattleRewardManager::SetDefaultVictoryReward(int clearedStageId, int nextStageId)
{
    std::vector<SBattleRewardItem> items;
    int gold = 0;
    int exp = 0;

    switch (clearedStageId)
    {
    case 1:
        gold = 150;
        exp = 80;
        items.push_back(SBattleRewardItem(201, 1)); 
        items.push_back(SBattleRewardItem(301, 1)); 
        items.push_back(SBattleRewardItem(101, 2)); 
        break;

    case 2:
        gold = 260;
        exp = 130;
        items.push_back(SBattleRewardItem(202, 1)); 
        items.push_back(SBattleRewardItem(302, 1));
        items.push_back(SBattleRewardItem(401, 1)); 
        items.push_back(SBattleRewardItem(102, 2)); 
        break;

    case 3:
        gold = 420;
        exp = 220;
        items.push_back(SBattleRewardItem(203, 1));
        items.push_back(SBattleRewardItem(402, 1));
        items.push_back(SBattleRewardItem(103, 2)); 
        break;

    case 4:
        gold = 520;
        exp = 300;
        items.push_back(SBattleRewardItem(202, 1)); 
        items.push_back(SBattleRewardItem(203, 1));
        items.push_back(SBattleRewardItem(102, 3)); 
        items.push_back(SBattleRewardItem(103, 2)); 
        break;

    case 5:
        gold = 700;
        exp = 450;
        items.push_back(SBattleRewardItem(302, 1)); 
        items.push_back(SBattleRewardItem(401, 1)); 
        items.push_back(SBattleRewardItem(402, 1)); 
        items.push_back(SBattleRewardItem(103, 3)); 
        items.push_back(SBattleRewardItem(102, 3)); 
        break;

    default:
        gold = 200;
        exp = 100;
        items.push_back(SBattleRewardItem(101, 2));
        break;
    }

    SetReward(
        true,
        clearedStageId,
        nextStageId,
        gold,
        exp,
        items
    );
}

bool BattleRewardManager::HasReward() const
{
    return HasRewardData;
}

bool BattleRewardManager::IsVictory() const
{
    return Victory;
}

int BattleRewardManager::GetClearedStageId() const
{
    return ClearedStageId;
}

int BattleRewardManager::GetNextStageId() const
{
    return NextStageId;
}

int BattleRewardManager::GetRewardGold() const
{
    return RewardGold;
}

int BattleRewardManager::GetRewardExp() const
{
    return RewardExp;
}

const std::vector<SBattleRewardItem>& BattleRewardManager::GetRewardItems() const
{
    return RewardItems;
}
