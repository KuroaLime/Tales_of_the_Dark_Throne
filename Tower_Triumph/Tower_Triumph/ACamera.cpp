#include "ACamera.h"

ACamera::ACamera(int N_CamX, int N_CamY, int PointIndexX, int PointIndexY) {
    g_CamX = N_CamX * 0.8f;
    g_CamY = N_CamY * 0.8f;

    PointGridX = (float)PointIndexX;
    PointGridY = (float)PointIndexY;

    currentZoom = 1.0f;
    targetZoom = 1.0f;
}
void ACamera::Reset() {
    g_CamX = 0;
    g_CamY = 0;

    PointGridX = 0;
    PointGridY = 0;
    currentZoom = 1.0f;
    targetZoom = 1.0f;
}
void ACamera::Update(int TILE_W, int TILE_H, int TILE_SIZE, int WIN_WIDTH, int WIN_HEIGHT) {
    // Smooth zoom transition
    currentZoom += (targetZoom - currentZoom) * 0.1f;

    // Center on the target point
    float targetX = (float)(PointGridX * TILE_SIZE - (WIN_WIDTH / 2) / currentZoom + TILE_SIZE / 2);
    float targetY = (float)(PointGridY * TILE_SIZE - (WIN_HEIGHT / 2) / currentZoom + TILE_SIZE / 2);

    // Bound check (considering zoom)
    float maxW = (float)TILE_W * TILE_SIZE;
    float maxH = (float)TILE_H * TILE_SIZE;
    float visibleW = (float)WIN_WIDTH / currentZoom;
    float visibleH = (float)WIN_HEIGHT / currentZoom;

    if (targetX < 0) targetX = 0;
    if (targetY < 0) targetY = 0;
    if (targetX > (maxW - visibleW)) targetX = maxW - visibleW;
    if (targetY > (maxH - visibleH)) targetY = maxH - visibleH;

    g_CamX += (targetX - g_CamX) * 0.1f;
    g_CamY += (targetY - g_CamY) * 0.1f;
}
void ACamera::SetPointGrid(float x, float y) {
    PointGridX = x;
    PointGridY = y;
}
std::pair<float, float> ACamera::Getg_Cam() {
    return {g_CamX,g_CamY};
}
std::pair<float, float> ACamera::GetPointGrid() {
    return { PointGridX,PointGridY };
}