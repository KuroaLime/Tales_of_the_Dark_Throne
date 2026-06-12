#include "STurnTimeline.h"

STurnTimeline::STurnTimeline()
{
}

STurnTimeline::~STurnTimeline()
{
}

void STurnTimeline::Init(const std::vector<Character*>& playerTeam, const std::vector<Character*>& enemyTeam)
{
    turnOrder.clear();

    for (auto c : playerTeam)
    {
        if (c != nullptr && c->IsAlive())
        {
            turnOrder.push_back({ c, false });
        }
    }

    for (auto c : enemyTeam)
    {
        if (c != nullptr && c->IsAlive())
        {
            turnOrder.push_back({ c, true });
        }
    }

    std::sort(
        turnOrder.begin(),
        turnOrder.end(),
        [](const TurnEntry& a, const TurnEntry& b)
        {
            if (a.character == nullptr)
            {
                return false;
            }

            if (b.character == nullptr)
            {
                return true;
            }

            return a.character->GetStat().SPD > b.character->GetStat().SPD;
        }
    );

    RemoveDeadUnits();
}

void STurnTimeline::NextTurn()
{
    RemoveDeadUnits();

    if (turnOrder.size() <= 1)
    {
        return;
    }

    TurnEntry current = turnOrder[0];

    turnOrder.erase(turnOrder.begin());

    if (current.character != nullptr && current.character->IsAlive())
    {
        turnOrder.push_back(current);
    }

    RemoveDeadUnits();
}

void STurnTimeline::RemoveDeadUnits()
{
    turnOrder.erase(
        std::remove_if(
            turnOrder.begin(),
            turnOrder.end(),
            [](const TurnEntry& entry)
            {
                return entry.character == nullptr || !entry.character->IsAlive();
            }
        ),
        turnOrder.end()
    );
}

bool STurnTimeline::HasAlivePlayer() const
{
    for (const auto& entry : turnOrder)
    {
        if (entry.character == nullptr)
        {
            continue;
        }

        if (!entry.character->IsAlive())
        {
            continue;
        }

        if (!entry.isEnemy)
        {
            return true;
        }
    }

    return false;
}

bool STurnTimeline::HasAliveEnemy() const
{
    for (const auto& entry : turnOrder)
    {
        if (entry.character == nullptr)
        {
            continue;
        }

        if (!entry.character->IsAlive())
        {
            continue;
        }

        if (entry.isEnemy)
        {
            return true;
        }
    }

    return false;
}

bool STurnTimeline::IsBattleFinished() const
{
    return !(HasAlivePlayer() && HasAliveEnemy());
}