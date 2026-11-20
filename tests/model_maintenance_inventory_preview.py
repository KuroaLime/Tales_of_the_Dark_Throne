"""Independent inventory-potion UI model; never native runtime verification.

The Windows-blocked game/test executable is not launched. This standalone
Pillow demonstration uses production layout/timing/data and old backdrop/chrome
pixels, with new UI drawn separately. Full HP/MP still consumes one potion and
reports zero recovery, matching the unchanged GameManager rule.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import json
import re
import time
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def smooth(value: float) -> float:
    value = min(1., max(0., value))
    return value * value * (3 - 2 * value)


def mix(left, right, weight):
    return tuple(round(a + (b - a) * weight) for a, b in zip(left, right))


NAVY, INK, GOLD, MUTED = (17, 20, 41), (241, 231, 211), (211, 180, 120), (187, 179, 201)
NORMAL, SELECTED, BORDER = (27, 32, 54), (52, 48, 71), (101, 88, 112)
FONT_PATH = Path("C:/Windows/Fonts/malgun.ttf")
FONTS = {size: ImageFont.truetype(str(FONT_PATH), size) for size in range(12, 24)}


def text(image, value, x, y, size=18, color=INK, align="left", width=0):
    draw = ImageDraw.Draw(image)
    if align != "left":
        length = draw.textlength(value, font=FONTS[size])
        x += (width - length) / (2 if align == "center" else 1)
    draw.text((x, y), value, font=FONTS[size], fill=color, anchor="lt")


def surface(image, rect, fill=NORMAL, border=BORDER):
    ImageDraw.Draw(image).rounded_rectangle((rect[0], rect[1], rect[2] - 1, rect[3] - 1), radius=3, fill=fill, outline=border)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", type=Path, default=Path("tests/bin/maintenance-inventory-model"))
    args = parser.parse_args()
    started = time.monotonic()
    root = Path(__file__).resolve().parent.parent
    source_root = root / "Tower_Triumph/Tower_Triumph"
    cpp = (source_root / "Maintenance_Stage.cpp").read_text(encoding="utf-8-sig")
    item_cpp = (source_root / "ItemData.cpp").read_text(encoding="utf-8-sig")
    character_cpp = (source_root / "Character.cpp").read_text(encoding="utf-8-sig")
    warrior = re.search(r"case ECharacterClass::Warrior:\s*newStat.MaxHP[\s\S]*?break;", character_cpp).group(0)
    defaults = {key: int(value) for key, value in re.findall(r"newStat\.(\w+)\s*=\s*(\d+)", warrior)}
    items = {}
    for match in re.finditer(r'SItemData\(\s*(\d+),\s*L"([^"]+)",\s*L"([^"]+)",\s*EGameItemType::(\w+),\s*EEquipPart::\w+,\s*(\d+),\s*(\d+),\s*(\d+),', item_cpp):
        item_id = int(match.group(1))
        items[item_id] = {"id": item_id, "name": match.group(2), "description": match.group(3), "kind": match.group(4),
                         "price": int(match.group(5)), "healHP": int(match.group(6)), "healMP": int(match.group(7))}
    assert items[101]["healHP"] == 50 and items[102]["healMP"] == 30
    durations = {name: float(value) for name, value in re.findall(r"constexpr double (Inventory\w+|Potion\w+)\s*=\s*([\d.]+)", cpp)}
    assert durations, "Production inventory timing must exist before modeling"
    enter_duration = durations["InventoryNoticeTransitionDuration"]
    feedback_duration = durations["InventoryFeedbackDuration"]
    notice_hold, notice_exit = durations["InventoryNoticeHoldEnd"], durations["InventoryNoticeExitDuration"]
    pulse_hold, pulse_exit = durations["InventoryPulseHoldEnd"], durations["InventoryPulseExitDuration"]
    bar_duration = durations["InventoryBarDuration"]
    templates, retained_sources = {}, []
    for viewport in ((1280, 720), (1264, 644)):
        source = root / f"tests/bin/maintenance-map-ui-checks/equipment-character-0-{viewport[0]}x{viewport[1]}.bmp"
        with Image.open(source) as opened:
            templates[viewport] = opened.convert("RGB")
        retained_sources.append({"path": str(source), "sha256": sha256(source), "use": "Earlier native backdrop/chrome only; not new-feature evidence"})
    tab_names = ["1. 보상", "2. 파티", "3. 인벤토리", "4. 장비", "5. 스킬", "6. 상점"]
    party_names = ["Fire Warrior", "Water Warrior", "Grass Archer", "Fire Mage", "Light Mage"]
    state = {"gold": 275, "hp": 37, "mp": 7, "maxHP": defaults["MaxHP"], "maxMP": defaults["MaxMP"],
             "bag": {101: 3, 102: 2}, "eventAt": -100., "item": -1, "hpGain": 0, "mpGain": 0,
             "countBefore": 0, "message": "", "pulseFrom": 0.,
             "hpFrom": 37 / defaults["MaxHP"], "mpFrom": 7 / defaults["MaxMP"],
             "displayedHP": 37 / defaults["MaxHP"], "displayedMP": 7 / defaults["MaxMP"]}
    outgoing_notice = Image.new("RGB", (1064, 27), NAVY)

    def base(compact=False):
        viewport = (1264, 644) if compact else (1280, 720)
        dx, bottom = (-8, 612) if compact else (0, 688)
        image = templates[viewport].copy()
        draw = ImageDraw.Draw(image)
        draw.rectangle((96 + dx, 155, 1183 + dx, 203), fill=NAVY)
        draw.rectangle((96 + dx, 206, 1183 + dx, bottom - 9), fill=NAVY)
        draw.rectangle((900 + dx, 85, 1032 + dx, 127), fill=NAVY)
        text(image, f'GOLD {state["gold"]}', 945 + dx, 96, 18, GOLD)
        for i, name in enumerate(tab_names):
            x = 108 + dx + i * 178
            surface(image, (x, 160, x + 170, 200), SELECTED if i == 2 else NORMAL, GOLD if i == 2 else BORDER)
            text(image, name, x + 12, 171, 17, INK, "center", 146)
        for i, name in enumerate(party_names):
            x = 108 + dx + i * 214
            surface(image, (x, 214, x + 206, 252), SELECTED if i == 0 else NORMAL, GOLD if i == 0 else BORDER)
            text(image, name, x + 12, 225, 17, INK, "center", 182)
        return image

    # The production layout is mirrored here, not executed. Any text added to
    # the demonstration must also exist in the production drawing/message code.
    def pulse_at(seconds):
        age = seconds - state["eventAt"]
        if not state["message"] or not 0 <= age < feedback_duration:
            return 0.
        enter = smooth(age / enter_duration)
        return (state["pulseFrom"] + (1 - state["pulseFrom"]) * enter) * (1 - smooth((age - pulse_hold) / pulse_exit))

    def render(seconds, compact=False):
        dx, bottom = (-8, 612) if compact else (0, 688)
        image = base(compact)
        text(image, f'INVENTORY     GOLD: 275     Target: Fire Warrior', 108 + dx, 263, 17, GOLD)
        age, pulse = seconds - state["eventAt"], pulse_at(seconds)
        active = bool(state["message"]) and 0 <= age < feedback_duration
        no_recovery = state["hpGain"] == 0 and state["mpGain"] == 0
        hp_accent, mp_accent = (128, 220, 173), (139, 181, 247)
        success = GOLD if no_recovery else (128, 220, 173)
        for label, value, maximum, gain, requested, x, accent in (
                ("HP", state["hp"], state["maxHP"], state["hpGain"], active and items[state["item"]]["healHP"] > 0, 716 + dx, hp_accent),
                ("MP", state["mp"], state["maxMP"], state["mpGain"], active and items[state["item"]]["healMP"] > 0, 952 + dx, mp_accent)):
            text(image, f"{label} {value} / {maximum}", x, 263, 17, INK)
            if requested and pulse:
                text(image, f"+{gain}", x, 263, 15, mix(NAVY, GOLD if no_recovery else accent, pulse), "right", 220)
            draw = ImageDraw.Draw(image)
            draw.rectangle((x, 289, x + 219, 292), fill=(42, 43, 65))
            actual = value / maximum if maximum else 0.
            start_fraction = state["hpFrom" if label == "HP" else "mpFrom"]
            progress = smooth(age / bar_duration) if active else 1.
            displayed_fraction = start_fraction + (actual - start_fraction) * progress
            state["displayedHP" if label == "HP" else "displayedMP"] = displayed_fraction
            fill_width = int(220 * displayed_fraction + .5)
            if fill_width:
                draw.rectangle((x, 289, x + fill_width - 1, 292), fill=mix(accent, INK, pulse * .25 if requested else 0.))
        for i, item_id in enumerate(key for key in (101, 102) if state["bag"][key] > 0):
            item = items[item_id]
            x = 108 + dx + (i % 2) * 537
            card_height = 42 if compact else 58
            rect = (x, 302, x + 527, 302 + card_height)
            emphasis = pulse if item_id == state["item"] else 0.
            fill = mix(NORMAL, success, emphasis * .14)
            border = mix(BORDER, success, emphasis * .85)
            surface(image, rect, fill, border)
            text(image, item["name"], x + 12, rect[1] + 3, 16 if compact else 17)
            detail_y = rect[1] + 24 if compact else rect[3] - 22
            detail_font = 12 if compact else 14
            text(image, f'x{state["bag"][item_id]}  [Use]', x + 12, detail_y, detail_font, MUTED)
            if emphasis:
                text(image, "-1", rect[2] - 50, detail_y, detail_font, mix(fill, success, emphasis), "right", 38)
        if not any(state["bag"].values()):
            text(image, "No Items Found.", 108 + dx, 326, 20, MUTED)
        text(image, "Click equipment to equip it to the selected character. Click potion to use it.", 108 + dx, bottom - 62, 14, MUTED)
        if active:
            opacity = 1 - smooth((age - notice_hold) / notice_exit)
            color = mix(NAVY, success, opacity)
            text(image, state["message"], 108 + dx, bottom - 39, 16, color)
            if age < enter_duration:
                notice_rect = (108 + dx, bottom - 39, 1172 + dx, bottom - 12)
                target = image.crop(notice_rect)
                blended = Image.blend(outgoing_notice, target, smooth(age / enter_duration))
                image.paste(blended, notice_rect[:2])
                target.close(); blended.close()
        return image

    actions = [(36, 101), (120, 101), (123, 101), (225, 102), (324, 102)]
    fps, count, width, height = 30, 480, 960, 540
    args.output_dir.mkdir(parents=True, exist_ok=True)
    raw_path, trace_path = args.output_dir / "frames.bgr", args.output_dir / "frame-trace.csv"
    proofs = [54, 141, 243, 342]
    keys = sorted(set(proofs + [0, count - 1] + [frame for frame, _ in actions] + [frame + 2 for frame, _ in actions]))
    action_index, keyframes, transactions = 0, [], []
    displayed = render(0.)
    with raw_path.open("wb") as raw, trace_path.open("w", encoding="utf-8", newline="") as trace_file:
        trace = csv.writer(trace_file)
        trace.writerow(["frame", "seconds", "modeled_hp", "modeled_mp", "small_potion_count", "mana_potion_count", "actual_hp_gain", "actual_mp_gain"])
        for index in range(count):
            seconds = index / fps
            if action_index < len(actions) and index == actions[action_index][0]:
                _, item_id = actions[action_index]
                item = items[item_id]
                before_hp, before_mp, before_count = state["hp"], state["mp"], state["bag"][item_id]
                assert before_count > 0, "Only a currently present inventory card may be used"
                previous_pulse = pulse_at(seconds) if item_id == state["item"] else 0.
                outgoing_notice.close()
                outgoing_notice = displayed.crop((108, 649, 1172, 676))
                state["hpFrom"], state["mpFrom"] = state["displayedHP"], state["displayedMP"]
                state["hp"] = min(state["maxHP"], state["hp"] + item["healHP"])
                state["mp"] = min(state["maxMP"], state["mp"] + item["healMP"])
                state["bag"][item_id] -= 1  # Includes a valid zero-recovery use at full HP/MP.
                state.update(item=item_id, eventAt=seconds, hpGain=state["hp"] - before_hp,
                             mpGain=state["mp"] - before_mp, countBefore=before_count, pulseFrom=previous_pulse)
                recoveries = []
                if item["healHP"]:
                    recoveries.append(f'HP +{state["hpGain"]}' + (" (already full)" if state["hpGain"] == 0 else ""))
                if item["healMP"]:
                    recoveries.append(f'MP +{state["mpGain"]}' + (" (already full)" if state["mpGain"] == 0 else ""))
                state["message"] = f'Used {item["name"]}. ' + ". ".join(recoveries) + f'. Bag: {state["bag"][item_id]} (-1).'
                transactions.append({"time": seconds, "itemId": item_id, "consumed": 1, "hpBefore": before_hp,
                                     "hpAfter": state["hp"], "mpBefore": before_mp, "mpAfter": state["mp"],
                                     "countBefore": before_count, "countAfter": state["bag"][item_id],
                                     "actualHPGain": state["hpGain"], "actualMPGain": state["mpGain"]})
                action_index += 1
            displayed.close()
            displayed = render(seconds)
            if index in (54, 342):
                compact_image = render(seconds, True)
                compact_name = "compact-inventory-recovery-1264x644.png" if index == 54 else "compact-inventory-full-mp-use-1264x644.png"
                compact_image.save(args.output_dir / compact_name)
                compact_image.close()
            scaled = displayed.resize((width, height), Image.Resampling.LANCZOS)
            raw.write(scaled.tobytes("raw", "BGR"))
            if index in keys:
                filename = f"{index:04d}-inventory.png"
                scaled.save(args.output_dir / filename)
                keyframes.append({"index": index, "label": "inventory-potion-model", "file": filename, "independentModel": True})
            scaled.close()
            trace.writerow([index, seconds, state["hp"], state["mp"], state["bag"][101], state["bag"][102], state["hpGain"], state["mpGain"]])
    assert [entry["hpAfter"] for entry in transactions] == [87, 137, 150, 150, 150]
    assert [entry["mpAfter"] for entry in transactions] == [7, 7, 7, 20, 20]
    assert [entry["actualHPGain"] for entry in transactions] == [50, 50, 13, 0, 0]
    assert [entry["actualMPGain"] for entry in transactions] == [0, 0, 0, 13, 0]
    assert not any(state["bag"].values()) and transactions[-1]["consumed"] == 1
    assert raw_path.stat().st_size == count * width * height * 3
    assert all(sha256(Path(source["path"])) == source["sha256"] for source in retained_sources)
    source_files = [source_root / name for name in ("Maintenance_Stage.cpp", "StarlightUI.h", "Character.cpp", "GameManager.cpp", "ItemData.cpp")]
    scope = ("Independent Python/Pillow inventory-potion UI timing model, NOT native rendering, live game recording, input replay, "
             "or runtime verification. The Windows-blocked native harness was not launched. Earlier screenshots provide backdrop/chrome only. "
             "New UI and state changes are modeled from production layout/colors/timing/data. Full HP/MP still consumes one potion and "
             "reports zero recovery. No nonexistent-item click or synthetic business-logic failure is depicted. Loop restarts the fixture.")
    manifest = {"format": "bgr24", "rawFile": raw_path.name, "width": width, "height": height, "stride": width * 3,
                "fps": fps, "frameCount": count, "durationSeconds": count / fps, "rawSha256": sha256(raw_path),
                "scope": scope, "encodingScope": scope + " Encoding checks verify media only, not game behavior.",
                "captureMethod": "independent-source-based-inventory-ui-model", "nativeViewport": [1280, 720],
                "fixture": {"character": "Fire Warrior", "gold": 275, "initialHP": 37, "initialMP": 7,
                            "maxHP": defaults["MaxHP"], "maxMP": defaults["MaxMP"], "initialInventory": {101: 3, 102: 2},
                            "items": [items[101], items[102]], "savedGame": False},
                "transactions": transactions, "presentationDurations": durations, "stillFrameIndices": proofs, "keyframes": keyframes,
                "retainedOldVisualSources": retained_sources,
                "compactProof": {"file": "compact-inventory-full-mp-use-1264x644.png", "nativeViewport": [1264, 644], "independentModel": True},
                "compactCardProof": {"file": "compact-inventory-recovery-1264x644.png", "nativeViewport": [1264, 644],
                                     "cardHeight": 42, "independentModel": True},
                "productionSources": [{"path": str(path), "sha256": sha256(path)} for path in source_files],
                "font": {"path": str(FONT_PATH), "sha256": sha256(FONT_PATH)},
                "timeline": [{"start": 0, "scene": "inventory-model-initial"}] + [{"start": entry["time"], "scene": "potion-used", "itemId": entry["itemId"], "actualHPGain": entry["actualHPGain"], "actualMPGain": entry["actualMPGain"]} for entry in transactions],
                "checks": {"mediaModelRendered": True, "modelRecoveryAndInventoryConsistent": True, "oldSourcesUnchanged": True,
                           "nativeExecutionVerified": False, "newNativeCaptures": 0}, "renderWallSeconds": time.monotonic() - started}
    (args.output_dir / "frames.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    displayed.close(); outgoing_notice.close()
    for template in templates.values():
        template.close()
    print(json.dumps({"modelRendered": True, "runtimeVerified": False, "frames": count, "seconds": count / fps,
                      "manifest": str((args.output_dir / "frames.json").resolve())}), flush=True)


if __name__ == "__main__":
    main()
