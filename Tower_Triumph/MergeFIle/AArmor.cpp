#include "AArmor.h"
#include "Character.h"
#include <algorithm>

AArmor::AArmor()
{
    ItemType = EItemType::Armor;
    Part = EArmorPart::Armor;
}

void AArmor::Use(Character* user)
{
    // 장비 아이템의 경우 Use를 호출하면 장착하도록 처리
    Equip(user);
}

void AArmor::Equip(Character* target)
{
    if (!target) return;

    CharacterStat currentStat = target->GetStat();
    
    currentStat.MaxHP += StatBonus.MaxHP;
    currentStat.CurHP += StatBonus.MaxHP; // 최대치 증가 시 현재치도 보정
    currentStat.MaxMP += StatBonus.MaxMP;
    currentStat.CurMP += StatBonus.MaxMP;
    currentStat.STR += StatBonus.STR;
    currentStat.INT += StatBonus.INT;
    currentStat.DEF += StatBonus.DEF;
    currentStat.SPD += StatBonus.SPD;

    target->SetStat(currentStat);
}

void AArmor::Unequip(Character* target)
{
    if (!target) return;

    CharacterStat currentStat = target->GetStat();

    currentStat.MaxHP -= StatBonus.MaxHP;
    currentStat.CurHP = (std::min)(currentStat.CurHP, currentStat.MaxHP);
    currentStat.MaxMP -= StatBonus.MaxMP;
    currentStat.CurMP = (std::min)(currentStat.CurMP, currentStat.MaxMP);
    currentStat.STR -= StatBonus.STR;
    currentStat.INT -= StatBonus.INT;
    currentStat.DEF -= StatBonus.DEF;
    currentStat.SPD -= StatBonus.SPD;

    target->SetStat(currentStat);
}
