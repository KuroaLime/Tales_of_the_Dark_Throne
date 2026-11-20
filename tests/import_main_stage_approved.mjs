// Reproduce the approved world map and 40px writing-quill game bitmaps.
// node tests/import_main_stage_approved.mjs         imports both assets
// node tests/import_main_stage_approved.mjs --check verifies without writing
import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {createHash} from 'node:crypto';
import {createRequire} from 'node:module';
import {fileURLToPath} from 'node:url';
const require = createRequire(import.meta.url);
let sharp;
try { sharp = require('sharp'); }
catch { sharp = require(path.resolve(path.dirname(process.execPath), '../node_modules/sharp')); }
const repo = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const archive = path.join(repo, 'Source/Concepts/MainStage/approved');
const manifestPath = path.join(archive, 'approved-assets.json');
const manifest = JSON.parse(fs.readFileSync(manifestPath, 'utf8').replace(/^\uFEFF/, ''));
const sha256 = bytes => createHash('sha256').update(bytes).digest('hex');
const checkOnly = process.argv.includes('--check');
const destinations = {
  worldMap: 'Tower_Triumph/Tower_Triumph/Source/SMap/background/Map_Background.bmp',
  cursor: 'Tower_Triumph/Tower_Triumph/Source/MouseCursor/MouseCusor.bmp'
};
for (const record of manifest.archived) {
  const bytes = fs.readFileSync(path.join(archive, record.file));
  assert.equal(bytes.length, record.bytes, record.file + ': archive size mismatch');
  assert.equal(sha256(bytes), record.sha256, record.file + ': approved archive hash mismatch');
}
function makeBitmap(rgba, width, height, premultiplied) {
  const bits = premultiplied ? 32 : 24, pixelBytes = bits / 8;
  const stride = Math.ceil(width * pixelBytes / 4) * 4, byteSize = 54 + stride * height;
  const output = Buffer.alloc(byteSize);
  output.write('BM', 0, 'ascii'); output.writeUInt32LE(byteSize, 2); output.writeUInt32LE(54, 10);
  output.writeUInt32LE(40, 14); output.writeInt32LE(width, 18); output.writeInt32LE(height, 22);
  output.writeUInt16LE(1, 26); output.writeUInt16LE(bits, 28); output.writeUInt32LE(0, 30);
  output.writeUInt32LE(stride * height, 34); output.writeInt32LE(3780, 38); output.writeInt32LE(3780, 42);
  for (let y = 0; y < height; y++) for (let x = 0; x < width; x++) {
    const from = (y * width + x) * 4, to = 54 + (height - y - 1) * stride + x * pixelBytes;
    const alpha = rgba[from + 3];
    if (premultiplied) {
      output[to] = Math.round(rgba[from + 2] * alpha / 255);
      output[to + 1] = Math.round(rgba[from + 1] * alpha / 255);
      output[to + 2] = Math.round(rgba[from] * alpha / 255);
      output[to + 3] = alpha;
    } else {
      assert.equal(alpha, 255, 'Approved map must be fully opaque');
      output[to] = rgba[from + 2]; output[to + 1] = rgba[from + 1]; output[to + 2] = rgba[from];
    }
  }
  return output;
}
function verifyBitmap(bitmap, rgba, width, height, premultiplied) {
  assert.equal(bitmap.toString('ascii', 0, 2), 'BM'); assert.equal(bitmap.readUInt32LE(2), bitmap.length);
  assert.equal(bitmap.readUInt32LE(14), 40); assert.equal(bitmap.readInt32LE(18), width);
  assert.equal(bitmap.readInt32LE(22), height); assert.equal(bitmap.readUInt16LE(26), 1);
  assert.equal(bitmap.readUInt16LE(28), premultiplied ? 32 : 24); assert.equal(bitmap.readUInt32LE(30), 0);
  const pixelBytes = bitmap.readUInt16LE(28) / 8, offset = bitmap.readUInt32LE(10);
  const stride = Math.floor((width * bitmap.readUInt16LE(28) + 31) / 32) * 4;
  let transparent = 0, partialAlpha = 0, opaque = 0;
  for (let y = 0; y < height; y++) for (let x = 0; x < width; x++) {
    const from = offset + (height - 1 - y) * stride + x * pixelBytes, reference = (y * width + x) * 4;
    const alpha = rgba[reference + 3];
    if (alpha === 0) transparent++; else if (alpha === 255) opaque++; else partialAlpha++;
    for (let channel = 0; channel < 3; channel++) {
      const original = rgba[reference + channel];
      // Integer arithmetic independent of the encoder's Math.round expression.
      const expected = premultiplied ? Math.floor((original * alpha + 127) / 255) : original;
      assert.equal(bitmap[from + 2 - channel], expected, `Pixel ${x},${y} channel ${channel}`);
      if (premultiplied) assert(bitmap[from + 2 - channel] <= alpha);
    }
    if (premultiplied) assert.equal(bitmap[from + 3], alpha, 'Alpha byte changed');
  }
  return { checkedPixels: width * height, transparentPixels: transparent, partialAlphaPixels: partialAlpha, opaquePixels: opaque,
    approvedRGBExact: !premultiplied, premultipliedRGBExact: premultiplied, alphaExact: true };
}
const prepared = [], exports = {};
for (const id of ['worldMap', 'cursor']) {
  const config = manifest.assets[id], premultiplied = id === 'cursor';
  assert.equal(config.runtime, destinations[id], 'Unexpected runtime output path');
  const input = path.join(archive, config.png), {data, info} = await sharp(input).ensureAlpha().raw().toBuffer({resolveWithObject: true});
  assert.deepEqual([info.width, info.height], config.size);
  assert.deepEqual(config.size, premultiplied ? [40, 40] : [1672, 941]);
  if (premultiplied) {
    assert.deepEqual(config.hotspot, [2, 37]);
    assert(data[(config.hotspot[1] * info.width + config.hotspot[0]) * 4 + 3] >= 128, 'Click hotspot must lie on the visible nib');
  }
  const bitmap = makeBitmap(data, info.width, info.height, premultiplied);
  const verification = verifyBitmap(bitmap, data, info.width, info.height, premultiplied);
  const destination = path.join(repo, config.runtime);
  if (checkOnly) {
    const actual = fs.readFileSync(destination);
    assert(actual.equals(bitmap), config.runtime + ': runtime bytes differ from approved conversion');
    verifyBitmap(actual, data, info.width, info.height, premultiplied);
  }
  prepared.push({destination, bitmap});
  exports[id] = { file: config.runtime, sha256: sha256(bitmap), bytes: bitmap.length,
    width: info.width, height: info.height, bitsPerPixel: premultiplied ? 32 : 24,
    bitmapHeader: 'BITMAPINFOHEADER (40 bytes), BI_RGB, bottom-up rows',
    alphaMode: premultiplied ? 'Premultiplied BGRA for AlphaBlend(AC_SRC_ALPHA)' : 'Opaque BGR',
    ...(premultiplied ? {hotspot: config.hotspot, sourceConstantAlpha: 255,
      loadRequirement: 'Preserve DIB alpha bytes: direct RT_BITMAP/CreateDIBSection or LR_CREATEDIBSECTION; avoid opaque LoadBitmap conversion.'} : {}), verification };
}
if (!checkOnly) {
  for (const output of prepared) fs.writeFileSync(output.destination, output.bitmap);
  manifest.runtimeExports = exports;
  manifest.reproduce = { script: 'tests/import_main_stage_approved.mjs', scriptSha256: sha256(fs.readFileSync(fileURLToPath(import.meta.url))),
    importCommand: 'node tests/import_main_stage_approved.mjs', checkCommand: 'node tests/import_main_stage_approved.mjs --check' };
  fs.writeFileSync(manifestPath, JSON.stringify(manifest, null, 2) + '\n');
}
console.log(JSON.stringify({status: 'PASS', mode: checkOnly ? 'check-only' : 'import', exports}));
