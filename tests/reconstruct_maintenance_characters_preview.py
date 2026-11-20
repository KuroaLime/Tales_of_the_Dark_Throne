"""Reconstruct presentation timing from completed-state GDI regression snapshots.

This does not launch the game, a native renderer, or replay gameplay input.
It preserves snapshot text/artwork and only blends the production presentation
rectangles with the source smoothstep timings. Use encode_main_stage_flow.py
afterwards to encode and independently decode the BGR sequence.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import json
import re
import time
from pathlib import Path

from PIL import Image, ImageChops


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--stills", type=Path, default=Path("tests/bin/maintenance-map-ui-checks/character-stills-1280x720.json"))
    parser.add_argument("--output-dir", type=Path, default=Path("tests/bin/maintenance-characters-reconstruction"))
    args = parser.parse_args()
    started = time.monotonic()
    source_metadata = args.stills.resolve()
    metadata = json.loads(source_metadata.read_text(encoding="utf-8-sig"))
    root = Path(__file__).resolve().parent.parent
    production_path = root / "Tower_Triumph/Tower_Triumph/Maintenance_Stage.cpp"
    production = production_path.read_text(encoding="utf-8-sig")
    character_duration = float(re.search(r"CharacterPresentationDuration\s*=\s*([\d.]+)", production).group(1))
    tab_duration = float(re.search(r"TabPresentationDuration\s*=\s*([\d.]+)", production).group(1))
    assert character_duration == 0.16 and tab_duration == 0.18
    width, height = metadata["width"], metadata["height"]
    assert (width, height) == (1280, 720)
    panel = metadata["panelRect"]
    body = tuple(metadata["bodyRect"])
    detail = tuple(metadata["detailRect"])
    assert body == (panel[0] + 16, panel[1] + 150, panel[2] - 16, panel[3] - 8)
    assert detail == (body[0], body[1] + 52, body[2], body[3])
    assert metadata["fixtureGold"] == 0 and metadata["selectedSkillSlotIndex"] == 2
    snapshots: dict[tuple[str, int], Image.Image] = {}
    sources = []
    for actor in metadata["actors"]:
        for tab, key in [("equipment", "equipmentFile"), ("skill", "skillFile")]:
            path = (source_metadata.parent / actor[key]).resolve()
            digest = sha256(path)
            with Image.open(path) as opened:
                assert opened.size == (width, height)
                snapshots[tab, actor["partyIndex"]] = opened.convert("RGB")
            sources.append({"path": str(path), "sha256": digest, "tab": tab,
                            "partyIndex": actor["partyIndex"], "characterId": actor["characterId"],
                            "fixtureWeaponId": actor["equipmentWeaponId"]})
    assert len(snapshots) == 10
    # State choices only: none of these events call native Input or mutate data.
    actions = [
        (42, "equipment", 2, "character", "equipment-archer"),
        (90, "equipment", 3, "character", "equipment-mage"),
        (138, "skill", 3, "tab", "skill-mage"),
        (180, "skill", 2, "character", "skill-archer"),
        (222, "skill", 0, "character", "skill-warrior"),
        (270, "skill", 2, "character", "rapid-archer"),
        (273, "skill", 3, "character", "rapid-mage"),
        (276, "skill", 0, "character", "rapid-warrior"),
        (330, "equipment", 0, "tab", "equipment-warrior-return"),
    ]
    fps, count = 30, 420
    output_width, output_height = 800, 450
    keyframes = {0: "equipment-warrior", 60: "equipment-archer", 108: "equipment-mage",
                 192: "skill-archer-slot-three", 419: "equipment-warrior-final"}
    for index, _, _, _, label in actions:
        keyframes[index] = label + "-start"
        keyframes[index + 2] = label + "-middle"
    args.output_dir.mkdir(parents=True, exist_ok=True)
    raw_path = args.output_dir / "frames.bgr"
    trace_path = args.output_dir / "frame-trace.csv"
    target = snapshots["equipment", 0]
    displayed = target.copy()
    outgoing = displayed.copy()
    current_tab, current_member, label = "equipment", 0, "equipment-warrior"
    active = False
    kind, duration, elapsed = "character", character_duration, character_duration
    next_action = 0
    rapid_retargets = transition_frames = stable_frames = 0
    key_records = []
    with raw_path.open("wb") as raw, trace_path.open("w", encoding="utf-8", newline="") as trace_file:
        trace = csv.writer(trace_file)
        trace.writerow(["index", "time", "scene", "tab", "party_index", "selected_slot_index",
                        "transition_kind", "transition_progress", "old_opacity", "active", "state_selection"])
        for index in range(count):
            selected = False
            if next_action < len(actions) and index == actions[next_action][0]:
                _, current_tab, current_member, kind, label = actions[next_action]
                if index in (273, 276):
                    assert active and elapsed < duration
                    rapid_retargets += 1
                outgoing.close()
                outgoing = displayed.copy()  # Retarget from the previous displayed blend.
                target = snapshots[current_tab, current_member]
                duration = character_duration if kind == "character" else tab_duration
                elapsed, active, selected = 0.0, True, True
                next_action += 1
            # The reconstruction samples two 60Hz presentation ticks per frame.
            if active:
                elapsed = min(duration, elapsed + 1 / fps)
                active = elapsed < duration
            progress = elapsed / duration if active else 1.0
            eased = progress * progress * (3 - 2 * progress)
            opacity = int((1 - eased) * 255 + 0.5) if active else 0
            displayed.close()
            displayed = target.copy()
            bounds = detail if kind == "character" else body
            if active:
                new_crop, old_crop = target.crop(bounds), outgoing.crop(bounds)
                mixture = Image.blend(new_crop, old_crop, opacity / 255)
                displayed.paste(mixture, bounds[:2])
                new_crop.close(); old_crop.close(); mixture.close()
                transition_frames += 1
            else:
                assert ImageChops.difference(displayed, target).getbbox() is None
                stable_frames += 1
            # Pixels outside the production blend rectangle are exact target pixels:
            # header/tab selection is immediate, as is the character selector.
            difference = ImageChops.difference(displayed, target)
            difference.paste((0, 0, 0), bounds)
            assert difference.getbbox() is None
            difference.close()
            scaled = displayed.resize((output_width, output_height), Image.Resampling.LANCZOS)
            raw.write(scaled.tobytes("raw", "BGR"))
            if index in keyframes:
                filename = f"{keyframes[index]}-800x450.png"
                scaled.save(args.output_dir / filename)
                key_records.append({"index": index, "label": keyframes[index], "file": filename,
                                    "outputFile": filename, "reconstruction": True})
            scaled.close()
            trace.writerow([index, index / fps, label, current_tab, current_member, 2,
                            kind, progress, opacity, active, selected])
    assert next_action == len(actions) and rapid_retargets == 2 and not active
    assert raw_path.stat().st_size == count * output_width * output_height * 3
    assert all(sha256(Path(source["path"])) == source["sha256"] for source in sources)
    input_regression = source_metadata.parent / "interaction-checks.json"
    regression_reference = {"path": str(input_regression), "sha256": sha256(input_regression)} if input_regression.exists() else None
    timeline = [{"start": 0, "end": actions[0][0] / fps, "scene": "equipment-warrior"}]
    for i, action in enumerate(actions):
        timeline.append({"start": action[0] / fps, "end": actions[i + 1][0] / fps if i + 1 < len(actions) else count / fps,
                         "scene": action[4], "presentation": action[3]})
    scope = ("Snapshot-based timing reconstruction from actual completed-state GDI regression screenshots. "
             "Not a live game recording, not native sequence rendering, and not a replay of game Input. "
             "Python applies the production 160ms character/180ms tab smoothstep timings and rectangle bounds to the unchanged screenshots. "
             "Functional game logic is tested separately. No native executable is launched by this script.")
    manifest = {
        "format": "bgr24", "rawFile": raw_path.name, "width": output_width, "height": output_height,
        "stride": output_width * 3, "fps": fps, "frameCount": count, "durationSeconds": count / fps,
        "nativeViewport": [width, height], "scope": scope, "captureMethod": "gdi-still-timing-reconstruction",
        "encodingScope": scope + " The encoder only decodes BGR, encodes WebP, and verifies timing/image error; it adds no effects or labels.",
        "rawSha256": sha256(raw_path), "sourceMetadata": str(source_metadata), "sourceMetadataSha256": sha256(source_metadata),
        "sourceStills": sources, "productionSource": str(production_path), "productionSourceSha256": sha256(production_path),
        "separateNativeRegression": regression_reference,
        "fixture": metadata["fixtureNote"] + " Existing recent-action footer messages from earlier regression steps are retained verbatim; these independent completed-state screenshots are not a continuous gameplay session. Weapons follow actual unrestricted equipment rules.",
        "simulation": {"samplingHz": 30, "presentationHz": 60, "characterDurationSeconds": character_duration,
                       "tabDurationSeconds": tab_duration, "bodyRect": body, "detailRect": detail,
                       "ease": "smoothstep", "oldSourceAlpha": "round((1-smoothstep(progress))*255)",
                       "blend": "Pillow RGB blend; independent reconstruction, not bit-identical GDI intermediate output",
                       "resize": "uniform Lanczos 1280x720 to 800x450", "cursor": "not drawn",
                       "selectedSkillSlot": 3, "gold": 0, "fixtureWeapons": True,
                       "rapidRetarget": "from the last displayed RGB blend"},
        "timeline": timeline, "stillFrameIndices": [0, 60, 108, 192], "keyframes": key_records,
        "checks": {"passed": True, "sourceStillsUnchanged": True, "fixedRegionsUnchanged": True,
                   "settledFramesExactlyMatchTargetBeforeResize": True, "transitionFrames": transition_frames,
                   "settledFrames": stable_frames, "rapidRetargets": rapid_retargets,
                   "nativeInputReplay": False, "liveCapture": False},
        "renderWallSeconds": time.monotonic() - started,
    }
    (args.output_dir / "frames.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    displayed.close(); outgoing.close()
    for snapshot in snapshots.values(): snapshot.close()
    print(json.dumps({"passed": True, "method": manifest["captureMethod"], "frames": count,
                      "seconds": count / fps, "rapidRetargets": rapid_retargets,
                      "manifest": str((args.output_dir / "frames.json").resolve())}), flush=True)


if __name__ == "__main__":
    main()
