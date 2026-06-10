#include "STurnTimeline.h"
#include "Character.h"
STurnTimeline::STurnTimeline()
{
}

STurnTimeline::~STurnTimeline()
{
}

void STurnTimeline::Init(const std::vector<Character*>& playerTeam, const std::vector<Character*>& enemyTeam)
{
    turnOrder.clear();

    for (auto c : playerTeam) {
        if (c && c->IsAlive()) {
            turnOrder.push_back({ c, false });
        }
    }

    for (auto c : enemyTeam) {
        if (c && c->IsAlive()) {
            turnOrder.push_back({ c, true });
        }
    }

    // Sort by SPD descending
    std::sort(turnOrder.begin(), turnOrder.end(), [](const TurnEntry& a, const TurnEntry& b) {
        return a.character->GetStat().SPD > b.character->GetStat().SPD;
    });
}

void STurnTimeline::NextTurn()
{
    if (turnOrder.size() <= 1) return;

    // Move the first element to the end
    TurnEntry current = turnOrder[0];
    turnOrder.erase(turnOrder.begin());
    turnOrder.push_back(current);
}
