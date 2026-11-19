#pragma once
#include "BattleSkillArea.h"
#include <cstdint>

// Portable presentation-only sampling. No random clock, gameplay state or
// resource changes: the preview and GDI consume the same cell-local samples.
namespace AreaSkillVisuals
{
    using Element = BattleActionTiming::ElementVisual;
    constexpr float Lifetime = 0.62f;
    constexpr float BurstFrameSeconds = 0.04f;
    constexpr float BurstSeconds = BurstFrameSeconds * 8;
    constexpr float BodyFrameSeconds = 0.03f;
    constexpr float BodyAfterglowSeconds = 0.10f;
    constexpr float BodySeconds = BodyFrameSeconds * 8 + BodyAfterglowSeconds;
    constexpr int BodyDrawSize = 64;
    constexpr int AlphaBands = 12;
    constexpr int LightRaySegments = 4;
    struct Color { int r, g, b; };
    struct Palette { Color main, bright, accent; };
    constexpr Palette Colors(Element element)
    {
        switch (element)
        {
        case Element::Fire: return { {255,94,28}, {255,236,142}, {255,167,49} };
        case Element::Water: return { {30,175,255}, {192,252,255}, {82,226,255} };
        case Element::Grass: return { {87,202,62}, {222,255,145}, {51,143,55} };
        case Element::Light: return { {255,208,77}, {255,255,221}, {255,239,151} };
        case Element::Dark: return { {139,69,235}, {234,187,255}, {192,81,227} };
        default: return { {170,210,255}, {245,252,255}, {120,181,239} };
        }
    }
    constexpr float Clamp(float x) { return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x); }
    constexpr float Fade(float age, float duration) { return Clamp(1.0f - age / duration); }
    constexpr int Band(float alpha) { return static_cast<int>(Clamp(alpha) * AlphaBands + 0.5f); }
    constexpr float CenterStrength(BattleSkillArea::Kind kind, int x, int y, int originX, int originY)
    { return kind != BattleSkillArea::Kind::Line && x == originX && y == originY ? 1.22f : 1.0f; }
    constexpr bool Visible(float age) { return age >= -BattleSkillArea::EventEpsilon && age < Lifetime; }
    constexpr int BurstFrame(float age)
    {
        if (age < -BattleSkillArea::EventEpsilon || age >= BurstSeconds) return -1;
        const int frame = static_cast<int>(((age > 0.0f ? age : 0.0f) + BattleSkillArea::EventEpsilon) / BurstFrameSeconds);
        return frame < 8 ? frame : 7;
    }
    struct CellState
    {
        float glowAlpha, glowWidth, glowHeight;
        float ringAlpha, ringWidth, ringHeight;
        float spriteAlpha, floorWidth, floorHeight;
        float plumeWidth, plumeHeight, plumeAlpha, beamAlpha;
        int frame;
    };
    constexpr CellState SampleCell(Element element, float age, float strength = 1.0f, float variation = 0.5f)
    {
        if (!Visible(age)) return {};
        age = age > 0.0f ? age : 0.0f;
        const float fade = Fade(age, Lifetime);
        const float rise = Clamp(age / 0.08f);
        const float size = 1.0f + (strength - 1.0f) * 0.55f;
        const float plumeWidth = element == Element::Light ? 0.78f : element == Element::Water ? 1.20f : 1.12f;
        const float plumeHeight = element == Element::Light ? 1.72f : element == Element::Grass ? 1.38f : 1.32f;
        return { Clamp(0.56f * fade * fade * strength), (0.65f + 0.20f * rise) * size, 0.42f * size,
            Clamp(0.72f * Fade(age, 0.28f) * strength) * (element == Element::Light ? 0.5f : 1.0f),
            (0.25f + 0.90f * Clamp(age / 0.23f)) * size,
            (0.12f + 0.42f * Clamp(age / 0.23f)) * size,
            Clamp(0.90f * Fade(age, 0.44f) * strength) * (element == Element::Light ? 0.75f : 1.0f),
            1.15f * size, 0.66f * size,
            plumeWidth * (0.72f + 0.28f * rise) * size * (0.92f + variation * 0.16f),
            plumeHeight * (0.72f + 0.28f * rise) * size * (0.80f + variation * 0.40f),
            Clamp(0.68f * Fade(age, 0.46f) * strength), Clamp(0.30f * Fade(age, 0.48f) * strength), BurstFrame(age) };
    }
    enum class Shape { Ember, Droplet, Leaf, Star, Shard };
    struct Particle { float x, y, size, alpha, dx, dy; Shape shape; bool front; };
    constexpr int ParticleCount(Element element) { return element == Element::Grass || element == Element::Light ? 8 : 10; }
    constexpr std::uint32_t Seed(int x, int y, int index)
    {
        std::uint32_t value = static_cast<std::uint32_t>(x) * 73856093u ^ static_cast<std::uint32_t>(y) * 19349663u ^
            static_cast<std::uint32_t>(index + 1) * 83492791u;
        value ^= value >> 13; value *= 1274126177u; return value ^ (value >> 16);
    }
    struct RaySegment { float x0, y0, x1, y1, alpha; int width; };
    constexpr RaySegment SampleLightRay(int cellX, int cellY, float age, float strength, int segment)
    {
        if (!Visible(age) || segment < 0 || segment >= LightRaySegments) return {};
        const float variation = (Seed(cellX,cellY,31) & 255u) / 255.0f;
        const auto state = SampleCell(Element::Light,age,strength,variation);
        const float t0 = segment / static_cast<float>(LightRaySegments);
        const float t1 = (segment + 1) / static_cast<float>(LightRaySegments);
        const float tilt = (variation - 0.5f) * 0.26f;
        const float alpha = Clamp(0.82f * Fade(age > 0.0f ? age : 0.0f,0.44f) * strength) * (1.0f-t0) * (1.0f-t0);
        return {tilt*t0,-state.plumeHeight*t0,tilt*t1,-state.plumeHeight*t1,alpha,segment<2?2:1};
    }
    constexpr Particle SampleParticle(Element element, int cellX, int cellY, int index, float age, float strength = 1.0f)
    {
        const auto seed = Seed(cellX, cellY, index);
        const float variation = (seed & 255u) / 255.0f;
        const float delay = static_cast<float>((seed >> 8) & 3u) * 0.012f;
        const float life = 0.40f + variation * 0.14f;
        const float t = (age - delay) / life;
        const bool front = index % 5 == 0;
        if (t < 0.0f || t >= 1.0f || index >= ParticleCount(element)) return {0,0,0,0,0,0,Shape::Ember,front};
        const int direction = static_cast<int>((seed >> 10) & 15u);
        const float dx = BattleActionTiming::DirectionCosines[direction];
        const float dy = BattleActionTiming::DirectionCosines[(direction + 12) % 16];
        float x = dx * (0.10f + 0.46f * t);
        float y = -0.08f - (0.65f + variation * 0.25f) * t + 0.30f * t * t;
        Shape shape = Shape::Ember;
        switch (element)
        {
        case Element::Water: shape = Shape::Droplet; y = -0.08f - (0.32f + variation * 0.32f) * 4.0f * t * (1.0f-t); break;
        case Element::Grass: shape = Shape::Leaf; x = dx * (0.1f + 0.30f*t) + dy * 0.18f*t*(1.0f-t); break;
        case Element::Light: shape = Shape::Star; x = dx * (0.1f + 0.35f*t); y = -0.12f - (0.65f + variation*0.3f)*t; break;
        case Element::Dark: shape = Shape::Shard; x = dx*0.50f*(1.0f-t)-dy*0.42f*t*(1.0f-t); y = -0.18f+dy*0.22f*(1.0f-t)-0.45f*t; break;
        default: break;
        }
        if (front) { x *= 0.75f; y = y < -0.28f ? -0.28f : y; }
        const float opacity = (1.0f-t)*(1.0f-t)*(front ? 0.68f : 0.94f)*strength;
        return {x,y,(0.040f+variation*0.035f)*(front?0.8f:1.0f),Clamp(opacity),dx,dy,shape,front};
    }
    constexpr int BodyFrame(float age)
    {
        const int frame = static_cast<int>((age + BattleSkillArea::EventEpsilon) / BodyFrameSeconds);
        return frame < 0 ? 0 : (frame < 8 ? frame : 7);
    }
    constexpr float BodyAlpha(float age)
    { return age < BodyFrameSeconds * 8 ? 1.0f : 0.45f * Fade(age - BodyFrameSeconds * 8, BodyAfterglowSeconds); }
}
