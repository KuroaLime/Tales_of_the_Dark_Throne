#pragma once
#include "UContainerWidget.h"
#include "UButton.h"

class UBattleActionMenu : public UContainerWidget
{
private:
    UButton* btnMove;
    UButton* btnAttack;
    UButton* btnSkill;
    UButton* btnDefend;
    UButton* btnItem;
    UButton* btnEndTurn;

    RECT ScreenRect;
    int PanelWidth;
    int PanelHeight;

private:
    void SetupCommandButton(UButton* button, const std::wstring& text);
    void LayoutButtons(int panelLeft, int panelTop);
    int ClampInt(int value, int minValue, int maxValue) const;
    bool IsRectIntersect(const RECT& a, const RECT& b) const;
    bool IsCandidateUsable(const RECT& candidate, const RECT& avoidRect, int maxPanelBottom) const;

public:
    UBattleActionMenu();
    virtual ~UBattleActionMenu();

    virtual void Init() override;
    virtual void Draw(HDC hMemDC, HDC hMemDC1) override;

    void SetScreenRect(const RECT& clientRect);
    void UpdatePosition(int screenX, int screenY);
    void UpdateButtonStates(class Character* character);

    void SetOnMoveClick(std::function<void()> callback) { btnMove->SetOnClick(callback); }
    void SetOnAttackClick(std::function<void()> callback) { btnAttack->SetOnClick(callback); }
    void SetOnSkillClick(std::function<void()> callback) { btnSkill->SetOnClick(callback); }
    void SetOnDefendClick(std::function<void()> callback) { btnDefend->SetOnClick(callback); }
    void SetOnItemClick(std::function<void()> callback) { btnItem->SetOnClick(callback); }
    void SetOnEndTurnClick(std::function<void()> callback) { btnEndTurn->SetOnClick(callback); }
};
