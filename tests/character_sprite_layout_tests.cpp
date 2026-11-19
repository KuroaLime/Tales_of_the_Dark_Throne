// This host compiles this file with /c only. Runtime assertions are provided
// for a host that permits native test execution; compiling is not a runtime pass.
#include "../Tower_Triumph/Tower_Triumph/CharacterSpriteLayout.h"

#include <cassert>
#include <type_traits>

static_assert(!CharacterSpriteLayout::Detail::IsOpaque(0xffff00ffu),
    "Magenta remains transparent regardless of the bitmap alpha byte");
static_assert(CharacterSpriteLayout::Detail::IsOpaque(0x00000000u),
    "Black remains opaque; alpha is not the color-key contract");
static_assert(std::is_same<decltype(CharacterSpriteLayout::Analyze(nullptr, 0, 0, 0, 0, false)),
    CharacterSpriteLayout::Layout>::value, "Layout analysis stays independent of Win32 handles");

constexpr CharacterSpriteLayout::Frame crop{ { 10, 20, 110, 220 }, 40.0f, 220.0f };
constexpr auto normal = CharacterSpriteLayout::PlaceAtFoot(crop, 400, 64, 24, 36, false);
constexpr auto mirrored = CharacterSpriteLayout::PlaceAtFoot(crop, 400, 64, 24, 36, true);
static_assert(normal.width == 16 && normal.height == 32,
    "Both axes use one scale, preserving the cropped body's source proportions");
static_assert(normal.x == 19 && normal.y == 4,
    "Removing empty sheet padding must not move the ground anchor");
static_assert(mirrored.x == 13 && mirrored.y == normal.y && mirrored.width == normal.width,
    "Mirroring a noncentral anchor must move its rectangle to retain the same feet position");
constexpr CharacterSpriteLayout::Frame offsetAnchor{ { 10, 20, 20, 30 }, 5.0f, 30.0f };
constexpr auto negativeOffset = CharacterSpriteLayout::PlaceAtFoot(offsetAnchor, 20, 10, 24, 36, false);
static_assert(negativeOffset.x == 27 && negativeOffset.y == 31,
    "Negative half-pixel offsets round consistently with positive offsets");
static_assert(CharacterSpriteLayout::PlaceAtFoot(crop, 0, 64, 24, 36, false).width == 0,
    "Invalid reference geometry must not divide by zero");
static_assert(CharacterSpriteLayout::PlaceAtFoot({}, 100, 64, 24, 36, false).width == 0,
    "Empty source rectangles must not produce a draw operation");

constexpr auto walkContact = CharacterSpriteLayout::WithWalkCycleAnchor({ { 16, 7, 80, 88 }, 36.0f, 88.0f }, 0);
constexpr auto walkPassing = CharacterSpriteLayout::WithWalkCycleAnchor({ { 112, 9, 176, 87 }, 158.0f, 87.0f }, 1);
static_assert(walkContact.anchorX == 48.0f && walkPassing.anchorX == 144.0f &&
    walkContact.anchorY == 88.0f && walkPassing.anchorY == 88.0f,
    "Authored walking must keep a fixed root and ground despite the changing foot silhouette");
constexpr auto walkLastRow = CharacterSpriteLayout::WithWalkCycleAnchor({}, 11);
static_assert(walkLastRow.anchorX == 336.0f && walkLastRow.anchorY == 280.0f,
    "All twelve authored poses must use the four-column, three-row layout");
constexpr auto contactDraw = CharacterSpriteLayout::PlaceAtFoot(walkContact, 96, 64, 24, 36, false);
constexpr auto passingDraw = CharacterSpriteLayout::PlaceAtFoot(walkPassing, 96, 64, 24, 36, false);
static_assert(contactDraw.x == passingDraw.x && contactDraw.width == passingDraw.width,
    "Moving feet cannot drag the body's fixed root sideways between poses");

static_assert(CharacterSpriteLayout::IdlePose::HasDimensions(96, 96) &&
    !CharacterSpriteLayout::IdlePose::HasDimensions(384, 288) &&
    !CharacterSpriteLayout::IdlePose::HasDimensions(96, 95),
    "Only the explicit single-cell idle format can use the new static-pose path");
static_assert(CharacterSpriteLayout::IdlePose::Columns == 1 &&
    CharacterSpriteLayout::IdlePose::Rows == 1 && CharacterSpriteLayout::IdlePose::FrameCount == 1,
    "A static approved idle pose must not be split or fabricated into an animation");
constexpr auto idlePose = CharacterSpriteLayout::WithIdlePoseAnchor({ { 16, 7, 80, 88 }, 40.0f, 86.0f });
constexpr auto idleDraw = CharacterSpriteLayout::PlaceAtFoot(idlePose,
    CharacterSpriteLayout::IdlePose::CellSize, 64, 24, 36, false);
static_assert(idlePose.anchorX == walkContact.anchorX && idlePose.anchorY == walkContact.anchorY &&
    idleDraw.x == contactDraw.x && idleDraw.y == contactDraw.y &&
    idleDraw.width == contactDraw.width && idleDraw.height == contactDraw.height,
    "Switching from idle to walking must preserve the shared body root and scale");
constexpr auto idleMirrored = CharacterSpriteLayout::PlaceAtFoot(idlePose, 96, 64, 24, 36, true);
constexpr auto contactMirrored = CharacterSpriteLayout::PlaceAtFoot(walkContact, 96, 64, 24, 36, true);
static_assert(idleMirrored.x == contactMirrored.x && idleMirrored.y == contactMirrored.y,
    "Idle/walk registration must also agree while facing left");

static_assert(CharacterSpriteLayout::Action::HasDimensions(512, 384) &&
    !CharacterSpriteLayout::Action::HasDimensions(384, 288) &&
    CharacterSpriteLayout::Action::FrameCount == 12,
    "Padded action cells have an explicit twelve-pose format");
constexpr auto actionStanding = CharacterSpriteLayout::WithActionAnchor({ { 32, 31, 96, 112 }, 1.0f, 1.0f }, 0);
constexpr auto actionDraw = CharacterSpriteLayout::PlaceAtFoot(actionStanding,
    CharacterSpriteLayout::Action::ReferenceFrameHeight, 64, 24, 36, false);
constexpr auto actionMirrored = CharacterSpriteLayout::PlaceAtFoot(actionStanding,
    CharacterSpriteLayout::Action::ReferenceFrameHeight, 64, 24, 36, true);
static_assert(actionDraw.x == idleDraw.x && actionDraw.y == idleDraw.y &&
    actionDraw.width == idleDraw.width && actionDraw.height == idleDraw.height &&
    actionMirrored.x == idleMirrored.x && actionMirrored.y == idleMirrored.y,
    "Extra action padding must not change body scale or root when entering combat");
constexpr auto actionLast = CharacterSpriteLayout::WithActionAnchor({}, 11);
static_assert(actionLast.anchorX == 448.0f && actionLast.anchorY == 368.0f,
    "The final action pose must read row three using the padded four-column grid");
constexpr auto collapsed = CharacterSpriteLayout::WithActionAnchor({ { 20, 90, 110, 112 }, 90.0f, 100.0f }, 0);
constexpr auto collapsedDraw = CharacterSpriteLayout::PlaceAtFoot(collapsed,
    CharacterSpriteLayout::Action::ReferenceFrameHeight, 64, 24, 36, false);
static_assert(collapsedDraw.y + collapsedDraw.height == 36 && collapsedDraw.width == 60,
    "A collapsed death pose must stay on the ground without stretching to standing height");

static_assert(CharacterSpriteLayout::IdleCycle::HasDimensions(384, 288) &&
    !CharacterSpriteLayout::IdleCycle::HasDimensions(512, 384) &&
    CharacterSpriteLayout::IdleCycle::RootX == CharacterSpriteLayout::WalkCycle::RootX &&
    CharacterSpriteLayout::IdleCycle::GroundY == CharacterSpriteLayout::WalkCycle::GroundY,
    "Animated idle uses the same twelve-cell canvas and ground registration as walking");
static_assert(CharacterSpriteLayout::IdleTransition::HasDimensions(512, 384) &&
    !CharacterSpriteLayout::IdleTransition::HasDimensions(384, 288) &&
    CharacterSpriteLayout::IdleTransition::ReferenceFrameHeight == CharacterSpriteLayout::IdleCycle::CellSize,
    "Stop and return padding must retain the idle/walk body scale");

int main()
{
    using namespace CharacterSpriteLayout;
    assert(Analyze(nullptr, 12, 18, 2, 3, true).frames.empty());
    std::vector<std::uint32_t> pixels(12 * 18, 0x00ff00ffu);
    auto paint = [&](int x, int y) { pixels[static_cast<std::size_t>(y) * 12 + x] = 0x00334455u; };

    // First pose extends below the nominal six-pixel row. A transparent gutter
    // after the feet should retain it rather than leaking the feet into row two.
    for (int y = 1; y <= 6; ++y)
        for (int x = 1; x <= 3; ++x) paint(x, y);
    paint(4, 1);
    paint(5, 1); // Long high weapon must not drag the low foot anchor sideways.
    for (int y = 1; y <= 6; ++y)
        for (int x = 8; x <= 10; ++x) paint(x, y);
    for (int y = 9; y <= 10; ++y)
        for (int x = 1; x <= 3; ++x) paint(x, y);

    const Layout player = Analyze(pixels.data(), 12, 18, 2, 3, true);
    assert(player.frames.size() == 6);
    assert(player.nominalFrameWidth == 6 && player.nominalFrameHeight == 6);
    assert(player.frames[0].source.left == 1 && player.frames[0].source.right == 6);
    assert(player.frames[0].source.top == 1 && player.frames[0].source.bottom == 7);
    assert(player.frames[0].anchorX == 2.5f && player.frames[0].anchorY == 7.0f);
    assert(player.frames[2].source.top == 9 && player.frames[2].source.bottom == 11);
    for (const auto& frame : player.frames)
    {
        assert(frame.source.left >= 0 && frame.source.right <= 12);
        assert(frame.source.top >= 0 && frame.source.bottom <= 18);
        assert(frame.source.left < frame.source.right && frame.source.top < frame.source.bottom);
    }

    const Layout monster = Analyze(pixels.data(), 12, 18, 2, 3, false);
    assert(monster.frames[0].anchorX == 3.0f && monster.frames[0].anchorY == 6.0f);
    assert(monster.frames[2].anchorX == 3.0f && monster.frames[2].anchorY == 12.0f);

    // Effects can cover every candidate separator. Uniform fallback still
    // returns valid complete cells without inventing empty-frame geometry.
    std::fill(pixels.begin(), pixels.end(), 0x00000000u);
    const Layout noGutters = Analyze(pixels.data(), 12, 18, 2, 3, false);
    assert(noGutters.frames[0].source.right == 6 && noGutters.frames[0].source.bottom == 6);
    assert(noGutters.frames[5].source.left == 6 && noGutters.frames[5].source.top == 12);
    assert(Analyze(pixels.data(), 12, 18, 13, 3, true).frames.empty());
    return 0;
}
