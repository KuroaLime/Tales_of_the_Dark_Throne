"""Independent source-to-runtime pixel audit; no native game/test executable."""
import hashlib
import json
import math
from pathlib import Path

from verify_battle_projectile_assets import read_png
from verify_character_action_assets import read_bitmap, require

ROOT = Path(__file__).resolve().parents[1]
RESOURCE = ROOT / 'Tower_Triumph/Tower_Triumph/Resource'
KEY = (255, 0, 255)


def main():
    report = []
    for effect, ground, core, split in [('Meteor', [64, 114], [86, 76], 108), ('Sanctuary', [64, 104], None, 96)]:
        name = f'Skill_Mage_{effect}_Ultimate'
        metadata = json.loads((RESOURCE / (name + '.layout.json')).read_text(encoding='utf-8-sig'))
        for key in ('source', 'bitmap', 'preview'):
            actual = hashlib.sha256((RESOURCE / metadata[key]).read_bytes()).hexdigest()
            require(actual == metadata[key+'Sha256'], f'{effect}: {key} hash')
        require(metadata['groundAnchor'] == ground and metadata['flightCoreAnchor'] == core and
                metadata['groundBandStartY'] == split, f'{effect}: measured anchor contract')
        require(metadata['frameCount'] == 12 and metadata['columns'] == 4 and metadata['rows'] == 3 and
                metadata['cellSize'] == 128, f'{effect}: 12-frame layout')
        sw, sh, source = read_png(RESOURCE / metadata['source'])
        bw, bh, bitmap = read_bitmap(RESOURCE / metadata['bitmap'])
        pw, ph, preview = read_png(RESOURCE / metadata['preview'])
        require((bw, bh) == (pw, ph) == (512, 384) and sw*3 == sh*4, f'{effect}: sheet sizes')
        require(abs(metadata['uniformScale'] - 128/(sw/4)) < 1e-12, f'{effect}: single common scale')
        scale = metadata['uniformScale']
        frame_reports = []
        for frame in range(12):
            registration = metadata['registration']['frames'][frame]
            ax, ay = registration['sourceAnchor']
            tx, ty = registration['targetAnchor']
            require(abs(ax*scale-tx) < 1e-10 and abs(ay*scale-ty) < 1e-10,
                    f'{effect}/{frame}: no unrecorded recentering')
            x0, y0 = (frame % 4)*128, (frame//4)*128
            left, top = (frame % 4)*sw//4, (frame//4)*sh//3
            occupied, red = [], 0
            for y in range(128):
                for x in range(128):
                    sx = math.floor((x+.5-tx)/scale+ax+1e-9)
                    sy = math.floor((y+.5-ty)/scale+ay+1e-9)
                    r, g, b, alpha = source(left+sx, top+sy)
                    expected = (r,g,b) if alpha >= 128 else KEY
                    actual = bitmap(x0+x, y0+y)
                    rgba = preview(x0+x, y0+y)
                    require(actual == expected, f'{effect}/{frame}: source RGB/sample mismatch')
                    require(rgba[3] in (0,255) and rgba[3] == (255 if alpha >= 128 else 0),
                            f'{effect}/{frame}: alpha threshold mismatch')
                    if alpha >= 128:
                        require(rgba[:3] == expected, f'{effect}/{frame}: preview RGB changed')
                        occupied.append((x,y))
                        red += int(r > 180 and g < 100 and b < 100)
            require(occupied, f'{effect}/{frame}: empty frame')
            bounds = [min(x for x,y in occupied), min(y for x,y in occupied),
                      max(x for x,y in occupied)+1, max(y for x,y in occupied)+1]
            require(bounds == metadata['frames'][frame]['RuntimeBounds'], f'{effect}/{frame}: bounds metadata')
            gutter = min(bounds[0], bounds[1], 128-bounds[2], 128-bounds[3])
            require(gutter >= 1 and metadata['frames'][frame]['SourceEdgePixels'] == 0,
                    f'{effect}/{frame}: foreground touches cell edge')
            frame_reports.append({'frame':frame,'bounds':bounds,'opaquePixels':len(occupied),
                                  'transparentGutter':gutter,'redPixels':red})
        report.append({'effect':effect,'sourceSha256':metadata['sourceSha256'],'frames':frame_reports,
                       'runtimeRedPixels':sum(frame['redPixels'] for frame in frame_reports)})
    destination = ROOT / 'tests/bin/ultimate-import/asset-verification.json'
    destination.write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print('PASS: 24 nonempty frames; original PNG-to-BMP/PNG pixels, uniform scale, anchors, alpha and gutters match.')
    print('Sanctuary opaque red runtime pixels:', report[1]['runtimeRedPixels'])


if __name__ == '__main__':
    main()
