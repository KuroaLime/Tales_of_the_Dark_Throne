#!/usr/bin/env python3
"""Check Stage 02 geometry, authored artwork integrity and unaffected stages.

These are source-data and asset checks, not execution of the game or automated
recognition of painted obstacles. Review the foot/grid preview after art edits.
"""

from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import sys

from verify_graveyard import (
    BLOCKED, HAZARDS, SOURCE, assigned_int, body_of, flood,
    parse_enemies, parse_players, require, strip_comments, validate_other_stages,
)


SYMBOL_TYPES = {
    "~": "SkyVoid", ".": "Normal", "+": "Road", "=": "Bridge",
    "#": "Wall", "B": "Building", "T": "Tree",
}


def load_celestial(source: str):
    header = strip_comments((SOURCE / "CelestialMapLayout.h").read_text(encoding="utf-8-sig"))
    width, height, tile = (assigned_int(header, name) for name in ("Width", "Height", "TileSize"))
    require((width, height, tile) == (33, 18, 48), "Celestial scene must use 33x18 cells of 48px")
    match = re.search(r"\bRows\s*\[[^\]]*\]\s*=\s*\{([\s\S]*?)\}\s*;", header)
    require(match is not None, "Missing celestial ASCII Rows")
    rows = re.findall(r'"([^"\n]*)"', match[1])
    require(len(rows) == height, f"Expected {height} rows, found {len(rows)}")
    for y, row in enumerate(rows):
        require(len(row) == width, f"Row {y} has {len(row)} cells, expected {width}")
        require(set(row) <= SYMBOL_TYPES.keys(), f"Unknown terrain in row {y}: {set(row) - SYMBOL_TYPES.keys()}")
    stage = body_of(source, "BuildStage02")
    require(re.search(r"map\.gridAlignedBackground\s*=\s*true\s*;", stage) is not None,
            "Celestial artwork must use exact grid-aligned loading")
    require(re.search(r"map\.allowElementalSkyTraversal\s*=\s*false\s*;", stage) is not None,
            "Celestial void must block all elements, including Light")
    tiles = [[SYMBOL_TYPES[value] for value in row] for row in rows]
    return width, height, tile, rows, tiles, [], parse_players(stage), parse_enemies(stage)


def validate_geometry(source: str) -> None:
    width, height, tile, rows, tiles, _, players, enemies = load_celestial(source)
    walkable = {(x, y) for y in range(height) for x in range(width) if tiles[y][x] not in BLOCKED}
    require(walkable, "No walkable celestial ground")
    border = {(x, y) for y in range(height) for x in range(width)
              if x in (0, width - 1) or y in (0, height - 1)}
    require(all(rows[y][x] == "~" for x, y in border), "Floating platforms must remain inside the sky border")
    require(len(players) == 5 and len(enemies) == 4, "Expected five players and four enemies")
    starts = players + enemies
    require(len(set(starts)) == len(starts), "Starting positions overlap")
    for x, y in starts:
        require((x, y) in walkable, f"Start {(x, y)} is blocked or outside the map")
        require(tiles[y][x] not in HAZARDS, f"Start {(x, y)} occupies damaging ground")
    reached = flood(players[0], walkable)
    require(reached == walkable, f"Disconnected platform cells: {sorted(walkable - reached)}")

    for x, y in walkable:
        require(not (((x - 1, y) not in walkable and (x + 1, y) not in walkable) or
                     ((x, y - 1) not in walkable and (x, y + 1) not in walkable)),
                f"One-cell corridor pinch at {(x, y)}")
        roomy = any({(sx, sy), (sx + 1, sy), (sx, sy + 1), (sx + 1, sy + 1)} <= walkable
                    for sx in (x - 1, x) for sy in (y - 1, y))
        require(roomy, f"Cell {(x, y)} lacks a complete 2x2 standing area")

    pending = {(x, y) for y, row in enumerate(rows) for x, value in enumerate(row) if value == "="}
    require(pending, "Floating temple has no authored bridge cells")
    bridges = []
    while pending:
        component = flood(next(iter(pending)), pending)
        pending -= component
        bridges.append(component)
        xs, ys = zip(*component)
        span_x, span_y = max(xs) - min(xs) + 1, max(ys) - min(ys) + 1
        require(min(span_x, span_y) >= 2,
                f"Bridge is narrower than two cells: {sorted(component)}")
        require(len(component) == span_x * span_y,
                f"Authored rectangular bridge has a hole or narrowed bend: {sorted(component)}")
        require(component <= walkable, "A bridge contains blocked cells")
        west = {y for y in ys if (min(xs) - 1, y) in walkable - component}
        east = {y for y in ys if (max(xs) + 1, y) in walkable - component}
        north = {x for x in xs if (x, min(ys) - 1) in walkable - component}
        south = {x for x in xs if (x, max(ys) + 1) in walkable - component}
        def wide_contact(values):
            return any(value + 1 in values for value in values)
        require((wide_contact(west) and wide_contact(east)) or
                (wide_contact(north) and wide_contact(south)),
                f"Bridge lacks two-cell approaches on opposite banks: {sorted(component)}")

    # The combat terraces should have a route around any single failed crossing,
    # so a narrow bridge cannot be the sole link between the two teams.
    require(len(bridges) >= 2, "Expected alternate crossings")
    for component in bridges:
        remaining = walkable - component
        require(set(starts) <= remaining, "Do not place a starting unit on a bridge")
        require(set(starts) <= flood(players[0], remaining),
                f"Removing bridge {sorted(component)} disconnects a starting unit; no alternate route")

    edges = 0
    for x, y in sorted(walkable):
        for nx, ny in ((x + 1, y), (x, y + 1)):
            if (nx, ny) not in walkable:
                continue
            left, right = min(x, nx) * tile + 16, max(x, nx) * tile + 32
            top, bottom = min(y, ny) * tile + 30, max(y, ny) * tile + 42
            # Cardinal translation sweeps exactly this rectangle, including all
            # intermediate animation positions rather than only cell centers.
            for cy in range(top // tile, (bottom - 1) // tile + 1):
                for cx in range(left // tile, (right - 1) // tile + 1):
                    require((cx, cy) in walkable,
                            f"Footprint crosses blocked {(cx, cy)} on {(x, y)} -> {(nx, ny)}")
            edges += 1
    counts = Counter(value for row in tiles for value in row)
    print(f"PASS: celestial {width}x{height}, {len(walkable)} connected walkable cells; closed sky border")
    print(f"PASS: {len(players)} player + {len(enemies)} enemy starts safe; all walkable cells have 2x2 clearance")
    print(f"PASS: {len(bridges)} bridges and their approaches >=2 cells wide; any one crossing has an alternate route")
    print(f"PASS: void blocks all elements; exact foot sweep clears {edges} cardinal edges")
    print("Terrain:", ", ".join(f"{key}={value}" for key, value in sorted(counts.items())))


def background_path(source: str) -> Path:
    stage = body_of(source, "BuildStage02")
    match = re.search(r'map\.backgroundFilePath\s*=\s*L("(?:\\.|[^"\\])*")\s*;', stage)
    require(match is not None, "Celestial scene has no background file path")
    return SOURCE / json.loads(match[1]).replace("\\", "/")


def validate_painted_scene(source: str) -> None:
    width, height, tile, rows, *_ = load_celestial(source)
    asset = background_path(source)
    data = asset.read_bytes()
    require(len(data) >= 54 and data[:2] == b"BM", "Celestial scene is not a Windows bitmap")
    image_width, image_height = struct.unpack_from("<ii", data, 18)
    require((image_width, abs(image_height)) == (width * tile, height * tile),
            "Celestial artwork must match world pixels without runtime stretching")
    manifest = json.loads(asset.with_suffix(".layout.json").read_text(encoding="utf-8-sig"))
    require(manifest["rows"] == rows and manifest["tileSize"] == tile,
            "Collision layout changed since the celestial scene was aligned")
    require(manifest["bitmapSha256"] == hashlib.sha256(data).hexdigest(),
            "Celestial painting changed without updating its alignment manifest")
    print(f"PASS: celestial scene {image_width}x{abs(image_height)} and alignment manifest match")


def validate_graveyard_baseline(source: str, baseline_path: Path) -> None:
    baseline = json.loads(baseline_path.read_text(encoding="utf-8-sig"))
    actual = hashlib.sha256(body_of(source, "BuildStage01").encode()).hexdigest()
    require(actual == baseline["stageBodySha256"], "Stage01 builder changed during celestial work")
    for relative, expected in baseline["files"].items():
        require(hashlib.sha256((SOURCE / relative).read_bytes()).hexdigest() == expected,
                f"Graveyard file changed during celestial work: {relative}")
    print("PASS: graveyard builder, terrain and painted asset match the pre-celestial working-state snapshot")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--geometry-only", action="store_true", help="check source geometry while artwork is being authored")
    parser.add_argument("--check-unchanged-behavior", "--check-unchanged-stages",
                        dest="check_unchanged_behavior", action="store_true",
                        help="compare unchanged base/test-map, routing and monster helpers with HEAD; old --check-unchanged-stages name is deprecated")
    parser.add_argument("--graveyard-baseline", type=Path, help="compare Stage01 with a pre-edit working-state hash snapshot")
    options = parser.parse_args()
    try:
        source = (SOURCE / "BattleMapData.cpp").read_text(encoding="utf-8-sig")
        validate_geometry(source)
        if not options.geometry_only:
            validate_painted_scene(source)
        if options.check_unchanged_behavior:
            validate_other_stages(source)
        if options.graveyard_baseline:
            validate_graveyard_baseline(source, options.graveyard_baseline)
    except (AssertionError, KeyError, OSError, ValueError, subprocess.CalledProcessError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
