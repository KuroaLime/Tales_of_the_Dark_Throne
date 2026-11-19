#include "../Tower_Triumph/Tower_Triumph/AreaSkillEffectRenderer.h"

using namespace AreaSkillEffectRenderer;
static_assert(!IsCellVisible(0.40f, 0.41f), "Future cells must not light up.");
static_assert(IsCellVisible(0.41f, 0.41f), "Ground starts on the damage boundary.");
static_assert(IsCellVisible(0.41f - 0.0000005f, 0.41f), "Use the damage event's shared epsilon.");
static_assert(IsCellVisible(0.70f, 0.41f), "The wave tail remains after the small contact.");
static_assert(!IsCellVisible(1.04f, 0.41f), "Expired cells must leave the clip region.");
static_assert(ContactFrame(0.40f, 0.41f) == -1, "No future sprite frame.");
static_assert(ContactFrame(0.41f, 0.41f) == 0, "Arrival starts the first source pose.");
static_assert(ContactFrame(0.41f - 0.0000005f, 0.41f) == 0, "Roundoff cannot omit the arrival pose.");
static_assert(ContactFrame(0.69f, 0.41f) == 7, "Last ground burst frame before the particle tail.");
static_assert(ContactFrame(0.74f, 0.41f) == -1, "No frozen terminal ground burst.");
static_assert(Detail::BandBit(0.0f) == 0 && Detail::BandBit(1.0f) == (1u << AreaSkillVisuals::AlphaBands),
    "Empty alpha bands must not allocate copies or blend an invisible pass.");

// Compile the complete standalone GDI path without launching a native test.
void CompileAreaRenderer(HDC target, HDC scratch, HBITMAP bitmap)
{
    auto cells = BattleSkillArea::BuildCells(BattleSkillArea::Kind::Splash2,
        { 5, 5 }, { 8, 5 }, 4, 33, 18);
    for (auto& cell : cells)
        cell.arrivalSeconds = BattleSkillArea::SplashArrival(0.3f, 0.2f,
            BattleSkillArea::Distance({cell.x, cell.y}, {8, 5}));
    Options options;
    options.kind = BattleSkillArea::Kind::Splash2;
    options.originX = 8;
    options.originY = 5;
    options.contactBitmap = bitmap;
    Draw(target, scratch, cells, 0.61f, options);
    DrawFront(target, scratch, cells, 0.61f, options);
    DrawBodyContact(target, scratch, bitmap, 100, 100, 0.29f, RGB(255,0,255));
}
