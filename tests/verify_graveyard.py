#!/usr/bin/env python3
"""Validate the graveyard's authored map and continuous ground-footprint clearance.

Uses only the Python standard library. Run from any directory:
    python tests/verify_graveyard.py
    python tests/verify_graveyard.py --check-unchanged-behavior

This reads the actual C++ layout and stage declarations. It checks geometric
properties, not screenshot colors. The C++ integration/build must be checked
separately because this is an independent source-data validator.
"""

from __future__ import annotations

import argparse
from collections import Counter, deque
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "Tower_Triumph" / "Tower_Triumph"
SYMBOL_TYPES = {
    ".": "Normal", "+": "Road", "~": "Water", "#": "Wall",
    "B": "Building", "T": "Tree", "G": "Grave",
    "C": "CursedGround", "M": "MagicHazard", "=": "Bridge",
}
BLOCKED = {"Water", "Building", "Wall", "Fence", "Tree", "Grave", "SkyVoid"}
HAZARDS = {"CursedGround", "MagicHazard", "Lava"}
DIRECTIONS = ((1, 0), (-1, 0), (0, 1), (0, -1))


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def strip_comments(text: str) -> str:
    # Preserve quoted C++ strings, including the backslashes in resource paths.
    pattern = r'"(?:\\.|[^"\\])*"|/\*[\s\S]*?\*/|//[^\n]*'
    return re.sub(pattern, lambda m: m[0] if m[0].startswith('"') else "", text)


def body_of(text: str, function_name: str) -> str:
    clean = strip_comments(text)
    match = re.search(r"\b" + re.escape(function_name) + r"\s*\([^;{}]*\)\s*\{", clean)
    require(match is not None, f"Missing function {function_name}")
    start = match.end()
    depth = 1
    for token in re.finditer(r'"(?:\\.|[^"\\])*"|[{}]', clean[start:]):
        value = token[0]
        if value == "{":
            depth += 1
        elif value == "}":
            depth -= 1
            if depth == 0:
                return clean[start:start + token.start()]
    raise AssertionError(f"Unterminated function {function_name}")


def assigned_int(text: str, name: str) -> int:
    match = re.search(r"\b" + re.escape(name) + r"\s*=\s*(\d+)\s*;", text)
    require(match is not None, f"Missing integer assignment for {name}")
    return int(match[1])


def parse_players(body: str) -> list[tuple[int, int]]:
    match = re.search(r"AddPlayerStarts\s*\(\s*map\s*,\s*\{([\s\S]*?)\}\s*\)", body)
    require(match is not None, "Missing player start declarations")
    return [tuple(map(int, pair)) for pair in re.findall(r"\{\s*(\d+)\s*,\s*(\d+)\s*\}", match[1])]


def parse_enemies(body: str) -> list[tuple[int, int]]:
    fixed = re.findall(
        r'MakeMonsterStart\s*\(\s*ECharacterClass::\w+\s*,\s*L"[^"\n]*"'
        r"\s*,\s*(\d+)\s*,\s*(\d+)", body,
    )
    random = re.findall(r"MakeRandomStage03Monster\s*\(\s*(\d+)\s*,\s*(\d+)", body)
    return [tuple(map(int, pair)) for pair in fixed + random]


def neighbors(point: tuple[int, int]):
    x, y = point
    for dx, dy in DIRECTIONS:
        yield x + dx, y + dy


def flood(seed: tuple[int, int], allowed: set[tuple[int, int]]) -> set[tuple[int, int]]:
    visited = {seed}
    pending = deque([seed])
    while pending:
        for point in neighbors(pending.popleft()):
            if point in allowed and point not in visited:
                visited.add(point)
                pending.append(point)
    return visited


def load_graveyard(source: str):
    header = strip_comments((SOURCE / "GraveyardMapLayout.h").read_text(encoding="utf-8-sig"))
    width = assigned_int(header, "Width")
    height = assigned_int(header, "Height")
    tile_size = assigned_int(header, "TileSize")
    require((width, height, tile_size) == (33, 18, 48), "Graveyard must use its declared 33x18, 48px design")
    rows_match = re.search(r"\bRows\s*\[[^\]]*\]\s*=\s*\{([\s\S]*?)\}\s*;", header)
    require(rows_match is not None, "Missing ASCII Rows array")
    rows = re.findall(r'"([^"\n]*)"', rows_match[1])
    require(len(rows) == height, f"Expected {height} rows, got {len(rows)}")
    for y, row in enumerate(rows):
        require(len(row) == width, f"Row {y}: expected {width} cells, got {len(row)}")
        require(set(row) <= SYMBOL_TYPES.keys(), f"Row {y}: unrecognized terrain symbols {set(row) - SYMBOL_TYPES.keys()}")
    tiles = [[SYMBOL_TYPES[value] for value in row] for row in rows]
    stage = body_of(source, "BuildStage01")
    props = []
    prop_cells = set()
    for match in re.finditer(
        r"AddMapProp\s*\(\s*map\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)"
        r"\s*,\s*ETileType::(\w+)\s*,\s*(\d+)\s*\)", stage,
    ):
        x, y, w, h = map(int, match.groups()[:4])
        tile_type, atlas_index = match[5], int(match[6])
        require(w > 0 and h > 0, f"Empty prop at {(x, y)}")
        require(0 <= x < x + w <= width and 0 <= y < y + h <= height, f"Out-of-bounds prop at {(x, y, w, h)}")
        require(tile_type in BLOCKED, f"Prop {(x, y)} occupies passable {tile_type}")
        require(0 <= atlas_index < 16, f"Prop {(x, y)} has an invalid atlas slot {atlas_index}")
        footprint = {(cx, cy) for cy in range(y, y + h) for cx in range(x, x + w)}
        require(not (footprint & prop_cells), f"Overlapping prop artwork at {sorted(footprint & prop_cells)}")
        prop_cells.update(footprint)
        for cy in range(y, y + h):
            for cx in range(x, x + w):
                require(tiles[cy][cx] in BLOCKED, f"Prop {(x, y)} covers authored walkable cell {(cx, cy)}")
                tiles[cy][cx] = tile_type
        props.append((x, y, w, h, tile_type, atlas_index))
    require(props or re.search(r"map\.gridAlignedBackground\s*=\s*true\s*;", stage),
            "Graveyard needs blocking props or an authored grid-aligned scene")
    water = re.search(r"map\.allowElementalWaterTraversal\s*=\s*(true|false)\s*;", stage)
    require(water is not None and water[1] == "false", "Graveyard river must block every element, including Water")
    return width, height, tile_size, rows, tiles, props, parse_players(stage), parse_enemies(stage)


def validate_geometry(source: str) -> None:
    width, height, tile_size, rows, tiles, props, players, enemies = load_graveyard(source)
    walkable = {(x, y) for y in range(height) for x in range(width) if tiles[y][x] not in BLOCKED}
    require(walkable, "No traversable ground")
    border = {(x, y) for y in range(height) for x in range(width) if x in (0, width - 1) or y in (0, height - 1)}
    require(not (border & walkable), f"Open outer border: {sorted(border & walkable)}")
    require(len(players) == 5 and len(enemies) == 4, f"Expected 5 players and 4 enemies, got {len(players)} and {len(enemies)}")
    starts = players + enemies
    require(len(set(starts)) == len(starts), "Spawn positions overlap")
    for point in starts:
        require(point in walkable, f"Spawn {point} is outside the map or blocked")
        require(tiles[point[1]][point[0]] not in HAZARDS, f"Spawn {point} starts on a damage tile")
    reached = flood(players[0], walkable)
    require(reached == walkable, f"Disconnected walkable cells: {sorted(walkable - reached)}")

    pinches = []
    for x, y in walkable:
        if ((x - 1, y) not in walkable and (x + 1, y) not in walkable) or ((x, y - 1) not in walkable and (x, y + 1) not in walkable):
            pinches.append((x, y))
    require(not pinches, f"One-cell corridor pinches: {sorted(pinches)}")
    for x, y in walkable:
        roomy = any(
            {(sx, sy), (sx + 1, sy), (sx, sy + 1), (sx + 1, sy + 1)} <= walkable
            for sx in (x - 1, x) for sy in (y - 1, y)
        )
        require(roomy, f"Walkable cell {(x, y)} has no complete 2x2 standing area")

    bridges = {(x, y) for y, row in enumerate(rows) for x, value in enumerate(row) if value == "="}
    require(bridges, "River layout has no authored bridges")
    bridge_count = 0
    while bridges:
        component = flood(next(iter(bridges)), bridges)
        bridges -= component
        bridge_count += 1
        xs, ys = zip(*component)
        span_x, span_y = max(xs) - min(xs) + 1, max(ys) - min(ys) + 1
        require(min(span_x, span_y) >= 2, f"Bridge is narrower than two cells: {sorted(component)}")
        require(len(component) == span_x * span_y, f"Bridge contains a hole or narrow bend: {sorted(component)}")
        require(component <= walkable, "A bridge is blocked by a prop")
        west = any((min(xs) - 1, y) in walkable for y in ys)
        east = any((max(xs) + 1, y) in walkable for y in ys)
        north = any((x, min(ys) - 1) in walkable for x in xs)
        south = any((x, max(ys) + 1) in walkable for x in xs)
        require((west and east) or (north and south), "Bridge does not connect opposite banks")

    # For cardinal translation the swept footprint is exactly this rectangle,
    # so exhaustive rectangle intersection proves clearance between frames too.
    checked_edges = 0
    for x, y in sorted(walkable):
        for nx, ny in ((x + 1, y), (x, y + 1)):
            if (nx, ny) not in walkable:
                continue
            left = min(x, nx) * tile_size + 24 - 8
            right = max(x, nx) * tile_size + 24 + 8
            top = min(y, ny) * tile_size + 36 - 6
            bottom = max(y, ny) * tile_size + 36 + 6
            for cy in range(top // tile_size, (bottom - 1) // tile_size + 1):
                for cx in range(left // tile_size, (right - 1) // tile_size + 1):
                    require((cx, cy) in walkable, f"Footprint crosses blocked {(cx, cy)} on edge {(x, y)} -> {(nx, ny)}")
            checked_edges += 1

    counts = Counter(tile for row in tiles for tile in row)
    print(f"PASS: {width}x{height} cells, {len(walkable)} connected walkable cells; closed border")
    print(f"PASS: {len(players)} player + {len(enemies)} enemy starts are unique, safe and connected")
    print(f"PASS: {len(props)} non-overlapping blocked props, {bridge_count} bridges >=2 cells wide; no one-cell pinches")
    print(f"PASS: river blocks all elements; exact foot sweep clears all {checked_edges} walkable edges")
    print("Terrain:", ", ".join(f"{key}={value}" for key, value in sorted(counts.items())))


def ordinary_stage_signature(source: str, stage_number: int):
    """Interpret only the declarative pieces used by unchanged stage 3."""
    base = body_of(source, "InitBaseMap")
    stage = body_of(source, f"BuildStage{stage_number:02d}")
    width, height, tile_size = (assigned_int(base, f"map.{name}") for name in ("width", "height", "tileSize"))
    fill = re.search(r"FillMapTiles\s*\(\s*map\s*,\s*ETileType::(\w+)\s*\)", stage)
    require(fill is not None, f"Stage {stage_number}: missing fill")
    tiles = [[fill[1] for _ in range(width)] for _ in range(height)]
    for x1, y1, x2, y2, tile_type in re.findall(
        r"SetMapRect\s*\(\s*map\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*ETileType::(\w+)\s*\)", stage,
    ):
        x1, y1, x2, y2 = map(int, (x1, y1, x2, y2))
        for y in range(max(0, min(y1, y2)), min(height, max(y1, y2) + 1)):
            for x in range(max(0, min(x1, x2)), min(width, max(x1, x2) + 1)):
                tiles[y][x] = tile_type
    background = re.search(r'map\.backgroundFilePath\s*=\s*L"([^"\n]*)"', stage)
    require(background is not None, f"Stage {stage_number}: missing background")
    return {
        "size": (width, height, tile_size), "tiles": tiles,
        "players": parse_players(stage), "enemies": parse_enemies(stage),
        "background": background[1],
        "camera": (assigned_int(stage, "map.cameraStartX"), assigned_int(stage, "map.cameraStartY")),
    }


def validate_painted_scene(source: str) -> None:
    stage = body_of(source, "BuildStage01")
    if not re.search(r"map\.gridAlignedBackground\s*=\s*true\s*;", stage):
        return
    match = re.search(r'map\.backgroundFilePath\s*=\s*L"([^"\n]*)"', stage)
    require(match is not None, "Painted graveyard has no background path")
    asset = SOURCE / match[1].replace("\\\\", "/")
    data = asset.read_bytes()
    require(data[:2] == b"BM" and len(data) >= 54, "Painted scene is not a Windows bitmap")
    image_width, image_height = struct.unpack_from("<ii", data, 18)
    width, height, tile, rows, *_ = load_graveyard(source)
    require((image_width, abs(image_height)) == (width * tile, height * tile),
            "Painted scene must match world pixels exactly without runtime stretching")
    manifest = json.loads(asset.with_suffix(".layout.json").read_text(encoding="utf-8-sig"))
    require(manifest["rows"] == rows and manifest["tileSize"] == tile,
            "Collision layout changed since the painted scene was aligned")
    require(manifest["bitmapSha256"] == hashlib.sha256(data).hexdigest(),
            "Painted scene changed without updating its alignment manifest")
    print(f"PASS: painted scene {image_width}x{abs(image_height)} and layout manifest match")


def validate_unchanged_behavior(source: str) -> None:
    """Protect unchanged helpers now that all five battle stages are authored.

    Stage builders intentionally differ from HEAD; per-stage working snapshots
    protect those assets separately. This comparison is never a claim that
    redesigned stage geometry still matches its original version.
    """
    result = subprocess.run(
        ["git", "show", "HEAD:Tower_Triumph/Tower_Triumph/BattleMapData.cpp"],
        cwd=ROOT, capture_output=True, check=True,
    )
    baseline = result.stdout.decode("utf-8-sig")
    functions = (
        "InitBaseMap", "BuildTest01", "FromStageId", "LoadByStageId", "Load",
        "MakeMonsterStart", "MakeRandomStage03Monster",
    )
    for name in functions:
        require(body_of(source, name) == body_of(baseline, name),
                f"Unchanged helper {name} differs from HEAD")
    print("PASS: base/test-map helpers, stage routing and monster/random-roster construction match HEAD")


def validate_other_stages(source: str) -> None:
    """Compatibility alias; no redesigned stage is compared with HEAD."""
    validate_unchanged_behavior(source)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check-unchanged-behavior", "--check-unchanged-stages",
                        dest="check_unchanged_behavior", action="store_true",
                        help="compare unchanged base/test-map, routing and monster helpers with HEAD; old --check-unchanged-stages name is deprecated")
    options = parser.parse_args()
    try:
        source = (SOURCE / "BattleMapData.cpp").read_text(encoding="utf-8-sig")
        validate_geometry(source)
        validate_painted_scene(source)
        if options.check_unchanged_behavior:
            validate_other_stages(source)
    except (AssertionError, OSError, subprocess.CalledProcessError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
