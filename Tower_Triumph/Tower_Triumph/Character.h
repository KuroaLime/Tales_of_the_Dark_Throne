#pragma once

#include <Windows.h>
#include <vector>
#include <string>
#include "Structure.h"

#pragma comment(lib, "Msimg32.lib")

// =====================================================
// Character.h
// 캐릭터 기본 데이터 / 애니메이션 / 전투 관련 인터페이스
// =====================================================

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
    int HP;

    int MaxMP;
    int MP;

    int STR;
    int INT;
    int DEF;
    int SPD;

    CharacterStat()
    {
        MaxHP = 100;
        HP = 100;

        MaxMP = 30;
        MP = 30;

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