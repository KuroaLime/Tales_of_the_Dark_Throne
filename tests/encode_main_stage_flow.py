"""Encode actual renderer BGR frames without adding artwork, transitions or labels.

Run with the bundled Python that includes Pillow, not the default Python 3.14.
  python tests/encode_main_stage_flow.py --manifest tests/bin/main-stage-flow/frames.json
"""
from __future__ import annotations

import argparse
import gc
import hashlib
import json
import math
import mmap
from pathlib import Path
from typing import Any

from PIL import Image, ImageChops, ImageStat, __version__ as pillow_version


DEFAULT_OUTPUT = Path("tests/bin/main-stage-flow-preview")


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def rounded_ratio(numerator: int, denominator: int) -> int:
    return (2 * numerator + denominator) // (2 * denominator)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=Path("tests/bin/main-stage-flow/frames.json"))
    parser.add_argument("--output-dir", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--gif-width", type=int, default=800)
    parser.add_argument("--webp-width", type=int, default=800)
    parser.add_argument("--quality", type=int, default=88)
    parser.add_argument("--format", choices=["both", "gif", "webp"], default="both")
    parser.add_argument("--verify-existing", action="store_true", help="Verify existing WebP and finish its report without re-encoding (requires --format webp)")
    args = parser.parse_args()
    if args.verify_existing and args.format != "webp":
        parser.error("--verify-existing requires --format webp")
    manifest_path = args.manifest.resolve()
    source = json.loads(manifest_path.read_text(encoding="utf-8-sig"))
    width, height = int(source["width"]), int(source["height"])
    fps, count = int(source["fps"]), int(source["frameCount"])
    assert source["format"].lower() == "bgr24", "Expected renderer BGR24 input"
    assert width > 0 and height > 0 and fps > 0 and count > 0
    stride = int(source.get("rowStride", source.get("stride", width * 3)))
    assert stride >= width * 3
    frame_bytes = stride * height
    raw_path = Path(source["rawFile"])
    if not raw_path.is_absolute():
        raw_path = manifest_path.parent / raw_path
    assert raw_path.stat().st_size == count * frame_bytes, "Incomplete raw capture"
    duration_ms = rounded_ratio(count * 1000, fps)
    assert abs(float(source["durationSeconds"]) * 1000 - duration_ms) <= 1
    raw_hash = sha256_file(raw_path)
    expected_hash = source.get("rawSha256", source.get("sha256"))
    if expected_hash:
        assert raw_hash == expected_hash, "Renderer source hash mismatch"
    args.output_dir.mkdir(parents=True, exist_ok=True)
    report: dict[str, Any] = {
        "sourceManifest": str(manifest_path), "sourceManifestSha256": sha256_file(manifest_path),
        "rawFile": str(raw_path), "rawSha256": raw_hash, "rawBytes": raw_path.stat().st_size,
        "width": width, "height": height, "sourceFPS": fps, "sourceFrameCount": count,
        "durationMilliseconds": duration_ms, "pillowVersion": pillow_version,
        "scope": source.get("encodingScope", "Production renderer capture reproduction; not a live game-window recording. Only BGR-to-RGB decode, uniform downscale, palette quantization and animation encoding. No invented fades, text, effects or retouching."),
        "captureMethod": source.get("captureMethod", "native-renderer-replay"),
        "timeline": source.get("timeline"), "outputs": {}, "stills": [],
    }
    stream = raw_path.open("rb")
    raw = mmap.mmap(stream.fileno(), 0, access=mmap.ACCESS_READ)

    def frame(index: int, target_width: int = width) -> Image.Image:
        assert 0 <= index < count
        start = index * frame_bytes
        image = Image.frombytes("RGB", (width, height), raw[start:start + frame_bytes], "raw", "BGR", stride, 1)
        target_width = min(width, target_width)
        assert target_width > 0
        if target_width != width:
            target_height = max(1, rounded_ratio(height * target_width, width))
            image = image.resize((target_width, target_height), Image.Resampling.LANCZOS)
        return image

    def metadata_label(index: int) -> str:
        keys = source.get("keyframes", [])
        if not keys:
            return "frame " + str(index)
        key = min(keys, key=lambda record: abs(int(record["index"]) - index))
        return str(key.get("label", "frame " + str(index)))

    # Source RGB proofs, including the first frame. These are never composited into the animation.
    stills_dir = args.output_dir / "main-stage-flow-stills"
    stills_dir.mkdir(exist_ok=True)
    still_indexes = source.get("stillFrameIndices", [0] + [
        min(count - 1, rounded_ratio(int(seconds * 1000) * fps, 1000)) for seconds in [4.5, 7.3, 10.7]])
    still_indexes = [int(index) for index in still_indexes]
    assert still_indexes and all(0 <= index < count for index in still_indexes)
    for number, index in enumerate(dict.fromkeys(still_indexes), 1):
        output = stills_dir / f"{number:02d}-frame-{index:04d}.png"
        image = frame(index)
        image.save(output, format="PNG")
        with Image.open(output) as reopened:
            assert ImageChops.difference(image, reopened.convert("RGB")).getbbox() is None
        image.close()
        report["stills"].append({"file": str(output), "sourceIndex": index, "sourceTimeSeconds": index / fps,
                                  "nearestCaptureLabel": metadata_label(index), "sha256": sha256_file(output),
                                  "exactSourceRGB": True})

    key_indices = sorted(set(still_indexes + [0, count - 1] + [int(k["index"]) for k in source.get("keyframes", [])]))
    key_indices = [i for i in key_indices if 0 <= i < count]

    def validate(output: Path, requested_indices: list[int], durations: list[int], target_width: int) -> dict[str, Any]:
        starts, time_ms = [], 0
        for d in durations:
            starts.append(time_ms)
            time_ms += d
        assert time_ms == duration_ms
        # Keyframes are checked against the source frame used by the encoder at that time.
        samples = []
        for source_index in key_indices:
            sample_time = min(duration_ms - 1, rounded_ratio(source_index * 1000, fps))
            slot = next((i for i in range(len(starts) - 1, -1, -1) if starts[i] <= sample_time), 0)
            samples.append({"sourceKeyIndex": source_index, "timeMs": sample_time,
                            "encodedInputIndex": requested_indices[slot]})
        total = 0
        found: dict[int, Image.Image] = {}
        decoded_durations = []
        with Image.open(output) as animation:
            decoded_count = animation.n_frames
            dimensions = animation.size
            assert dimensions[0] == min(width, target_width)
            assert animation.info.get("loop") == 0, "Animation must loop"
            for i in range(decoded_count):
                animation.seek(i)
                rgb = animation.convert("RGB")  # WebP populates duration only after decoding.
                delay = int(round(animation.info.get("duration", 0)))
                assert delay > 0, "Animation frame has no duration"
                for j, sample in enumerate(samples):
                    if total <= sample["timeMs"] < total + delay:
                        found[j] = rgb.copy()
                decoded_durations.append(delay)
                total += delay
                rgb.close()
        assert total == duration_ms, f"Playback duration changed: {total} vs {duration_ms}"
        assert len(found) == len(samples), "A keyframe was not decoded"
        for j, sample in enumerate(samples):
            expected = frame(sample["encodedInputIndex"], target_width)
            difference = ImageChops.difference(expected, found[j])
            mean_absolute = sum(ImageStat.Stat(difference).mean) / 3
            mse = sum(v * v for v in ImageStat.Stat(difference).rms) / 3
            sample["meanAbsoluteRGBError"] = round(mean_absolute, 4)
            sample["psnrDb"] = round(10 * math.log10(255 * 255 / mse), 3) if mse else None
            sample["expectedSourceRGBSha256"] = hashlib.sha256(expected.tobytes()).hexdigest()
            sample["decodedRGBSha256"] = hashlib.sha256(found[j].tobytes()).hexdigest()
            # Lossy encoders need an image error bound, not a misleading exact-byte claim.
            assert mean_absolute < 16, f"Unexpected color/frame error at {sample['timeMs']}ms: {mean_absolute}"
            expected.close()
            found[j].close()
        return {"file": str(output), "bytes": output.stat().st_size, "sha256": sha256_file(output),
                "size": list(dimensions), "loop": 0, "inputFrames": len(requested_indices),
                "decodedFrames": decoded_count, "decodedDurationMs": total, "decodedDurationsMs": decoded_durations,
                "note": "Identical consecutive frames may be coalesced by the encoder; their full hold duration is preserved.",
                "keyframeComparisons": samples, "maximumKeyframeMAE": max(x["meanAbsoluteRGBError"] for x in samples)}

    try:
        if args.format in ("both", "gif"):
            gif_fps = 20
            gif_count = rounded_ratio(duration_ms * gif_fps, 1000)
            indices = [min(count - 1, rounded_ratio(i * fps, gif_fps)) for i in range(gif_count)]
            gif_width = min(width, args.gif_width)
            gif_height = max(1, rounded_ratio(height * gif_width, width))
            # One shared palette avoids unrelated color changes between stationary UI frames.
            palette_indices = sorted(set(key_indices + [rounded_ratio(i * (count - 1), 15) for i in range(16)]))
            tile_width, tile_height = 160, max(1, rounded_ratio(height * 160, width))
            colors = Image.new("RGB", (tile_width * 4, tile_height * math.ceil(len(palette_indices) / 4)))
            for i, index in enumerate(palette_indices):
                sample = frame(index).resize((tile_width, tile_height), Image.Resampling.LANCZOS)
                colors.paste(sample, ((i % 4) * tile_width, (i // 4) * tile_height))
                sample.close()
            palette = colors.quantize(colors=256, method=Image.Quantize.MEDIANCUT)
            colors.close()
            frames = []
            for index in indices:
                rgb = frame(index, gif_width)
                frames.append(rgb.quantize(palette=palette, dither=Image.Dither.FLOYDSTEINBERG))
                rgb.close()
            durations = [50] * gif_count
            output = args.output_dir / "main-stage-flow-preview.gif"
            frames[0].save(output, format="GIF", save_all=True, append_images=frames[1:],
                           duration=durations, loop=0, optimize=True, disposal=1)
            for image in frames:
                image.close()
            palette.close()
            del frames
            gc.collect()
            report["outputs"]["gif"] = validate(output, indices, durations, gif_width)
            report["outputs"]["gif"].update({"fps": gif_fps, "palette": "Shared 256-color palette, Floyd-Steinberg dithering", "targetBytes": 20_000_000})
            assert output.stat().st_size < 20_000_000, "GIF exceeds requested size budget; rerun --gif-width 768"
            print(json.dumps({"format": "gif", "status": "PASS", "bytes": output.stat().st_size}), flush=True)

        if args.format in ("both", "webp"):
            target_width = min(width, args.webp_width)
            indices = list(range(count))
            durations = [rounded_ratio((i + 1) * 1000, fps) - rounded_ratio(i * 1000, fps) for i in indices]
            output = args.output_dir / "main-stage-flow-preview.webp"
            if not args.verify_existing:
                frames = [frame(i, target_width) for i in indices]
                frames[0].save(output, format="WEBP", save_all=True, append_images=frames[1:], duration=durations,
                               loop=0, quality=args.quality, lossless=False, method=4, allow_mixed=True, minimize_size=True)
                for image in frames:
                    image.close()
                del frames
                gc.collect()
            report["outputs"]["webp"] = validate(output, indices, durations, target_width)
            target_bytes = max(15_000_000, rounded_ratio(15_000_000 * duration_ms, 12000))
            report["outputs"]["webp"].update({"fps": fps, "configuredQuality": args.quality, "minimizeSize": None if args.verify_existing else True,
                                               "verificationOnly": args.verify_existing, "targetBytes": target_bytes,
                                               "sizeBudgetPolicy": "15 MB per 12 seconds, with a minimum 15 MB budget"})
            assert output.stat().st_size < target_bytes, "WebP exceeds duration-adjusted size budget"
            print(json.dumps({"format": "webp", "status": "PASS", "bytes": output.stat().st_size}), flush=True)
    finally:
        raw.close()
        stream.close()
    assert sha256_file(raw_path) == raw_hash, "Raw renderer frames changed during encoding"
    report["passed"] = True
    report["rawSourceUnchanged"] = True
    output_report = manifest_path.parent / "encoding-report.json"
    # Preserve the other format's record when re-encoding just one output.
    if output_report.exists() and args.format != "both":
        previous = json.loads(output_report.read_text(encoding="utf-8"))
        if previous.get("rawSha256") == raw_hash:
            report["outputs"] = {**previous.get("outputs", {}), **report["outputs"]}
    output_report.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"passed": True, "report": str(output_report), "outputs": {k: {f: v[f] for f in ["file", "bytes", "decodedFrames", "decodedDurationMs", "maximumKeyframeMAE"]} for k, v in report["outputs"].items()}}, ensure_ascii=False), flush=True)


if __name__ == "__main__":
    main()
