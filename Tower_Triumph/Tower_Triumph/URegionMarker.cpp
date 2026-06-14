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

void URegionMarker::Setup(HINSTANCE hInst, int x, int y, const std::wstring& regionPath, WORD iconQResID, WORD iconEResID)
{
	SetLocation(x, y);
	Size.sX = 120;
	Size.sY = 120;

	regionImage = new UImage();
	regionImage->SetLocation(x, y);
	regionImage->SetFrameFromFile({ regionPath });
	regionImage->SetUIID(100);

	iconImage = new UImage();
	baseIconY = y - 70;
	iconImage->SetLocation(x, baseIconY);
	iconImage->SetFrame(hInst, { iconQResID, iconEResID }); // 0: ?, 1: !
	iconImage->SetUIID(101);
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
}

void URegionMarker::DrawWithOffset(HDC hMemDC, HDC hMemDC1, int offsetX, int offsetY, float zoom)
{
	if (!bVisible) return;

	int drawX = (int)((Loc.x - offsetX) * zoom);
	int drawY = (int)((Loc.y - offsetY) * zoom);

	// Draw Region Image with Scale
	int scaledSX = (int)(Size.sX * currentScale * zoom);
	int scaledSY = (int)(Size.sY * currentScale * zoom);

	if (regionImage && !regionImage->GetBitmap().empty()) {
		HBITMAP hBit = regionImage->GetBitmap()[0];
		BITMAP info = regionImage->GetMapInfo()[0];
		HBITMAP old = (HBITMAP)SelectObject(hMemDC1, hBit);
		TransparentBlt(hMemDC, drawX - scaledSX / 2, drawY - scaledSY / 2, scaledSX, scaledSY, 
			hMemDC1, 0, 0, info.bmWidth, info.bmHeight, RGB(255, 255, 255));
		SelectObject(hMemDC1, old);
	}

	// Draw Icon Image
	int iconWorldY = iconImage->GetLocation().y;
	int iconDrawY = (int)((iconWorldY - offsetY) * zoom);
	int iconSize = (int)(64 * zoom);

	if (iconImage && !iconImage->GetBitmap().empty()) {
		int frame = bIsActivated ? 1 : 0;
		HBITMAP hBit = iconImage->GetBitmap()[frame];
		BITMAP info = iconImage->GetMapInfo()[frame];
		HBITMAP old = (HBITMAP)SelectObject(hMemDC1, hBit);
		TransparentBlt(hMemDC, drawX - iconSize / 2, iconDrawY - iconSize / 2, iconSize, iconSize, 
			hMemDC1, 0, 0, info.bmWidth, info.bmHeight, RGB(255, 255, 255));
		SelectObject(hMemDC1, old);
	}
}

bool URegionMarker::ContainsPointWithOffset(int MouseX, int MouseY, int offsetX, int offsetY, float zoom)
{
	int worldMX = (int)(MouseX / zoom + offsetX);
	int worldMY = (int)(MouseY / zoom + offsetY);
	return ContainsPoint(worldMX, worldMY);
}

void URegionMarker::Activate()
{
	bIsActivated = true;
	targetScale = 1.5f;
}

void URegionMarker::Deactivate()
{
	bIsActivated = false;
	targetScale = 1.0f;
}
