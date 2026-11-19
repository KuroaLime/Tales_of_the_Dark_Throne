# Battle presentation checks

## Character proportions and motion

The battle camera draws the authored map at one world pixel per screen pixel;
one cell is 48px. `Character::DrawAtFoot` retains the previous 64px reference
frame height but applies one uniform source-pixel scale to both axes. Most
player frames are nearly square, so the old 40 x 64 destination compressed
their bodies horizontally. Their visible idle/walk bodies now occupy roughly
38–44px of width without changing the map grid or movement coordinates.

`CharacterSpriteLayout.h` locates transparent gutters near nominal sheet
boundaries, crops empty padding and anchors player poses by their low opaque
pixels. This prevents the old equal-row cuts from clipping the archer's feet
or the mage's staff. Monster padding/pivots retain their original nominal
anchors. Scale stays tied to the nominal frame height, never to each pose's
tight height, so crouches and swings are not resized independently. If an
effect fills a gutter, the separator falls back to the nominal cell boundary.
The existing Warrior Attack/Skill art has two such effect boundaries.

Walk poses continue over intermediate path cells and stop at the destination.
Elapsed time drives both animation and travel, with unused travel time carried
into the next cardinal step. Player walk/action animation uses 30 poses per
second from the existing 18-pose sheets; monster walks use 12 poses per second.
Movement speeds are 225/270/240 pixels per second for warrior/archer/mage,
respectively. Long update gaps are capped at 100ms. These are simulation rates,
not a claim that the Windows message loop actually presents at 30 or 60 FPS.

```powershell
./tests/run_character_checks.ps1
./tests/run_character_checks.ps1 -BaselinePath tests/bin/character-map-baseline.json
python tests/verify_character_rendering.py
```

The PowerShell runner only compiles object files. Its `static_assert` tests
exercise production movement arithmetic, timing independence, leftover time,
uniform sprite scaling, cropped-foot anchoring and mirrored placement.
Synthetic gutter/cropping fixtures in `character_sprite_layout_tests.cpp`
are compiled but **not executed** on this host. The Python checker verifies
source integration and, with the optional ignored working snapshot, preserves
all five map builders plus 55 layout/background/character-art files. It does
not reproduce the sprite algorithm and claim that reproduction proves C++
runtime behavior. Native game execution, GDI rendering and actual displayed
frame rate still require a host that permits running the executable.

The implementation-time ratio, crop and on-map comparisons in `tests/bin/`
are diagnostic renders of existing assets, not screenshots from the game.

## Map artwork

The cemetery and lava scenes received a surface-texture repair to remove
smeared paving and repeated rectangular edit patches. Their
`*.texture-repair.prompt.txt` files record the built-in image edits. Collision
layouts and stage builders remain unchanged; the final images were reviewed
against continuous foot sweeps before updating their bitmap hashes.

## Royal stone hall (Stage 03)

`HallMapLayout.h` defines a 33 x 18 grid of 48px cells. `HallScene.bmp` is
exactly 1584 x 864 and uses the shared exact-size file/resource loader. The
original `Test05.bmp` remains the atmosphere reference. The closed hall has
cool window light, warm braziers and royal blue/gold banners. Compact broken
screens, a damaged knight sculpture, a low ruined divider and wall-backed
storage frame the floor while separate northern and southern routes remain
at least two cells wide. Walls and architecture are impassable; the flat
central medallion is floor. The four random enemy slots retain their existing
roster-generation behavior and order.

```powershell
python tests/verify_hall.py
python tests/verify_hall.py --geometry-only
python tests/verify_hall.py --check-unchanged-behavior --previous-stages-baseline tests/bin/hall-previous-stages-baseline.json
./tests/render_hall_preview.ps1 -Footprints -Sweeps
```

The geometry check covers nine safe unique starts, connected ground, a closed
wall border, complete 2 x 2 standing areas and no one-cell corridor pinches.
It also routes an entire 2 x 2 area between the team sides independently in
nonoverlapping northern and southern bands. A conservative 16 x 12 ground
footprint centered at `(24,36)` is checked throughout every cardinal edge.
The bitmap dimensions, hash and terrain rows must match `HallScene.layout.json`
unless `--geometry-only` is used. Source loading flags and the four random
monster slot indices are checked too.

`--candidate <json>` validates proposed `width`, `height`, `tileSize`, `rows`,
`players` and `enemies`, skipping production flags and asset integrity.
`render_hall_preview.ps1 -CandidatePath <json>` overlays that proposal on the
production bitmap. The preview refuses wrongly sized art and writes clean,
collision-grid, optional foot-rectangle and continuous-sweep images. These
are source-data and asset checks, not native execution or recognition of
painted obstacles. Review the painting against the sweep overlay before
updating its manifest. Native game execution is blocked on this host.

The optional pre-hall working snapshot preserves Stage 01/02/04/05 builders,
terrain, images and manifests plus the five original backgrounds. It lives
in ignored `tests/bin/hall-previous-stages-baseline.json`. The generated source
image, composition guide, prompts and alignment manifest live beside the
production `HallScene.bmp`; the manifest binds the reviewed final bitmap to
the actual collision rows, rather than to the initial composition guide.

All five stages now intentionally differ from Git HEAD. Across these scripts,
`--check-unchanged-behavior` compares the unchanged `InitBaseMap`, `BuildTest01`,
stage-routing helpers, `MakeMonsterStart` and `MakeRandomStage03Monster` bodies
with HEAD. The old `--check-unchanged-stages` spelling remains a deprecated
alias; it no longer claims any redesigned stage matches its old geometry.
Per-stage snapshots separately protect prior uncommitted map work.

## Lava caldera (Stage 05)

`LavaMapLayout.h` defines a 33 x 18 grid of 48px cells around the caldera.
`LavaScene.bmp` is the painted scene at exactly 1584 x 864, using the shared
exact-size file/resource loader. The original `Test03.bmp` is the atmosphere
reference. Two broad basalt causeways provide alternate routes between the
combat courts. Ruins, rock walls and lava are impassable.

Six compact gothic/obsidian ornaments frame the caldera. The low rock piles
and southern altar were fitted to their reserved pockets, and the northern
ritual group was moved upward to leave two rows of paving below it.
`LavaScene.prompt.txt`, `.cleanup.prompt.txt`, and `.clearance.prompt.txt`
record the built-in image generation passes. `LavaScene.guide.png` is the
initial design reference; the final runtime mask is the reviewed header and
image/layout manifest, refined against actual painted rim and foot positions.

Stage 05 sets `allowLavaTraversal=false`, including for Fire-element enemies.
The flag defaults to true for other maps; their lava damage and elemental
damage exemptions are unchanged. The validator checks this source contract
as well as the stage's loading flag and movement data.

```powershell
python tests/verify_lava.py
python tests/verify_lava.py --geometry-only
python tests/verify_lava.py --check-unchanged-behavior --previous-stages-baseline tests/bin/lava-previous-stages-baseline.json
./tests/render_lava_preview.ps1 -Footprints -Sweeps
```

Geometry checks cover nine safe unique starts, connected walkable ground,
complete 2 x 2 standing areas, two causeways with two-cell-wide opposite-bank
approaches, and an alternate route when either crossing is removed. The
16 x 12 ground footprint at `(24,36)` is checked along each cardinal edge.
The BMP dimensions, image hash and terrain rows must match
`LavaScene.layout.json`, unless `--geometry-only` is used.

`--candidate <json>` checks a proposed layout without production loading or
asset checks. `render_lava_preview.ps1 -CandidatePath <json>` overlays such a
proposal on the actual scene. Preview output includes the clean image, a
collision grid, optional foot rectangles, and continuous foot sweeps.
These are source/asset checks and diagnostic previews, not native gameplay
execution or automated recognition of painted obstacles. Review the final
painting against the sweep overlay before updating its manifest.

The optional previous-stage snapshot protects the uncommitted Stage 01/02/04
builders, terrain, painted assets and manifests, plus the original backgrounds.
It is stored in ignored `tests/bin/lava-previous-stages-baseline.json`.
`--check-unchanged-behavior` protects the unchanged helpers described above.

## Sea ruins (Stage 04)

`SeaRuinsMapLayout.h` defines a 33 x 18 grid of 48px cells. Its four crossings
connect the dry ruined courts with alternate routes. `SeaRuinsScene.bmp` is
the final painted background at exactly 1584 x 864; the original `Test04.bmp`
remains the atmosphere reference. Buildings, ruined walls, columns, plants and
water are blocked. Water-element units also use the authored crossings.

Six distinct ruin groups occupy the courts, including a central shrine and
broken arch. Two stone causeways and two timber decks connect the islands.
The painted ruins were fitted to the reserved ornament pockets, and the
central eastern shore was widened to keep a dry passage beside the ruins.
`SeaRuinsScene.prompt.txt`, `.fit.prompt.txt`, and `.shore.prompt.txt` record
the built-in image generation passes; `.guide.png` and `.fit-guide.png` are
their planning references. The final runtime mask is in `SeaRuinsMapLayout.h`
and the reviewed image/layout manifest, rather than the initial guide.

```powershell
python tests/verify_sea_ruins.py
python tests/verify_sea_ruins.py --geometry-only
python tests/verify_sea_ruins.py --check-unchanged-behavior --previous-stages-baseline tests/bin/sea-previous-stages-baseline.json
./tests/render_sea_ruins_preview.ps1 -Footprints -Sweeps
```

The validator checks connected dry ground, nine unique safe starts, complete
2 x 2 standing areas, four rectangular bridge components with two-cell-wide
opposite-bank approaches, and alternate routes when any one bridge is removed.
It checks the logical 16 x 12 foot rectangle centered at `(24,36)` throughout
every cardinal movement edge. The bitmap dimensions, hash and terrain rows
must match `SeaRuinsScene.layout.json`, unless `--geometry-only` is used.

`--candidate <json>` checks a proposed layout during art design. The JSON uses
`width`, `height`, `tileSize`, `rows`, `players` and `enemies`; this mode skips
the production loading flags and painted-asset checks. The production preview
reads the actual scene and collision data, refuses a wrongly sized bitmap,
and writes clean, grid, optional foot-rectangle and continuous-sweep images.
These checks do not recognize painted obstacles or execute native gameplay;
the final artwork still needs visual comparison with the swept-foot overlay.
The preview's optional `-CandidatePath <json>` draws a proposed collision mask
over the production bitmap without changing the runtime terrain header.

The optional previous-stage baseline compares the Stage 01/02 builders,
terrain headers, painted images and manifests with a working-state snapshot
captured before this sea redesign. This task's snapshot is in ignored
`tests/bin/sea-previous-stages-baseline.json`, because the earlier authored
maps have not been committed. `--check-unchanged-behavior` protects the unchanged helpers described above. The original stage images remain unchanged.

## Celestial temple (Stage 02)

`CelestialMapLayout.h` defines a 33 x 18 grid of 48px cells for the floating
temple terraces. `CelestialScene.bmp` is the single painted background at
1584 x 864, loaded through the same exact-dimension file/embedded-resource
path as Stage 01. The original `Test02.bmp` remains the atmosphere reference.
Buildings, statues, walls, planted beds and sky void are impassable. Stage 02
disables elemental sky traversal, including Light; all units use the bridges.

The central court now has a compact guardian sculpture and a staggered pair
of ceremonial columns; the lower shrine includes a kneeling guardian and
carved pillars. Their reserved terrain preserves all five bridges, two-cell
approaches and alternate routes. The central statue was resized after a
foot-sweep review so its wings and base clear the surrounding walking lanes.

```powershell
python tests/verify_celestial.py
python tests/verify_celestial.py --geometry-only
python tests/verify_celestial.py --check-unchanged-behavior
./tests/render_celestial_preview.ps1 -Footprints -Sweeps
```

The source-data validator checks connected walkable terrain, safe starts,
complete 2 x 2 standing areas, bridge width and two-cell bank approaches, and
the exact 16 x 12 foot sweep along every cardinal movement edge. It also
requires all starting positions to remain connected when any single bridge
is removed, guarding the alternate routes. Unless `--geometry-only` is used,
the BMP dimensions, image hash and terrain rows must match
`CelestialScene.layout.json`. These checks do not recognize painted objects
or execute native movement code; the art still requires a visual foot audit.

`render_celestial_preview.ps1` writes the actual authored image, a collision
grid with start positions, and optionally every walkable cell's foot rectangle.
`-Sweeps` also overlays the complete continuous movement corridors at low
opacity, so painted obstacles between standing positions can be inspected.
It reads current production data and never rescales an incorrectly sized
asset. These are asset/geometry previews, not game screenshots. The source
image, guide and generation prompt live beside `CelestialScene.bmp`.
`CelestialScene.alignment.prompt.txt` records the floor and obstacle correction;
`CelestialScene.rim.prompt.txt` records the final lower-terrace curb repair.
`CelestialScene.central-guide.png`, `CelestialScene.central-edit.prompt.txt`
and `CelestialScene.central-fit.prompt.txt` record the added sculpture groups
and the central statue's clearance adjustment.
The guide records the initial composition, while `CelestialMapLayout.h` and
the manifest hold the final collision mask refined against the painting.
The manifest binds that reviewed mask to the resulting bitmap.

The optional `--graveyard-baseline <path>` compares Stage 01's builder, layout
and painted asset to a working-state hash snapshot captured before celestial
edits. This task's snapshot is in ignored
`tests/bin/celestial-graveyard-baseline.json`; it is not a Git HEAD baseline
because the preceding graveyard work has not been committed. The separate
`--check-unchanged-behavior` option protects the unchanged helpers described above.

## Graveyard (Stage 01)

Stage 01 uses a 33 x 18 grid of 48px cells. `GraveyardMapLayout.h` defines
the collision terrain. `GraveyardScene.bmp` is a single painted scene at
1584 x 864, replacing the repeated grave plots and wall fragments of the atlas
composition. Its paths, burial grounds, architecture and vegetation share one
continuous composition. The original `Test01.bmp` remains a style reference.

Three varied burial areas now occupy parts of the central clearing, with newly
painted low walls matched to the scene's stone, moss and lighting. One area
joins the existing southeastern cemetery boundary. Connected paths at least
two cells wide, both river crossings and all starting positions are retained.

Two small western burial pockets extend the existing northwestern graves and
southwestern crypt garden. Their varied markers and collapsed low walls frame
the bridge approach while the two-cell path between them and the original
player starting positions remain clear.

`BattleMapData::BuildStage01()` selects the scene with `gridAlignedBackground`.
`Battle_Stage` searches for the scene beside the executable and in project paths,
then tries its embedded bitmap resource. Only an exact match to the grid's
pixel dimensions is accepted, and drawing uses a 1:1 `BitBlt`. If both copies
are missing or invalid, `GraveyardRenderer` composes an atlas fallback from the
same collision terrain; it never substitutes the unrelated world-map image.

The river blocks every element. Authored bridge cells supply the crossings.
Buildings, trees, graves and walls are blocked terrain. The image's visible
obstacles and usable paths must be reviewed against the grid overlay whenever
either art or collision data changes. The layout manifest records the reviewed
image hash and terrain rows so later mismatched edits fail validation.

The atlas fallback samples materials continuously in world space and blends
material edges inside their own cells. Its water-bank and prop artwork remain
inside blocked footprints. This fallback is kept for missing-art recovery;
its tile-based appearance is not the primary scene.

Characters' sprite bottoms are inset to `(24, 36)` within their cells. A
conservative 16 x 12 ground footprint is checked throughout each cardinal
movement segment. Sprite upper bodies may overlap the scenery behind them;
they are not the ground footprint. Path submission rejects nonadjacent steps.

## Geometry and regression checks

```powershell
python tests/verify_graveyard.py
python tests/verify_graveyard.py --check-unchanged-behavior
```

The optional comparison protects unchanged base/test-map, routing and monster
helpers against Git HEAD. The validator checks all walkable cells, starts, any declared prop bounds and
overlaps, bridge widths, two-cell clearance, and the continuous footprint on
every adjacent edge. It also checks the painted BMP dimensions and the scene's
layout manifest. These are source-data and asset-integrity checks; they do not
recognize obstacles from image pixels or execute the game.

## Native renderer smoke test

```powershell
./tests/run_graveyard_smoke.ps1
```

This compiles the production map builder and renderer with a small console
harness. It loads the painted bitmap from its production path, checks exact
grid dimensions, and writes the scene and a collision/spawn overlay. Separately,
it checks atlas-rendered dimensions, missing-atlas fallback, malformed input,
and pixel-level containment of fallback river banks and any declared props.
It does not exercise the game window's path search or embedded-resource loader,
and fallback containment does not prove alignment of painted obstacles.
Images and test binaries go under ignored `tests/bin/`. `-BuildOnly` compiles
without launching it.

On the implementation host, the test harness compiled and linked, but Windows
application control blocked its execution. Its runtime assertions have **not**
been reported as passing. The game itself built successfully using x64 host
tools; live gameplay still needs verification on a host that permits execution.

## Design preview

`render_graveyard_preview.ps1` loads the actual painted BMP when the map selects
it, then draws a collision/spawn overlay from the current source declarations.
Its legacy System.Drawing atlas reconstruction remains available for fallback
maps. These are asset/geometry previews, not game screenshots or evidence of
native renderer test execution. The native harness also writes
`graveyard-painted-*` diagnostics when that harness can run.

## Art source

`Source/SMap/background/GraveyardScene.bmp` is the production painted scene.
Its generated source image, composition guide and prompt are saved beside it
as `GraveyardScene.source.png`, `GraveyardScene.guide.png` and
`GraveyardScene.prompt.txt`; `GraveyardScene.layout.json` binds the bitmap hash
to its collision layout. The scene is embedded in `Resource.rc` as
`IDB_GRAVEYARD_SCENE`.

The central burial additions are documented by
`GraveyardScene.central-edit.prompt.txt` and `GraveyardScene.central-guide.png`
beside the scene. The guide records the placement request; the layout manifest
records the final collision alignment after reviewing the edited artwork.

The western bridge-approach additions use `GraveyardScene.west-edit.prompt.txt`
and `GraveyardScene.west-guide.png`. As with the central additions, their guide
describes the requested placement and the manifest records the reviewed result.

`GraveyardAtlas.bmp` remains the fallback atlas produced with the built-in image
generation tool. Its prompt is `GraveyardAtlas.prompt.txt`; slot ordering and
crop rules are defined in `GraveyardRenderer.cpp`. It is embedded separately
as `IDB_GRAVEYARD_ATLAS` so missing external files do not remove the fallback.
