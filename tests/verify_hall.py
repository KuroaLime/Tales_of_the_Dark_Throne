#!/usr/bin/env python3
"""Check Stage 03 hall geometry, artwork integrity and preserved map behavior.

These are source-data checks, not native execution or recognition of painted
obstacles. Review the continuous-foot-sweep preview after every art change.
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
    BLOCKED, SOURCE, assigned_int, body_of, flood, parse_enemies, parse_players,
    require, strip_comments, validate_unchanged_behavior,
)


SYMBOL_TYPES = {".": "Normal", "+": "Road", "#": "Wall", "B": "Building"}


def make_layout(width, height, tile, rows, players, enemies):
    require((width, height, tile) == (33, 18, 48), "Hall must use 33x18 cells of 48px")
    require(len(rows) == height, f"Expected {height} rows, found {len(rows)}")
    for y, row in enumerate(rows):
        require(len(row) == width, f"Row {y} has {len(row)} cells, expected {width}")
        require(set(row) <= SYMBOL_TYPES.keys(), f"Unknown terrain in row {y}: {set(row) - SYMBOL_TYPES.keys()}")
    tiles = [[SYMBOL_TYPES[value] for value in row] for row in rows]
    return width, height, tile, rows, tiles, [], [tuple(p) for p in players], [tuple(p) for p in enemies]


def load_candidate(path: Path):
    data = json.loads(path.read_text(encoding="utf-8-sig"))
    return make_layout(data["width"], data["height"], data["tileSize"],
                       data["rows"], data["players"], data["enemies"])


def load_hall(source: str):
    header = strip_comments((SOURCE / "HallMapLayout.h").read_text(encoding="utf-8-sig"))
    width, height, tile = (assigned_int(header, name) for name in ("Width", "Height", "TileSize"))
    match = re.search(r"\bRows\s*\[[^\]]*\]\s*=\s*\{([\s\S]*?)\}\s*;", header)
    require(match is not None, "Missing hall ASCII Rows")
    rows = re.findall(r'"([^"\n]*)"', match[1])
    stage = body_of(source, "BuildStage03")
    require(re.search(r"map\.gridAlignedBackground\s*=\s*true\s*;", stage) is not None,
            "Hall artwork must use exact grid-aligned loading")
    require("InitBaseMap" not in stage, "Hall must import its authored layout directly")
    calls = re.findall(r"MakeRandomStage03Monster\s*\(\s*\d+\s*,\s*\d+\s*,\s*(\d+)\s*\)", stage)
    require(calls == ["0", "1", "2", "3"], "Preserve the four random monster slots and their order")
    return make_layout(width, height, tile, rows, parse_players(stage), parse_enemies(stage))


def has_wide_crossing(walkable, west, east, minimum_y, maximum_y):
    """Route a complete 2x2 area, rather than count unrelated adjacent cells."""
    anchors = {(x, y) for x, y in walkable
               if west <= x < east and minimum_y <= y < maximum_y
               and {(x, y), (x + 1, y), (x, y + 1), (x + 1, y + 1)} <= walkable}
    starts = {p for p in anchors if p[0] == west}
    ends = {p for p in anchors if p[0] == east - 1}
    return bool(ends) and any(flood(start, anchors) & ends for start in starts)


def validate_geometry(source: str, candidate=None) -> None:
    width, height, tile, rows, tiles, _, players, enemies = candidate or load_hall(source)
    walkable = {(x, y) for y in range(height) for x in range(width) if tiles[y][x] not in BLOCKED}
    require(walkable, "No walkable hall floor")
    border = {(x, y) for y in range(height) for x in range(width)
              if x in (0, width - 1) or y in (0, height - 1)}
    require(all(rows[y][x] == "#" for x, y in border), "Hall needs a closed wall border")
    require(len(players) == 5 and len(enemies) == 4, "Expected five players and four enemies")
    starts = players + enemies
    require(len(set(starts)) == len(starts), "Starting positions overlap")
    for point in starts:
        require(point in walkable, f"Start {point} is blocked or outside the map")
    reached = flood(players[0], walkable)
    require(reached == walkable, f"Disconnected floor cells: {sorted(walkable - reached)}")
    for x, y in walkable:
        require(not (((x - 1, y) not in walkable and (x + 1, y) not in walkable) or
                     ((x, y - 1) not in walkable and (x, y + 1) not in walkable)),
                f"One-cell corridor pinch at {(x, y)}")
        roomy = any({(sx, sy), (sx + 1, sy), (sx, sy + 1), (sx + 1, sy + 1)} <= walkable
                    for sx in (x - 1, x) for sy in (y - 1, y))
        require(roomy, f"Cell {(x, y)} lacks a complete 2x2 standing area")

    west, east = min(x for x, _ in players), max(x for x, _ in enemies)
    require(west < east, "Expected players to approach from west and enemies from east")
    mid = height // 2
    # The half-map bands do not overlap. Each must carry a complete moving 2x2
    # block between the two team areas, proving separate broad north/south routes.
    require(has_wide_crossing(walkable, west, east, 1, mid - 1),
            "No continuous two-cell-wide northern route between team areas")
    require(has_wide_crossing(walkable, west, east, mid, height - 2),
            "No continuous two-cell-wide southern route between team areas")

    edges = 0
    for x, y in sorted(walkable):
        for nx, ny in ((x + 1, y), (x, y + 1)):
            if (nx, ny) not in walkable:
                continue
            left, right = min(x, nx) * tile + 16, max(x, nx) * tile + 32
            top, bottom = min(y, ny) * tile + 30, max(y, ny) * tile + 42
            for cy in range(top // tile, (bottom - 1) // tile + 1):
                for cx in range(left // tile, (right - 1) // tile + 1):
                    require((cx, cy) in walkable,
                            f"Footprint crosses blocked {(cx, cy)} on {(x, y)} -> {(nx, ny)}")
            edges += 1
    counts = Counter(value for row in tiles for value in row)
    print(f"PASS: hall {width}x{height}, {len(walkable)} connected walkable cells; closed wall border")
    print(f"PASS: {len(players)} player + {len(enemies)} enemy starts safe; all walkable cells have 2x2 clearance")
    print("PASS: separate north/south routes each carry a continuous 2x2 area between team sides")
    print(f"PASS: logical ground-foot sweep clears {edges} cardinal edges")
    print("Terrain:", ", ".join(f"{key}={value}" for key, value in sorted(counts.items())))


def background_path(source: str) -> Path:
    stage = body_of(source, "BuildStage03")
    match = re.search(r'map\.backgroundFilePath\s*=\s*L("(?:\\.|[^"\\])*")\s*;', stage)
    require(match is not None, "Hall scene has no background file path")
    return SOURCE / json.loads(match[1]).replace("\\", "/")


def validate_painted_scene(source: str) -> None:
    width, height, tile, rows, *_ = load_hall(source)
    asset = background_path(source)
    data = asset.read_bytes()
    require(len(data) >= 54 and data[:2] == b"BM", "Hall scene is not a Windows bitmap")
    image_width, image_height = struct.unpack_from("<ii", data, 18)
    require((image_width, abs(image_height)) == (width * tile, height * tile),
            "Hall artwork must match world pixels without runtime stretching")
    manifest = json.loads(asset.with_suffix(".layout.json").read_text(encoding="utf-8-sig"))
    require((manifest["width"], manifest["height"], manifest["tileSize"]) == (width, height, tile)
            and manifest["rows"] == rows, "Collision layout changed since hall artwork alignment")
    require(manifest["bitmapSha256"] == hashlib.sha256(data).hexdigest(),
            "Hall painting changed without updating its alignment manifest")
    print(f"PASS: hall scene {image_width}x{abs(image_height)} and alignment manifest match")


def validate_previous_stages(source: str, baseline_path: Path) -> None:
    baseline = json.loads(baseline_path.read_text(encoding="utf-8-sig"))
    for number in (1, 2, 4, 5):
        actual = hashlib.sha256(body_of(source, f"BuildStage{number:02d}").encode()).hexdigest()
        require(actual == baseline["stageBodySha256"][str(number)],
                f"Stage{number:02d} builder changed during hall work")
    for relative, expected in baseline["files"].items():
        require(hashlib.sha256((SOURCE / relative).read_bytes()).hexdigest() == expected,
                f"Previously authored stage or original background changed during hall work: {relative}")
    print("PASS: Stage01/02/04/05 builders, terrain and painted assets match the pre-hall snapshot; originals preserved")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--geometry-only", action="store_true", help="check source geometry while artwork is being authored")
    parser.add_argument("--candidate", type=Path, help="check a proposed JSON layout instead of production geometry and artwork")
    parser.add_argument("--check-unchanged-behavior", "--check-unchanged-stages",
                        dest="check_unchanged_behavior", action="store_true",
                        help="compare unchanged base/test-map, routing and monster helpers with HEAD; old --check-unchanged-stages name is deprecated")
    parser.add_argument("--previous-stages-baseline", type=Path,
                        help="compare authored Stage01/02/04/05 with a pre-edit working-state hash snapshot")
    options = parser.parse_args()
    try:
        source = (SOURCE / "BattleMapData.cpp").read_text(encoding="utf-8-sig")
        validate_geometry(source, load_candidate(options.candidate) if options.candidate else None)
        if options.candidate:
            print("NOTE: candidate check skips production loading flags and artwork integrity")
        elif not options.geometry_only:
            validate_painted_scene(source)
        if options.check_unchanged_behavior:
            validate_unchanged_behavior(source)
        if options.previous_stages_baseline:
            validate_previous_stages(source, options.previous_stages_baseline)
    except (AssertionError, KeyError, OSError, ValueError, TypeError, subprocess.CalledProcessError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
