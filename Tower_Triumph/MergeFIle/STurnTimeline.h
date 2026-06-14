#pragma once

#include "Character.h"

#include <vector>
#include <algorithm>

struct TurnEntry
{
    Character* character;
    bool isEnemy;
};

class STurnTimeline
{
private:
    std::vector<TurnEntry> turnOrder;

public:
    STurnTimeline();
    ~STurnTimeline();

    void Init(const std::vector<Character*>& playerTeam, const std::vector<Character*>& enemyTeam);

    void NextTurn();

    void RemoveDeadUnits();

    bool IsBattleFinished() const;
    bool HasAlivePlayer() const;
    bool HasAliveEnemy() const;

    const std::vector<TurnEntry>& GetTurnOrder() const
    {
        return turnOrder;
    }

    Character* GetCurrentTurnCharacter() const
    {
        if (turnOrder.empty())
        {
            return nullptr;
        }

        return turnOrder[0].character;
    }

    bool IsCurrentTurnEnemy() const
    {
        if (turnOrder.empty())
        {
            return false;
        }

        return turnOrder[0].isEnemy;
    }
};