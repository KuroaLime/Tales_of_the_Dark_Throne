#include "ACamera.h"

ACamera::ACamera(int N_CamX, int N_CamY, int PointIndexX, int PointIndexY) {
    g_CamX = N_CamX * 0.8;
    g_CamY = N_CamY * 0.8;

    PointGridX = PointIndexX;
    PointGridY = PointIndexY;
}
void ACamera::Reset() {
    g_CamX = 0;
    g_CamY = 0;

    PointGridX = 0;
    PointGridY = 0;
}
void ACamera::Update(int TILE_W, int TILE_H, int TILE_SIZE, int WIN_WIDTH, int WIN_HEIGHT) {
    float targetX = (float)(PointGridX * TILE_SIZE - WIN_WIDTH / 2 + TILE_SIZE / 2);
    float targetY = (float)(PointGridY * TILE_SIZE - WIN_HEIGHT / 2 + TILE_SIZE / 2);

    if (targetX < 0) targetX = 0;
    if (targetY < 0) targetY = 0;
    if (targetX > (TILE_W * TILE_SIZE - WIN_WIDTH)) targetX = (float)(TILE_W * TILE_SIZE - WIN_WIDTH);
    if (targetY > (TILE_H * TILE_SIZE - WIN_HEIGHT)) targetY = (float)(TILE_H * TILE_SIZE - WIN_HEIGHT);

    g_CamX += (targetX - g_CamX) * 0.1f;
    g_CamY += (targetY - g_CamY) * 0.1f;
}
void ACamera::SetPointGrid(int x, int y) {
    PointGridX = x;
    PointGridY = y;
}
std::pair<float, float> ACamera::Getg_Cam() {
    return {g_CamX,g_CamY};
}
std::pair<float, float> ACamera::GetPointGrid() {
    return { PointGridX,PointGridY };
}