# Battle presentation checks

## Battle facing and mirrored sprite edges

Battle placement explicitly starts allies facing right and enemies facing left,
including party members returning from a previous battle. Movement and target
facing continue to use the existing direction updates.

`Character::DrawAtFoot` starts negative-width GDI copies at `sourceWidth - 1`.
The reused mirror surface is cleared to the magenta key, and only a successful
copy is composited. Invalid source bounds are rejected. Black remains opaque
because enemy artwork contains intentional dark pixels.

```powershell
./tests/build_main_stage_isolated.ps1
./tests/run_character_checks.ps1
./tests/run_character_mirror_checks.ps1 -BuildOnly
```

The mirror regression uses public Character APIs and production objects with
synthetic color-key sprites. It checks complete output pixels in both directions,
including changing crop sizes and reused mirror storage. `-BuildOnly` compiles
and links without executing; runtime pixel assertions have not been run on this
host under its existing application-control restriction. On an allowed host,
omit `-BuildOnly` to execute the offscreen checks.

## World-map landmarks and maintenance UI

World-map buildings are separate transparent sprites over the clean landscape.
Hovering or selecting a building eases it to 1.05 scale around its foot position;
the original world-space hit rectangle stays fixed. The floating open/locked
quest markers are decorative and never add another hit region. Region details
keep the existing Starlight popup and a complete building overview image.

The world map starts at 1.3x zoom and focuses a selected region at 1.6x.
Closing the region popup by button or Escape returns to the same 1.3x overview.
These values are shared by the map and replay fixture. The camera retains its
configured starting zoom on Reset; callers without a custom zoom keep 1.5x.

Maintenance uses the same navy/gold panel, portraits, map backdrop and 40px
quill. All six tabs still read live party, inventory, equipment, skill, shop
and reward data. Preview fixtures are not installed in the game. Reward
navigation also handles the existing fourth and fifth battle stages.

```powershell
node tests/import_main_stage_landmarks.mjs --check
./tests/build_main_stage_isolated.ps1
./tests/run_popup_ui_checks.ps1
./tests/run_maintenance_map_ui_checks.ps1
```

The maintenance/map runner links production objects and bitmap resources,
executes real input and game-data operations against in-memory fixtures, and
captures the native GDI drawing output. Its reports and screenshots stay under
`tests/bin/maintenance-map-ui-checks`. It does not launch an interactive game
or alter a saved game. The isolated build leaves Visual Studio output folders
untouched.

The initial integration run passed 3,543 maintenance/map assertions and the existing
490 popup assertions. Native captures were reviewed at 1264x644, 1280x720
and 1600x900, including the compact skill cards, all five reward rows, and the
region popup's building image. These are offscreen production renders and
input tests, not a claim of a live gameplay session or window-resize coverage.

### Maintenance presentation ownership

`Maintenance_Stage` retains the existing `CStage` interface, input routing,
layout and selection rules. It calls `GameManager` for gameplay changes, then
passes the completed result to separate equipment, skill, shop, inventory and
party presentation objects in `MaintenanceFeedback.h`. These objects own their
private result, message and timing state; they do not access `GameManager` or
change the character, party, inventory or gold.

`UIFrameSnapshot` owns and releases its GDI resources. `UIRegionTransition`
owns an outgoing snapshot and its clock, including region clipping and resize
cancellation. No GDI handles or mutable presentation state escape these
objects. Rapid changes still start from the last displayed blended frame.
The existing whole-scene `StageTransition` retains its separate input policy.

The maintenance regression harness includes these production classes outside
the legacy map fixture's access-remapping macros. Selection is inspected with
read-only queries; messages are checked in rendered text. Snapshot and region
transition checks also use their public interfaces. The popup regression
checks cover `UWidget*` visibility dispatch into `URegionInfoPopup`, whose
`SetVisible` now explicitly overrides the base virtual function.

```powershell
./tests/build_main_stage_isolated.ps1
./tests/run_maintenance_map_ui_checks.ps1 -BuildOnly
./tests/run_popup_ui_checks.ps1 -BuildOnly
./tests/run_main_stage_flow.ps1 -BuildOnly
```

This refactor's validation is compile/link only under the existing Windows
application-control restriction. Added or updated native assertions have not
executed; earlier runtime reports and previews are historical evidence, not
execution results for this revision.

The source comparison in `tests/bin/maintenance-oop-baseline` confirms that
GameManager, Character, ItemData and SkillData sources are unchanged. Twenty-one
layout/input/reward/portrait helpers, all presentation timing constants and
nonempty UI text literals also match the pre-refactor implementation.

### Full maintenance session preview

`maintenance_full_flow_scenario.inc` is shared by the native regression source
and `model_maintenance_full_flow_preview.py`. It describes one 39-second
session: party membership, equipment and character changes, skill assignment,
shop purchases, potion use, then revisiting the equipment/skill/party tabs.
The fixture starts with 200 gold, one Iron Armor, two Small Potions, one Mana
Potion and the Fire Warrior at 37 HP / 7 MP. These are preview-only values;
neither the game defaults nor a save is modified.

The state persists between tabs. Armor affects both current and maximum HP,
so the final re-equip leaves 57/170 HP. Potion gains are then +50, +50 and +13
HP; MP gains are +13 and +0, with one item consumed each time. Purchases leave
80 gold, and the rejected Iron Armor purchase reports a 220-gold shortfall.
The ending party is `[1, 2, 8, 11, 15]`, armor 302 remains equipped, skill 1103
remains in slot 2, HP/MP are 170/20 and the consumable bag is empty.

The continuous media is an independent source-based model, not a recording of
the refactored C++ running. Its game state, presentation and drawing are
separate objects, and its manifest records the production source and fixture
hashes. The legacy skill notice follows 180 Update ticks at a modeled 60 Hz;
the other transition/notice timings use seconds. Model state/media checks
must not be presented as native functional validation.

The native sequence uses the same fixture and public UI input within one
stage session. Presentation-only time advances must preserve gameplay state;
the legacy frame-based skill notice lifetime remains covered by its separate
public-Update check. Under the existing Windows application-control restriction,
the updated native harness is compiled and linked only, without execution.

```powershell
./tests/run_maintenance_map_ui_checks.ps1 -BuildOnly
python tests/model_maintenance_full_flow_preview.py
python tests/encode_main_stage_flow.py --manifest tests/bin/maintenance-full-flow-model/frames.json --output-dir tests/bin/maintenance-full-flow-preview --format webp --webp-width 960
```

### Party membership feedback

Party changes use the existing GameManager toggle and its one-to-five-member
limits. Membership and the selected-party-index rules update immediately;
the active list and roster emphasis transition visually from the last shown
frame. A successful change identifies the character and count delta. Full
party and last-member removal attempts retain the actual roster/count and
show the original restriction wording with a brief warning accent.

Party notices are separate from skill-assignment messages, so switching tabs
does not show a party restriction as a skill result. Character names,
portraits, elements, ACTIVE markers and roster hit rectangles remain intact.
The current Windows application-control restriction limits native checking to
compile/link; the delivered continuous preview is an independent source-based
UI model rather than a game recording or native execution evidence.

List/roster/notice changes blend for 160 ms; the live count remains outside
the blended regions. Notices remain until 2.2 seconds then fade out over
400 ms. The layout keeps the final roster row, guide and notice separate at
1264x644. Roster controls remain responsive during the local blend; the
existing shared tab-transition input guard is retained.

```powershell
./tests/build_main_stage_isolated.ps1
./tests/run_maintenance_map_ui_checks.ps1 -BuildOnly
python tests/model_maintenance_party_preview.py
python tests/encode_main_stage_flow.py --manifest tests/bin/maintenance-party-model/frames.json --output-dir tests/bin/maintenance-party-preview --format webp --webp-width 960
```

The isolated game and expanded check harness compile/link successfully. The
GameManager/Character source hashes and eleven other maintenance helpers
(including inventory, equipment, skill and shop actions/drawing) are unchanged
in `tests/bin/maintenance-party-baseline/source-preservation.json`. The
native regression source covers public-input success/failure/rapid changes,
original index selection, preserved character data, separate Party/Skill
messages, first-frame/metafile handling and three viewport layouts. These
new cases have not executed on this host.

The independent 18-second 960x540 model shows a full-party attempt, removal and
addition, quick consecutive removals to one member, rejected final-member
removal, then restoration to the default five members. It reads the roster,
party limits and presentation constants from source and renders the three
approved runtime portrait bitmaps using their magenta transparency key.
Four proof frames and two compact images were visually reviewed. WebP decode
verification confirms 18,000 ms; media checks are not native game validation.

### Inventory potion feedback

Potion feedback uses actual before/after character and inventory values. HP/MP
numbers reflect the capped recovery immediately, and the consumed item shows
its remaining bag count. Feedback is attached to the character and item ID so
removing an exhausted stack does not highlight the next item in its old slot.
The final use result remains visible even when the inventory becomes empty.

The user explicitly retained the original rule: a potion is consumed even if
its relevant resource is already full. This is a successful use with zero
recovery, not a rejected action. The UI states that result instead of showing
a fictitious heal or silently changing consumption rules. Rejected uses leave
HP/MP and inventory unchanged. GameManager and ItemData remain authoritative.

Native checks for this revision are compile-only because the existing Windows
application-control restriction remains. The continuous preview is a separate
source-based UI model, not native execution or a live game recording.

HP/MP gauges ease from their last displayed fractions over 280 ms while their
numbers stay authoritative. Notices enter/retarget over 160 ms, hold until
2.2 seconds and fade out over 400 ms. The 42px compact cards use separate name
and quantity/type rows. The guide and result fit beneath the full five-row
inventory without changing item hit rectangles.

```powershell
./tests/build_main_stage_isolated.ps1
./tests/run_maintenance_map_ui_checks.ps1 -BuildOnly
python tests/model_maintenance_inventory_preview.py
python tests/encode_main_stage_flow.py --manifest tests/bin/maintenance-inventory-model/frames.json --output-dir tests/bin/maintenance-inventory-preview --format webp --webp-width 960
```

The game build and expanded native harness compile/link. No new native
functional checks have executed. GameManager, Character and ItemData hashes,
plus ten other maintenance helpers, are unchanged in
`tests/bin/maintenance-inventory-baseline/source-preservation.json`. Defensive
missing/invalid item cases use explicitly malformed test data through public
Input; the normal non-consumable equipment route remains intact. The existing
item table has ten kinds, so the four-to-two-column case is an explicitly
synthetic layout-extension fixture, not a normal gameplay capture.

The independently rendered 16-second 960x540 model shows HP
37 -> 87 -> 137 -> 150 and MP 7 -> 20 -> 20, consuming three small potions and
two mana potions. It includes 100 ms consecutive uses, capped recovery,
depleted stacks and the final full-MP use with zero recovery and one consumed.
Four proof frames and compact 1264x644 images were visually reviewed; WebP
decode verification confirms 16,000 ms. It does not depict a nonexistent-item
click or claim that a defensive failure fixture is an ordinary game flow.

### Shop purchase feedback

Shop cards retain their item name, price and Use/Equip type while showing the
current `Bag` quantity (inventory only; equipped items are not counted).
Purchases update the actual gold and inventory immediately. Feedback shows the
last transaction's gold deduction and quantity increase; failed purchases show
the missing gold without changing either value. Rapid clicks still perform
each original purchase and retarget the visual notice. Navigation clears old
purchase feedback without changing the selected character or skill slot.

Notices enter/retarget over 160 ms, remain until 2.2 seconds, then fade out over
400 ms. Quantity/gold accents use the same clock; repeated purchases of the
same item continue from the current emphasis. The notice uses the original
purchase message and adds a separate exact shortfall on failure. Compact shop
cards keep the name/price on the first line and type/Bag count on the second.

The existing GameManager purchase rules and catalog tiers remain authoritative.
The new native regression cases are compiled only: the earlier application
control block still prevents runtime verification on this host. Historical
passed reports do not validate this revision. The shop preview is a separate
Python UI model of the source design/timing, not a game recording or native
functional-check result.

```powershell
./tests/build_main_stage_isolated.ps1
./tests/run_maintenance_map_ui_checks.ps1 -BuildOnly
python tests/model_maintenance_shop_preview.py
python tests/encode_main_stage_flow.py --manifest tests/bin/maintenance-shop-model/frames.json --output-dir tests/bin/maintenance-shop-preview --format webp --webp-width 960
```

The isolated game build and expanded check harness compiled/linked successfully.
The GameManager/ItemData source hashes and ten existing maintenance helpers
(including other tabs' action handlers and shop catalog/hit rectangles) are
unchanged; see `tests/bin/maintenance-shop-baseline/source-preservation.json`.
The model reads item prices/types, shop order and animation constants from
source, uses a clearly declared in-memory fixture, and preserves the earlier
backdrop/chrome assets. Its media checks do not establish native runtime
correctness.

The 15-second 960x540 preview includes a sword purchase, two quick potion
purchases, an unaffordable armor attempt and a subsequent successful potion
purchase. GOLD follows 450 -> 250 -> 200 -> 150 -> 150 -> 100; the failed armor
attempt reports a 150G shortfall and adds no item. Four proof images and a
1264x644 compact model image were visually reviewed. The WebP passed 15,000 ms
decode verification; its loop restarts the fixture, not an in-game refund.

### Equipment changes and skill assignment feedback

Equipment changes keep the authoritative stat values immediate. Changed stat
cards show signed deltas and a brief color emphasis; HP/MP maximum changes are
marked `MAX`, alongside the existing current/maximum values. Feedback belongs
to the selected character and is cleared on navigation. Empty/failed actions
do not invent a successful change. Equipment/inventory accounting remains in
the original GameManager code.

Skill selection and assignment keep their immediate logical behavior, while
the slot content and matching learned-skill emphasis fade locally. Repeated
input can retarget from the currently displayed mixture without blocking the
next assignment. Existing MP costs, range, skill names and status messages stay
visible; assigning the same skill to multiple slots remains supported.

Equipment emphasis lasts 1.6 seconds with a short fade in/out. Skill selection
takes 140 ms, replacement takes 160 ms and a successful assignment has a 600 ms
accent. Invalid skill assignments do not start a new presentation transition.

The current game build and the expanded native check harness compile/link.
Windows Application Control blocked this updated harness before execution, so
these new action checks have **not run** and no new native action screenshots
were produced. Do not use the older 5,349-assertion report as validation of this
change. The blocked execution record is retained at
`tests/bin/maintenance-actions-baseline/application-control-blocked.json`.
`-BuildOnly` compiles the check without attempting execution.

GameManager, Character, ItemData and SkillData source hashes are unchanged;
`tests/bin/maintenance-actions-baseline/logic-preservation.json` records this
source comparison. The delivered action preview is an independent rendering
of the UI design and timing, not a game recording or execution of these checks.

```powershell
./tests/build_main_stage_isolated.ps1
./tests/run_maintenance_map_ui_checks.ps1 -BuildOnly
python tests/model_maintenance_actions_preview.py
python tests/encode_main_stage_flow.py --manifest tests/bin/maintenance-actions-model/frames.json --output-dir tests/bin/maintenance-actions-preview --format webp --webp-width 960
```

The model requires Pillow and Windows' Malgun Gothic font. It reads Warrior
stats, Iron Armor bonuses, skill data and timing constants from source; the
fixture uses GOLD 275, HP 37 and MP 7. It shows armor equip/unequip, slot
selection, assignment and rapid reassignment. Older screenshots supply only
the backdrop, panel chrome and approved portrait. New action content is drawn
independently, with different font rasterization from GDI. Media verification
does not validate native behavior. The resulting 960x540 looping WebP passed
14,000 ms media decode verification. No new native captures were produced by
the blocked run.

### Equipment and skill character transitions

Changing the selected character in Equipment or Skill fades the portrait and
detail area over 160 ms. The selected character updates immediately, while the
header, tabs and character selector remain sharp. The selected skill slot and
each character's equipment/skills are preserved. Rapid selection and switching
tabs start from the most recently displayed mixture. Detail actions wait until
the selected character is fully visible; the selector and tabs remain usable.
Inventory target selection retains its original immediate behavior.

```powershell
./tests/build_main_stage_isolated.ps1
./tests/run_maintenance_map_ui_checks.ps1
./tests/run_main_stage_flow.ps1 -MaintenanceCharacters -OutputName maintenance-characters-flow -BuildOnly
```

The new native replay mode compiles and links, but Windows application control
blocked its execution on this host. Omit `-BuildOnly` on a host that permits it
to capture the full 14-second Input/Draw replay. The delivered fallback preview
reconstructs the production transition timing from actual GDI test snapshots;
it is not a recording of that blocked replay or a live game window. Snapshot
fixture equipment and skill-slot selection are retained in the preview.

Before the subsequent action-feedback changes, the isolated game build passed
with zero warnings/errors and the native maintenance/map regression passed
5,349 assertions and saved 84 captures at
1280x720, 1600x900 and 1264x644. Coverage includes all five character targets,
immediate selection, retained skill slots/equipment, unchanged selector pixels,
rapid character/tab retargets, guarded detail actions during the fade and
working equipment/skill actions after settling. Those results apply to that
earlier revision only. `tests/bin/maintenance-map-ui-checks/execution-status.json`
identifies whether a report is current for its most recent invocation.

To reproduce the snapshot-based fallback using Python with Pillow:

```powershell
python tests/reconstruct_maintenance_characters_preview.py
python tests/encode_main_stage_flow.py --manifest tests/bin/maintenance-characters-reconstruction/frames.json --output-dir tests/bin/maintenance-characters-preview --format webp
```

This preview shows Equipment Warrior/Archer/Mage, Skill Mage/Archer/Warrior,
rapid character selection, then Equipment again. It preserves the actual
regression fixtures (GOLD 0, test weapons, slot 3 and recent-action messages);
the independent stills are not a continuous gameplay session. Source hashes,
fixed regions, settled states and two rapid retargets passed reconstruction
checks. The looping 800x450 WebP passed 14,000 ms decode verification; repeated
hold frames are coalesced without changing their duration.

### Maintenance tab transitions

Maintenance tab changes use a 180 ms smoothstep fade over the body only. The
title, gold, tab controls, map button and cursor stay sharp. Mouse selection,
number keys 1-6 and the inventory-to-equipment route share the same transition.
Rapid selection starts from the last displayed blended body; selecting the
same tab does not restart it. Tab navigation and returning to the map remain
available while ambiguous body actions are held until the content settles.

```powershell
./tests/build_main_stage_isolated.ps1
./tests/run_maintenance_map_ui_checks.ps1
./tests/run_main_stage_flow.ps1 -MaintenanceTabs -OutputName maintenance-tabs-flow
python tests/encode_main_stage_flow.py --manifest tests/bin/maintenance-tabs-flow/frames.json --output-dir tests/bin/maintenance-tabs-preview --format webp
```

The 13-second preview uses the actual maintenance Input/Draw code and public
presentation clock, with the normal default party and scripted pointer input.
It shows Party, Equipment, Skill and Shop, then switches three tabs 100 ms apart
before returning to Party. The encoder adds no transition effects. This is an
offscreen renderer replay, not a live game-window recording. Its native frames,
timeline and encoding report stay under `tests/bin/maintenance-tabs-flow`.

The tab-transition build passed with zero warnings/errors. The native
maintenance/map runner passed 4,449 assertions and 42 captures at 1280x720,
1600x900 and 1264x644. These include body-only pixel blending, rapid retarget
continuity, keyboard navigation, guarded body input and restored purchase,
equipment, consumable, skill and party behavior after settling. All existing
45-skill, reward and map checks also passed. The 390-frame replay and encoded
13,000 ms WebP passed frame/duration verification; default game data stayed
unchanged. `execution-status.json` identifies the current run and source/EXE
hashes, so an older report cannot be mistaken for a newly executed test.

### Continuous map-to-maintenance preview

After the isolated build above, render and encode the current production UI:

```powershell
./tests/run_main_stage_flow.ps1 -OutputName main-stage-flow-smooth
python tests/encode_main_stage_flow.py --manifest tests/bin/main-stage-flow-smooth/frames.json --output-dir tests/bin/main-stage-flow-preview --format webp
```

The encoder requires Python with Pillow and animated WebP support. The native
runner writes a 15-second, 800x450 BGR sequence and `frames.json` under
`tests/bin/main-stage-flow-smooth`; the encoder produces a looping WebP plus
`encoding-report.json`. Omit `--format webp` to also produce a GIF. WebP retains
the source colors more accurately. Raw frames temporarily use about 486 MiB.

This replays the linked production Input/Draw code with scripted client-pointer
coordinates, an in-memory default party and 60 Hz camera/marker updates. It
shows building hover, popup open/CLOSE/reopen, ENTER, maintenance tabs and Escape.
It preserves the initial 1.5 camera zoom and 180 ms building enlargement, and
uses the production popup fade (200 ms open, 160 ms close) and `StageTransition`
compositor (300 ms smoothstep) for map-to-maintenance and the return journey.
The cursor is drawn once above the transition. Startup is settled before the
first frame; the return camera runs from its normal initial state. This is an offscreen
renderer reconstruction, not a live game-window recording or a measurement of
displayed frame rate; stage Init, audio, OS pointer input and disk saves are
not exercised. No effects are added by the preview encoder.

The earlier popup/stage transition update built with zero warnings/errors. Its popup runner passed
626 assertions, including real GDI fade pixels, opening input guards, closing
input shielding, Escape reversal and one-shot ENTER callbacks. The 450-frame
production replay passed the open/close/reopen/ENTER/tab/return route and native
capture checks. The maintenance/map runner and the standalone compositor check
compiled and linked, but Windows application control blocked execution on this
run. The standalone compositor runtime assertions are not reported as passing.
The maintenance/map runner was subsequently expanded for tab transitions and
passed in the current run documented above.

```powershell
./tests/run_stage_transition_checks.ps1 -BuildOnly
```

Without `-BuildOnly`, that standalone check exercises actual GDI blending,
elapsed-time/input guards and GDI resource cleanup on a host that allows it.

## Starlight popup UI

The approved popup theme is shared by the world-map menu, region details,
character/profile/equipment views, inventory/target selection, title settings,
notices and exit confirmation. Production values still come from `Character`,
`GameManager` and `ItemData`; the preview's sample inventory is not installed.
The crest bitmap is a reproducible crop of the approved panel artwork.

```powershell
node tests/import_starlight_ui.mjs --check
./tests/build_main_stage_isolated.ps1
./tests/run_popup_ui_checks.ps1
```

The first build stays under `tests/bin/main-stage-build` and leaves Visual
Studio's normal output folders alone. The popup runner links those production
objects (excluding the game entry point) to a console check. It exercises real
button callbacks, modal hit testing, stage I/P/Escape input, press/release
matching, inventory consumption/equipment changes, and return-to-tab behavior.
It renders the real GDI popup drawing code at 1280×720 and 1600×900, writing BMP
snapshots and `interaction-checks.json` under `tests/bin/popup-ui-checks`.
It creates no game window, starts no stage/audio loop, and changes no disk save.
Use `-BuildOnly` to compile the runner without executing it. A successful build
alone is not evidence that its runtime assertions passed.

The implementation check passed 490 runtime assertions. The existing 32
character/inventory display fields, seven interactions, six region descriptions,
zero/negative equipment modifiers and zero healing values were also reviewed
against the original UI. This verifies popup behavior and rendering; it is not
a full interactive gameplay test.

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
