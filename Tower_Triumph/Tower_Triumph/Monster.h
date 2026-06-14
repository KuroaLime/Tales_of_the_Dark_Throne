#pragma once

#include <Windows.h>
#include <string>
#include <cmath>
#include <cstdlib>
#include <utility>

#pragma comment(lib, "Msimg32.lib")

#include "Character.h"

class Character;

#define MONSTER_SHEET_COL 4
#define MONSTER_SHEET_ROW 1

#define MONSTER_6X3_SHEET_COL 6
#define MONSTER_6X3_SHEET_ROW 3
#define MONSTER_6X3_FRAME_COUNT (MONSTER_6X3_SHEET_COL * MONSTER_6X3_SHEET_ROW)

#define MONSTER_TRANSPARENT_COLOR RGB(255, 0, 255)



enum class EMonsterType
{
	None,

	Slime,
	Goblin,

	Skeleton,
	Zombie,
	Ghost,
	DarkKnight,

	FireSlime,
	LavaGolem,
	FlameDemon,

	SeaSlime,
	Mermaid,
	SeaSerpent,

	Harpy,
	AngelGuardian,
	SkySpirit,

	Boss
};


enum class EMonsterState
{
	Idle,
	Walk,
	Attack,
	Hit,
	Dead
};



enum class EMonsterAIType
{
	None,

	Melee,

	Ranged,

	Caster,

	Stationary,

	Boss
};



enum class EMonsterAIState
{
	Wait,
	Move,
	Attack,
	End,
	Dead
};



enum class EMonsterDirection
{
	Left,
	Right
};



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


struct MonsterReward
{
	int Exp;
	int Money;

	int DropItemID;

	int DropRate;

	MonsterReward()
	{
		Exp = 10;
		Money = 5;

		DropItemID = -1;
		DropRate = 0;
	}
};



class Monster
{
private:
	int MonsterID;

	EMonsterType Type;
	EMonsterState State;
	EMonsterAIType AIType;
	EMonsterAIState AIState;
	EMonsterDirection Direction;
	EElement Element;

	bool IsBossMonster;

	MonsterStat Stat;
	MonsterReward Reward;

	int Level;

	int AP;

	int AttackRange;

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

	int CurFrame;
	int MaxFrame;
	int AniTick;
	int AniDelay;

	bool IsActionEnd;

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

	HBITMAP GetCurrentSprite() const;

	const BITMAP* GetCurrentSpriteInfo() const;

	int GetDirectionRow() const;

	bool IsNeedFlip() const;

	void UpdateMaxFrameByState();

	void InitMonsterValue();

	HBITMAP LoadBitmapFromFile(const std::wstring& filePath, BITMAP* outInfo);

	std::wstring GetMonsterPrefix() const;

public:
	Monster();
	~Monster();

	Monster(const Monster& other) = delete;
	Monster& operator=(const Monster& other) = delete;

	Monster(Monster&& other) noexcept;
	Monster& operator=(Monster&& other) noexcept;

	void Init(EMonsterType monsterType, int gridX, int gridY);
	void Release();


	bool LoadSprites(const std::wstring& folderPath);

	HBITMAP GetSpriteIdle() const;

	void Update();

	void Draw(HDC hMemDC, HDC hMemDC1);

	void Draw(HDC hMemDC, HDC hMemDC1, int drawW, int drawH);

	void SetState(EMonsterState newState);
	EMonsterState GetState() const;

	void SetAIState(EMonsterAIState newAIState);
	EMonsterAIState GetAIState() const;

	EMonsterAIType GetAIType() const;

	void SetDirection(EMonsterDirection newDirection);
	EMonsterDirection GetDirection() const;

	bool GetIsActionEnd() const;
	void SetIsActionEnd(bool value);

	void SetMonsterID(int id);
	int GetMonsterID() const;

	EMonsterType GetMonsterType() const;

	void SetElement(EElement newElement);
	EElement GetElement() const;
	std::wstring GetElementName() const;

	bool GetIsBossMonster() const;
	void SetIsBossMonster(bool value);

	void SetGridPos(int gx, int gy);
	Location GetGridPos() const;

	void SetPixelPos(int px, int py);
	Location GetPixelPos() const;

	void SyncWorldPixelToGrid(int tileSize);

	Location GetWorldPixelPos(int tileSize) const;

	bool StartMoveToGrid(int targetX, int targetY, int tileSize);

	bool GetIsMoving() const;

	bool MoveToGrid(int targetX, int targetY);

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

	MonsterReward GetReward() const;
	void SetReward(const MonsterReward& newReward);


	bool IsAlive() const;

	bool IsInMoveRange(int targetX, int targetY) const;

	bool IsInAttackRange(const Character& target) const;

	bool IsInAttackRangeByGrid(int targetX, int targetY) const;

	int CalcAttackDamage(const Character& target) const;

	void GetDamage(int damage);

	void Die();


	void LookAtGrid(int targetX, int targetY);

	Location GetNextStepToward(int targetX, int targetY) const;
};