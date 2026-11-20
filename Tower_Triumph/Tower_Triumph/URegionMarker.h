#pragma once
#include "UContainerWidget.h"
#include <chrono>
#include <string>

class URegionMarker : public UContainerWidget
{
private:
    struct SLayerBitmap { HBITMAP bitmap = nullptr; int width = 0; int height = 0; };
    bool bIsActivated;
    bool bIsUnlocked;
    bool presentationActive;
    RECT worldBounds;
    SLayerBitmap building, markerOpen, markerClosed;
    double floatSeconds, floatPhase, transitionSeconds;
    float visualScale, transitionStart, transitionTarget;
    std::chrono::steady_clock::time_point lastUpdate;
    std::wstring parentRegionName, regionName, regionDesc, environment;
    static bool LoadLayer(HINSTANCE instance, WORD resourceId, SLayerBitmap& layer);
    static void ReleaseLayer(SLayerBitmap& layer);
    static void DrawLayer(HDC target, HDC scratch, const SLayerBitmap& layer, const RECT& destination);
public:
    URegionMarker();
    virtual ~URegionMarker();
    virtual void Update() override;
    virtual void Draw(HDC hMemDC, HDC hMemDC1) override;
    virtual bool ContainsPoint(int MouseX, int MouseY) override;
    void SetupLandmark(const RECT& bounds, const std::wstring& label);
    bool LoadArtwork(HINSTANCE instance, WORD buildingResource, WORD openResource, WORD closedResource, double phaseSeconds = 0.0);
    bool HasArtwork() const { return building.bitmap && markerOpen.bitmap && markerClosed.bitmap; }
    RECT GetWorldBounds() const { return worldBounds; }
    float GetVisualScale() const { return visualScale; }
    void SetPresentationActive(bool active);
    void AdvanceAnimation(double elapsed);
    RECT GetBuildingScreenRect(int offsetX, int offsetY, float zoom = 1.0f) const;
    RECT GetFloatingMarkerScreenRect(int offsetX, int offsetY, float zoom = 1.0f) const;
    void SetRegionInfo(const std::wstring& parentName, const std::wstring& name, const std::wstring& desc, const std::wstring& env);
    void SetIsUnlocked(bool value) { bIsUnlocked = value; }
    bool IsUnlocked() const { return bIsUnlocked; }
    void Activate();
    void Deactivate();
    std::wstring GetParentName() const { return parentRegionName; }
    std::wstring GetName() const { return regionName; }
    std::wstring GetDesc() const { return regionDesc; }
    std::wstring GetEnv() const { return environment; }
    void DrawWithOffset(HDC hMemDC, HDC hMemDC1, int offsetX, int offsetY, float zoom = 1.0f);
    bool ContainsPointWithOffset(int MouseX, int MouseY, int offsetX, int offsetY, float zoom = 1.0f);
};
