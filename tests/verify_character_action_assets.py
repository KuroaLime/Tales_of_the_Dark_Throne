#!/usr/bin/env python3
"""Verify the approved combat/portrait resource pack without running the game."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[1]
RESOURCES = ROOT / 'Tower_Triumph' / 'Tower_Triumph' / 'Resource'
CLASSES = ('Warrior', 'Archer', 'Mage')
STATES = ('Attack', 'Defend', 'Skill', 'Hit', 'Dead')
KEY = (255, 0, 255)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def read_bitmap(path: Path):
    data = path.read_bytes()
    require(data[:2] == b'BM', f'{path.name}: BMP signature missing')
    offset = struct.unpack_from('<I', data, 10)[0]
    width, signed_height, planes, bits, compression = struct.unpack_from('<iiHHI', data, 18)
    require(width > 0 and signed_height != 0 and planes == 1 and bits == 24 and compression == 0,
            f'{path.name}: expected uncompressed 24-bit BMP')
    height = abs(signed_height)
    stride = (width * 3 + 3) & ~3
    require(offset + stride * height <= len(data), f'{path.name}: truncated pixel data')

    def pixel(x: int, y: int):
        row = height - 1 - y if signed_height > 0 else y
        index = offset + row * stride + x * 3
        blue, green, red = data[index:index + 3]
        return red, green, blue

    return width, height, pixel


def verify_file_hashes(layout: dict, name: str) -> None:
    for field, expected_field in (('source', 'sourceSha256'), ('bitmap', 'bitmapSha256')):
        path = RESOURCES / layout[field]
        require(hashlib.sha256(path.read_bytes()).hexdigest() == layout[expected_field],
                f'{name}: {field} content differs from its import record')


def main() -> None:
    count = 0
    for character in CLASSES:
        for state in STATES:
            name = f'{character}_{state}Action'
            layout = json.loads((RESOURCES / f'{name}.layout.json').read_text(encoding='utf-8-sig'))
            verify_file_hashes(layout, name)
            require(layout['frameCount'] == 12 and layout['columns'] == 4 and layout['rows'] == 3,
                    f'{name}: action must contain twelve poses')
            require(layout['cellSize'] == 128 and layout['nominalFrameHeight'] == 96 and
                    layout['root'] == [64, 112], f'{name}: action rendering contract differs')
            alignment = json.loads((RESOURCES / f'{name}.alignment.json').read_text(encoding='utf-8-sig'))
            require(alignment['scale'] == layout['uniformScale'] and len(alignment['frames']) == 12,
                    f'{name}: source alignment and imported scale differ')
            width, height, pixel = read_bitmap(RESOURCES / f'{name}.bmp')
            require((width, height) == (512, 384), f'{name}: unexpected bitmap dimensions')
            require(len(layout['frames']) == 12, f'{name}: missing pose records')
            for index in range(12):
                frame = layout['frames'][index]
                source_root = alignment['frames'][index]
                require(frame['index'] == index and frame['outputRoot'] == [64, 112] and
                        frame['sourceRoot'] == [source_root['rootX'], source_root['groundY']],
                        f'{name} pose {index}: source registration or fixed output root differs')
                cell_x, cell_y = index % 4 * 128, index // 4 * 128
                for edge in range(128):
                    require(all(pixel(cell_x + x, cell_y + y) == KEY for x, y in
                                ((edge, 0), (edge, 127), (0, edge), (127, edge))),
                            f'{name} pose {index}: color leaked into a cell boundary')
                opaque = [(x, y) for y in range(1, 127) for x in range(1, 127)
                          if pixel(cell_x + x, cell_y + y) != KEY]
                require(len(opaque) > 100, f'{name} pose {index}: empty or implausibly small character')
                bounds = [min(p[0] for p in opaque), min(p[1] for p in opaque),
                          max(p[0] for p in opaque) + 1, max(p[1] for p in opaque) + 1]
                require(bounds == layout['frames'][index]['outputBounds'],
                        f'{name} pose {index}: stored bounds differ from actual pixels')
            count += 1

        name = f'{character}_Portrait'
        layout = json.loads((RESOURCES / f'{name}.layout.json').read_text(encoding='utf-8-sig'))
        verify_file_hashes(layout, name)
        approved = ROOT / layout['approvedReference']
        require(hashlib.sha256(approved.read_bytes()).hexdigest() == layout['sourceSha256'],
                f'{name}: portrait does not originate from the approved character')
        width, height, pixel = read_bitmap(RESOURCES / f'{name}.bmp')
        require([width, height] == layout['sheetSize'] and height == 512,
                f'{name}: unexpected portrait dimensions')
        require(all(pixel(x, y) == KEY for x in range(width) for y in (0, height - 1)),
                f'{name}: portrait touches top/bottom edge')
        require(all(pixel(x, y) == KEY for y in range(height) for x in (0, width - 1)),
                f'{name}: portrait touches left/right edge')
    print(f'PASS: {count} combat sheets / 180 poses and three exact-source portraits; hashes, sizes, roots, bounds and keyed gutters verified.')
    print('NOTE: asset-format checks do not execute game code or judge animation anatomy.')


if __name__ == '__main__':
    main()
