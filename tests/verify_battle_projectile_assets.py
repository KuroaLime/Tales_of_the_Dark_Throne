#!/usr/bin/env python3
"""Independently verify imported projectile art without launching the game."""
import hashlib
import json
import math
from pathlib import Path
import struct
import zlib

from verify_character_action_assets import read_bitmap, require

ROOT = Path(__file__).resolve().parents[1]
RESOURCE = ROOT / 'Tower_Triumph' / 'Tower_Triumph' / 'Resource'
KEY = (255, 0, 255)
ELEMENTS = ('Fire', 'Water', 'Grass', 'Light', 'Dark')
PNG_CACHE = {}


def read_png(path):
    """Read RGB/RGBA8 noninterlaced PNGs with the standard library only."""
    if path in PNG_CACHE:
        return PNG_CACHE[path]
    data = path.read_bytes()
    require(data[:8] == b'\x89PNG\r\n\x1a\n', f'{path.name}: PNG signature')
    pos, packed = 8, bytearray()
    while pos < len(data):
        length = struct.unpack_from('>I', data, pos)[0]
        kind, chunk = data[pos+4:pos+8], data[pos+8:pos+8+length]
        if kind == b'IHDR':
            width, height, bits, mode, compression, filtering, interlace = struct.unpack('>IIBBBBB', chunk)
            require(bits == 8 and mode in (2, 6) and not interlace, f'{path.name}: unsupported PNG format')
        elif kind == b'IDAT':
            packed.extend(chunk)
        pos += length+12
    channels = 4 if mode == 6 else 3
    stride = width*channels
    raw = zlib.decompress(packed)
    result, previous = bytearray(), bytearray(stride)
    for y in range(height):
        offset = y*(stride+1)
        method, row = raw[offset], bytearray(raw[offset+1:offset+stride+1])
        for i in range(stride):
            left = row[i-channels] if i >= channels else 0
            up = previous[i]
            corner = previous[i-channels] if i >= channels else 0
            if method == 1: prediction = left
            elif method == 2: prediction = up
            elif method == 3: prediction = (left+up)//2
            elif method == 4:
                p = left+up-corner
                a, b, c = abs(p-left), abs(p-up), abs(p-corner)
                prediction = left if a <= b and a <= c else up if b <= c else corner
            else:
                require(method == 0, f'{path.name}: invalid PNG filter')
                prediction = 0
            row[i] = (row[i]+prediction) & 255
        result.extend(row)
        previous = row
    def pixel(x, y):
        index = (y*width+x)*channels
        rgb = tuple(result[index:index+3])
        return rgb + ((result[index+3] if channels == 4 else 255),)
    PNG_CACHE[path] = width, height, pixel
    return PNG_CACHE[path]


def sampled_source(metadata):
    width, height, pixel = read_png(RESOURCE / metadata['source'])
    rows = metadata.get('sourceRows', 2)
    def normalized(x, y):
        r, g, b, alpha = pixel(math.floor((x+.5)*width/256), math.floor((y+.5)*height/(rows*64)))
        return (r, g, b) if alpha >= 128 else KEY
    return normalized


def main():
    cells = 0
    for name, element in [(name, element) for element in ('',)+ELEMENTS for name in ('Archer', 'Mage')]:
        suffix = '_'+element if element else ''
        metadata = json.loads((RESOURCE / f'{name}_Projectile{suffix}.layout.json').read_text(encoding='utf-8-sig'))
        for field in ('source', 'bitmap'):
            require(hashlib.sha256((RESOURCE / metadata[field]).read_bytes()).hexdigest() == metadata[field+'Sha256'], f'{name}: {field} hash')
        source_size = struct.unpack_from('>II', (RESOURCE / metadata['source']).read_bytes(), 16)
        source_rows = metadata.get('sourceRows', 2)
        require(list(source_size) == metadata['sourceSize'] and source_size[0]*source_rows == 4*source_size[1], f'{name}: source grid')
        require(metadata['columns'] == 16 and metadata['rows'] == 4 and metadata['cellSize'] == 64 and
                metadata['rotationCenter'] == [32, 32] and metadata['recommendedDrawSize'] == 32 and
                metadata['animationFrameSeconds'] == .06, f'{name}: runtime contract')
        width, height, pixel = read_bitmap(RESOURCE / metadata['bitmap'])
        require((width, height) == (1024, 256), f'{name}: atlas dimensions')
        require(len(metadata['frames']) == 64 and len(metadata['directionAnchors']) == 16, f'{name}: records')
        ax, ay = metadata['registeredAnchor']
        require([ax, ay] == ([54, 33.5] if name == 'Archer' else [45, 32]), f'{name}: common anchor changed')
        if element:
            source_pixel = sampled_source(metadata)
            source_row = 0 if name == 'Archer' else 1
            group_scale = metadata.get('registrationGroupScale', 1)
            for frame in range(4):
                sax, say = metadata['sourceAnchors'][frame]
                for y in range(64):
                    for x in range(64):
                        sx = math.floor((x+.5-ax)/group_scale+sax+1e-9)
                        sy = math.floor((y+.5-ay)/group_scale+say+1e-9)
                        expected = source_pixel(frame*64+sx, source_row*64+sy) if 0 <= sx < 64 and 0 <= sy < 64 else KEY
                        require(pixel(x, frame*64+y) == expected, f'{name}/{element}: unrecorded source alteration')
        for direction in range(16):
            angle = direction * math.pi / 8
            cos, sin = round(math.cos(angle), 12), round(math.sin(angle), 12)
            anchor = [32 + cos*(ax-32) - sin*(ay-32), 32 + sin*(ax-32) + cos*(ay-32)]
            require(all(abs(a-b) < .000001 for a, b in zip(anchor, metadata['directionAnchors'][direction])), f'{name}: anchor {direction}')
            for frame in range(4):
                points = []
                for y in range(64):
                    for x in range(64):
                        color = pixel(direction*64+x, frame*64+y)
                        dx, dy = x+.5-32, y+.5-32
                        sx = math.floor(cos*dx+sin*dy+32+1e-9)
                        sy = math.floor(-sin*dx+cos*dy+32+1e-9)
                        expected = pixel(sx, frame*64+sy) if 0 <= sx < 64 and 0 <= sy < 64 else KEY
                        require(color == expected, f'{name}: non-mechanical rotation at {direction}/{frame}/{x}/{y}')
                        if color != KEY:
                            points.append((x, y))
                require(points, f'{name}: empty cell {direction}/{frame}')
                bounds = [min(x for x, y in points), min(y for x, y in points), max(x for x, y in points)+1, max(y for x, y in points)+1]
                record = metadata['frames'][direction*4+frame]
                require(record['frame'] == frame and record['direction'] == direction and
                        record['bounds'] == bounds and record['opaquePixels'] == len(points), f'{name}: bounds record')
                require(bounds[0] > 0 and bounds[1] > 0 and bounds[2] < 64 and bounds[3] < 64, f'{name}: clipped edge')
                cells += 1
    impacts = 0
    for element in ELEMENTS:
        metadata = json.loads((RESOURCE / f'Element_Impact_{element}.layout.json').read_text(encoding='utf-8-sig'))
        for field in ('source', 'bitmap'):
            require(hashlib.sha256((RESOURCE / metadata[field]).read_bytes()).hexdigest() == metadata[field+'Sha256'], f'{element}: {field} hash')
        width, height, pixel = read_bitmap(RESOURCE / metadata['bitmap'])
        require((width, height) == (256, 128) and metadata['center'] == [32, 32] and
                metadata['frameCount'] == 8 and metadata['animationFrameSeconds'] == .025 and
                metadata['recommendedDrawSize'] == 48, f'{element}: impact contract')
        source_pixel = sampled_source(metadata)
        shift_x, shift_y = metadata.get('sourceSharedTranslation', [0, 0])
        for frame in range(8):
            points = []
            for y in range(64):
                for x in range(64):
                    sx, sy = x-shift_x, y-shift_y
                    expected = source_pixel(frame%4*64+sx, 128+frame//4*64+sy) if 0 <= sx < 64 and 0 <= sy < 64 else KEY
                    color = pixel(frame%4*64+x, frame//4*64+y)
                    require(color == expected, f'{element}: unrecorded contact alteration')
                    if color != KEY: points.append((x, y))
            require(points, f'{element}: empty impact {frame}')
            bounds = [min(x for x, y in points), min(y for x, y in points), max(x for x, y in points)+1, max(y for x, y in points)+1]
            require(metadata['frames'][frame]['bounds'] == bounds and metadata['frames'][frame]['opaquePixels'] == len(points), f'{element}: contact metadata')
            require(bounds[0] > 0 and bounds[1] > 0 and bounds[2] < 64 and bounds[3] < 64, f'{element}: clipped contact')
            impacts += 1
    print(f'PASS: {cells} projectile cells and {impacts} contact poses; 5 source sets, exact source registration/sampling, all16 rotations, common anchors and keyed gutters.')


if __name__ == '__main__':
    main()
