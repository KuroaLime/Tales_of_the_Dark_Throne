#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <utility>

#include "Structure.h"
#pragma comment(lib, "Msimg32.lib")

// ============================================================
// Character Sprite 설정
// ============================================================

// 캐릭터 스프라이트는 좌우 방향만 사용한다.
// 실제 BMP는 1행 4프레임 구조로 사용한다.
//
// 예)
// Warrior_Idle.bmp
// [0][1][2][3]
//
// 프레임 크기는 Character.cpp에서 BMP 실제 크기를 이용해 자동 계산한다.
// frameW = 전체 BMP 가로 / CHARACTER_SHEET_COL
// frameH = 전체 BMP 세로 / CHARACTER_SHEET_ROW
#define CHARACTER_SHEET_COL 4
#define CHARACTER_SHEET_ROW 1

// TransparentBlt에서 사용할 투명 컬러키
// BMP 배경은 반드시 순수 마젠타 RGB(255, 0, 255)여야 한다.
#define CHARACTER_TRANSPARENT_COLOR RGB(255, 0, 255)


// ============================================================
// Character Class
// 캐릭터 직업 구분
// ============================================================

enum class ECharacterClass
{
	Warrior,
	Archer,
	Mage
};


// ============================================================
// Character State
// 캐릭터의 현재 행동 상태
// ============================================================

enum class ECharacterState
{
	Idle,
	Walk,
	Attack,
	Defend,
	Skill,
	Hit,
	Dead
};


// ============================================================
// Direction
// 현재 캐릭터/몬스터 스프라이트 구조에 맞춰 좌우 방향만 사용한다.
// 원본 스프라이트가 한쪽 방향만 있어도 반대 방향은 Draw에서 반전한다.
// ============================================================

enum class EDirection
{
	Left,
	Right
};


// ============================================================
// Element
// 속성 구분
// ============================================================

enum class EElement
{
	None,
	Fire,
	Water,
	Grass,
	Light,
	Dark,
	Heal
};


// ============================================================
// Equip Part
// 장비 부위 구분
// ============================================================

enum class EEquipPart
{
	Weapon,
	Armor,
	Accessory
};


// ============================================================
// CharacterEquipment
// 캐릭터 장비 ID 저장
// 실제 아이템 정보는 Item 데이터에서 ID로 참조하는 방식
// ============================================================

struct CharacterEquipment
{
	int WeaponID;
	int ArmorID;
	int AccessoryID;

	CharacterEquipment()
		: WeaponID(-1), ArmorID(-1), AccessoryID(-1)
	{
	}
};


// ============================================================
// Character Class
// SRPG 전투맵 위에 배치되는 플레이어 유닛
// ============================================================

class Character
{
private:
	// ------------------------------------------------------------
	// 기본 정보
	// ------------------------------------------------------------
	int CharacterID;

	ECharacterClass ClassType;
	ECharacterState State;
	EDirection Direction;
	EElement Element;

	std::wstring Name;

	// ------------------------------------------------------------
	// 성장 및 전투 정보
	// ------------------------------------------------------------
	CharacterStat Stat;
	CharacterEquipment Equip;

	int Level;
	int Exp;
	int Money;

	std::vector<int> LearnedSkillIDs;

	// 이동 가능 범위
	int AP;

	// 방어 상태일 때 받는 피해 비율
	// 예: 0.5f면 피해 50% 감소
	float DefendRate;

	// ------------------------------------------------------------
	// 위치 정보
	// GridPos    : 타일 좌표
	// PixelPos   : 실제 화면 출력 좌표
	// WorldPixel : 카메라 적용 전 맵 내부 픽셀 좌표
	// ------------------------------------------------------------
	Location GridPos;
	Location PixelPos;

	bool IsMoving;

	float WorldPixelX;
	float WorldPixelY;

	float TargetPixelX;
	float TargetPixelY;

	int TargetGridX;
	int TargetGridY;

	float MoveSpeed;

	// ------------------------------------------------------------
	// 애니메이션 정보
	// ------------------------------------------------------------
	int CurFrame;
	int MaxFrame;
	int AniTick;
	int AniDelay;

	bool IsActionEnd;

	// ------------------------------------------------------------
	// 스프라이트 핸들
	//
	// 파일명 예:
	// Resource\\Warrior_Idle.bmp
	// Resource\\Warrior_Walk.bmp
	// Resource\\Warrior_Attack.bmp
	// Resource\\Warrior_Defend.bmp
	// Resource\\Warrior_Skill.bmp
	// Resource\\Warrior_Hit.bmp
	// Resource\\Warrior_Dead.bmp
	//
	// 각 BMP는 1행 4프레임 구조를 기본으로 한다.
	// ------------------------------------------------------------
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
	// ------------------------------------------------------------
	// 내부 보조 함수
	// ------------------------------------------------------------

	// 현재 State에 맞는 HBITMAP 반환
	HBITMAP GetCurrentSprite() const;

	// 현재 State에 맞는 BITMAP 정보 반환
	const BITMAP* GetCurrentSpriteInfo() const;

	// 캐릭터는 1행 스프라이트만 사용하므로 항상 0을 반환
	int GetDirectionRow() const;

	// 좌우 반전 출력이 필요한지 확인
	// 기본 가정: 원본 스프라이트는 왼쪽 방향 기준
	// Right 방향이면 반전한다.
	bool IsNeedFlip() const;

	// 현재 State에 맞게 MaxFrame 설정
	void UpdateMaxFrameByState();

	// 직업별 기본 스탯/AP/방어 계수 설정
	void InitClassValue();

	// 파일 경로에서 BMP 로드
	HBITMAP LoadBitmapFromFile(const std::wstring& filePath, BITMAP* outInfo);

	// 직업에 따른 파일명 접두어 반환
	std::wstring GetCharacterPrefix() const;

public:
	// ------------------------------------------------------------
	// 생성 / 해제
	// ------------------------------------------------------------
	Character();
	~Character();

	// HBITMAP을 들고 있으므로 복사 금지
	Character(const Character& other) = delete;
	Character& operator=(const Character& other) = delete;

	// vector 사용을 대비한 이동 생성/대입
	Character(Character&& other) noexcept;
	Character& operator=(Character&& other) noexcept;

	void Init(ECharacterClass classType, int gridX, int gridY);
	void Release();

	// ------------------------------------------------------------
	// 스프라이트
	// ------------------------------------------------------------

	// folderPath 예:
	// L"Resource"
	//
	// 실제 파일 예:
	// Resource\\Warrior_Idle.bmp
	// Resource\\Archer_Idle.bmp
	// Resource\\Mage_Idle.bmp
	bool LoadSprites(const std::wstring& folderPath);

	// 상태창/턴 UI에서 Idle 스프라이트를 직접 참조할 때 사용
	HBITMAP GetSpriteIdle() const;

	// ------------------------------------------------------------
	// 업데이트 / 출력
	// ------------------------------------------------------------
	void Update();

	// 기존 코드 호환용 2인자 Draw
	// 내부에서 4인자 Draw를 drawW=0, drawH=0으로 호출한다.
	void Draw(HDC hMemDC, HDC hMemDC1);

	// hMemDC  : 실제 출력 대상 백버퍼 DC
	// hMemDC1 : 스프라이트를 SelectObject할 임시 DC
	//
	// drawW, drawH:
	// 맵 위에 실제로 출력할 크기
	// 예: TILE_SIZE, TILE_SIZE + 16
	void Draw(HDC hMemDC, HDC hMemDC1, int drawW, int drawH);

	// ------------------------------------------------------------
	// 상태
	// ------------------------------------------------------------
	void SetState(ECharacterState newState);
	ECharacterState GetState() const;

	void SetDirection(EDirection newDirection);
	EDirection GetDirection() const;

	bool GetIsActionEnd() const;
	void SetIsActionEnd(bool value);

	// ------------------------------------------------------------
	// ID / 이름 / 직업 / 속성
	// ------------------------------------------------------------
	void SetCharacterID(int id);
	int GetCharacterID() const;

	void SetName(const std::wstring& newName);
	std::wstring GetName() const;

	ECharacterClass GetClassType() const;

	void SetElement(EElement newElement);
	EElement GetElement() const;

	// ------------------------------------------------------------
	// 위치
	// ------------------------------------------------------------
	void SetGridPos(int gx, int gy);
	Location GetGridPos() const;

	void SetPixelPos(int px, int py);
	Location GetPixelPos() const;

	// GridPos를 WorldPixel 좌표와 동기화
	void SyncWorldPixelToGrid(int tileSize);

	// 이동 중이면 WorldPixel 위치 반환
	// 이동 중이 아니면 GridPos * tileSize 위치 반환
	Location GetWorldPixelPos(int tileSize) const;

	// 목표 타일까지 Walk 상태로 이동 시작
	bool StartMoveToGrid(int targetX, int targetY, int tileSize);

	bool GetIsMoving() const;

	// 기존 즉시 이동용 함수
	// 애니메이션 없이 GridPos만 바로 바꾸고 싶을 때 사용
	bool MoveToGrid(int targetX, int targetY);

	// ------------------------------------------------------------
	// 스탯
	// ------------------------------------------------------------
	CharacterStat GetStat() const;
	void SetStat(const CharacterStat& newStat);

	int GetHP() const;
	int GetMaxHP() const;

	int GetMP() const;
	int GetMaxMP() const;

	int GetLevel() const;
	void SetLevel(int newLevel);

	int GetExp() const;
	void AddExp(int addExp);

	int GetMoney() const;
	void AddMoney(int addMoney);
	bool UseMoney(int useMoney);

	int GetAP() const;
	void SetAP(int newAP);

	// ------------------------------------------------------------
	// 턴 행동 기록
	// ------------------------------------------------------------
	bool HasMoved() const { return bHasMoved; }
	void SetHasMoved(bool value) { bHasMoved = value; }

	bool HasActed() const { return bHasActed; }
	void SetHasActed(bool value) { bHasActed = value; }

	void ResetTurnState() { bHasMoved = false; bHasActed = false; }

private:
	bool bHasMoved;
	bool bHasActed;

public:
	// ------------------------------------------------------------
	// 장비
	// ------------------------------------------------------------
	CharacterEquipment GetEquipment() const;
	void SetEquipment(const CharacterEquipment& newEquipment);

	void SetWeaponID(int itemID);
	void SetArmorID(int itemID);
	void SetAccessoryID(int itemID);

	// ------------------------------------------------------------
	// 스킬
	// ------------------------------------------------------------
	void AddSkillID(int skillID);
	bool HasSkillID(int skillID) const;
	std::vector<int> GetLearnedSkillIDs() const;

	// ------------------------------------------------------------
	// 전투 판정
	// ------------------------------------------------------------

	// 살아있는지 확인
	bool IsAlive() const;

	// 목표 타일이 이동 범위 안인지 확인
	// SRPG식 마름모 범위 기준
	bool IsInMoveRange(int targetX, int targetY) const;

	// 대상 캐릭터가 공격 범위 안인지 확인
	bool IsInAttackRange(const Character& target) const;

	// 좌표 기준 공격 범위 판정
	bool IsInAttackRangeByGrid(int targetX, int targetY) const;

	// 일반 공격 데미지 계산
	int CalcAttackDamage(const Character& target) const;

	// 스킬 데미지 계산
	// 현재는 임시 공식으로 INT 기반 처리
	int CalcSkillDamage(const Character& target) const;

	// 피해 받기
	void GetDamage(int damage);

	// 사망 처리
	void Die();

	// 방어 피해 비율
	float GetDefendRate() const;
	void SetDefendRate(float rate);

	// ------------------------------------------------------------
	// 방향 보조
	// ------------------------------------------------------------

	// 현재 캐릭터 기준으로 목표 좌표 방향을 바라보도록 설정
	// 좌우 방향만 사용한다.
	void LookAtGrid(int targetX, int targetY);
};