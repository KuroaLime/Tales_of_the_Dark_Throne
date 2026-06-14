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
	float currentZoom;

public:
	UMouseMark();
	virtual void Update() override;
	virtual void Draw(HDC hMemDC, HDC hMemDC1) override;

	void SetTarget(URegionMarker* target) { targetMarker = target; }
	void SetMousePos(POINT pt) { currentMousePos = pt; }
	void SetCameraInfo(int x, int y, float zoom) { cameraX = x; cameraY = y; currentZoom = zoom; }
};

