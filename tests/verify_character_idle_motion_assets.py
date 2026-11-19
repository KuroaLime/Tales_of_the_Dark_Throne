#!/usr/bin/env python3
"""Check authored idle/settling sheets and their exact packed endpoint pixels."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

from verify_character_action_assets import read_bitmap, require

ROOT = Path(__file__).resolve().parents[1]
RESOURCES = ROOT / 'Tower_Triumph' / 'Tower_Triumph' / 'Resource'
KEY = (255, 0, 255)
CLASSES = ('Warrior', 'Archer', 'Mage')
KINDS = ('IdleCycle', 'WalkStop', 'IdleReturn')


def frame_pixels(pixel, index: int, cell: int, columns: int = 4):
    x0, y0 = index % columns * cell, index // columns * cell
    return tuple(pixel(x0 + x, y0 + y) for y in range(cell) for x in range(cell))


def verify(character: str, kind: str) -> None:
    name = f'{character}_{kind}'
    layout = json.loads((RESOURCES / f'{name}.layout.json').read_text(encoding='utf-8-sig'))
    cell = 96 if kind == 'IdleCycle' else 128
    root = [48, 88] if cell == 96 else [64, 112]
    require(layout['cellSize'] == cell and layout['root'] == root and
            layout['nominalFrameHeight'] == 96 and layout['frameCount'] == 12,
            f'{name}: incorrect runtime contract')
    for field in ('source', 'bitmap'):
        require(hashlib.sha256((RESOURCES / layout[field]).read_bytes()).hexdigest() == layout[field + 'Sha256'],
                f'{name}: {field} hash differs')
    alignment = json.loads((RESOURCES / f'{name}.alignment.json').read_text(encoding='utf-8-sig'))
    require(alignment['scale'] == layout['uniformScale'], f'{name}: uniform scale differs')
    width, height, pixel = read_bitmap(RESOURCES / f'{name}.bmp')
    require((width, height) == (cell * 4, cell * 3), f'{name}: invalid dimensions')
    frames = []
    for index in range(12):
        colors = frame_pixels(pixel, index, cell)
        frames.append(colors)
        points = [(x, y) for y in range(cell) for x in range(cell) if colors[y * cell + x] != KEY]
        require(len(points) > 100, f'{name} #{index}: empty or implausibly small pose')
        bounds = [min(x for x, _ in points), min(y for _, y in points),
                  max(x for x, _ in points) + 1, max(y for _, y in points) + 1]
        require(bounds[0] > 0 and bounds[1] > 0 and bounds[2] < cell and bounds[3] < cell,
                f'{name} #{index}: cell boundary clipping')
        require(bounds == layout['frames'][index]['outputBounds'], f'{name} #{index}: stored bounds differ')
        require(layout['frames'][index]['outputRoot'] == root, f'{name} #{index}: root differs')
    expected_boundaries = {'IdleCycle': {11}, 'WalkStop': {0, 5, 6, 11}, 'IdleReturn': {0, 3, 4, 7, 8, 11}}[kind]
    require({b['destinationFrame'] for b in layout['boundarySources']} == expected_boundaries,
            f'{name}: missing boundary copies')
    for boundary in layout['boundarySources']:
        path = RESOURCES / boundary['source']
        require(hashlib.sha256(path.read_bytes()).hexdigest() == boundary['sourceSha256'],
                f'{name}: boundary source changed after packing: {path.name}')
        _, _, source_pixel = read_bitmap(path)
        source_cell = boundary['sourceCellSize']
        original = frame_pixels(source_pixel, boundary['sourceFrame'], source_cell, boundary['sourceColumns'])
        ox, oy = boundary['offset']
        packed = frames[boundary['destinationFrame']]
        for y in range(cell):
            for x in range(cell):
                expected = original[(y - oy) * source_cell + x - ox] if ox <= x < ox + source_cell and oy <= y < oy + source_cell else KEY
                require(packed[y * cell + x] == expected,
                        f'{name} #{boundary["destinationFrame"]}: endpoint pixels differ at {x},{y}')
    if kind == 'IdleCycle':
        require(frames[11] == frames[0], f'{name}: loop seam differs')
        require(len(set(frames)) >= 4 and frames[9] != frames[0], f'{name}: authored idle motion/blink missing')
        require(layout['animation']['seconds'] == 2.4, f'{name}: idle timing differs')
    else:
        length = 6 if kind == 'WalkStop' else 4
        for start in range(0, 12, length):
            require(any(frames[index] != frames[start] and frames[index] != frames[start + length - 1]
                        for index in range(start + 1, start + length - 1)), f'{name}: branch {start} has no authored in-between')
    print(f'PASS {name}: 12 poses, fixed scale/root, unchanged source, exact boundary pixels')


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument('--allow-partial', action='store_true')
    args = parser.parse_args()
    count = 0
    for character in CLASSES:
        for kind in KINDS:
            if args.allow_partial and not (RESOURCES / f'{character}_{kind}.bmp').exists():
                continue
            verify(character, kind)
            count += 1
    print(f'PASS: {count}/9 idle/settling sheets checked. No native-game execution.')


if __name__ == '__main__':
    main()
