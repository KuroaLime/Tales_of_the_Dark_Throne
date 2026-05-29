#include "Character.h"

Character::Character()
{
    CharacterID = -1;

    ClassType = ECharacterClass::Warrior;
    State = ECharacterState::Idle;
    Direction = EDirection::Down;
    Element = EElement::None;

    Level = 1;
    Exp = 0;
    Money = 0;

    GridPos.x = 0;
    GridPos.y = 0;

    PixelPos.x = 0;
    PixelPos.y = 0;

    AP = 3;
    DefendRate = 0.5f;

    CurFrame = 0;
    MaxFrame = 4;

    AniTick = 0;
    AniDelay = 8;

    IsActionEnd = false;

    hSpriteIdle = NULL;
    hSpriteWalk = NULL;
    hSpriteAttack = NULL;
    hSpriteDefend = NULL;
    hSpriteSkill = NULL;
    hSpriteHit = NULL;
    hSpriteDead = NULL;

    ZeroMemory(&SpriteInfoIdle, sizeof(BITMAP));
    ZeroMemory(&SpriteInfoWalk, sizeof(BITMAP));
    ZeroMemory(&SpriteInfoAttack, sizeof(BITMAP));
    ZeroMemory(&SpriteInfoDefend, sizeof(BITMAP));
    ZeroMemory(&SpriteInfoSkill, sizeof(BITMAP));
    ZeroMemory(&SpriteInfoHit, sizeof(BITMAP));
    ZeroMemory(&SpriteInfoDead, sizeof(BITMAP));
}

Character::~Character()
{
    Release();
}

// =====================================================
// 초기화 / 해제
// =====================================================

void Character::Init(ECharacterClass newClass, int gridX, int gridY)
{
    ClassType = newClass;

    State = ECharacterState::Idle;
    Direction = EDirection::Down;
    Element = EElement::None;

    Level = 1;
    Exp = 0;
    Money = 0;

    GridPos.x = gridX;
    GridPos.y = gridY;

    // 일단 기본값
    PixelPos.x = gridX;
    PixelPos.y = gridY;

    CurFrame = 0;
    MaxFrame = 4;
    AniTick = 0;
    AniDelay = 8;
    IsActionEnd = false;

    LearnedSkillIDs.clear();

    InitClassValue();
}

void Character::Release()
{
    if (hSpriteIdle)
    {
        DeleteObject(hSpriteIdle);
        hSpriteIdle = NULL;
    }

    if (hSpriteWalk)
    {
        DeleteObject(hSpriteWalk);
        hSpriteWalk = NULL;
    }

    if (hSpriteAttack)
    {
        DeleteObject(hSpriteAttack);
        hSpriteAttack = NULL;
    }

    if (hSpriteDefend)
    {
        DeleteObject(hSpriteDefend);
        hSpriteDefend = NULL;
    }

    if (hSpriteSkill)
    {
        DeleteObject(hSpriteSkill);
        hSpriteSkill = NULL;
    }

    if (hSpriteHit)
    {
        DeleteObject(hSpriteHit);
        hSpriteHit = NULL;
    }

    if (hSpriteDead)
    {
        DeleteObject(hSpriteDead);
        hSpriteDead = NULL;
    }
}

// =====================================================
// 직업별 기본값
// =====================================================

void Character::InitClassValue()
{
    switch (ClassType)
    {
    case ECharacterClass::Warrior:
        AP = 3;
        DefendRate = 0.45f;

        Stat.MaxHP = 130;
        Stat.CurHP = Stat.MaxHP;

        Stat.MaxMP = 20;
        Stat.CurMP = Stat.MaxMP;

        Stat.STR = 16;
        Stat.INT = 5;
        Stat.DEF = 12;
        Stat.SPD = 8;
        break;

    case ECharacterClass::Archer:
        AP = 4;
        DefendRate = 0.65f;

        Stat.MaxHP = 100;
        Stat.CurHP = Stat.MaxHP;

        Stat.MaxMP = 25;
        Stat.CurMP = Stat.MaxMP;

        Stat.STR = 13;
        Stat.INT = 7;
        Stat.DEF = 8;
        Stat.SPD = 13;
        break;

    case ECharacterClass::Mage:
        AP = 3;
        DefendRate = 0.75f;

        Stat.MaxHP = 80;
        Stat.CurHP = Stat.MaxHP;

        Stat.MaxMP = 60;
        Stat.CurMP = Stat.MaxMP;

        Stat.STR = 5;
        Stat.INT = 17;
        Stat.DEF = 5;
        Stat.SPD = 9;
        break;
    }
}

// =====================================================
// 스프라이트 로드
// =====================================================

bool Character::LoadBitmapFromFile(const wchar_t* filePath, HBITMAP& outBitmap, BITMAP& outInfo)
{
    outBitmap = (HBITMAP)LoadImageW(
        NULL,
        filePath,
        IMAGE_BITMAP,
        0,
        0,
        LR_LOADFROMFILE | LR_CREATEDIBSECTION
    );

    if (outBitmap == NULL)
    {
        ZeroMemory(&outInfo, sizeof(BITMAP));
        return false;
    }

    GetObject(outBitmap, sizeof(BITMAP), &outInfo);
    return true;
}

bool Character::LoadSprites(const std::wstring& folderPath)
{
    Release();

    std::wstring prefix;

    switch (ClassType)
    {
    case ECharacterClass::Warrior:
        prefix = L"Warrior";
        break;

    case ECharacterClass::Archer:
        prefix = L"Archer";
        break;

    case ECharacterClass::Mage:
        prefix = L"Mage";
        break;
    }

    std::wstring basePath = folderPath;

    if (!basePath.empty())
    {
        wchar_t lastChar = basePath[basePath.length() - 1];

        if (lastChar != L'/' && lastChar != L'\\')
        {
            basePath += L"\\";
        }
    }

    bool result = true;

    result &= LoadBitmapFromFile((basePath + prefix + L"_Idle.bmp").c_str(), hSpriteIdle, SpriteInfoIdle);
    result &= LoadBitmapFromFile((basePath + prefix + L"_Walk.bmp").c_str(), hSpriteWalk, SpriteInfoWalk);
    result &= LoadBitmapFromFile((basePath + prefix + L"_Attack.bmp").c_str(), hSpriteAttack, SpriteInfoAttack);
    result &= LoadBitmapFromFile((basePath + prefix + L"_Defend.bmp").c_str(), hSpriteDefend, SpriteInfoDefend);
    result &= LoadBitmapFromFile((basePath + prefix + L"_Skill.bmp").c_str(), hSpriteSkill, SpriteInfoSkill);
    result &= LoadBitmapFromFile((basePath + prefix + L"_Hit.bmp").c_str(), hSpriteHit, SpriteInfoHit);
    result &= LoadBitmapFromFile((basePath + prefix + L"_Dead.bmp").c_str(), hSpriteDead, SpriteInfoDead);

    return result;
}

// =====================================================
// 현재 상태 스프라이트 얻기
// =====================================================

HBITMAP Character::GetCurrentSprite() const
{
    switch (State)
    {
    case ECharacterState::Idle:
        return hSpriteIdle;

    case ECharacterState::Walk:
        return hSpriteWalk;

    case ECharacterState::Attack:
        return hSpriteAttack;

    case ECharacterState::Defend:
        return hSpriteDefend;

    case ECharacterState::Skill:
        return hSpriteSkill;

    case ECharacterState::Hit:
        return hSpriteHit;

    case ECharacterState::Dead:
        return hSpriteDead;
    }

    return hSpriteIdle;
}

BITMAP Character::GetCurrentSpriteInfo() const
{
    switch (State)
    {
    case ECharacterState::Idle:
        return SpriteInfoIdle;

    case ECharacterState::Walk:
        return SpriteInfoWalk;

    case ECharacterState::Attack:
        return SpriteInfoAttack;

    case ECharacterState::Defend:
        return SpriteInfoDefend;

    case ECharacterState::Skill:
        return SpriteInfoSkill;

    case ECharacterState::Hit:
        return SpriteInfoHit;

    case ECharacterState::Dead:
        return SpriteInfoDead;
    }

    return SpriteInfoIdle;
}

// =====================================================
// 방향 처리
// =====================================================

int Character::GetDirectionRow() const
{
    switch (Direction)
    {
    case EDirection::Down:
        return 0;

    case EDirection::Left:
        return 1;

    case EDirection::Right:
        return 1;

    case EDirection::Up:
        return 2;
    }

    return 0;
}

bool Character::IsNeedFlip() const
{
    // 오른쪽을 볼 때만 좌우 반전.
    if (Direction == EDirection::Right)
    {
        return true;
    }

    return false;
}

void Character::UpdateMaxFrameByState()
{
    MaxFrame = CHARACTER_SHEET_COL;

    if (CurFrame >= MaxFrame)
    {
        CurFrame = 0;
    }
}

// =====================================================
// 갱신 / 출력
// =====================================================

void Character::Update()
{
    if (Stat.CurHP <= 0 && State != ECharacterState::Dead)
    {
        SetState(ECharacterState::Dead);
    }

    AniTick++;

    if (AniTick < AniDelay)
    {
        return;
    }

    AniTick = 0;

    CurFrame++;

    if (CurFrame >= MaxFrame)
    {
        switch (State)
        {
        case ECharacterState::Idle:
        case ECharacterState::Walk:
            CurFrame = 0;
            break;

        case ECharacterState::Attack:
        case ECharacterState::Defend:
        case ECharacterState::Skill:
        case ECharacterState::Hit:
            CurFrame = MaxFrame - 1;
            IsActionEnd = true;
            break;

        case ECharacterState::Dead:
            CurFrame = MaxFrame - 1;
            IsActionEnd = true;
            break;
        }
    }
}

void Character::Draw(HDC hMemDC, HDC hBackDC)
{
    HBITMAP curSprite = GetCurrentSprite();

    if (curSprite == NULL)
    {
        return;
    }

    int row = GetDirectionRow();

    int srcX = CurFrame * CHARACTER_FRAME_WIDTH;
    int srcY = row * CHARACTER_FRAME_HEIGHT;

    int dstX = PixelPos.x;
    int dstY = PixelPos.y;

    int dstW = CHARACTER_FRAME_WIDTH;
    int dstH = CHARACTER_FRAME_HEIGHT;

    HBITMAP oldBitmap = (HBITMAP)SelectObject(hMemDC, curSprite);

    if (!IsNeedFlip())
    {
        TransparentBlt(
            hBackDC,
            dstX,
            dstY,
            dstW,
            dstH,
            hMemDC,
            srcX,
            srcY,
            CHARACTER_FRAME_WIDTH,
            CHARACTER_FRAME_HEIGHT,
            CHARACTER_TRANSPARENT_COLOR
        );
    }
    else
    {
        // 좌우 반전 처리를 위한 임시 DC / 임시 비트맵
        HDC hFlipDC = CreateCompatibleDC(hBackDC);
        HBITMAP hFlipBitmap = CreateCompatibleBitmap(
            hBackDC,
            CHARACTER_FRAME_WIDTH,
            CHARACTER_FRAME_HEIGHT
        );

        HBITMAP hOldFlipBitmap = (HBITMAP)SelectObject(hFlipDC, hFlipBitmap);

        // 임시 비트맵 배경을 마젠타로 채움
        HBRUSH hBrush = CreateSolidBrush(CHARACTER_TRANSPARENT_COLOR);
        RECT rc;
        rc.left = 0;
        rc.top = 0;
        rc.right = CHARACTER_FRAME_WIDTH;
        rc.bottom = CHARACTER_FRAME_HEIGHT;
        FillRect(hFlipDC, &rc, hBrush);
        DeleteObject(hBrush);

        // StretchBlt의 음수 폭을 이용해서 좌우 반전 복사
        StretchBlt(
            hFlipDC,
            CHARACTER_FRAME_WIDTH,
            0,
            -CHARACTER_FRAME_WIDTH,
            CHARACTER_FRAME_HEIGHT,
            hMemDC,
            srcX,
            srcY,
            CHARACTER_FRAME_WIDTH,
            CHARACTER_FRAME_HEIGHT,
            SRCCOPY
        );

        TransparentBlt(
            hBackDC,
            dstX,
            dstY,
            dstW,
            dstH,
            hFlipDC,
            0,
            0,
            CHARACTER_FRAME_WIDTH,
            CHARACTER_FRAME_HEIGHT,
            CHARACTER_TRANSPARENT_COLOR
        );

        SelectObject(hFlipDC, hOldFlipBitmap);
        DeleteObject(hFlipBitmap);
        DeleteDC(hFlipDC);
    }

    SelectObject(hMemDC, oldBitmap);
}

// =====================================================
// 상태 / 방향
// =====================================================

void Character::SetState(ECharacterState newState)
{
    if (State == newState)
    {
        return;
    }

    State = newState;

    CurFrame = 0;
    AniTick = 0;
    IsActionEnd = false;

    UpdateMaxFrameByState();
}

ECharacterState Character::GetState() const
{
    return State;
}

void Character::SetDirection(EDirection newDirection)
{
    Direction = newDirection;
}

EDirection Character::GetDirection() const
{
    return Direction;
}

bool Character::GetIsActionEnd() const
{
    return IsActionEnd;
}

void Character::SetIsActionEnd(bool newValue)
{
    IsActionEnd = newValue;
}

// =====================================================
// 위치
// =====================================================

void Character::SetGridPos(int x, int y)
{
    GridPos.x = x;
    GridPos.y = y;
}

Location Character::GetGridPos() const
{
    return GridPos;
}

void Character::SetPixelPos(int x, int y)
{
    PixelPos.x = x;
    PixelPos.y = y;
}

Location Character::GetPixelPos() const
{
    return PixelPos;
}

// =====================================================
// 기본 정보
// =====================================================

void Character::SetCharacterID(int id)
{
    CharacterID = id;
}

int Character::GetCharacterID() const
{
    return CharacterID;
}

ECharacterClass Character::GetClassType() const
{
    return ClassType;
}

void Character::SetElement(EElement newElement)
{
    Element = newElement;
}

EElement Character::GetElement() const
{
    return Element;
}

// =====================================================
// 스탯 / 성장
// =====================================================

CharacterStat Character::GetStat() const
{
    return Stat;
}

void Character::SetStat(const CharacterStat& newStat)
{
    Stat = newStat;
}

int Character::GetLevel() const
{
    return Level;
}

void Character::SetLevel(int newLevel)
{
    if (newLevel < 1)
    {
        newLevel = 1;
    }

    Level = newLevel;
}

int Character::GetExp() const
{
    return Exp;
}

void Character::AddExp(int addExp)
{
    if (addExp <= 0)
    {
        return;
    }

    Exp += addExp;

    // 일단 임시 레벨업 공식
    while (Exp >= Level * 100)
    {
        Exp -= Level * 100;
        Level++;

        Stat.STR += 2;
        Stat.INT += 2;
        Stat.DEF += 1;
        Stat.SPD += 1;

        Stat.MaxHP = 80 + Stat.STR * 5;
        Stat.MaxMP = 20 + Stat.INT * 3;

        Stat.CurHP = Stat.MaxHP;
        Stat.CurMP = Stat.MaxMP;
    }
}

int Character::GetMoney() const
{
    return Money;
}

void Character::AddMoney(int addMoney)
{
    if (addMoney <= 0)
    {
        return;
    }

    Money += addMoney;
}

bool Character::UseMoney(int useMoney)
{
    if (useMoney <= 0)
    {
        return true;
    }

    if (Money < useMoney)
    {
        return false;
    }

    Money -= useMoney;
    return true;
}

// =====================================================
// 장비
// =====================================================

CharacterEquipment Character::GetEquipment() const
{
    return Equip;
}

void Character::SetEquipment(const CharacterEquipment& newEquip)
{
    Equip = newEquip;
}

void Character::SetWeaponID(int itemID)
{
    Equip.WeaponID = itemID;
}

void Character::SetArmorID(int itemID)
{
    Equip.ArmorID = itemID;
}

void Character::SetAccessoryID(int itemID)
{
    Equip.AccessoryID = itemID;
}

// =====================================================
// 스킬
// =====================================================

void Character::AddSkillID(int skillID)
{
    if (!HasSkillID(skillID))
    {
        LearnedSkillIDs.push_back(skillID);
    }
}

bool Character::HasSkillID(int skillID) const
{
    for (int id : LearnedSkillIDs)
    {
        if (id == skillID)
        {
            return true;
        }
    }

    return false;
}

std::vector<int> Character::GetLearnedSkillIDs() const
{
    return LearnedSkillIDs;
}

// =====================================================
// 이동 관련
// =====================================================

int Character::GetAP() const
{
    return AP;
}

void Character::SetAP(int newAP)
{
    if (newAP < 0)
    {
        newAP = 0;
    }

    AP = newAP;
}

bool Character::IsInMoveRange(int targetX, int targetY) const
{
    int dx = targetX - GridPos.x;
    int dy = targetY - GridPos.y;

    int distSquare = dx * dx + dy * dy;

    return distSquare <= AP * AP;
}

void Character::MoveToGrid(int targetX, int targetY)
{
    if (!IsInMoveRange(targetX, targetY))
    {
        return;
    }

    GridPos.x = targetX;
    GridPos.y = targetY;

    // 실제 픽셀 좌표 변환은 맵 타일 크기 정해진 뒤 연결할 예정

    SetState(ECharacterState::Walk);
}

// =====================================================
// 공격 / 방어 / 피격
// =====================================================

bool Character::IsAlive() const
{
    return Stat.CurHP > 0;
}

bool Character::IsInAttackRange(const Character& target) const
{
    Location targetPos = target.GetGridPos();

    int dx = targetPos.x - GridPos.x;
    int dy = targetPos.y - GridPos.y;

    int dist = abs(dx) + abs(dy);

    int attackRange = 1;

    switch (ClassType)
    {
    case ECharacterClass::Warrior:
        attackRange = 1;
        break;

    case ECharacterClass::Archer:
        attackRange = 3;
        break;

    case ECharacterClass::Mage:
        attackRange = 2;
        break;
    }

    return dist <= attackRange;
}

int Character::CalcAttackDamage(const Character& target) const
{
    CharacterStat targetStat = target.GetStat();

    int damage = Stat.STR * 2 - targetStat.DEF;

    if (damage < 1)
    {
        damage = 1;
    }

    return damage;
}

int Character::CalcSkillDamage(const Character& target, int skillID) const
{
    CharacterStat targetStat = target.GetStat();

    // Skill 데미지 임시 공식
    int damage = Stat.INT * 3 - targetStat.DEF;

    if (damage < 1)
    {
        damage = 1;
    }

    return damage;
}

void Character::GetDamage(int damage)
{
    if (damage <= 0)
    {
        return;
    }

    if (State == ECharacterState::Defend)
    {
        damage = static_cast<int>(damage * DefendRate);

        if (damage < 1)
        {
            damage = 1;
        }
    }

    Stat.CurHP -= damage;

    if (Stat.CurHP <= 0)
    {
        Stat.CurHP = 0;
        SetState(ECharacterState::Dead);
    }
    else
    {
        SetState(ECharacterState::Hit);
    }
}

float Character::GetDefendRate() const
{
    return DefendRate;
}

void Character::SetDefendRate(float newRate)
{
    if (newRate < 0.0f)
    {
        newRate = 0.0f;
    }

    if (newRate > 1.0f)
    {
        newRate = 1.0f;
    }

    DefendRate = newRate;
}