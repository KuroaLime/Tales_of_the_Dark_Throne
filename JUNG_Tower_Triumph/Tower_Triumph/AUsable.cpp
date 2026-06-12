#include "AUsable.h"
#include "Character.h"

AUsable::AUsable()
{
    ItemType = EItemType::Usable;
    Amount = 0;
    bIsHPPotion = true;
}

void AUsable::Use(Character* user)
{
    if (!user) return;

    CharacterStat stat = user->GetStat();

    if (bIsHPPotion)
    {
        stat.CurHP += Amount;
        if (stat.CurHP > stat.MaxHP) stat.CurHP = stat.MaxHP;
    }
    else
    {
        stat.CurMP += Amount;
        if (stat.CurMP > stat.MaxMP) stat.CurMP = stat.MaxMP;
    }

    user->SetStat(stat);
    
    // 소모성 아이템이므로 사용 후 제거 로직은 인벤토리에서 처리 필요
}
