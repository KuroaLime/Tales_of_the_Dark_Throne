#include "ACamera.h"

ACamera::ACamera(int N_CamX, int N_CamY, int PointIndexX, int PointIndexY, float initialZoom)
    : defaultZoom(initialZoom) {
    g_CamX = (float)N_CamX;
    g_CamY = (float)N_CamY;

    PointGridX = (float)PointIndexX;
    PointGridY = (float)PointIndexY;

    currentZoom = defaultZoom;
    targetZoom = defaultZoom;
}
void ACamera::Reset() {
    g_CamX = 0;
    g_CamY = 0;

    PointGridX = 0;
    PointGridY = 0;
    currentZoom = defaultZoom;
    targetZoom = defaultZoom;
}
void ACamera::Update(int TILE_W, int TILE_H, int TILE_SIZE, int WIN_WIDTH, int WIN_HEIGHT) {
    currentZoom += (targetZoom - currentZoom) * 0.1f;

    float targetX = (float)(PointGridX * TILE_SIZE - (WIN_WIDTH / 2.0f) / currentZoom);
    float targetY = (float)(PointGridY * TILE_SIZE - (WIN_HEIGHT / 2.0f) / currentZoom);

    float mapW = (float)TILE_W * TILE_SIZE;
    float mapH = (float)TILE_H * TILE_SIZE;
    
    float viewW = (float)WIN_WIDTH / currentZoom;
    float viewH = (float)WIN_HEIGHT / currentZoom;

    if (mapW <= viewW) {
        targetX = (mapW - viewW) / 2.0f;
    } else {
        if (targetX < 0) targetX = 0;
        if (targetX > (mapW - viewW)) targetX = mapW - viewW;
    }

    if (mapH <= viewH) {
        targetY = (mapH - viewH) / 2.0f;
    } else {
        if (targetY < 0) targetY = 0;
        if (targetY > (mapH - viewH)) targetY = mapH - viewH;
    }

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
