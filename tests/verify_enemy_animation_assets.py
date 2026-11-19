"""Check approved enemy atlases against original pixels and the live map roster.

This checks file integrity, registration, keying and motion-frame diversity. It
does not execute the game or establish the quality/anatomy of drawn animation.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import re

from verify_battle_projectile_assets import read_png
from verify_character_action_assets import read_bitmap, require

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'Tower_Triumph/Tower_Triumph'
APPROVED = SOURCE / 'Resource/Monster/Approved'
KEY = (255, 0, 255)
STATES = ('Idle', 'Walk', 'Attack', 'Hit', 'Dead', 'WalkStop', 'IdleReturn')


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def roster() -> list[str]:
    text = (SOURCE / 'BattleMapData.cpp').read_text(encoding='utf-8-sig')
    names = sorted(set(re.findall(r'L"(S[1245]_[A-Za-z]+)\.bmp"', text)))
    require(len(names) == 14, 'Expected the fourteen existing battlefield enemy identities')
    return names


def bounds(points: list[tuple[int, int]]) -> list[int]:
    require(bool(points), 'Empty animation frame')
    return [min(x for x, _ in points), min(y for _, y in points),
            max(x for x, _ in points) + 1, max(y for _, y in points) + 1]


def frame_state(index: int) -> str:
    return STATES[index // 8] if index < 40 else STATES[5 + (index - 40) // 4]


def verify(stem: str) -> dict:
    name = stem + '_Animated'
    layout = json.loads((APPROVED / (name + '.layout.json')).read_text(encoding='utf-8-sig'))
    for field in ('source', 'png', 'bitmap'):
        require(digest(APPROVED / layout[field]) == layout[field + 'Sha256'], name + ': ' + field + ' hash')
    require((APPROVED / (name + '.prompt.txt')).stat().st_size > 0, name + ': generation prompt missing')
    generation = layout['generation']
    require(generation['tool'] == 'image_gen' and generation['transparentBackground'] is True,
            name + ': built-in transparent generation provenance missing')
    require(digest(APPROVED / generation['prompt']) == generation['promptSha256'], name + ': prompt hash differs')
    require((ROOT / generation['styleReference']).is_file(), name + ': approved style reference missing')
    require(digest(Path(generation['returnedImagePath'])) == layout['sourceSha256'],
            name + ': imported source differs from the recorded generated output')
    require(layout['columns'] == 8 and layout['rows'] == 6 and layout['frameCount'] == 48 and
            layout['cellSize'] == 128 and layout['sheetSize'] == [1024, 768] and
            layout['root'] == [64, 112] and layout['nominalFrameHeight'] == 96 and
            layout['sourceFacesRight'] is True and layout['alphaThreshold'] == 128,
            name + ': runtime layout differs')
    require(len(layout['frames']) == 48, name + ': missing frame metadata')
    require(layout['impactFrame'] == (5 if stem == 'S2_BAngel' else 4),
            name + ': contact metadata differs from the reviewed production profile')
    sw, sh, source = read_png(APPROVED / layout['source'])
    pw, ph, png = read_png(APPROVED / layout['png'])
    bw, bh, bitmap = read_bitmap(APPROVED / layout['bitmap'])
    require([sw, sh] == layout['sourceSheetSize'] and (pw, ph) == (bw, bh) == (1024, 768),
            name + ': decoded image dimensions differ')
    scale = layout['uniformScale']
    require(0 < scale <= 8, name + ': unreasonable common scale')
    records = []
    frame_hashes = []
    for index, frame in enumerate(layout['frames']):
        require(frame['index'] == index and frame['state'] == frame_state(index) and
                frame['outputRoot'] == [64, 112], name + ': frame identity/root differs')
        left, top, right, bottom = frame['sourceCell']
        require(0 <= left < right <= sw and 0 <= top < bottom <= sh, name + ': source cell outside image')
        ax, ay = frame['sourceRoot']
        cell_w, cell_h = right - left, bottom - top
        require(0 <= ax <= cell_w and 0 <= ay <= cell_h, name + ': source root is not cell-local')
        x0, y0 = index % 8 * 128, index // 8 * 128
        source_points = [(x, y) for y in range(cell_h) for x in range(cell_w)
                         if source(left + x, top + y)[3] >= 128]
        source_bounds = bounds(source_points)
        require(source_bounds == frame['sourceBounds'], name + ': original frame bounds differ')
        require(source_bounds[0] > 0 and source_bounds[1] > 0 and
                source_bounds[2] < cell_w and source_bounds[3] < cell_h,
                f'{name} #{index}: original drawing touches a source cell edge')
        require(64 + (source_bounds[0] - ax) * scale >= 0 and
                112 + (source_bounds[1] - ay) * scale >= 0 and
                64 + (source_bounds[2] - ax) * scale <= 128 and
                112 + (source_bounds[3] - ay) * scale <= 128,
                f'{name} #{index}: registered source would be clipped')
        points = []
        packed = bytearray()
        for y in range(128):
            for x in range(128):
                sx = math.floor(ax + (x + .5 - 64) / scale)
                sy = math.floor(ay + (y + .5 - 112) / scale)
                original = source(left + sx, top + sy) if 0 <= sx < cell_w and 0 <= sy < cell_h else (0, 0, 0, 0)
                actual = png(x0 + x, y0 + y)
                require(actual == original, f'{name} #{index}: source RGBA changed at {x},{y}')
                expected = original[:3] if original[3] >= 128 else KEY
                require(bitmap(x0 + x, y0 + y) == expected,
                        f'{name} #{index}: keyed BMP differs at {x},{y}')
                packed.extend(actual)
                if original[3] >= 128:
                    require(original[:3] != KEY, name + ': visible art collides with the transparent color key')
                    points.append((x, y))
        output_bounds = bounds(points)
        require(len(points) > 20 and output_bounds[0] > 0 and output_bounds[1] > 0 and
                output_bounds[2] < 128 and output_bounds[3] < 128,
                f'{name} #{index}: empty pose or missing transparent cell gutter')
        require(output_bounds == frame['outputBounds'] and len(points) == frame['opaquePixels'],
                f'{name} #{index}: output bounds/occupancy metadata differs')
        frame_hashes.append(hashlib.sha256(packed).hexdigest())
        records.append({'index': index, 'state': frame_state(index), 'bounds': output_bounds,
                        'opaquePixels': len(points)})
    for row in range(5):
        require(len(set(frame_hashes[row * 8:(row + 1) * 8])) >= 4,
                f'{name}: {STATES[row]} lacks distinct authored poses')
    for start in (40, 44):
        require(len(set(frame_hashes[start:start + 4])) >= 3,
                f'{name}: {frame_state(start)} lacks settling poses')
    portrait = layout['portrait']
    require(portrait['sourceFrame'] == 0, name + ': portrait must derive from approved idle')
    for field in ('png', 'bitmap'):
        require(digest(APPROVED / portrait[field]) == portrait[field + 'Sha256'], name + ': portrait hash')
    iw, ih, icon = read_png(APPROVED / portrait['png'])
    ibw, ibh, icon_bmp = read_bitmap(APPROVED / portrait['bitmap'])
    require((iw, ih) == (ibw, ibh) == (128, 128), name + ': portrait dimensions')
    for y in range(128):
        for x in range(128):
            require(icon(x, y) == png(x, y) and icon_bmp(x, y) == bitmap(x, y),
                    name + ': portrait differs from the normalized idle frame')
    return {'enemy': stem, 'frameCount': 48, 'frames': records,
            'sourceSha256': layout['sourceSha256'], 'bitmapSha256': layout['bitmapSha256'],
            'portraitSha256': portrait['bitmapSha256'], 'generation': generation,
            'impactFrame': layout['impactFrame']}


def verify_manifest() -> dict:
    path = APPROVED / 'Enemy_Animations.manifest.json'
    manifest = json.loads(path.read_text(encoding='utf-8-sig'))
    require(manifest['generationTool'] == 'image_gen' and manifest['speciesCount'] == 14 and
            manifest['frameCount'] == 672 and manifest['portraitCount'] == 14,
            'Enemy manifest counts or generation tool differ')
    reference = manifest['styleReference']
    require(digest(ROOT / reference['file']) == reference['sha256'], 'Approved style reference hash differs')
    require(manifest['sheet'] == {'columns': 8, 'rows': 6, 'cellSize': 128, 'width': 1024, 'height': 768,
                                 'root': [64, 112], 'nominalFrameHeight': 96, 'sourceFacesRight': True},
            'Enemy manifest runtime sheet contract differs')
    require(manifest['states'] == [
        {'name': state, 'first': index * 8 if index < 5 else 40 + (index - 5) * 4,
         'count': 8 if index < 5 else 4} for index, state in enumerate(STATES)],
        'Enemy manifest state frame ranges differ')
    assets = manifest['assets']
    require(len(assets) == 14 and {asset['species'] for asset in assets} == set(roster()),
            'Enemy manifest must cover each live map identity once')
    references = 1
    for asset in assets:
        stem = asset['species']
        require(asset['stage'] == int(stem[1]) and asset['usedInMixedStage3'] is True,
                stem + ': stage/mixed-stage mapping differs')
        require(asset['legacyFile'] == '../' + stem + '.bmp' and
                digest(APPROVED / asset['legacyFile']) == asset['legacySha256'],
                stem + ': original fallback file changed')
        references += 1
        for field in ('source', 'png', 'bitmap', 'layout', 'alignment'):
            require(digest(APPROVED / asset[field]) == asset[field + 'Sha256'], stem + ': manifest ' + field + ' hash')
            references += 1
        layout = json.loads((APPROVED / asset['layout']).read_text(encoding='utf-8-sig'))
        for field in ('source', 'png', 'bitmap', 'sourceSha256', 'pngSha256', 'bitmapSha256',
                      'portrait', 'generation', 'impactFrame', 'uniformScale'):
            require(asset[field] == layout[field], stem + ': manifest/layout ' + field + ' differs')
        require(asset['generation']['styleReference'] == reference['file'], stem + ': style source differs')
        for field in ('png', 'bitmap'):
            require(digest(APPROVED / asset['portrait'][field]) == asset['portrait'][field + 'Sha256'],
                    stem + ': manifest portrait hash differs')
            references += 1
        require(bool(asset['prompts']), stem + ': final prompt missing from manifest')
        for prompt in asset['prompts']:
            require(digest(APPROVED / prompt['file']) == prompt['sha256'], stem + ': manifest prompt hash differs')
            references += 1
    return {'passed': True, 'file': str(path), 'sha256': digest(path), 'checkedReferences': references}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--partial', action='store_true', help='Check only imported enemies during asset production')
    parser.add_argument('--output', type=Path, default=ROOT / 'tests/bin/enemy-style-audit/asset-verification.json')
    args = parser.parse_args()
    reports = []
    for stem in roster():
        if args.partial and not (APPROVED / (stem + '_Animated.layout.json')).exists():
            continue
        reports.append(verify(stem))
        print('PASS:', stem, '48 registered animation frames and exact idle portrait')
    require(args.partial or len(reports) == 14, 'All live battlefield enemy identities must be covered')
    result = {'passed': True, 'partial': args.partial, 'enemyCount': len(reports),
              'frameCount': 48 * len(reports), 'assets': reports}
    if not args.partial:
        result['manifest'] = verify_manifest()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print(f'PASS: {len(reports)}/14 enemy atlases; no native game or animation-anatomy claim.')


if __name__ == '__main__':
    main()
