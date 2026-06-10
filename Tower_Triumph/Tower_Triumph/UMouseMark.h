#pragma once
#include "UImage.h"

class URegionMarker;

class UMouseMark : public UImage
{
private:
	URegionMarker* targetMarker;
	float angle;
	POINT currentMousePos;
	int cameraX, cameraY;

public:
	UMouseMark();
	virtual void Update() override;
	virtual void Draw(HDC hMemDC, HDC hMemDC1) override;

	void SetTarget(URegionMarker* target) { targetMarker = target; }
	void SetMousePos(POINT pt) { currentMousePos = pt; }
	void SetCameraOffset(int x, int y) { cameraX = x; cameraY = y; }
};

