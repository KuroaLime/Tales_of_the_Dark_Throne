#pragma once
#include <vector>
#include <algorithm>

class Character;

struct TurnEntry {
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
    
    const std::vector<TurnEntry>& GetTurnOrder() const { return turnOrder; }
    Character* GetCurrentTurnCharacter() const { return turnOrder.empty() ? nullptr : turnOrder[0].character; }
    bool IsCurrentTurnEnemy() const { return turnOrder.empty() ? false : turnOrder[0].isEnemy; }
};
