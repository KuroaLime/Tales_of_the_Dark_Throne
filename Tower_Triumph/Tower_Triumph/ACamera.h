#pragma once
#include<utility>
class ACamera
{
protected:
	float g_CamX;
	float g_CamY;

	float PointGridX;
	float PointGridY;

	float currentZoom;
	float targetZoom;
	const float defaultZoom;
public:
	ACamera(int N_CamX, int N_CamY, int PointIndexX, int PointIndexY, float initialZoom = 1.5f);
	virtual void Reset();
	virtual void Update(int TILE_W, int TILE_H, int TILE_SIZE, int WIN_WIDTH, int WIN_HEIGHT);
	
	void SetPointGrid(float x, float y);
	std::pair<float, float> Getg_Cam();
	std::pair<float, float> GetPointGrid();

	void SetZoom(float zoom) { targetZoom = zoom; }
	float GetZoom() const { return currentZoom; }

};

