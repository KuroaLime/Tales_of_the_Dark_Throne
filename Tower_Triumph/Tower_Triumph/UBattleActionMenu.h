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

public:
    UBattleActionMenu();
    virtual ~UBattleActionMenu();

    virtual void Init() override;
    virtual void Draw(HDC hMemDC, HDC hMemDC1) override;
    
    // 캐릭터 위치를 기준으로 메뉴 위치 업데이트
    void UpdatePosition(int screenX, int screenY);

    // 버튼 활성화 상태 업데이트
    void UpdateButtonStates(class Character* character);

    // 콜백 설정
    void SetOnMoveClick(std::function<void()> callback) { btnMove->SetOnClick(callback); }
    void SetOnAttackClick(std::function<void()> callback) { btnAttack->SetOnClick(callback); }
    void SetOnSkillClick(std::function<void()> callback) { btnSkill->SetOnClick(callback); }
    void SetOnDefendClick(std::function<void()> callback) { btnDefend->SetOnClick(callback); }
    void SetOnItemClick(std::function<void()> callback) { btnItem->SetOnClick(callback); }
    void SetOnEndTurnClick(std::function<void()> callback) { btnEndTurn->SetOnClick(callback); }
};
