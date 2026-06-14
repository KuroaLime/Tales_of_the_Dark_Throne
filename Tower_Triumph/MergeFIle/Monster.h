#pragma once

#include <Windows.h>
#include <string>
#include <cmath>
#include <cstdlib>
#include <utility>

#pragma comment(lib, "Msimg32.lib")

#include "Structure.h"

class Character;

// ============================================================
// Monster Sprite 설정
// ============================================================

// 기존 몬스터 스프라이트는 1행 4프레임 구조를 기본으로 사용한다.
// 새 몬스터 스프라이트는 6열 x 3행 = 총 18프레임 구조도 지원한다.
//
// 예)
// Skeleton_Idle.bmp        : 4 x 1 기존 시트
// FireSlime_Attack.bmp    : 6 x 3 신규 시트
//
// 프레임 크기는 Monster.cpp에서 BMP 실제 크기를 이용해 자동 계산한다.
#define MONSTER_SHEET_COL 4
#define MONSTER_SHEET_ROW 1

#define MONSTER_6X3_SHEET_COL 6
#define MONSTER_6X3_SHEET_ROW 3
#define MONSTER_6X3_FRAME_COUNT (MONSTER_6X3_SHEET_COL * MONSTER_6X3_SHEET_ROW)

// TransparentBlt에서 사용할 투명 컬러키
// BMP 배경은 반드시 순수 마젠타 RGB(255, 0, 255)여야 한다.
#define MONSTER_TRANSPARENT_COLOR RGB(255, 0, 255)


// ============================================================
// Monster Type
// 몬스터 종류 구분
// ============================================================

enum class EMonsterType
{
	None,

	// 기본/공용
	Slime,
	Goblin,

	// 묘지 스테이지용
	Skeleton,
	Zombie,
	Ghost,
	DarkKnight,

	// 용암 스테이지용
	FireSlime,
	LavaGolem,
	FlameDemon,

	// 바다 스테이지용
	SeaSlime,
	Mermaid,
	SeaSerpent,

	// 하늘 스테이지용
	Harpy,
	AngelGuardian,
	SkySpirit,

	// 보스
	Boss
};


// ============================================================
// Monster State
// 몬스터의 현재 행동 상태
// ============================================================

enum class EMonsterState
{
	Idle,
	Walk,
	Attack,
	Hit,
	Dead
};


// ============================================================
// Monster AI Type
// 몬스터 행동 방식 구분
// ============================================================

enum class EMonsterAIType
{
	None,

	// 가까운 캐릭터에게 접근 후 근접 공격
	Melee,

	// 일정 거리에서 원거리 공격
	Ranged,

	// 마법 공격 위주
	Caster,

	// 이동하지 않고 고정 위치에서 공격
	Stationary,

	// 보스 패턴
	Boss
};


// ============================================================
// Monster AI State
// 현재 AI 진행 상태
// ============================================================

enum class EMonsterAIState
{
	Wait,
	Move,
	Attack,
	End,
	Dead
};


// ============================================================
// Monster Direction
// 몬스터는 좌우 방향만 사용한다.
// 원본 스프라이트가 한쪽 방향만 있어도 반대 방향은 Draw에서 반전한다.
// ============================================================

enum class EMonsterDirection
{
	Left,
	Right
};


// ============================================================
// MonsterStat
// 몬스터 전투 스탯
// CharacterStat과 맞춰 CurHP / CurMP 기준으로 사용
// ============================================================

struct MonsterStat
{
	int MaxHP;
	int CurHP;

	int MaxMP;
	int CurMP;

	int STR;
	int INT;
	int DEF;
	int SPD;

	MonsterStat()
	{
		MaxHP = 50;
		CurHP = 50;

		MaxMP = 0;
		CurMP = 0;

		STR = 8;
		INT = 0;
		DEF = 3;
		SPD = 5;
	}
};


// ============================================================
// MonsterReward
// 몬스터 처치 보상
// ============================================================

struct MonsterReward
{
	int Exp;
	int Money;

	// 드랍 아이템 ID
	// -1이면 드랍 없음
	int DropItemID;

	// 0 ~ 100 기준 드랍 확률
	int DropRate;

	MonsterReward()
	{
		Exp = 10;
		Money = 5;

		DropItemID = -1;
		DropRate = 0;
	}
};


// ============================================================
// Monster Class
// SRPG 전투맵 위에 배치되는 적 유닛
// ============================================================

class Monster
{
private:
	// ------------------------------------------------------------
	// 기본 정보
	// ------------------------------------------------------------
	int MonsterID;

	EMonsterType Type;
	EMonsterState State;
	EMonsterAIType AIType;
	EMonsterAIState AIState;
	EMonsterDirection Direction;

	bool IsBossMonster;

	// ------------------------------------------------------------
	// 전투 정보
	// ------------------------------------------------------------
	MonsterStat Stat;
	MonsterReward Reward;

	int Level;

	// 이동 가능 범위
	int AP;

	// 공격 사거리
	int AttackRange;

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
	// Resource\\Skeleton_Idle.bmp
	// Resource\\Skeleton_Walk.bmp
	// Resource\\Skeleton_Attack.bmp
	// Resource\\Skeleton_Hit.bmp
	// Resource\\Skeleton_Dead.bmp
	//
	// 각 BMP는 기존 4x1 또는 신규 6x3 구조를 지원한다.
	// ------------------------------------------------------------
	HBITMAP hSpriteIdle;
	HBITMAP hSpriteWalk;
	HBITMAP hSpriteAttack;
	HBITMAP hSpriteHit;
	HBITMAP hSpriteDead;

	BITMAP SpriteInfoIdle;
	BITMAP SpriteInfoWalk;
	BITMAP SpriteInfoAttack;
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

	// 현재는 Draw 내부에서 동적 행 계산을 하므로 호환용으로 남겨둔다.
	int GetDirectionRow() const;

	// 좌우 반전 출력이 필요한지 확인
	// 기본 가정: 신규 원본 스프라이트는 오른쪽 방향 기준
	// Left 방향이면 반전한다.
	bool IsNeedFlip() const;

	// 현재 State에 맞게 MaxFrame 설정
	void UpdateMaxFrameByState();

	// 몬스터 타입별 기본 스탯/AI/보상 설정
	void InitMonsterValue();

	// 파일 경로에서 BMP 로드
	HBITMAP LoadBitmapFromFile(const std::wstring& filePath, BITMAP* outInfo);

	// 몬스터 타입에 따른 파일명 접두어 반환
	std::wstring GetMonsterPrefix() const;

public:
	// ------------------------------------------------------------
	// 생성 / 해제
	// ------------------------------------------------------------
	Monster();
	~Monster();

	// HBITMAP을 들고 있으므로 복사 금지
	Monster(const Monster& other) = delete;
	Monster& operator=(const Monster& other) = delete;

	// vector 사용을 대비한 이동 생성/대입
	Monster(Monster&& other) noexcept;
	Monster& operator=(Monster&& other) noexcept;

	void Init(EMonsterType monsterType, int gridX, int gridY);
	void Release();

	// ------------------------------------------------------------
	// 스프라이트
	// ------------------------------------------------------------

	// folderPath 예:
	// L"Resource"
	//
	// 실제 파일 예:
	// Resource\\Skeleton_Idle.bmp
	// Resource\\Skeleton_Walk.bmp
	// Resource\\Skeleton_Attack.bmp
	// Resource\\Skeleton_Hit.bmp
	// Resource\\Skeleton_Dead.bmp
	bool LoadSprites(const std::wstring& folderPath);

	// 상태창/디버그 UI 등에서 Idle 스프라이트를 직접 참조할 때 사용
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
	void SetState(EMonsterState newState);
	EMonsterState GetState() const;

	void SetAIState(EMonsterAIState newAIState);
	EMonsterAIState GetAIState() const;

	EMonsterAIType GetAIType() const;

	void SetDirection(EMonsterDirection newDirection);
	EMonsterDirection GetDirection() const;

	bool GetIsActionEnd() const;
	void SetIsActionEnd(bool value);

	// ------------------------------------------------------------
	// ID / 타입
	// ------------------------------------------------------------
	void SetMonsterID(int id);
	int GetMonsterID() const;

	EMonsterType GetMonsterType() const;

	bool GetIsBossMonster() const;
	void SetIsBossMonster(bool value);

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
	MonsterStat GetStat() const;
	void SetStat(const MonsterStat& newStat);

	int GetHP() const;
	int GetMaxHP() const;

	int GetMP() const;
	int GetMaxMP() const;

	int GetLevel() const;
	void SetLevel(int newLevel);

	int GetAP() const;
	void SetAP(int newAP);

	int GetAttackRange() const;
	void SetAttackRange(int newRange);

	// ------------------------------------------------------------
	// 보상
	// ------------------------------------------------------------
	MonsterReward GetReward() const;
	void SetReward(const MonsterReward& newReward);

	// ------------------------------------------------------------
	// 전투 판정
	// ------------------------------------------------------------

	// 살아있는지 확인
	bool IsAlive() const;

	// 목표 타일이 이동 범위 안인지 확인
	// SRPG식 마름모 범위 기준
	bool IsInMoveRange(int targetX, int targetY) const;

	// 캐릭터가 공격 범위 안에 있는지 확인
	bool IsInAttackRange(const Character& target) const;

	// 좌표 기준 공격 범위 판정
	bool IsInAttackRangeByGrid(int targetX, int targetY) const;

	// 일반 공격 데미지 계산
	int CalcAttackDamage(const Character& target) const;

	// 피해 받기
	void GetDamage(int damage);

	// 사망 처리
	void Die();

	// ------------------------------------------------------------
	// AI 보조
	// ------------------------------------------------------------

	// 현재 몬스터 기준으로 목표 좌표 방향을 바라보도록 설정
	// 좌우 방향만 사용한다.
	void LookAtGrid(int targetX, int targetY);

	// 대상 좌표를 향해 한 칸 이동할 때 사용할 임시 보조 함수
	// 실제 A* 경로 탐색은 Battle_Stage에서 처리하는 것이 좋다.
	Location GetNextStepToward(int targetX, int targetY) const;
};