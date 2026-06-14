#pragma once
#include "UContainerWidget.h"
#include "UImage.h"
#include <string>

class URegionMarker : public UContainerWidget
{
private:
	UImage* regionImage;
	UImage* iconImage;
	
	float animationTime;
	float floatingRange;
	int baseIconY;
	
	bool bIsActivated; // true if clicked (!)
	std::wstring parentRegionName; // broad region (e.g. Hyzante)
	std::wstring regionName;       // specific point (e.g. Bridge)
	std::wstring regionDesc;
	std::wstring environment;

	float currentScale;
	float targetScale;

public:
	URegionMarker();
	virtual void Update() override;
	virtual void Draw(HDC hMemDC, HDC hMemDC1) override;

	void Setup(HINSTANCE hInst, int x, int y, const std::wstring& regionPath, WORD iconQResID, WORD iconEResID);
	void SetRegionInfo(const std::wstring& parentName, const std::wstring& name, const std::wstring& desc, const std::wstring& env);
	
	void Activate();
	void Deactivate();

	std::wstring GetParentName() const { return parentRegionName; }
	std::wstring GetName() const { return regionName; }
	std::wstring GetDesc() const { return regionDesc; }
	std::wstring GetEnv() const { return environment; }
	
	Location GetIconLocation() const { return iconImage ? iconImage->GetLocation() : Loc; }
	UImage* GetIconImage() const { return iconImage; }

	// For camera offset
	void DrawWithOffset(HDC hMemDC, HDC hMemDC1, int offsetX, int offsetY, float zoom = 1.0f);
	bool ContainsPointWithOffset(int MouseX, int MouseY, int offsetX, int offsetY, float zoom = 1.0f);
};
