#include "URegionMarker.h"
#include <cmath>

URegionMarker::URegionMarker()
{
	regionImage = nullptr;
	iconImage = nullptr;
	animationTime = 0.0f;
	floatingRange = 15.0f;
	bIsActivated = false;
	currentScale = 1.0f;
	targetScale = 1.0f;
}

void URegionMarker::Setup(HINSTANCE hInst, int x, int y, WORD regionResID, WORD iconQResID, WORD iconEResID)
{
	SetLocation(x, y);
	Size.sX = 120;
	Size.sY = 120;

	regionImage = new UImage();
	regionImage->SetLocation(x, y);
	regionImage->SetFrame(hInst, { regionResID });
	regionImage->SetUIID(100);
	// AddChild(regionImage); // We will draw manually to handle scaling

	iconImage = new UImage();
	baseIconY = y - 70;
	iconImage->SetLocation(x, baseIconY);
	iconImage->SetFrame(hInst, { iconQResID, iconEResID }); // 0: ?, 1: !
	iconImage->SetUIID(101);
	// AddChild(iconImage);
}

void URegionMarker::SetRegionInfo(const std::wstring& parentName, const std::wstring& name, const std::wstring& desc, const std::wstring& env)
{
	parentRegionName = parentName;
	regionName = name;
	regionDesc = desc;
	environment = env;
}

void URegionMarker::Update()
{
	if (!bVisible) return;

	// Floating Animation
	animationTime += 0.05f;
	int offsetY = (int)(sin(animationTime) * floatingRange);
	iconImage->SetLocation(Loc.x, baseIconY + offsetY);

	// Scale Interpolation
	currentScale += (targetScale - currentScale) * 0.2f;

	UContainerWidget::Update();
}

void URegionMarker::Draw(HDC hMemDC, HDC hMemDC1)
{
	// Default Draw might not be used if Map_Stage calls DrawWithOffset
}

void URegionMarker::DrawWithOffset(HDC hMemDC, HDC hMemDC1, int offsetX, int offsetY)
{
	if (!bVisible) return;

	int drawX = Loc.x - offsetX;
	int drawY = Loc.y - offsetY;

	// Draw Region Image with Scale
	int scaledSX = (int)(Size.sX * currentScale);
	int scaledSY = (int)(Size.sY * currentScale);

	if (regionImage && !regionImage->GetBitmap().empty()) {
		HBITMAP hBit = regionImage->GetBitmap()[0];
		BITMAP info = regionImage->GetMapInfo()[0];
		HBITMAP old = (HBITMAP)SelectObject(hMemDC1, hBit);
		TransparentBlt(hMemDC, drawX - scaledSX / 2, drawY - scaledSY / 2, scaledSX, scaledSY, 
			hMemDC1, 0, 0, info.bmWidth, info.bmHeight, RGB(255, 255, 255));
		SelectObject(hMemDC1, old);
	}

	// Draw Icon Image
	int iconDrawY = iconImage->GetLocation().y - offsetY;
	if (iconImage && !iconImage->GetBitmap().empty()) {
		int frame = bIsActivated ? 1 : 0;
		HBITMAP hBit = iconImage->GetBitmap()[frame];
		BITMAP info = iconImage->GetMapInfo()[frame];
		HBITMAP old = (HBITMAP)SelectObject(hMemDC1, hBit);
		TransparentBlt(hMemDC, drawX - 32, iconDrawY - 32, 64, 64, 
			hMemDC1, 0, 0, info.bmWidth, info.bmHeight, RGB(255, 255, 255));
		SelectObject(hMemDC1, old);
	}
}

bool URegionMarker::ContainsPointWithOffset(int MouseX, int MouseY, int offsetX, int offsetY)
{
	int worldMX = MouseX + offsetX;
	int worldMY = MouseY + offsetY;
	return ContainsPoint(worldMX, worldMY);
}

void URegionMarker::Activate()
{
	bIsActivated = true;
	targetScale = 1.3f;
}

void URegionMarker::Deactivate()
{
	bIsActivated = false;
	targetScale = 1.0f;
}
