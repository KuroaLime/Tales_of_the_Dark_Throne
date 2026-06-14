#pragma once

#include "UContainerWidget.h"
#include "UButton.h"
#include "UProfilePanel.h"
#include "UStatePanel.h"
#include "Character.h"

#include <functional>
#include <vector>

class UCharacterInfoPopup : public UContainerWidget
{
private:
    Character* SelectedCharacter;
    std::vector<Character*> TeamMembers;

    UProfilePanel* ProfilePanel;
    UStatePanel* StatePanel;
    std::vector<UButton*> CharacterButtons;
    UButton* BackButton;

private:
    void RebuildCharacterButtons();

public:
    std::function<void()> OnBackCallback;

public:
    UCharacterInfoPopup();
    virtual ~UCharacterInfoPopup();

    virtual void Init() override;
    virtual void Draw(HDC hMemDC, HDC hMemDC1) override;

    void Setup(RECT clientRect);
    void SetTeamData(const std::vector<Character*>& team);
    void SelectCharacter(Character* character);
};
