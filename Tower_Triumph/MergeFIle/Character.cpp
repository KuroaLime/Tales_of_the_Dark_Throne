#include "Character.h"
#include "SoundManager.h"
#include "SkillData.h"

// ============================================================
// 생성자 / 소멸자
// ============================================================

Character::Character()
{
	CharacterID = -1;

	ClassType = ECharacterClass::Warrior;
	State = ECharacterState::Idle;
	Direction = EDirection::Left;
	Element = EElement::None;

	Name = L"";

	Level = 1;
	Exp = 0;
	Money = 0;

	AP = 3;
	DefendRate = 0.5f;

	GridPos.x = 0;
	GridPos.y = 0;

	PixelPos.x = 0;
	PixelPos.y = 0;

	IsMoving = false;

	WorldPixelX = 0.0f;
	WorldPixelY = 0.0f;

	TargetPixelX = 0.0f;
	TargetPixelY = 0.0f;

	TargetGridX = 0;
	TargetGridY = 0;

	MoveSpeed = 8.0f;

	CurFrame = 0;
	MaxFrame = CHARACTER_SHEET_COL;
	AniTick = 0;
	AniDelay = 8;

	IsActionEnd = false;

	bHasMoved = false;
	bHasActed = false;

	hSpriteIdle = NULL;
	hSpriteWalk = NULL;
	hSpriteAttack = NULL;
	hSpriteDefend = NULL;
	hSpriteSkill = NULL;
	hSpriteHit = NULL;
	hSpriteDead = NULL;
	hIllustration = NULL;

	ZeroMemory(&SpriteInfoIdle, sizeof(BITMAP));
	ZeroMemory(&SpriteInfoWalk, sizeof(BITMAP));
	ZeroMemory(&SpriteInfoAttack, sizeof(BITMAP));
	ZeroMemory(&SpriteInfoDefend, sizeof(BITMAP));
	ZeroMemory(&SpriteInfoSkill, sizeof(BITMAP));
	ZeroMemory(&SpriteInfoHit, sizeof(BITMAP));
	ZeroMemory(&SpriteInfoDead, sizeof(BITMAP));
	ZeroMemory(&SpriteInfoIllustration, sizeof(BITMAP));

	ClearEquippedSkills();
}

Character::~Character()
{
	Release();
}

// ============================================================
// 이동 생성자 / 이동 대입 연산자
// HBITMAP 중복 삭제 방지용
// ============================================================

Character::Character(Character&& other) noexcept
{
	CharacterID = other.CharacterID;

	ClassType = other.ClassType;
	State = other.State;
	Direction = other.Direction;
	Element = other.Element;

	Name = std::move(other.Name);

	Stat = other.Stat;
	Equip = other.Equip;

	Level = other.Level;
	Exp = other.Exp;
	Money = other.Money;

	LearnedSkillIDs = std::move(other.LearnedSkillIDs);
	for (int i = 0; i < 3; ++i)
	{
		EquippedSkillIDs[i] = other.EquippedSkillIDs[i];
		other.EquippedSkillIDs[i] = -1;
	}

	AP = other.AP;
	DefendRate = other.DefendRate;

	GridPos = other.GridPos;
	PixelPos = other.PixelPos;

	IsMoving = other.IsMoving;

	WorldPixelX = other.WorldPixelX;
	WorldPixelY = other.WorldPixelY;

	TargetPixelX = other.TargetPixelX;
	TargetPixelY = other.TargetPixelY;

	TargetGridX = other.TargetGridX;
	TargetGridY = other.TargetGridY;

	MoveSpeed = other.MoveSpeed;

	CurFrame = other.CurFrame;
	MaxFrame = other.MaxFrame;
	AniTick = other.AniTick;
	AniDelay = other.AniDelay;

	IsActionEnd = other.IsActionEnd;
	bHasMoved = other.bHasMoved;
	bHasActed = other.bHasActed;

	hSpriteIdle = other.hSpriteIdle;
	hSpriteWalk = other.hSpriteWalk;
	hSpriteAttack = other.hSpriteAttack;
	hSpriteDefend = other.hSpriteDefend;
	hSpriteSkill = other.hSpriteSkill;
	hSpriteHit = other.hSpriteHit;
	hSpriteDead = other.hSpriteDead;
	hIllustration = other.hIllustration;

	SpriteInfoIdle = other.SpriteInfoIdle;
	SpriteInfoWalk = other.SpriteInfoWalk;
	SpriteInfoAttack = other.SpriteInfoAttack;
	SpriteInfoDefend = other.SpriteInfoDefend;
	SpriteInfoSkill = other.SpriteInfoSkill;
	SpriteInfoHit = other.SpriteInfoHit;
	SpriteInfoDead = other.SpriteInfoDead;
	SpriteInfoIllustration = other.SpriteInfoIllustration;

	other.hSpriteIdle = NULL;
	other.hSpriteWalk = NULL;
	other.hSpriteAttack = NULL;
	other.hSpriteDefend = NULL;
	other.hSpriteSkill = NULL;
	other.hSpriteHit = NULL;
	other.hSpriteDead = NULL;
	other.hIllustration = NULL;
}

Character& Character::operator=(Character&& other) noexcept
{
	if (this == &other)
	{
		return *this;
	}

	Release();

	CharacterID = other.CharacterID;

	ClassType = other.ClassType;
	State = other.State;
	Direction = other.Direction;
	Element = other.Element;

	Name = std::move(other.Name);

	Stat = other.Stat;
	Equip = other.Equip;

	Level = other.Level;
	Exp = other.Exp;
	Money = other.Money;

	LearnedSkillIDs = std::move(other.LearnedSkillIDs);
	for (int i = 0; i < 3; ++i)
	{
		EquippedSkillIDs[i] = other.EquippedSkillIDs[i];
		other.EquippedSkillIDs[i] = -1;
	}

	AP = other.AP;
	DefendRate = other.DefendRate;

	GridPos = other.GridPos;
	PixelPos = other.PixelPos;

	IsMoving = other.IsMoving;

	WorldPixelX = other.WorldPixelX;
	WorldPixelY = other.WorldPixelY;

	TargetPixelX = other.TargetPixelX;
	TargetPixelY = other.TargetPixelY;

	TargetGridX = other.TargetGridX;
	TargetGridY = other.TargetGridY;

	MoveSpeed = other.MoveSpeed;

	CurFrame = other.CurFrame;
	MaxFrame = other.MaxFrame;
	AniTick = other.AniTick;
	AniDelay = other.AniDelay;

	IsActionEnd = other.IsActionEnd;
	bHasMoved = other.bHasMoved;
	bHasActed = other.bHasActed;

	hSpriteIdle = other.hSpriteIdle;
	hSpriteWalk = other.hSpriteWalk;
	hSpriteAttack = other.hSpriteAttack;
	hSpriteDefend = other.hSpriteDefend;
	hSpriteSkill = other.hSpriteSkill;
	hSpriteHit = other.hSpriteHit;
	hSpriteDead = other.hSpriteDead;
	hIllustration = other.hIllustration;

	SpriteInfoIdle = other.SpriteInfoIdle;
	SpriteInfoWalk = other.SpriteInfoWalk;
	SpriteInfoAttack = other.SpriteInfoAttack;
	SpriteInfoDefend = other.SpriteInfoDefend;
	SpriteInfoSkill = other.SpriteInfoSkill;
	SpriteInfoHit = other.SpriteInfoHit;
	SpriteInfoDead = other.SpriteInfoDead;
	SpriteInfoIllustration = other.SpriteInfoIllustration;

	other.hSpriteIdle = NULL;
	other.hSpriteWalk = NULL;
	other.hSpriteAttack = NULL;
	other.hSpriteDefend = NULL;
	other.hSpriteSkill = NULL;
	other.hSpriteHit = NULL;
	other.hSpriteDead = NULL;

	other.hIllustration = NULL;
	return *this;
}

// ============================================================
// 초기화 / 해제
// ============================================================

void Character::Init(ECharacterClass classType, int gridX, int gridY)
{
	ClassType = classType;

	State = ECharacterState::Idle;
	Direction = EDirection::Left;
	Element = EElement::None;

	GridPos.x = gridX;
	GridPos.y = gridY;

	PixelPos.x = 0;
	PixelPos.y = 0;

	IsMoving = false;

	WorldPixelX = 0.0f;
	WorldPixelY = 0.0f;

	TargetPixelX = 0.0f;
	TargetPixelY = 0.0f;

	TargetGridX = gridX;
	TargetGridY = gridY;

	MoveSpeed = 8.0f;

	CurFrame = 0;
	MaxFrame = CHARACTER_SHEET_COL;
	AniTick = 0;
	AniDelay = 8;

	IsActionEnd = false;

	Level = 1;
	Exp = 0;
	Money = 0;

	LearnedSkillIDs.clear();
	ClearEquippedSkills();

	Equip = CharacterEquipment();

	bHasMoved = false;
	bHasActed = false;

	InitClassValue();
}

void Character::Release()
{
	HBITMAP handles[8] =
	{
		hSpriteIdle,
		hSpriteWalk,
		hSpriteAttack,
		hSpriteDefend,
		hSpriteSkill,
		hSpriteHit,
		hSpriteDead,
		hIllustration
	};

	for (int i = 0; i < 8; ++i)
	{
		if (handles[i] == NULL)
		{
			continue;
		}

		bool alreadyDeleted = false;

		for (int j = 0; j < i; ++j)
		{
			if (handles[i] == handles[j])
			{
				alreadyDeleted = true;
				break;
			}
		}

		if (!alreadyDeleted)
		{
			DeleteObject(handles[i]);
		}
	}

	hSpriteIdle = NULL;
	hSpriteWalk = NULL;
	hSpriteAttack = NULL;
	hSpriteDefend = NULL;
	hSpriteSkill = NULL;
	hSpriteHit = NULL;
	hSpriteDead = NULL;
	hIllustration = NULL;

	ZeroMemory(&SpriteInfoIdle, sizeof(BITMAP));
	ZeroMemory(&SpriteInfoWalk, sizeof(BITMAP));
	ZeroMemory(&SpriteInfoAttack, sizeof(BITMAP));
	ZeroMemory(&SpriteInfoDefend, sizeof(BITMAP));
	ZeroMemory(&SpriteInfoSkill, sizeof(BITMAP));
	ZeroMemory(&SpriteInfoHit, sizeof(BITMAP));
	ZeroMemory(&SpriteInfoDead, sizeof(BITMAP));
	ZeroMemory(&SpriteInfoIllustration, sizeof(BITMAP));
}

// ============================================================
// 직업별 기본값
// ============================================================

void Character::InitClassValue()
{
	CharacterStat newStat;
	LearnedSkillIDs.clear();

	switch (ClassType)
	{
	case ECharacterClass::Warrior:
		newStat.MaxHP = 150;
		newStat.CurHP = 150;
		newStat.MaxMP = 20;
		newStat.CurMP = 20;
		newStat.STR = 22;
		newStat.INT = 4;
		newStat.DEF = 15;
		newStat.SPD = 8;

		AP = 3;
		DefendRate = 0.45f;
		Element = EElement::Fire;
		MoveSpeed = 7.5f;
		break;

	case ECharacterClass::Archer:
		newStat.MaxHP = 100;
		newStat.CurHP = 100;
		newStat.MaxMP = 30;
		newStat.CurMP = 30;
		newStat.STR = 16;
		newStat.INT = 8;
		newStat.DEF = 7;
		newStat.SPD = 16;

		AP = 4;
		DefendRate = 0.65f;
		Element = EElement::Water;
		MoveSpeed = 9.0f;
		break;

	case ECharacterClass::Mage:
		newStat.MaxHP = 80;
		newStat.CurHP = 80;
		newStat.MaxMP = 80;
		newStat.CurMP = 80;
		newStat.STR = 6;
		newStat.INT = 26;
		newStat.DEF = 4;
		newStat.SPD = 10;

		AP = 3;
		DefendRate = 0.75f;
		Element = EElement::Grass;
		MoveSpeed = 8.0f;
		break;

	default:
		newStat.MaxHP = 100;
		newStat.CurHP = 100;
		newStat.MaxMP = 30;
		newStat.CurMP = 30;
		newStat.STR = 10;
		newStat.INT = 10;
		newStat.DEF = 5;
		newStat.SPD = 10;

		AP = 3;
		DefendRate = 0.5f;
		Element = EElement::None;
		MoveSpeed = 8.0f;
		break;
	}

	Stat = newStat;
	RebuildSkillsFromClassElement();
}

// ============================================================
// 스프라이트 로드
// ============================================================

HBITMAP Character::LoadBitmapFromFile(const std::wstring& filePath, BITMAP* outInfo)
{
	HBITMAP hBitmap = (HBITMAP)LoadImageW(
		NULL,
		filePath.c_str(),
		IMAGE_BITMAP,
		0,
		0,
		LR_LOADFROMFILE | LR_CREATEDIBSECTION
	);

	if (hBitmap != NULL && outInfo != nullptr)
	{
		GetObject(hBitmap, sizeof(BITMAP), outInfo);
	}

	return hBitmap;
}

std::wstring Character::GetCharacterPrefix() const
{
	switch (ClassType)
	{
	case ECharacterClass::Warrior:
		return L"Warrior";

	case ECharacterClass::Archer:
		return L"Archer";

	case ECharacterClass::Mage:
		return L"Mage";

	default:
		return L"Character";
	}
}

bool Character::LoadSprites(const std::wstring& folderPath)
{
	Release();

	std::wstring basePath = folderPath;

	if (!basePath.empty())
	{
		wchar_t lastChar = basePath[basePath.length() - 1];

		if (lastChar != L'\\' && lastChar != L'/')
		{
			basePath += L"\\";
		}
	}

	std::wstring prefix = GetCharacterPrefix();

	hSpriteIdle = LoadBitmapFromFile(basePath + prefix + L"_Idle.bmp", &SpriteInfoIdle);
	hSpriteWalk = LoadBitmapFromFile(basePath + prefix + L"_Walk.bmp", &SpriteInfoWalk);
	hSpriteAttack = LoadBitmapFromFile(basePath + prefix + L"_Attack.bmp", &SpriteInfoAttack);
	hSpriteDefend = LoadBitmapFromFile(basePath + prefix + L"_Defend.bmp", &SpriteInfoDefend);
	hSpriteSkill = LoadBitmapFromFile(basePath + prefix + L"_Skill.bmp", &SpriteInfoSkill);
	hSpriteHit = LoadBitmapFromFile(basePath + prefix + L"_Hit.bmp", &SpriteInfoHit);
	hSpriteDead = LoadBitmapFromFile(basePath + prefix + L"_Dead.bmp", &SpriteInfoDead);
	hIllustration = LoadBitmapFromFile(basePath + prefix + L"_Illustration.bmp", &SpriteInfoIllustration);

	if (hSpriteIdle == NULL)
	{
		return false;
	}

	return true;
}

HBITMAP Character::GetSpriteIdle() const
{
	return hSpriteIdle;
}

HBITMAP Character::GetIllustration() const
{
	return hIllustration;
}

const BITMAP* Character::GetIllustrationInfo() const
{
	if (hIllustration == NULL)
	{
		return nullptr;
	}

	return &SpriteInfoIllustration;
}

// ============================================================
// 현재 상태에 맞는 스프라이트 반환
// ============================================================

HBITMAP Character::GetCurrentSprite() const
{
	switch (State)
	{
	case ECharacterState::Idle:
		if (hSpriteIdle != NULL) return hSpriteIdle;
		break;

	case ECharacterState::Walk:
		if (hSpriteWalk != NULL) return hSpriteWalk;
		break;

	case ECharacterState::Attack:
		if (hSpriteAttack != NULL) return hSpriteAttack;
		break;

	case ECharacterState::Defend:
		if (hSpriteDefend != NULL) return hSpriteDefend;
		break;

	case ECharacterState::Skill:
		if (hSpriteSkill != NULL) return hSpriteSkill;
		break;

	case ECharacterState::Hit:
		if (hSpriteHit != NULL) return hSpriteHit;
		break;

	case ECharacterState::Dead:
		if (hSpriteDead != NULL) return hSpriteDead;
		break;
	}

	return hSpriteIdle;
}

const BITMAP* Character::GetCurrentSpriteInfo() const
{
	switch (State)
	{
	case ECharacterState::Idle:
		if (hSpriteIdle != NULL) return &SpriteInfoIdle;
		break;

	case ECharacterState::Walk:
		if (hSpriteWalk != NULL) return &SpriteInfoWalk;
		break;

	case ECharacterState::Attack:
		if (hSpriteAttack != NULL) return &SpriteInfoAttack;
		break;

	case ECharacterState::Defend:
		if (hSpriteDefend != NULL) return &SpriteInfoDefend;
		break;

	case ECharacterState::Skill:
		if (hSpriteSkill != NULL) return &SpriteInfoSkill;
		break;

	case ECharacterState::Hit:
		if (hSpriteHit != NULL) return &SpriteInfoHit;
		break;

	case ECharacterState::Dead:
		if (hSpriteDead != NULL) return &SpriteInfoDead;
		break;
	}

	return &SpriteInfoIdle;
}

int Character::GetDirectionRow() const
{
	return 0;
}

bool Character::IsNeedFlip() const
{
	// 기본 가정:
	// 원본 캐릭터 스프라이트는 왼쪽 방향 기준.
	// 오른쪽 방향일 때 좌우 반전한다.
	return Direction == EDirection::Right;
}

void Character::UpdateMaxFrameByState()
{
	MaxFrame = CHARACTER_SHEET_COL;

	if (MaxFrame <= 0)
	{
		MaxFrame = 1;
	}

	if (CurFrame >= MaxFrame)
	{
		CurFrame = 0;
	}
}

// ============================================================
// Update / Draw
// ============================================================

void Character::Update()
{
	if (Stat.CurHP <= 0 && State != ECharacterState::Dead)
	{
		Die();
	}

	if (IsMoving)
	{
		float dx = TargetPixelX - WorldPixelX;
		float dy = TargetPixelY - WorldPixelY;

		float dist = sqrtf(dx * dx + dy * dy);

		if (dist <= MoveSpeed)
		{
			WorldPixelX = TargetPixelX;
			WorldPixelY = TargetPixelY;

			GridPos.x = TargetGridX;
			GridPos.y = TargetGridY;

			IsMoving = false;
			SetState(ECharacterState::Idle);
		}
		else if (dist > 0.0f)
		{
			WorldPixelX += (dx / dist) * MoveSpeed;
			WorldPixelY += (dy / dist) * MoveSpeed;
		}
	}

	if (State == ECharacterState::Idle)
	{
		CurFrame = 0;
		return;
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
		case ECharacterState::Walk:
			CurFrame = 0;
			break;

		case ECharacterState::Attack:
		case ECharacterState::Defend:
		case ECharacterState::Skill:
		case ECharacterState::Hit:
		case ECharacterState::Dead:
			CurFrame = MaxFrame - 1;
			IsActionEnd = true;
			break;

		default:
			CurFrame = 0;
			break;
		}
	}
}

void Character::Draw(HDC hMemDC, HDC hMemDC1)
{
	Draw(hMemDC, hMemDC1, 0, 0);
}

void Character::Draw(HDC hMemDC, HDC hMemDC1, int drawW, int drawH)
{
	HBITMAP curSprite = GetCurrentSprite();
	const BITMAP* curInfo = GetCurrentSpriteInfo();

	if (curSprite == NULL || curInfo == nullptr)
	{
		return;
	}

	if (curInfo->bmWidth <= 0 || curInfo->bmHeight <= 0)
	{
		return;
	}

	// 기본은 4열 기준
	int frameW = curInfo->bmWidth / CHARACTER_SHEET_COL;

	if (frameW <= 0)
	{
		return;
	}

	// 현재 프로젝트 임시 리소스는 4x3 구조:
	// 1448 x 1086 => frameW 362, frameH 362
	//
	// 나중에 1x4 구조로 바꾸면:
	// 전체 높이 = frameH, rowCount = 1
	int rowCount = 1;

	if (curInfo->bmHeight % 3 == 0)
	{
		int candidateFrameH = curInfo->bmHeight / 3;

		// 4x3 임시 스프라이트는 프레임이 정사각형 362x362이므로
		// frameW와 candidateFrameH가 같으면 3행 시트로 판단
		if (candidateFrameH == frameW)
		{
			rowCount = 3;
		}
	}

	int frameH = curInfo->bmHeight / rowCount;

	if (frameH <= 0)
	{
		return;
	}

	if (CurFrame < 0)
	{
		CurFrame = 0;
	}

	if (CurFrame >= CHARACTER_SHEET_COL)
	{
		CurFrame = CHARACTER_SHEET_COL - 1;
	}

	if (drawW <= 0)
	{
		drawW = frameW;
	}

	if (drawH <= 0)
	{
		drawH = frameH;
	}

	// 4x3이면 가운데 행이 좌우 방향이라고 보고 사용
	// 1x4이면 0행 사용
	int srcRow = 0;

	if (rowCount == 3)
	{
		srcRow = 1;
	}

	int srcX = CurFrame * frameW;
	int srcY = srcRow * frameH;

	int dstX = PixelPos.x;
	int dstY = PixelPos.y;

	HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC1, curSprite);

	if (!IsNeedFlip())
	{
		TransparentBlt(
			hMemDC,
			dstX,
			dstY,
			drawW,
			drawH,
			hMemDC1,
			srcX,
			srcY,
			frameW,
			frameH,
			CHARACTER_TRANSPARENT_COLOR
		);
	}
	else
	{
		HDC hFlipDC = CreateCompatibleDC(hMemDC);

		HBITMAP hFlipBitmap = CreateCompatibleBitmap(
			hMemDC,
			frameW,
			frameH
		);

		HBITMAP hOldFlipBitmap = (HBITMAP)SelectObject(hFlipDC, hFlipBitmap);

		HBRUSH hBrush = CreateSolidBrush(CHARACTER_TRANSPARENT_COLOR);

		RECT rc;
		rc.left = 0;
		rc.top = 0;
		rc.right = frameW;
		rc.bottom = frameH;

		FillRect(hFlipDC, &rc, hBrush);
		DeleteObject(hBrush);

		StretchBlt(
			hFlipDC,
			frameW,
			0,
			-frameW,
			frameH,
			hMemDC1,
			srcX,
			srcY,
			frameW,
			frameH,
			SRCCOPY
		);

		TransparentBlt(
			hMemDC,
			dstX,
			dstY,
			drawW,
			drawH,
			hFlipDC,
			0,
			0,
			frameW,
			frameH,
			CHARACTER_TRANSPARENT_COLOR
		);

		SelectObject(hFlipDC, hOldFlipBitmap);
		DeleteObject(hFlipBitmap);
		DeleteDC(hFlipDC);
	}

	SelectObject(hMemDC1, hOldBitmap);
}

// ============================================================
// 상태
// ============================================================

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

	// 캐릭터 상태별 효과음
	switch (newState)
	{
	case ECharacterState::Attack:
		SoundManager::PlaySFX(L"Resource\\Sound\\Character_Attack.wav");
		break;

	case ECharacterState::Skill:
		SoundManager::PlaySFX(L"Resource\\Sound\\Character_Skill.wav");
		break;

	case ECharacterState::Defend:
		SoundManager::PlaySFX(L"Resource\\Sound\\Guard.wav");
		break;

	case ECharacterState::Hit:
		SoundManager::PlaySFX(L"Resource\\Sound\\Character_Hit.wav");
		break;

	case ECharacterState::Dead:
		SoundManager::PlaySFX(L"Resource\\Sound\\Character_Dead.wav");
		break;
	}
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

void Character::SetIsActionEnd(bool value)
{
	IsActionEnd = value;
}

// ============================================================
// ID / 이름 / 직업 / 속성
// ============================================================

void Character::SetCharacterID(int id)
{
	CharacterID = id;
}

int Character::GetCharacterID() const
{
	return CharacterID;
}

void Character::SetName(const std::wstring& newName)
{
	Name = newName;
}

std::wstring Character::GetName() const
{
	return Name;
}

ECharacterClass Character::GetClassType() const
{
	return ClassType;
}

void Character::SetElement(EElement newElement)
{
	Element = newElement;
	RebuildSkillsFromClassElement();
}

EElement Character::GetElement() const
{
	return Element;
}

std::wstring Character::GetElementName() const
{
	return SkillData::GetElementName(Element);
}

// ============================================================
// 위치
// ============================================================

void Character::SetGridPos(int gx, int gy)
{
	GridPos.x = gx;
	GridPos.y = gy;
}

Location Character::GetGridPos() const
{
	return GridPos;
}

void Character::SetPixelPos(int px, int py)
{
	PixelPos.x = px;
	PixelPos.y = py;
}

Location Character::GetPixelPos() const
{
	return PixelPos;
}

void Character::SyncWorldPixelToGrid(int tileSize)
{
	WorldPixelX = static_cast<float>(GridPos.x * tileSize);
	WorldPixelY = static_cast<float>(GridPos.y * tileSize);

	TargetPixelX = WorldPixelX;
	TargetPixelY = WorldPixelY;

	TargetGridX = GridPos.x;
	TargetGridY = GridPos.y;
}

Location Character::GetWorldPixelPos(int tileSize) const
{
	Location result;

	if (IsMoving)
	{
		result.x = static_cast<int>(WorldPixelX);
		result.y = static_cast<int>(WorldPixelY);
	}
	else
	{
		result.x = GridPos.x * tileSize;
		result.y = GridPos.y * tileSize;
	}

	return result;
}

bool Character::StartMoveToGrid(int targetX, int targetY, int tileSize)
{
	if (IsMoving)
	{
		return false;
	}

	if (!IsInMoveRange(targetX, targetY))
	{
		return false;
	}

	TargetGridX = targetX;
	TargetGridY = targetY;

	WorldPixelX = static_cast<float>(GridPos.x * tileSize);
	WorldPixelY = static_cast<float>(GridPos.y * tileSize);

	TargetPixelX = static_cast<float>(targetX * tileSize);
	TargetPixelY = static_cast<float>(targetY * tileSize);

	LookAtGrid(targetX, targetY);

	IsMoving = true;
	SetState(ECharacterState::Walk);

	return true;
}

bool Character::GetIsMoving() const
{
	return IsMoving;
}

bool Character::MoveToGrid(int targetX, int targetY)
{
	if (!IsInMoveRange(targetX, targetY))
	{
		return false;
	}

	LookAtGrid(targetX, targetY);

	GridPos.x = targetX;
	GridPos.y = targetY;

	IsMoving = false;

	SyncWorldPixelToGrid(1);

	return true;
}

// ============================================================
// 스탯
// ============================================================

CharacterStat Character::GetStat() const
{
	return Stat;
}

void Character::SetStat(const CharacterStat& newStat)
{
	Stat = newStat;

	if (Stat.MaxHP < 1)
	{
		Stat.MaxHP = 1;
	}

	if (Stat.CurHP > Stat.MaxHP)
	{
		Stat.CurHP = Stat.MaxHP;
	}

	if (Stat.CurHP < 0)
	{
		Stat.CurHP = 0;
	}

	if (Stat.MaxMP < 0)
	{
		Stat.MaxMP = 0;
	}

	if (Stat.CurMP > Stat.MaxMP)
	{
		Stat.CurMP = Stat.MaxMP;
	}

	if (Stat.CurMP < 0)
	{
		Stat.CurMP = 0;
	}
}

int Character::GetHP() const
{
	return Stat.CurHP;
}

int Character::GetMaxHP() const
{
	return Stat.MaxHP;
}

int Character::GetMP() const
{
	return Stat.CurMP;
}

int Character::GetMaxMP() const
{
	return Stat.MaxMP;
}

int Character::GetLevel() const
{
	return Level;
}

void Character::SetLevel(int newLevel)
{
	Level = newLevel;

	if (Level < 1)
	{
		Level = 1;
	}
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

	while (Exp >= Level * 100)
	{
		Exp -= Level * 100;
		Level++;

		switch (ClassType)
		{
		case ECharacterClass::Warrior:
			Stat.MaxHP += 15;
			Stat.MaxMP += 2;
			Stat.STR += 3;
			Stat.INT += 1;
			Stat.DEF += 2;
			Stat.SPD += 1;
			break;

		case ECharacterClass::Archer:
			Stat.MaxHP += 10;
			Stat.MaxMP += 4;
			Stat.STR += 2;
			Stat.INT += 1;
			Stat.DEF += 1;
			Stat.SPD += 2;
			break;

		case ECharacterClass::Mage:
			Stat.MaxHP += 7;
			Stat.MaxMP += 8;
			Stat.STR += 1;
			Stat.INT += 3;
			Stat.DEF += 1;
			Stat.SPD += 1;
			break;

		default:
			Stat.MaxHP += 10;
			Stat.MaxMP += 5;
			Stat.STR += 2;
			Stat.INT += 2;
			Stat.DEF += 1;
			Stat.SPD += 1;
			break;
		}

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

int Character::GetAP() const
{
	return AP;
}

void Character::SetAP(int newAP)
{
	AP = newAP;

	if (AP < 0)
	{
		AP = 0;
	}
}

// ============================================================
// 장비
// ============================================================

CharacterEquipment Character::GetEquipment() const
{
	return Equip;
}

void Character::SetEquipment(const CharacterEquipment& newEquipment)
{
	Equip = newEquipment;
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

// ============================================================
// 스킬
// ============================================================

void Character::AddSkillID(int skillID)
{
	for (int id : LearnedSkillIDs)
	{
		if (id == skillID)
		{
			return;
		}
	}

	LearnedSkillIDs.push_back(skillID);
}

void Character::ClearSkillIDs()
{
	LearnedSkillIDs.clear();
	ClearEquippedSkills();
}

void Character::RebuildSkillsFromClassElement()
{
	LearnedSkillIDs.clear();
	ClearEquippedSkills();

	std::vector<int> skillIDs = SkillData::GetSkillIDs(ClassType, Element);

	for (int skillID : skillIDs)
	{
		LearnedSkillIDs.push_back(skillID);
	}

	int equipCount = (int)skillIDs.size();
	if (equipCount > 3)
	{
		equipCount = 3;
	}

	for (int i = 0; i < equipCount; ++i)
	{
		EquippedSkillIDs[i] = skillIDs[i];
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

void Character::SetEquippedSkillID(int slot, int skillID)
{
	if (slot < 0 || slot >= 3)
	{
		return;
	}

	if (skillID >= 0)
	{
		if (!HasSkillID(skillID) || !SkillData::CanCharacterUse(this, skillID))
		{
			return;
		}
	}

	EquippedSkillIDs[slot] = skillID;
}

int Character::GetEquippedSkillID(int slot) const
{
	if (slot < 0 || slot >= 3)
	{
		return -1;
	}

	return EquippedSkillIDs[slot];
}

std::vector<int> Character::GetEquippedSkillIDs() const
{
	std::vector<int> result;

	for (int i = 0; i < 3; ++i)
	{
		if (EquippedSkillIDs[i] >= 0 && HasSkillID(EquippedSkillIDs[i]))
		{
			result.push_back(EquippedSkillIDs[i]);
		}
	}

	return result;
}

void Character::ClearEquippedSkills()
{
	for (int i = 0; i < 3; ++i)
	{
		EquippedSkillIDs[i] = -1;
	}
}

// ============================================================
// 전투 판정
// ============================================================

bool Character::IsAlive() const
{
	return Stat.CurHP > 0;
}

bool Character::IsInMoveRange(int targetX, int targetY) const
{
	int dx = std::abs(targetX - GridPos.x);
	int dy = std::abs(targetY - GridPos.y);

	return dx + dy <= AP;
}

bool Character::IsInAttackRange(const Character& target) const
{
	Location targetPos = target.GetGridPos();

	return IsInAttackRangeByGrid(targetPos.x, targetPos.y);
}

bool Character::IsInAttackRangeByGrid(int targetX, int targetY) const
{
	int dx = std::abs(targetX - GridPos.x);
	int dy = std::abs(targetY - GridPos.y);

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

	default:
		attackRange = 1;
		break;
	}

	return dx + dy <= attackRange;
}

int Character::CalcAttackDamage(const Character& target) const
{
	CharacterStat targetStat = target.GetStat();

	int damage = 1;

	switch (ClassType)
	{
	case ECharacterClass::Warrior:
		damage = Stat.STR * 2 - targetStat.DEF;
		break;

	case ECharacterClass::Archer:
		damage = Stat.STR * 2 + Stat.SPD / 2 - targetStat.DEF;
		break;

	case ECharacterClass::Mage:
		damage = Stat.INT + Stat.STR - targetStat.DEF;
		break;

	default:
		damage = Stat.STR * 2 - targetStat.DEF;
		break;
	}

	if (damage < 1)
	{
		damage = 1;
	}

	return damage;
}

int Character::CalcSkillDamage(const Character& target) const
{
	CharacterStat targetStat = target.GetStat();

	int damage = 1;

	switch (ClassType)
	{
	case ECharacterClass::Warrior:
		damage = Stat.STR * 3 - targetStat.DEF;
		break;

	case ECharacterClass::Archer:
		damage = Stat.STR * 2 + Stat.SPD - targetStat.DEF / 2;
		break;

	case ECharacterClass::Mage:
		damage = Stat.INT * 3 - targetStat.DEF / 2;
		break;

	default:
		damage = Stat.INT * 2 - targetStat.DEF;
		break;
	}

	if (damage < 1)
	{
		damage = 1;
	}

	return damage;
}

void Character::GetDamage(int damage)
{
	if (State == ECharacterState::Dead)
	{
		return;
	}

	if (damage < 0)
	{
		damage = 0;
	}

	if (State == ECharacterState::Defend)
	{
		damage = static_cast<int>(damage * DefendRate);
	}

	Stat.CurHP -= damage;

	if (Stat.CurHP <= 0)
	{
		Stat.CurHP = 0;
		Die();
	}
	else
	{
		SetState(ECharacterState::Hit);
	}
}

void Character::Die()
{
	Stat.CurHP = 0;
	IsMoving = false;

	SetState(ECharacterState::Dead);
}

float Character::GetDefendRate() const
{
	return DefendRate;
}

void Character::SetDefendRate(float rate)
{
	DefendRate = rate;

	if (DefendRate < 0.0f)
	{
		DefendRate = 0.0f;
	}

	if (DefendRate > 1.0f)
	{
		DefendRate = 1.0f;
	}
}

// ============================================================
// 방향 보조
// ============================================================

void Character::LookAtGrid(int targetX, int targetY)
{
	int dx = targetX - GridPos.x;

	if (dx < 0)
	{
		SetDirection(EDirection::Left);
	}
	else if (dx > 0)
	{
		SetDirection(EDirection::Right);
	}
}