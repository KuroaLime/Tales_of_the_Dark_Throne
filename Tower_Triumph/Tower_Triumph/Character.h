#pragma once

#include <Windows.h>
#include <vector>
#include <string>
#include <algorithm>
#include "Structure.h"

#pragma comment(lib, "Msimg32.lib")

// 스프라이트 시트
#define CHARACTER_SHEET_COL 4
#define CHARACTER_SHEET_ROW 3

#define CHARACTER_FRAME_WIDTH  362
#define CHARACTER_FRAME_HEIGHT 362

#define CHARACTER_TRANSPARENT_COLOR RGB(255, 0, 255)

// -----------------------------------------------------
// 직업
// -----------------------------------------------------
enum class ECharacterClass
{
    Warrior,    // 전사
    Archer,     // 궁수
    Mage        // 마법사
};

// -----------------------------------------------------
// 캐릭터 상태
// -----------------------------------------------------
enum class ECharacterState
{
    Idle,       // 대기
    Walk,       // 걷기
    Attack,     // 일반 공격
    Defend,     // 방어
    Skill,      // 스킬 사용
    Hit,        // 피격
    Dead        // 사망
};

// -----------------------------------------------------
// 방향
// 좌/우는 Draw에서 좌우 반전 처리
// -----------------------------------------------------
enum class EDirection
{
    Down,       // 정면 / 아래
    Left,       // 왼쪽
    Right,      // 오른쪽
    Up          // 후면 / 위
};

// -----------------------------------------------------
// 속성
// -----------------------------------------------------
enum class EElement
{
    None,

    Fire,       // 불
    Water,      // 물
    Grass,      // 풀

    Light,      // 빛
    Dark,       // 어둠

    Heal        // 회복 계열
};

// -----------------------------------------------------
// 장비 부위
// 무기 / 방어구 / 장신구는 시간 남으면
// -----------------------------------------------------
enum class EEquipPart
{
    Weapon,
    Armor,
    Accessory
};

// -----------------------------------------------------
// 스탯
// HP / MP / STR / INT / DEF / SPD
// -----------------------------------------------------
struct CharacterStat
{
    int MaxHP;
    int CurHP;

    int MaxMP;
    int CurMP;

    int STR;
    int INT;
    int DEF;
    int SPD;

    CharacterStat()
    {
        MaxHP = 100;
        CurHP = 100;

        MaxMP = 30;
        CurMP = 30;

        STR = 10;
        INT = 10;
        DEF = 5;
        SPD = 10;
    }
};

// -----------------------------------------------------
// 장비 정보
// 일단 Item 클래스를 아직 안 만들었으므로 ID로만 연결
// -----------------------------------------------------
struct CharacterEquipment
{
    int WeaponID;
    int ArmorID;
    int AccessoryID;

    CharacterEquipment()
    {
        WeaponID = -1;
        ArmorID = -1;
        AccessoryID = -1;
    }
};

// -----------------------------------------------------
// 캐릭터 클래스
// -----------------------------------------------------
class Character
{
private:
    // ------------------------------
    // 기본 정보
    // ------------------------------
    int CharacterID;

    ECharacterClass ClassType;
    ECharacterState State;
    EDirection Direction;
    EElement Element;

    CharacterStat Stat;
    CharacterEquipment Equip;

    int Level;
    int Exp;
    int Money;

    std::vector<int> LearnedSkillIDs;

    // ------------------------------
    // 위치 정보
    // GridPos: 타일 좌표
    // PixelPos: 화면 출력 좌표
    // ------------------------------
    Location GridPos;
    Location PixelPos;

    // ------------------------------
    // 직업별 고정값
    // 방어 계수 / AP
    // ------------------------------
    int AP;
    float DefendRate;

    // ------------------------------
    // 애니메이션 정보
    // ------------------------------
    int CurFrame;
    int MaxFrame;

    int AniTick;
    int AniDelay;

    bool IsActionEnd;

    // ------------------------------
    // 스프라이트
    // 상태별 BMP
    // Idle / Walk / Attack / Defend / Skill / Hit / Dead
    // ------------------------------
    HBITMAP hSpriteIdle;
    HBITMAP hSpriteWalk;
    HBITMAP hSpriteAttack;
    HBITMAP hSpriteDefend;
    HBITMAP hSpriteSkill;
    HBITMAP hSpriteHit;
    HBITMAP hSpriteDead;

    BITMAP SpriteInfoIdle;
    BITMAP SpriteInfoWalk;
    BITMAP SpriteInfoAttack;
    BITMAP SpriteInfoDefend;
    BITMAP SpriteInfoSkill;
    BITMAP SpriteInfoHit;
    BITMAP SpriteInfoDead;

private:
    // 현재 상태에 맞는 비트맵 반환
    HBITMAP GetCurrentSprite() const;
    BITMAP GetCurrentSpriteInfo() const;

    // 방향에 따른 스프라이트 시트 행 번호 반환
    int GetDirectionRow() const;

    // 좌우 반전이 필요한지 판단
    bool IsNeedFlip() const;

    // 상태별 최대 프레임 설정
    void UpdateMaxFrameByState();

    // 직업별 기본값 세팅
    void InitClassValue();

    // BMP 하나 로드
    bool LoadBitmapFromFile(const wchar_t* filePath, HBITMAP& outBitmap, BITMAP& outInfo);

public:
    Character();
    ~Character();

    // ------------------------------
    // 초기화 / 해제
    // ------------------------------
    void Init(ECharacterClass newClass, int gridX, int gridY);
    void Release();

    // 캐릭터 직업에 맞는 스프라이트 로드
    bool LoadSprites(const std::wstring& folderPath);

    // ------------------------------
    // 갱신 / 출력
    // ------------------------------
    void Update();
    void Draw(HDC hMemDC, HDC hBackDC);

    // ------------------------------
    // 상태 / 방향
    // ------------------------------
    void SetState(ECharacterState newState);
    ECharacterState GetState() const;

    void SetDirection(EDirection newDirection);
    EDirection GetDirection() const;

    bool GetIsActionEnd() const;
    void SetIsActionEnd(bool newValue);

    // ------------------------------
    // 위치
    // ------------------------------
    void SetGridPos(int x, int y);
    Location GetGridPos() const;

    void SetPixelPos(int x, int y);
    Location GetPixelPos() const;

    // ------------------------------
    // 기본 정보
    // ------------------------------
    void SetCharacterID(int id);
    int GetCharacterID() const;

    ECharacterClass GetClassType() const;

    void SetElement(EElement newElement);
    EElement GetElement() const;

    // ------------------------------
    // 스탯 / 성장
    // ------------------------------
    CharacterStat GetStat() const;
    void SetStat(const CharacterStat& newStat);

    int GetLevel() const;
    void SetLevel(int newLevel);

    int GetExp() const;
    void AddExp(int addExp);

    int GetMoney() const;
    void AddMoney(int addMoney);
    bool UseMoney(int useMoney);

    // ------------------------------
    // 장비
    // ------------------------------
    CharacterEquipment GetEquipment() const;
    void SetEquipment(const CharacterEquipment& newEquip);

    void SetWeaponID(int itemID);
    void SetArmorID(int itemID);
    void SetAccessoryID(int itemID);

    // ------------------------------
    // 스킬
    // ------------------------------
    void AddSkillID(int skillID);
    bool HasSkillID(int skillID) const;
    std::vector<int> GetLearnedSkillIDs() const;

    // ------------------------------
    // 이동 관련
    // ------------------------------
    int GetAP() const;
    void SetAP(int newAP);

    // 목표 타일이 이동 범위 안인지 확인
    bool IsInMoveRange(int targetX, int targetY) const;

    // 실제 이동은 나중에 A* 붙이기 전까지 간단 이동으로 처리 가능
    void MoveToGrid(int targetX, int targetY);

    // ------------------------------
    // 공격 / 방어 / 피격
    // ------------------------------
    bool IsAlive() const;

    // target이 공격 범위 안인지 확인
    bool IsInAttackRange(const Character& target) const;

    // 일반 공격 데미지 계산
    int CalcAttackDamage(const Character& target) const;

    // 스킬 데미지 계산
    int CalcSkillDamage(const Character& target, int skillID) const;

    // 피해 받기
    void GetDamage(int damage);

    // 방어 상태 피해 감소율
    float GetDefendRate() const;
    void SetDefendRate(float newRate);
};

/*
============================================================
 Character.h 설명
============================================================

[상수 정의]

CHARACTER_SHEET_COL
 -> 스프라이트 시트의 가로 프레임 개수(4열)

CHARACTER_SHEET_ROW
 -> 스프라이트 시트의 세로 방향 개수(3행)

CHARACTER_FRAME_WIDTH
 -> 한 프레임의 가로 크기(362)

CHARACTER_FRAME_HEIGHT
 -> 한 프레임의 세로 크기(362

CHARACTER_TRANSPARENT_COLOR
 -> TransparentBlt에서 투명색으로 사용할 컬러키(RGB(255, 0, 255), 순수 마젠타)

============================================================

[Enum 구성]

ECharacterClass
 -> 캐릭터의 직업을 구분(Warrior, Archer, Mage)

ECharacterState
 -> 캐릭터의 현재 행동 상태를 구분(Idle, Walk, Attack, Defend, Skill, Hit, Dead)

EDirection
 -> 캐릭터가 바라보는 방향을 구분(Down, Left, Right(반전해서 사용), Up)

EElement
 -> 캐릭터 또는 공격의 속성을 구분(None, Fire, Water, Grass, Light, Dark, Heal)

EEquipPart
 -> 장비 부위(Weapon, Armor, Accessory)

============================================================

[Struct 내부 구성]

CharacterStat-> 캐릭터의 전투 스탯
 - MaxHP : 최대 체력
 - HP    : 현재 체력
 - MaxMP : 최대 마나
 - MP    : 현재 마나
 - STR   : 힘, 일반 공격과 HP 성장에 사용
 - INT   : 지능, 마법 공격과 MP 성장에 사용
 - DEF   : 방어력, 받는 피해 감소에 사용
 - SPD   : 속도, 턴 순서 계산에 사용

 기본값
 - MaxHP = 100
 - HP    = 100
 - MaxMP = 30
 - MP    = 30
 - STR   = 10
 - INT   = 10
 - DEF   = 5
 - SPD   = 10


CharacterEquipment
 -> 캐릭터가 착용 중인 장비 ID -> 실제 아이템 정보는 Item 데이터에서 ID를 통해 참조하는 방식
 - WeaponID    : 착용 중인 무기 ID
 - ArmorID     : 착용 중인 방어구 ID
 - AccessoryID : 착용 중인 장신구 ID

 기본값
 - WeaponID    = -1
 - ArmorID     = -1
 - AccessoryID = -1

 -1은 장착한 장비가 없다는 의미

============================================================

[Character 클래스 내부 데이터 구성]

 - CharacterID : 캐릭터 고유 번호
 - ClassType   : 캐릭터 직업
 - State       : 현재 행동 상태
 - Direction   : 현재 바라보는 방향
 - Element     : 캐릭터 속성
 - Stat            : HP, MP, STR, INT, DEF, SPD 저장
 - Equip           : 착용 장비 ID 저장
 - Level           : 현재 레벨
 - Exp             : 현재 경험치
 - Money           : 보유 돈
 - LearnedSkillIDs : 습득한 스킬 ID 목록
 - GridPos  : 타일 좌표
 - PixelPos : 실제 화면 출력 좌표
 - AP         : 이동 가능 범위
 - DefendRate : 방어 상태일 때 받는 피해 비율
 - CurFrame    : 현재 애니메이션 프레임
 - MaxFrame    : 현재 상태의 최대 프레임 수
 - AniTick     : 애니메이션 시간 누적값
 - AniDelay    : 프레임 전환 지연값
 - IsActionEnd : 공격/스킬/피격/사망 등의 동작 완료 여부
 - hSpriteIdle   : 대기 스프라이트
 - hSpriteWalk   : 걷기 스프라이트
 - hSpriteAttack : 공격 스프라이트
 - hSpriteDefend : 방어 스프라이트
 - hSpriteSkill  : 스킬 스프라이트
 - hSpriteHit    : 피격 스프라이트
 - hSpriteDead   : 사망 스프라이트

 - SpriteInfoIdle ~ SpriteInfoDead
   각 BMP의 정보 저장용 BITMAP 구조체

============================================================

[Private 함수 역할]

GetCurrentSprite() -> 현재 State 값에 맞는 HBITMAP을 반환

GetCurrentSpriteInfo() -> 현재 State 값에 맞는 BITMAP 정보를 반환

GetDirectionRow() -> 현재 Direction 값에 따라 스프라이트 시트에서 사용할 행 번호를 반환
 -> Down = 0, Left/Right = 1, Up = 2

IsNeedFlip() -> 현재 방향에서 좌우 반전 출력이 필요한지 판단
    -> 측면 스프라이트를 한 방향 기준으로 두고 반대 방향은 반전해서 사용

UpdateMaxFrameByState() -> 현재 상태에 맞게 MaxFrame 값을 설정(4프레임)

InitClassValue() -> 직업별 기본 스탯, AP, 방어 계수를 설정

LoadBitmapFromFile() -> 파일 경로를 받아 BMP를 HBITMAP 형태로 로드
 -> LoadImageW와 LR_LOADFROMFILE을 사용한다.

============================================================

[Public 함수 역할]

Character() -> 기본값 초기화

~Character() -> Release()를 호출하여 BMP 핸들을 정리

Init() -> 캐릭터 직업, 초기 타일 위치, 기본 상태를 설정(직업별 기본값도 함께 설정)

Release() -> 로드된 BMP 핸들을 DeleteObject로 해제한다.

LoadSprites() -> 직업에 맞는 BMP 스프라이트 파일들을 Resource 폴더에서 로드

Update() -> 애니메이션 프레임을 갱신(HP가 0 이하이면 Dead 상태로 전환)

Draw() -> 현재 상태, 방향, 프레임에 맞는 스프라이트를 화면에 출력
    -> TransparentBlt를 사용하여 순수 마젠타 배경을 투명 처리

SetState() -> 캐릭터 상태를 변경 (상태 변경 시 프레임을 0으로 초기화)

GetState() -> 현재 캐릭터 상태를 반환

SetDirection() -> 캐릭터가 바라보는 방향을 설정

GetDirection() -> 현재 캐릭터 방향을 반환

GetIsActionEnd() -> 현재 행동 애니메이션이 끝났는지 반환

SetIsActionEnd() -> 행동 완료 여부 값을 직접 설정

SetGridPos() -> 캐릭터의 타일 좌표를 설정

GetGridPos() -> 캐릭터의 타일 좌표를 반환

SetPixelPos() -> 캐릭터의 화면 출력 좌표를 설정

GetPixelPos() -> 캐릭터의 화면 출력 좌표를 반환

SetCharacterID() -> 캐릭터 고유 ID를 설정

GetCharacterID() -> 캐릭터 고유 ID를 반환

GetClassType() -> 캐릭터 직업을 반환

SetElement() -> 캐릭터 속성을 설정

GetElement() -> 캐릭터 속성을 반환

GetStat() -> 캐릭터의 현재 스탯 구조체를 반환

SetStat() -> 캐릭터 스탯 구조체를 외부 값으로 변경

GetLevel() -> 현재 레벨을 반환

SetLevel() -> 현재 레벨을 설정

GetExp() -> 현재 경험치를 반환

AddExp() -> 경험치를 추가(일정 경험치 이상이면 레벨업 처리)

GetMoney() -> 현재 보유 돈을 반환

AddMoney() -> 돈을 추가

UseMoney() -> 돈을 사용(보유 돈이 부족하면 false를 반환)

GetEquipment() -> 현재 장비 ID 구조체를 반환

SetEquipment() -> 장비 ID 구조체를 외부 값으로 변경

SetWeaponID() -> 무기 ID를 설정

SetArmorID() -> 방어구 ID를 설정

SetAccessoryID() -> 장신구 ID를 설정

AddSkillID() -> 습득 스킬 ID를 추가(이미 존재하는 스킬이면 중복 추가하지 않음)

HasSkillID() -> 해당 스킬 ID를 가지고 있는지 확인

GetLearnedSkillIDs() -> 습득한 스킬 ID 목록을 반환

GetAP() -> 현재 이동 가능 범위 AP를 반환

SetAP() -> 이동 가능 범위 AP를 설정

IsInMoveRange() -> 목표 타일이 현재 캐릭터의 이동 범위 안에 있는지 확인

MoveToGrid() -> 목표 타일로 이동(현재 이동 범위 안일 때 GridPos만 변경)

IsAlive() -> 현재 HP가 0보다 큰지 확인

IsInAttackRange() -> 대상 캐릭터가 공격 범위 안에 있는지 확인

CalcAttackDamage() -> STR 기반 일반 공격 데미지를 계산

CalcSkillDamage() -> INT 기반 스킬 데미지를 계산(현재는 임시 공식)

GetDamage() -> 캐릭터가 피해를 받는 처리
 -> 방어 상태이면 피해를 DefendRate만큼 감소
 -> HP가 0 이하가 되면 Dead 상태로 변경

GetDefendRate() -> 현재 방어 피해 비율을 반환한다.

SetDefendRate() -> 방어 피해 비율을 설정(0.0 ~ 1.0)
*/