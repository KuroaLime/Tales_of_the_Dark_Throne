"""Independent source-based party UI model, not production runtime evidence.

No native executable is launched. The model draws the current source layout,
messages and timing in Pillow. Backdrop/chrome come from older verified stills;
portraits are the actual runtime bitmaps rendered with their magenta color key.
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
    parser.add_argument("--output-dir", type=Path, default=Path("tests/bin/maintenance-party-model"))
    args = parser.parse_args()
    started = time.monotonic()
    root = Path(__file__).resolve().parent.parent
    source_root = root / "Tower_Triumph/Tower_Triumph"
    cpp = (source_root / "Maintenance_Stage.cpp").read_text(encoding="utf-8-sig")
    game_cpp = (source_root / "GameManager.cpp").read_text(encoding="utf-8-sig")
    game_h = (source_root / "GameManager.h").read_text(encoding="utf-8-sig")
    roster = {}
    for i, match in enumerate(re.finditer(r'CreateRosterCharacter\(id\+\+, ECharacterClass::(\w+), EElement::(\w+),\s*L"([^"]+)"\)', game_cpp), 1):
        roster[i] = {"id": i, "class": match.group(1), "element": match.group(2), "name": match.group(3)}
    default_party = [int(index) + 1 for index in re.findall(r"PlayerParty.push_back\(Roster\[(\d+)\]\)", game_cpp)]
    maximum = int(re.search(r"MAX_ACTIVE_PARTY\s*=\s*(\d+)", game_h).group(1))
    remove_source = re.search(r"bool GameManager::RemoveActivePartyMember[\s\S]*?bool GameManager::ToggleActivePartyMember", game_cpp).group(0)
    minimum = int(re.search(r"PlayerParty.size\(\)\s*<=\s*(\d+)", remove_source).group(1))
    assert len(roster) == 15 and default_party == [1, 2, 8, 11, 14] and (minimum, maximum) == (1, 5)
    durations = {name: float(value) for name, value in re.findall(r"constexpr double (Party\w+)\s*=\s*([\d.]+)", cpp)}
    assert durations, "Party production timing must be implemented before modeling"
    presentation_duration = durations["PartyPresentationDuration"]
    feedback_duration = durations["PartyFeedbackDuration"]
    notice_hold, notice_exit = durations["PartyNoticeHoldEnd"], durations["PartyNoticeExitDuration"]
    pulse_hold, pulse_exit = durations["PartyPulseHoldEnd"], durations["PartyPulseExitDuration"]
    templates, visual_sources = {}, []
    for viewport in ((1280, 720), (1264, 644)):
        source = root / f"tests/bin/maintenance-map-ui-checks/maintenance-party-{viewport[0]}x{viewport[1]}.bmp"
        with Image.open(source) as opened:
            templates[viewport] = opened.convert("RGB")
        visual_sources.append({"path": str(source), "sha256": sha256(source), "use": "Earlier native backdrop/chrome only; not new-feature evidence"})
    portraits = {}
    for kind in ("Warrior", "Archer", "Mage"):
        source = source_root / f"Resource/{kind}_Portrait.bmp"
        with Image.open(source) as opened:
            portrait = opened.convert("RGBA")
        portrait.putdata([(r, g, b, 0 if (r, g, b) == (255, 0, 255) else 255) for r, g, b, _ in portrait.get_flattened_data()])
        portraits[kind] = portrait
        visual_sources.append({"path": str(source), "sha256": sha256(source), "use": "Actual approved runtime portrait, magenta color key, nearest resize"})
    state = {"party": list(default_party), "selected": 0, "target": -1, "eventAt": -100., "message": "",
             "success": False, "wasActive": False, "countDelta": 0, "pulseFrom": 0.}
    tab_names = ["1. 보상", "2. 파티", "3. 인벤토리", "4. 장비", "5. 스킬", "6. 상점"]

    def layout(compact):
        dx, bottom, row_height = (-8, 612, 53) if compact else (0, 688, 64)
        cards = [(408 + dx + (i % 3) * 258, 246 + (i // 3) * (row_height + 8),
                  656 + dx + (i % 3) * 258, 246 + (i // 3) * (row_height + 8) + row_height) for i in range(15)]
        return dx, bottom, row_height, cards

    def draw_portrait(image, member, bounds):
        portrait = portraits[roster[member]["class"]]
        scale = min((bounds[2] - bounds[0]) / portrait.width, (bounds[3] - bounds[1]) / portrait.height)
        size = (max(1, int(portrait.width * scale)), max(1, int(portrait.height * scale)))
        resized = portrait.resize(size, Image.Resampling.NEAREST)
        position = ((bounds[0] + bounds[2] - size[0]) // 2, (bounds[1] + bounds[3] - size[1]) // 2)
        image.paste(resized, position, resized)
        resized.close()

    def base(compact=False):
        viewport = (1264, 644) if compact else (1280, 720)
        dx, bottom, _, _ = layout(compact)
        image = templates[viewport].copy()
        draw = ImageDraw.Draw(image)
        draw.rectangle((96 + dx, 155, 1183 + dx, 203), fill=NAVY)
        draw.rectangle((96 + dx, 206, 1183 + dx, bottom - 9), fill=NAVY)
        for i, name in enumerate(tab_names):
            x = 108 + dx + i * 178
            surface(image, (x, 160, x + 170, 200), SELECTED if i == 1 else NORMAL, GOLD if i == 1 else BORDER)
            text(image, name, x + 12, 171, 17, INK, "center", 146)
        return image

    def pulse_at(seconds):
        age = seconds - state["eventAt"]
        if not state["message"] or not 0 <= age < feedback_duration:
            return 0.
        enter = smooth(age / presentation_duration)
        return (state["pulseFrom"] + (1 - state["pulseFrom"]) * enter) * (1 - smooth((age - pulse_hold) / pulse_exit))

    def target_frame(seconds, compact=False):
        dx, bottom, row_height, cards = layout(compact)
        image = base(compact)
        age, pulse = seconds - state["eventAt"], pulse_at(seconds)
        active_notice = bool(state["message"]) and 0 <= age < feedback_duration
        accent = ((211, 180, 120) if state["wasActive"] else (128, 220, 173)) if state["success"] else (245, 153, 152)
        text(image, f'ACTIVE PARTY  {len(state["party"])} / {maximum}', 108 + dx, 214, 18, GOLD)
        if pulse and state["countDelta"]:
            text(image, f'{state["countDelta"]:+d}', 324 + dx, 214, 18, mix(NAVY, accent, pulse), "right", 60)
        text(image, "ROSTER", 408 + dx, 214, 18, GOLD)
        for i, member in enumerate(state["party"]):
            character = roster[member]
            y = 246 + i * (row_height + 8)
            draw_portrait(image, member, (108 + dx, y + 2, 144 + dx, y + row_height - 2))
            text(image, f'{i + 1}. {character["name"]}', 154 + dx, y + 4, 17)
            text(image, "/ " + character["element"], 154 + dx, y + 29, 15, (188, 167, 203))
        for i, (member, character) in enumerate(roster.items()):
            rect = cards[i]
            active = member in state["party"]
            emphasis = pulse if active_notice and member == state["target"] else 0.
            fill = mix(SELECTED if active else NORMAL, accent, emphasis * .14)
            border = mix(GOLD if active else BORDER, accent, emphasis * .85)
            surface(image, rect, fill, border)
            draw_portrait(image, member, (rect[0] + 7, rect[1] + 5, rect[0] + 43, rect[3] - 5))
            text(image, character["name"], rect[0] + 52, rect[1] + 7, 16)
            if active:
                text(image, "ACTIVE", rect[0] + 52, rect[1] + 31, 13, (188, 167, 203))
        text(image, "Click a roster character to add/remove. At least 1 and up to 5 characters can be active.", 108 + dx, bottom - 57, 14, MUTED)
        if active_notice:
            text(image, state["message"], 108 + dx, bottom - 34, 17, mix(NAVY, accent, 1 - smooth((age - notice_hold) / notice_exit)))
        return image

    def regions(compact=False):
        dx, bottom, row_height, cards = layout(compact)
        return [(108 + dx, 246, 384 + dx, 246 + 4 * (row_height + 8) + row_height)] + cards + [(108 + dx, bottom - 34, 1172 + dx, bottom - 10)]

    actions = [(30, 15), (90, 14), (120, 15), (180, 15), (189, 11), (198, 8), (207, 2),
               (255, 1), (330, 2), (339, 8), (348, 11), (357, 14)]
    fps, count, width, height = 30, 540, 960, 540
    args.output_dir.mkdir(parents=True, exist_ok=True)
    raw_path, trace_path = args.output_dir / "frames.bgr", args.output_dir / "frame-trace.csv"
    proofs = [48, 138, 273, 375]
    keys = sorted(set(proofs + [0, count - 1] + [frame for frame, _ in actions] + [frame + 2 for frame, _ in actions]))
    action_index, keyframes, transactions = 0, [], []
    displayed = target_frame(0.)
    outgoing = displayed.copy()
    with raw_path.open("wb") as raw, trace_path.open("w", encoding="utf-8", newline="") as trace_file:
        trace = csv.writer(trace_file)
        trace.writerow(["frame", "seconds", "modeled_party_ids", "modeled_selected_index", "selected_character_id", "target_id", "success", "count_delta"])
        for index in range(count):
            seconds = index / fps
            if action_index < len(actions) and index == actions[action_index][0]:
                _, member = actions[action_index]
                outgoing.close(); outgoing = displayed.copy()
                before = list(state["party"])
                previous_selection = state["selected"]
                previous_pulse = pulse_at(seconds) if member == state["target"] else 0.
                was_active = member in before
                success = len(before) > minimum if was_active else len(before) < maximum
                if success:
                    if was_active:
                        state["party"].remove(member)
                    else:
                        state["party"].append(member)
                        state["selected"] = len(state["party"]) - 1
                    state["selected"] = min(state["selected"], len(state["party"]) - 1)
                state.update(target=member, eventAt=seconds, success=success, wasActive=was_active,
                             countDelta=len(state["party"]) - len(before), pulseFrom=previous_pulse)
                if success:
                    state["message"] = ("Removed " if was_active else "Added ") + roster[member]["name"] + (" from the active party." if was_active else " to the active party.")
                else:
                    state["message"] = "At least one active character is required." if was_active else "Active party is already full."
                transactions.append({"time": seconds, "memberId": member, "wasActive": was_active, "success": success,
                                     "beforeParty": before, "afterParty": list(state["party"]), "selectedIndexBefore": previous_selection,
                                     "selectedIndexAfter": state["selected"], "countDelta": state["countDelta"], "message": state["message"]})
                action_index += 1
            displayed.close()
            displayed = target_frame(seconds)
            age = seconds - state["eventAt"]
            if 0 <= age < presentation_duration:
                for rect in regions():
                    current, old = displayed.crop(rect), outgoing.crop(rect)
                    blended = Image.blend(old, current, smooth(age / presentation_duration))
                    displayed.paste(blended, rect[:2])
                    current.close(); old.close(); blended.close()
            if index in (273, 375):
                compact = target_frame(seconds, True)
                filename = "compact-party-last-member-1264x644.png" if index == 273 else "compact-party-restored-1264x644.png"
                compact.save(args.output_dir / filename); compact.close()
            scaled = displayed.resize((width, height), Image.Resampling.LANCZOS)
            raw.write(scaled.tobytes("raw", "BGR"))
            if index in keys:
                filename = f"{index:04d}-party.png"
                scaled.save(args.output_dir / filename)
                keyframes.append({"index": index, "label": "party-management-model", "file": filename, "independentModel": True})
            scaled.close()
            trace.writerow([index, seconds, list(state["party"]), state["selected"], state["party"][state["selected"]], state["target"], state["success"], state["countDelta"]])
    assert [len(entry["afterParty"]) for entry in transactions] == [5, 4, 5, 4, 3, 2, 1, 1, 2, 3, 4, 5]
    assert [entry["selectedIndexAfter"] for entry in transactions] == [0, 0, 4, 3, 2, 1, 0, 0, 1, 2, 3, 4]
    assert not transactions[0]["success"] and not transactions[7]["success"] and state["party"] == default_party
    assert raw_path.stat().st_size == count * width * height * 3
    assert all(sha256(Path(source["path"])) == source["sha256"] for source in visual_sources)
    source_files = [source_root / name for name in ("Maintenance_Stage.cpp", "StarlightUI.h", "Character.cpp", "GameManager.cpp", "GameManager.h")]
    scope = ("Independent Python/Pillow party UI timing model, NOT native rendering, live game recording, input replay, or runtime verification. "
             "The Windows-blocked native harness was not launched. Older screenshots supply backdrop/chrome only; portraits are unchanged runtime bitmaps. "
             "Source roster/default party/limits and selection-index rules are reproduced by a separate model. All new UI is drawn independently. "
             "The loop restarts the demonstration fixture; no saved game is accessed.")
    manifest = {"format": "bgr24", "rawFile": raw_path.name, "width": width, "height": height, "stride": width * 3,
                "fps": fps, "frameCount": count, "durationSeconds": count / fps, "rawSha256": sha256(raw_path), "scope": scope,
                "encodingScope": scope + " Encoding verification covers media only, not game behavior.",
                "captureMethod": "independent-source-based-party-ui-model", "nativeViewport": [1280, 720],
                "fixture": {"initialParty": default_party, "initialSelectedIndex": 0, "roster": list(roster.values()),
                            "minimum": minimum, "maximum": maximum, "gold": 0, "savedGame": False},
                "transactions": transactions, "presentationDurations": durations, "stillFrameIndices": proofs, "keyframes": keyframes,
                "visualSources": visual_sources, "productionSources": [{"path": str(path), "sha256": sha256(path)} for path in source_files],
                "font": {"path": str(FONT_PATH), "sha256": sha256(FONT_PATH)},
                "compactProofs": [{"file": name, "nativeViewport": [1264, 644], "rowHeight": 53, "independentModel": True} for name in (
                    "compact-party-last-member-1264x644.png", "compact-party-restored-1264x644.png")],
                "timeline": [{"start": 0, "scene": "default-party"}] + [{"start": entry["time"], "scene": entry["message"]} for entry in transactions],
                "checks": {"mediaModelRendered": True, "modelPartyCountsAndSelectionConsistent": True, "visualSourcesUnchanged": True,
                           "nativeExecutionVerified": False, "newNativeCaptures": 0}, "renderWallSeconds": time.monotonic() - started}
    (args.output_dir / "frames.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    displayed.close(); outgoing.close()
    for image in list(templates.values()) + list(portraits.values()):
        image.close()
    print(json.dumps({"modelRendered": True, "runtimeVerified": False, "frames": count, "seconds": count / fps,
                      "manifest": str((args.output_dir / "frames.json").resolve())}), flush=True)


if __name__ == "__main__":
    main()
