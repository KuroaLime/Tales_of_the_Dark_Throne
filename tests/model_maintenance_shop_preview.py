"""Independent shop UI timing demonstration, not native execution evidence.

The new native harness is blocked by Windows application control. This model
reads the production item table and presentation constants without launching
that executable. An older verified screenshot supplies only backdrop/chrome;
all shop contents and feedback are drawn independently in Pillow.
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
FONT_PATH = Path("C:/Windows/Fonts/malgun.ttf")
FONTS = {size: ImageFont.truetype(str(FONT_PATH), size) for size in range(13, 24)}


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
    parser.add_argument("--output-dir", type=Path, default=Path("tests/bin/maintenance-shop-model"))
    args = parser.parse_args()
    started = time.monotonic()
    root = Path(__file__).resolve().parent.parent
    source_root = root / "Tower_Triumph/Tower_Triumph"
    cpp = (source_root / "Maintenance_Stage.cpp").read_text(encoding="utf-8-sig")
    item_cpp = (source_root / "ItemData.cpp").read_text(encoding="utf-8-sig")
    items = {}
    for match in re.finditer(r'SItemData\(\s*(\d+),\s*L"([^"]+)",\s*L"([^"]+)",\s*EGameItemType::(\w+),\s*EEquipPart::\w+,\s*(\d+),', item_cpp):
        item_id = int(match.group(1))
        items[item_id] = {"id": item_id, "name": match.group(2), "description": match.group(3), "kind": match.group(4), "price": int(match.group(5))}
    stock_function = re.search(r"std::vector<int> Maintenance_Stage::GetShopItemIDs\(\) const[\s\S]*?return items;", cpp).group(0)
    stock = [int(value) for value in re.findall(r"items.push_back\((\d+)\)", stock_function)]
    assert len(stock) == 10 and all(item in items for item in stock)
    assert items[201]["price"] == 200 and items[101]["price"] == 50 and items[302]["price"] == 300
    durations = {name: float(value) for name, value in re.findall(r"constexpr double (Shop\w+)\s*=\s*([\d.]+)", cpp)}
    assert durations, "Shop production timing constants must exist before modeling"
    source_image = root / "tests/bin/maintenance-map-ui-checks/equipment-character-0-1280x720.bmp"
    with Image.open(source_image) as opened:
        template = opened.convert("RGB")
    assert template.size == (1280, 720)
    source_image_hash = sha256(source_image)
    compact_source = source_image.with_name("equipment-character-0-1264x644.bmp")
    with Image.open(compact_source) as opened:
        compact_template = opened.convert("RGB")
    compact_source_hash = sha256(compact_source)
    tab_names = ["1. 보상", "2. 파티", "3. 인벤토리", "4. 장비", "5. 스킬", "6. 상점"]
    expected_durations = {"ShopNoticeTransitionDuration": .16, "ShopFeedbackDuration": 2.6, "ShopNoticeHoldEnd": 2.2,
                          "ShopNoticeExitDuration": .4, "ShopPulseHoldEnd": 1.1, "ShopPulseExitDuration": .5}
    assert all(durations[key] == value for key, value in expected_durations.items())
    notice_rect = (108, 642, 1172, 672)
    outgoing_notice = Image.new("RGB", (notice_rect[2] - notice_rect[0], notice_rect[3] - notice_rect[1]), NAVY)
    state = {"gold": 450, "bag": {item_id: 0 for item_id in stock}, "item": -1, "eventAt": -100.,
             "successful": False, "goldSpent": 0, "countAdded": 0, "message": "", "missing": 0, "pulseFrom": 0.}

    def base(compact=False):
        dx, bottom = (-8, 612) if compact else (0, 688)
        image = (compact_template if compact else template).copy()
        draw = ImageDraw.Draw(image)
        draw.rectangle((96 + dx, 155, 1183 + dx, 203), fill=NAVY)
        draw.rectangle((96 + dx, 206, 1183 + dx, bottom - 9), fill=NAVY)
        draw.rectangle((900 + dx, 85, 1032 + dx, 127), fill=NAVY)
        text(image, f'GOLD {state["gold"]}', 945 + dx, 96, 18, GOLD)
        for i, name in enumerate(tab_names):
            x = 108 + dx + i * 178
            surface(image, (x, 160, x + 170, 200), SELECTED if i == 5 else NORMAL, GOLD if i == 5 else BORDER)
            text(image, name, x + 12, 171, 17, INK, "center", 146)
        return image

    # Drawing and temporal details are kept separate from the fixture's simple
    # economic model so neither can be mistaken for executing GameManager.
    def pulse_at(seconds):
        age = seconds - state["eventAt"]
        if not state["message"] or not 0 <= age < durations["ShopFeedbackDuration"]:
            return 0.
        enter = smooth(age / durations["ShopNoticeTransitionDuration"])
        return (state["pulseFrom"] + (1 - state["pulseFrom"]) * enter) * (1 - smooth((age - durations["ShopPulseHoldEnd"]) / durations["ShopPulseExitDuration"]))

    def render(seconds, compact=False):
        dx, bottom, card_height = (-8, 612, 48) if compact else (0, 688, 58)
        image = base(compact)
        text(image, f'SHOP     GOLD: {state["gold"]}     Shop Tier: 3', 108 + dx, 214, 20, GOLD)
        age = seconds - state["eventAt"]
        pulse = pulse_at(seconds)
        active = bool(state["message"]) and 0 <= age < durations["ShopFeedbackDuration"]
        accent = (128, 220, 173) if state["successful"] else (245, 153, 152)
        if state["goldSpent"] and pulse:
            text(image, f'-{state["goldSpent"]}G', 932 + dx, 214, 20, mix(NAVY, (245, 153, 152), pulse), "right", 240)
        for index, item_id in enumerate(stock):
            item = items[item_id]
            affordable = state["gold"] >= item["price"]
            x = 108 + dx + (index % 2) * 538
            y = 262 + (index // 2) * (card_height + 8)
            rect = (x, y, x + 526, y + card_height)
            emphasis = pulse if active and item_id == state["item"] else 0.
            fill = mix(NORMAL if affordable else (23, 27, 48), accent, emphasis * .14)
            border = mix((128, 107, 72) if affordable else (81, 73, 91), accent, emphasis * .85)
            surface(image, rect, fill, border)
            ink = INK if affordable else (178, 168, 192)
            text(image, item["name"], rect[0] + 12, rect[1] + 4, 18, ink)
            text(image, f'{item["price"]}G', rect[2] - 108, rect[1] + 4, 18, ink, "right", 96)
            details = ("[Use]" if item["kind"] == "Consumable" else "[Equip]") + f'    Bag: {state["bag"][item_id]}'
            text(image, details, rect[0] + 12, rect[3] - 22, 15, MUTED)
            if emphasis and state["countAdded"]:
                text(image, f'+{state["countAdded"]}', rect[2] - 76, rect[3] - 22, 15, mix(fill, accent, emphasis), "right", 64)
        text(image, "Click an item to buy one. More items appear as later stages are unlocked. Bag excludes equipped items.", 108 + dx, bottom - 76, 16, MUTED)
        if active:
            opacity = 1 - smooth((age - durations["ShopNoticeHoldEnd"]) / durations["ShopNoticeExitDuration"])
            if age >= durations["ShopNoticeTransitionDuration"]:
                opacity *= smooth(age / durations["ShopNoticeTransitionDuration"])
            color = mix(NAVY, accent, opacity)
            text(image, state["message"], 108 + dx, bottom - 46, 17, color)
            if state["missing"]:
                text(image, f'Need {state["missing"]}G more.', 932 + dx, bottom - 46, 17, color, "right", 240)
            if age < durations["ShopNoticeTransitionDuration"]:
                target_rect = (108 + dx, bottom - 46, 1172 + dx, bottom - 16)
                target_notice = image.crop(target_rect)
                blended = Image.blend(outgoing_notice, target_notice, smooth(age / durations["ShopNoticeTransitionDuration"]))
                image.paste(blended, target_rect[:2])
                target_notice.close(); blended.close()
        return image

    actions = [(36, 201), (120, 101), (123, 101), (210, 302), (306, 101)]
    fps, count, width, height = 30, 450, 960, 540
    args.output_dir.mkdir(parents=True, exist_ok=True)
    raw_path, trace_path = args.output_dir / "frames.bgr", args.output_dir / "frame-trace.csv"
    proofs = [54, 141, 228, 324]
    keys = sorted(set(proofs + [0, count - 1] + [frame for frame, _ in actions] + [frame + 2 for frame, _ in actions]))
    action_index = 0
    keyframes, transactions = [], []
    displayed = render(0.)
    with raw_path.open("wb") as raw, trace_path.open("w", encoding="utf-8", newline="") as trace_file:
        trace = csv.writer(trace_file)
        trace.writerow(["frame", "seconds", "modeled_gold", "modeled_sword_count", "modeled_potion_count", "attempt_item", "success"])
        for index in range(count):
            seconds = index / fps
            if action_index < len(actions) and index == actions[action_index][0]:
                _, item_id = actions[action_index]
                item = items[item_id]
                before_gold, before_count = state["gold"], state["bag"][item_id]
                previous_pulse = pulse_at(seconds) if item_id == state["item"] else 0.
                outgoing_notice.close()
                outgoing_notice = displayed.crop(notice_rect)
                success = before_gold >= item["price"]
                if success:
                    state["gold"] -= item["price"]
                    state["bag"][item_id] += 1
                state.update(item=item_id, eventAt=seconds, successful=success, goldSpent=item["price"] if success else 0,
                             countAdded=1 if success else 0, missing=max(0, item["price"] - before_gold), pulseFrom=previous_pulse)
                state["message"] = (f'Purchased {item["name"]} for {item["price"]}G.' if success else
                                    f'Cannot buy {item["name"]}. Not enough gold or invalid item.')
                transactions.append({"time": seconds, "itemId": item_id, "success": success, "goldBefore": before_gold,
                                     "goldAfter": state["gold"], "countBefore": before_count, "countAfter": state["bag"][item_id], "missingGold": state["missing"]})
                action_index += 1
            displayed.close()
            displayed = render(seconds)
            if index == 228:
                compact_image = render(seconds, True)
                compact_image.save(args.output_dir / "compact-shop-insufficient-1264x644.png")
                compact_image.close()
            scaled = displayed.resize((width, height), Image.Resampling.LANCZOS)
            raw.write(scaled.tobytes("raw", "BGR"))
            if index in keys:
                filename = f"{index:04d}-shop.png"
                scaled.save(args.output_dir / filename)
                keyframes.append({"index": index, "label": "shop-model", "file": filename, "independentModel": True})
            scaled.close()
            trace.writerow([index, seconds, state["gold"], state["bag"][201], state["bag"][101], state["item"], state["successful"]])
    assert [entry["goldAfter"] for entry in transactions] == [250, 200, 150, 150, 100]
    assert state["bag"][201] == 1 and state["bag"][101] == 3 and state["bag"][302] == 0
    assert raw_path.stat().st_size == count * width * height * 3
    assert sha256(source_image) == source_image_hash
    assert sha256(compact_source) == compact_source_hash
    source_files = [source_root / name for name in ("Maintenance_Stage.cpp", "StarlightUI.h", "GameManager.cpp", "ItemData.cpp")]
    scope = ("Independent Python/Pillow shop UI timing model, NOT native rendering, live game recording, input replay, or "
             "runtime verification. The native harness is blocked by Windows application control and was not launched. "
             "An older native screenshot supplies backdrop/chrome only. New UI is drawn from source layout/colors/data "
             "by a separate Python model; font rasterization differs from Windows GDI. The animation loop resets its fixture.")
    manifest = {"format": "bgr24", "rawFile": raw_path.name, "width": width, "height": height, "stride": width * 3,
                "fps": fps, "frameCount": count, "durationSeconds": count / fps, "rawSha256": sha256(raw_path),
                "scope": scope, "encodingScope": scope + " Encoder verification covers media integrity only, not game behavior.",
                "captureMethod": "independent-source-based-shop-ui-model", "nativeViewport": [1280, 720],
                "fixture": {"initialGold": 450, "initialInventory": [], "shopTier": 3, "stock": [items[i] for i in stock], "savedGame": False},
                "transactions": transactions, "presentationDurations": durations, "stillFrameIndices": proofs, "keyframes": keyframes,
                "retainedOldVisualSource": {"path": str(source_image), "sha256": source_image_hash, "use": "Prior backdrop/chrome only; not new-feature evidence"},
                "compactVisualSource": {"path": str(compact_source), "sha256": compact_source_hash, "use": "Prior backdrop/chrome only; not new-feature evidence"},
                "compactProof": {"file": "compact-shop-insufficient-1264x644.png", "nativeViewport": [1264, 644], "cardHeight": 48,
                                 "independentModel": True, "timeSeconds": 7.6},
                "productionSources": [{"path": str(path), "sha256": sha256(path)} for path in source_files],
                "font": {"path": str(FONT_PATH), "sha256": sha256(FONT_PATH)},
                "timeline": [{"start": 0, "scene": "shop-model-initial"}] + [{"start": entry["time"], "scene": "purchase-success" if entry["success"] else "insufficient-gold", "itemId": entry["itemId"]} for entry in transactions],
                "checks": {"mediaModelRendered": True, "modelEconomyConsistent": True, "oldSourceUnchanged": True,
                           "nativeExecutionVerified": False, "newNativeCaptures": 0}, "renderWallSeconds": time.monotonic() - started}
    (args.output_dir / "frames.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    template.close(); compact_template.close(); displayed.close(); outgoing_notice.close()
    print(json.dumps({"modelRendered": True, "runtimeVerified": False, "frames": count, "seconds": count / fps,
                      "manifest": str((args.output_dir / "frames.json").resolve())}), flush=True)


if __name__ == "__main__":
    main()
