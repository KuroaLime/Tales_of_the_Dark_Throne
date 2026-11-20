"""Independent Pillow UI timing model, not production execution or test evidence.

Windows application control blocked the new native functional harness. This
demonstration reads production design/data constants and models its intended
presentation in Python. Old verified GDI images supply only backdrop/chrome
and approved portrait pixels; all new action UI is drawn independently.
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
    value = min(1.0, max(0.0, value))
    return value * value * (3 - 2 * value)


def mix(left, right, weight):
    return tuple(round(a + (b - a) * weight) for a, b in zip(left, right))


NAVY, INK, GOLD, MUTED = (17, 20, 41), (241, 231, 211), (211, 180, 120), (187, 179, 201)
NORMAL, SELECTED, BORDER = (27, 32, 54), (52, 48, 71), (101, 88, 112)
BODY = (96, 206, 1184, 680)
PANEL = (80, 56, 1200, 688)
FONT_PATH = Path("C:/Windows/Fonts/malgun.ttf")
FONTS = {size: ImageFont.truetype(str(FONT_PATH), size) for size in (13, 15, 17, 18, 19, 20, 21, 23)}


def text(image, value, x, y, size=18, color=INK, align="left", width=0):
    draw = ImageDraw.Draw(image)
    font = FONTS[size]
    if align != "left":
        length = draw.textlength(value, font=font)
        x += (width - length) / (2 if align == "center" else 1)
    draw.text((x, y), value, font=font, fill=color, anchor="lt")


def box(image, rect, label, size=18, fill=NORMAL, border=BORDER, centered=False):
    draw = ImageDraw.Draw(image)
    draw.rounded_rectangle((rect[0], rect[1], rect[2] - 1, rect[3] - 1), radius=3, fill=fill, outline=border, width=1)
    lines = label.split("\n")
    line_height = size + 4
    y = rect[1] + (rect[3] - rect[1] - len(lines) * line_height) / 2
    for line in lines:
        text(image, line, rect[0] + 12, y, size, INK, "center" if centered else "left", rect[2] - rect[0] - 24)
        y += line_height


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", type=Path, default=Path("tests/bin/maintenance-actions-model"))
    args = parser.parse_args()
    started = time.monotonic()
    root = Path(__file__).resolve().parent.parent
    source_root = root / "Tower_Triumph/Tower_Triumph"
    cpp = (source_root / "Maintenance_Stage.cpp").read_text(encoding="utf-8-sig")
    durations = {name: float(re.search(r"constexpr double " + name + r"\s*=\s*([\d.]+)", cpp).group(1)) for name in (
        "TabPresentationDuration", "EquipmentFeedbackDuration", "SkillSelectionDuration", "SkillReplacementDuration", "SkillPulseDuration")}
    item_cpp = (source_root / "ItemData.cpp").read_text(encoding="utf-8-sig")
    armor_match = re.search(r'302,\s*L"Iron Armor"[\s\S]*?SItemStatBonus\(([^)]+)\)', item_cpp)
    armor_bonus = [int(value.strip()) for value in armor_match.group(1).split(",")]
    assert armor_bonus == [20, 0, 0, 0, 6, -1]
    char_cpp = (source_root / "Character.cpp").read_text(encoding="utf-8-sig")
    warrior = re.search(r"case ECharacterClass::Warrior:\s*newStat.MaxHP[\s\S]*?break;", char_cpp).group(0)
    stats = {key: int(value) for key, value in re.findall(r"newStat\.(\w+)\s*=\s*(\d+)", warrior)}
    stats.update(CurHP=37, CurMP=7)
    skill_cpp = (source_root / "SkillData.cpp").read_text(encoding="utf-8-sig")
    skills = []
    for skill_id in (1101, 1102, 1103):
        row = re.search(r"v.push_back\(\{ " + str(skill_id) + r', L"([^"]+)"[\s\S]*?ESkillStatType::STR, (\d+), (\d+), \d+, \d+, RGB\(([^)]+)\)', skill_cpp)
        skills.append({"id": skill_id, "name": row.group(1), "range": int(row.group(2)), "mp": int(row.group(3)),
                       "color": tuple(int(part.strip()) for part in row.group(4).split(","))})
    screenshot_root = root / "tests/bin/maintenance-map-ui-checks"
    background_path = screenshot_root / "equipment-character-0-1280x720.bmp"
    skill_path = screenshot_root / "skill-character-0-1280x720.bmp"
    with Image.open(background_path) as opened:
        template = opened.convert("RGB")
        portrait = template.crop((108, 266, 186, 361))
    with Image.open(skill_path) as opened:
        skill_portrait = opened.convert("RGB").crop((108, 263, 162, 331))
    assert template.size == (1280, 720)
    retained_sources = [{"path": str(path), "sha256": sha256(path), "use": role} for path, role in (
        (background_path, "Prior native backdrop/panel chrome and approved Warrior portrait only; not new-feature evidence"),
        (skill_path, "Approved Warrior portrait crop only; not new-feature evidence"))]
    tab_names = ["1. 보상", "2. 파티", "3. 인벤토리", "4. 장비", "5. 스킬", "6. 상점"]
    party_names = ["Fire Warrior", "Water Warrior", "Grass Archer", "Fire Mage", "Light Mage"]
    slot_rects = [(108 + i * 358, 360, 454 + i * 358, 446) for i in range(3)]
    learned_rects = [(rect[0], 492, rect[2], 578) for rect in slot_rects]
    footer = (108, 637, 1172, 671)
    local_regions = slot_rects + learned_rects + [footer]
    state = {"tab": 2, "armor": False, "selected": 0, "assigned": [0, 1, 2], "message": "",
             "equipmentAt": -100.0, "equipmentSign": 0, "pulseAt": -100.0, "pulseSkill": -1, "pulseSlot": -1}

    def base(tab_index):
        image = template.copy()
        draw = ImageDraw.Draw(image)
        draw.rectangle((96, 155, 1183, 203), fill=NAVY)
        draw.rectangle((96, 206, 1183, 679), fill=NAVY)
        draw.rectangle((900, 85, 1032, 127), fill=NAVY)
        text(image, "GOLD 275", 945, 96, 18, GOLD)
        for i, name in enumerate(tab_names):
            x = 108 + i * 178
            box(image, (x, 160, x + 170, 200), name, 17, SELECTED if i == tab_index else NORMAL,
                GOLD if i == tab_index else BORDER, True)
        for i, name in enumerate(party_names):
            x = 108 + i * 214
            box(image, (x, 214, x + 206, 252), name, 17, SELECTED if i == 0 else NORMAL, GOLD if i == 0 else BORDER, True)
        return image

    def target_frame(seconds):
        image = base(state["tab"])
        if state["tab"] == 2:
            text(image, "INVENTORY     GOLD: 275     Target: Fire Warrior", 108, 263, 19, GOLD)
            if not state["armor"]:
                box(image, (108, 302, 635, 360), "Iron Armor x1  [Equip]")
            else:
                text(image, "No Items Found.", 108, 326, 20, MUTED)
            text(image, "Click equipment to equip it to the selected character. Click potion to use it.", 108, 636, 17, MUTED)
        elif state["tab"] == 3:
            image.paste(portrait, (108, 266))
            text(image, "Fire Warrior", 204, 277, 23)
            text(image, "Current Stats", 204, 320, 18, GOLD)
            age = seconds - state["equipmentAt"]
            feedback = smooth(age / .12) * (1 - smooth((age - 1.1) / .5)) if 0 <= age < durations["EquipmentFeedbackDuration"] else 0
            current = dict(stats)
            if state["armor"]:
                for key, amount in zip(("MaxHP", "MaxMP", "STR", "INT", "DEF", "SPD"), armor_bonus):
                    current[key] += amount
                current["CurHP"] += armor_bonus[0]
            values = [f'{current["CurHP"]} / {current["MaxHP"]}', f'{current["CurMP"]} / {current["MaxMP"]}'] + [str(current[key]) for key in ("STR", "INT", "DEF", "SPD")]
            for i, label in enumerate(("HP", "MP", "STR", "INT", "DEF", "SPD")):
                x, y, w, h = 108 + (i % 2) * 256, 376 + (i // 2) * 62, 246, 54
                delta = armor_bonus[i] * state["equipmentSign"]
                accent = (128, 220, 173) if delta > 0 else (245, 153, 152)
                fill = mix((26, 28, 52), accent, feedback * .13 if delta else 0)
                border = mix((100, 84, 70), accent, feedback * .8 if delta else 0)
                ImageDraw.Draw(image).rounded_rectangle((x, y, x + w - 1, y + h - 1), radius=3, fill=fill, outline=border)
                text(image, label, x + 10, y + 3, 13, MUTED)
                text(image, values[i], x + 10, y + 20, 18, GOLD)
                if delta and feedback:
                    badge = f"{delta:+d}" + (" MAX" if i < 2 else "")
                    text(image, badge, x + 45, y + 3, 13, mix(fill, accent, feedback), "right", w - 55)
            text(image, "Equipped Items", 660, 266, 20, GOLD)
            for i, part in enumerate(("Weapon", "Armor", "Accessory")):
                equipped = i == 1 and state["armor"]
                label = part + (": Iron Armor\n[Click to Unequip]" if equipped else ": Empty")
                emphasis = feedback if i == 1 else 0
                box(image, (660, 302 + i * 82, 1172, 372 + i * 82), label, 18,
                    mix(NORMAL, (70, 59, 65), emphasis), mix(GOLD if equipped else BORDER, (246, 215, 157), emphasis))
            text(image, "Use the Inventory tab to equip new items. Click an equipped slot here to unequip it.", 108, 636, 17, MUTED)
        else:
            image.paste(skill_portrait, (108, 263))
            text(image, "SKILL SETTING", 176, 266, 15, GOLD)
            text(image, "Fire Warrior / Fire", 176, 295, 21)
            text(image, "Equipped Skills", 108, 332, 17, GOLD)
            pulse = 1 - smooth((seconds - state["pulseAt"]) / durations["SkillPulseDuration"])
            for i, skill_index in enumerate(state["assigned"]):
                skill = skills[skill_index]
                emphasis = pulse if i == state["pulseSlot"] else 0
                box(image, slot_rects[i], f'Slot {i + 1}: {skill["name"]}\nMP {skill["mp"]}', 17,
                    mix(SELECTED if i == state["selected"] else NORMAL, (85, 69, 69), emphasis),
                    mix(GOLD if i == state["selected"] else BORDER, (255, 229, 171), emphasis))
            text(image, "Learned Skills - click a skill to equip it into the selected slot.", 108, 457, 17, MUTED)
            for i, skill in enumerate(skills):
                emphasis = pulse if i == state["pulseSkill"] else 0
                selected = i == state["assigned"][state["selected"]]
                box(image, learned_rects[i], f'{skill["name"]}\nMP {skill["mp"]} / R{skill["range"]}', 18,
                    mix(SELECTED if selected else NORMAL, (85, 69, 69), emphasis),
                    mix(GOLD if selected else skill["color"], (255, 229, 171), emphasis))
            if state["message"]:
                text(image, state["message"], 108, 637, 18, GOLD)
        return image

    actions = [(30, "equip", None), (96, "unequip", None), (168, "tab", 4), (192, "select", 1),
               (216, "replace", 2), (270, "replace", 0), (273, "replace", 1), (360, "tab", 2)]
    fps, count, width, height = 30, 420, 960, 540
    args.output_dir.mkdir(parents=True, exist_ok=True)
    raw_path, trace_path = args.output_dir / "frames.bgr", args.output_dir / "frame-trace.csv"
    proofs = [48, 105, 204, 225]
    keys = sorted(set(proofs + [0, 419] + [a[0] for a in actions] + [a[0] + 2 for a in actions]))
    keyframes = []
    displayed = target_frame(0)
    outgoing = None
    transition = None
    action_index = 0
    scene = "inventory-before"
    with raw_path.open("wb") as raw, trace_path.open("w", encoding="utf-8", newline="") as trace_file:
        trace = csv.writer(trace_file)
        trace.writerow(["frame", "time", "scene", "modeled_tab", "modeled_selected_slot", "modeled_armor", "modeled_skills"])
        for index in range(count):
            seconds = index / fps
            if action_index < len(actions) and index == actions[action_index][0]:
                _, action, value = actions[action_index]
                if outgoing is not None:
                    outgoing.close()
                outgoing = displayed.copy()
                if action in ("tab", "equip"):
                    state["tab"] = 3 if action == "equip" else value
                    state["pulseAt"] = -100
                    state["equipmentAt"] = seconds if action == "equip" else -100
                    transition = ("tab", seconds, durations["TabPresentationDuration"])
                if action == "equip":
                    state.update(armor=True, equipmentSign=1)
                elif action == "unequip":
                    state.update(armor=False, equipmentSign=-1, equipmentAt=seconds)
                    transition = None
                elif action == "select":
                    state["selected"] = value
                    transition = ("skill", seconds, durations["SkillSelectionDuration"])
                elif action == "replace":
                    state["assigned"][state["selected"]] = value
                    state.update(pulseAt=seconds, pulseSkill=value, pulseSlot=state["selected"],
                                 message=f'Equipped {skills[value]["name"]} to slot {state["selected"] + 1}.')
                    transition = ("skill", seconds, durations["SkillReplacementDuration"])
                scene = action + ("-" + str(value) if value is not None else "")
                action_index += 1
            displayed.close()
            displayed = target_frame(seconds)
            if transition is not None:
                kind, start, duration = transition
                progress = (seconds - start) / duration
                if progress < 1:
                    for rect in [BODY] if kind == "tab" else local_regions:
                        target_crop, source_crop = displayed.crop(rect), outgoing.crop(rect)
                        blended = Image.blend(source_crop, target_crop, smooth(progress))
                        displayed.paste(blended, rect[:2])
                        target_crop.close(); source_crop.close(); blended.close()
                else:
                    transition = None
            scaled = displayed.resize((width, height), Image.Resampling.LANCZOS)
            raw.write(scaled.tobytes("raw", "BGR"))
            if index in keys:
                filename = f"{index:04d}-{scene}.png"
                scaled.save(args.output_dir / filename)
                keyframes.append({"index": index, "label": scene, "file": filename, "independentModel": True})
            scaled.close()
            trace.writerow([index, seconds, scene, state["tab"], state["selected"], state["armor"], list(state["assigned"])])
    assert action_index == len(actions)
    assert raw_path.stat().st_size == count * width * height * 3
    with raw_path.open("rb") as stream:
        frame_bytes = width * height * 3
        first = stream.read(frame_bytes)
        stream.seek(-frame_bytes, 2)
        assert first == stream.read(frame_bytes), "Model loop boundary mismatch"
    assert all(sha256(Path(source["path"])) == source["sha256"] for source in retained_sources)
    source_files = [source_root / name for name in ("Maintenance_Stage.cpp", "StarlightUI.h", "Character.cpp", "GameManager.cpp", "ItemData.cpp", "SkillData.cpp")]
    scope = ("Independent Python/Pillow UI timing model, NOT a native render, live game recording, native input replay, "
             "or runtime verification of the new implementation. Windows application control blocked the new native harness. "
             "Old verified screenshots supply only backdrop/chrome and approved portrait crops. Newly drawn UI uses source "
             "layout/colors/data and a separate Python presentation model; font rasterization differs from Windows GDI.")
    manifest = {"format": "bgr24", "rawFile": raw_path.name, "width": width, "height": height, "stride": width * 3,
                "fps": fps, "frameCount": count, "durationSeconds": count / fps, "rawSha256": sha256(raw_path),
                "scope": scope, "encodingScope": scope + " Encoder verification covers the media file only, not game behavior.",
                "captureMethod": "independent-source-based-ui-model", "nativeViewport": [1280, 720],
                "fixture": {"character": "Fire Warrior", "gold": 275, "before": stats, "armorId": 302,
                            "armorBonus": armor_bonus, "skills": skills, "defaultEquipment": [-1, -1, -1], "savedGame": False},
                "presentationDurations": durations, "retainedOldVisualSources": retained_sources,
                "productionSources": [{"path": str(path), "sha256": sha256(path)} for path in source_files],
                "font": {"path": str(FONT_PATH), "sha256": sha256(FONT_PATH)}, "stillFrameIndices": proofs, "keyframes": keyframes,
                "timeline": [{"start": 0, "scene": "inventory-before"}] + [{"start": frame / fps, "scene": action, "value": value} for frame, action, value in actions],
                "checks": {"mediaModelRendered": True, "loopBoundaryMatches": True, "oldSourcesUnchanged": True,
                           "nativeExecutionVerified": False, "newNativeCaptures": 0}, "renderWallSeconds": time.monotonic() - started}
    (args.output_dir / "frames.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    displayed.close(); template.close(); portrait.close(); skill_portrait.close()
    if outgoing is not None:
        outgoing.close()
    print(json.dumps({"modelRendered": True, "runtimeVerified": False, "frames": count, "seconds": count / fps,
                      "manifest": str((args.output_dir / "frames.json").resolve())}), flush=True)


if __name__ == "__main__":
    main()
