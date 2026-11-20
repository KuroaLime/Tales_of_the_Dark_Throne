"""One continuous, independent Pillow model of the maintenance UI.

This does not execute the Windows-blocked native game or its test executable.
The shared scenario supplies the fixture and actions; production sources supply
catalogs/timing. Retained GDI pixels are backdrop/chrome only. Game state,
presentation state and drawing have separate responsibilities below.
"""
from __future__ import annotations

import argparse
import copy
import csv
import hashlib
import json
import math
import re
import struct
import time
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


NAVY, INK, GOLD, MUTED = (17, 20, 41), (241, 231, 211), (211, 180, 120), (187, 179, 201)
NORMAL, SELECTED, BORDER = (27, 32, 54), (52, 48, 71), (101, 88, 112)
GREEN, RED, HOVER = (128, 220, 173), (245, 153, 152), (58, 52, 80)
STAT_KEYS = ("MaxHP", "MaxMP", "STR", "INT", "DEF", "SPD")
PARTS = ("Weapon", "Armor", "Accessory")
TABS = ("Reward", "Party", "Inventory", "Equipment", "Skill", "Shop")
FONT_PATH = Path("C:/Windows/Fonts/malgun.ttf")


def sha256(path):
    digest = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def smooth(t):
    t = min(1., max(0., t))
    return t * t * (3 - 2 * t)


def mix(a, b, t):
    return tuple(int(x + (y - x) * t + .5) for x, y in zip(a, b))


def center(rect):
    return ((rect[0] + rect[2]) // 2, (rect[1] + rect[3]) // 2)


class Catalog:
    """Read-only source data, including the shared native-check scenario."""

    def __init__(self, root):
        self.root = root
        self.source = root / "Tower_Triumph/Tower_Triumph"
        names = ("Maintenance_Stage.cpp", "Maintenance_Stage.h", "MaintenanceFeedback.h", "UIFrameTransition.h",
                 "StarlightUI.h", "ItemData.cpp", "Character.cpp", "SkillData.cpp", "GameManager.cpp", "GameManager.h", "UMouseMark.h")
        self.sources = [self.source / name for name in names]
        texts = {path.name: path.read_text(encoding="utf-8-sig") for path in self.sources}
        self.durations = {name: float(value) for name, value in re.findall(r"constexpr double (\w+)\s*=\s*([\d.]+)", texts["Maintenance_Stage.cpp"])}
        self.roster = {}
        for i, match in enumerate(re.finditer(r'CreateRosterCharacter\(id\+\+,\s*ECharacterClass::(\w+),\s*EElement::(\w+),\s*L"([^"]+)"\)', texts["GameManager.cpp"]), 1):
            self.roster[i] = dict(id=i, kind=match[1], element=match[2], name=match[3])
        self.default_party = [int(i) + 1 for i in re.findall(r"PlayerParty.push_back\(Roster\[(\d+)\]\)", texts["GameManager.cpp"])]
        self.maximum = int(re.search(r"MAX_ACTIVE_PARTY\s*=\s*(\d+)", texts["GameManager.h"])[1])
        removal = re.search(r"bool GameManager::RemoveActivePartyMember[\s\S]*?bool GameManager::ToggleActivePartyMember", texts["GameManager.cpp"])[0]
        self.minimum = int(re.search(r"PlayerParty.size\(\)\s*<=\s*(\d+)", removal)[1])
        self.stats = {}
        for kind in ("Warrior", "Archer", "Mage"):
            block = re.search(r"case ECharacterClass::" + kind + r":\s*newStat.MaxHP[\s\S]*?break;", texts["Character.cpp"])[0]
            self.stats[kind] = {key: int(value) for key, value in re.findall(r"newStat\.(\w+)\s*=\s*(\d+)", block)}
        self.items = {}
        pattern = r'SItemData\(\s*(\d+),\s*L"([^"]+)",\s*L"([^"]+)",\s*EGameItemType::(\w+),\s*EEquipPart::(\w+),\s*(\d+),\s*(\d+),\s*(\d+),\s*(?:true|false),\s*SItemStatBonus\(([^)]*)\)'
        for m in re.finditer(pattern, texts["ItemData.cpp"]):
            self.items[int(m[1])] = dict(id=int(m[1]), name=m[2], kind=m[4], part=m[5], price=int(m[6]),
                                        hp=int(m[7]), mp=int(m[8]), bonus=[int(x.strip()) for x in m[9].split(",")] if m[9].strip() else [0] * 6)
        self.skills = {}
        pattern = r'v.push_back\(\{ (\d+), L"([^"]+)", L"[^"]+", ECharacterClass::(\w+), EElement::(\w+), ESkillAreaType::\w+, ESkillStatType::\w+, (\d+), (\d+), \d+, \d+, RGB\(([^)]+)\)'
        for m in re.finditer(pattern, texts["SkillData.cpp"]):
            self.skills[int(m[1])] = dict(id=int(m[1]), name=m[2], kind=m[3], element=m[4], range=int(m[5]), mp=int(m[6]),
                                         color=tuple(int(x.strip()) for x in m[7].split(",")))
        stock = re.search(r"std::vector<int> Maintenance_Stage::GetShopItemIDs\(\) const[\s\S]*?return items;", texts["Maintenance_Stage.cpp"])[0]
        self.stock = [int(x) for x in re.findall(r"items.push_back\((\d+)\)", stock)]
        self.tier = int(re.search(r"MaxUnlockedBattleStageId\((\d+)\)", texts["GameManager.cpp"])[1])
        self.scenario_path = root / "tests/maintenance_full_flow_scenario.inc"
        scenario = self.scenario_path.read_text(encoding="utf-8-sig")
        config = re.search(r"FLOW_CONFIG\(([^)]+)\)", scenario)[1].split(",")
        self.duration, self.fps, self.update_hz = float(config[0]), int(config[1]), int(config[2])
        self.gold = int(re.search(r"FLOW_GOLD\((\d+)\)", scenario)[1])
        self.initial_bag = {int(a): int(b) for a, b in re.findall(r"FLOW_ITEM\((\d+),\s*(\d+)\)", scenario)}
        self.resources = [(int(a), int(b), int(c)) for a, b, c in re.findall(r"FLOW_RESOURCES\((\d+),\s*(\d+),\s*(\d+)\)", scenario)]
        self.events = [dict(time=float(a), action=b, argument=int(c)) for a, b, c in re.findall(r"FLOW_EVENT\(([\d.]+),\s*(\w+),\s*(\d+)\)", scenario)]
        self.final_party = [int(x.strip()) for x in re.search(r"FLOW_FINAL_PARTY\(([^)]+)\)", scenario)[1].split(",")]
        self.final = [int(x.strip()) for x in re.search(r"FLOW_FINAL\(([^)]+)\)", scenario)[1].split(",")]
        self.source_hashes = [{"path": str(path), "sha256": sha256(path)} for path in self.sources + [self.scenario_path]]
        assert len(self.roster) == 15 and len(self.items) == 10 and self.default_party == [1, 2, 8, 11, 14]
        assert all(abs(round(e["time"] * self.fps) / self.fps - e["time"]) < 1e-8 for e in self.events)


class GameState:
    """Single persistent fixture. Only this object mutates gameplay-model data."""

    def __init__(self, catalog):
        self.catalog = catalog
        self.party = catalog.default_party.copy()
        self.selected = 0
        self.slot = 0
        self.tab = 1
        self.gold = catalog.gold
        self.bag = catalog.initial_bag.copy()  # Insertion order matches the real vector.
        self.characters = {}
        for cid, info in catalog.roster.items():
            learned = [sid for sid, skill in catalog.skills.items() if (skill["kind"], skill["element"]) == (info["kind"], info["element"])]
            self.characters[cid] = dict(stats=catalog.stats[info["kind"]].copy(), equipment=[-1] * 3,
                                        learned=learned, skills=learned[:3])
        for cid, hp, mp in catalog.resources:
            self.characters[cid]["stats"].update(CurHP=hp, CurMP=mp)

    @property
    def member(self):
        return self.party[self.selected]

    @property
    def character(self):
        return self.characters[self.member]

    def add_item(self, item, count):
        self.bag[item] = self.bag.get(item, 0) + count
        if not self.bag[item]:
            del self.bag[item]

    def apply_bonus(self, item, sign):
        stat = self.character["stats"]
        bonus = self.catalog.items[item]["bonus"]
        for key, amount in zip(STAT_KEYS, bonus):
            stat[key] += sign * amount
        stat["MaxHP"] = max(1, stat["MaxHP"])
        stat["MaxMP"] = max(0, stat["MaxMP"])
        stat["CurHP"] = max(1, min(stat["MaxHP"], stat["CurHP"] + sign * bonus[0]))
        stat["CurMP"] = max(0, min(stat["MaxMP"], stat["CurMP"] + sign * bonus[1]))

    def apply(self, event):
        action, argument = event["action"], event["argument"]
        result = dict(event, before=self.snapshot(), member=self.member, success=True)
        if action == "Roster":
            assert self.tab == 1
            was_active = argument in self.party
            success = len(self.party) > self.catalog.minimum if was_active else len(self.party) < self.catalog.maximum
            if success:
                if was_active:
                    self.party.remove(argument)
                else:
                    self.party.append(argument)
                    self.selected = len(self.party) - 1
                self.selected = min(self.selected, len(self.party) - 1)
            name = self.catalog.roster[argument]["name"]
            message = (("Removed " if was_active else "Added ") + name + (" from the active party." if was_active else " to the active party.")) if success else ("At least one active character is required." if was_active else "Active party is already full.")
            result.update(success=success, wasActive=was_active, countDelta=len(self.party) - len(result["before"]["party"]), message=message)
        elif action == "Tab":
            self.tab = argument
        elif action == "Member":
            assert self.tab in (2, 3, 4) and argument in self.party
            self.selected = self.party.index(argument)
        elif action == "InventoryItem":
            assert self.tab == 2 and self.bag.get(argument, 0) > 0, "Only a visible, owned inventory row may be clicked"
            item = self.catalog.items[argument]
            before = self.character["stats"].copy()
            if item["kind"] != "Consumable":
                part = PARTS.index(item["part"])
                old = self.character["equipment"][part]
                self.add_item(argument, -1)
                if old >= 0:
                    self.apply_bonus(old, -1)
                    self.add_item(old, 1)
                self.character["equipment"][part] = argument
                self.apply_bonus(argument, 1)
                self.tab = 3
                result.update(kind="equipment", part=part)
            else:
                stat = self.character["stats"]
                stat["CurHP"] = min(stat["MaxHP"], stat["CurHP"] + item["hp"])
                stat["CurMP"] = min(stat["MaxMP"], stat["CurMP"] + item["mp"])
                self.add_item(argument, -1)
                hp, mp = stat["CurHP"] - before["CurHP"], stat["CurMP"] - before["CurMP"]
                message = "Used " + item["name"] + "."
                if item["hp"]:
                    message += f" HP {hp:+d}"
                if item["mp"]:
                    message += f" MP {mp:+d}"
                if hp == mp == 0:
                    message += " (already full)"
                message += f". Bag: {self.bag.get(argument, 0)} (-1)."
                result.update(kind="potion", hp=hp, mp=mp, quantity=-1, remaining=self.bag.get(argument, 0), message=message)
            result.update(beforeStats=before, afterStats=self.character["stats"].copy())
        elif action == "Unequip":
            assert self.tab == 3 and self.character["equipment"][argument] >= 0
            before = self.character["stats"].copy()
            item = self.character["equipment"][argument]
            self.apply_bonus(item, -1)
            self.character["equipment"][argument] = -1
            self.add_item(item, 1)
            result.update(kind="equipment", part=argument, beforeStats=before, afterStats=self.character["stats"].copy())
        elif action == "SkillSlot":
            assert self.tab == 4 and 0 <= argument < 3
            self.slot = argument
        elif action == "LearnedSkill":
            assert self.tab == 4 and argument in self.character["learned"]
            self.character["skills"][self.slot] = argument  # Duplicates are allowed; no slot swap.
            result.update(message=f'Equipped {self.catalog.skills[argument]["name"]} to slot {self.slot + 1}.')
        elif action == "ShopItem":
            assert self.tab == 5 and argument in self.catalog.stock
            item = self.catalog.items[argument]
            success = self.gold >= item["price"]
            if success:
                self.gold -= item["price"]
                self.add_item(argument, 1)
            result.update(success=success, goldDelta=-item["price"] if success else 0, quantity=int(success),
                          missing=max(0, item["price"] - self.gold) if not success else 0,
                          message=f'Purchased {item["name"]} for {item["price"]}G.' if success else f'Cannot buy {item["name"]}. Not enough gold or invalid item.')
        else:
            raise ValueError(action)
        result["after"] = self.snapshot()
        return result

    def snapshot(self):
        return dict(party=self.party.copy(), selectedIndex=self.selected, selectedMember=self.member, selectedSlot=self.slot,
                    tab=self.tab, gold=self.gold, bag=self.bag.copy(), characters=copy.deepcopy(self.characters))

    def validate_final(self):
        gold, cid, hp, mp, armor, slot, skill = self.catalog.final
        assert self.party == self.catalog.final_party and self.gold == gold and not self.bag
        assert self.member == cid and self.slot == slot
        assert self.characters[cid]["stats"]["CurHP"] == hp and self.characters[cid]["stats"]["CurMP"] == mp
        assert self.characters[cid]["equipment"][1] == armor and self.characters[cid]["skills"][slot] == skill


class Layout:
    """Production rectangle formulas, independent of presentation or game rules."""

    def __init__(self, viewport=(1280, 720)):
        self.viewport = viewport
        width, height = min(1120, viewport[0] - 48), min(720, viewport[1] - 88)
        x, y = (viewport[0] - width) // 2, (viewport[1] - height) // 2 + 12
        self.panel = (x, y, x + width, y + height)
        self.body = (x + 16, y + 150, x + width - 16, y + height - 8)

    def card(self, kind, index, inventory_count=0):
        x, y, right, bottom = self.panel
        width, height = right - x, bottom - y
        if kind == "Tab":
            w, h, sx, sy, gap, columns = (width - 96) // 6, 40, x + 28, y + 104, 8, 6
        elif kind == "Member":
            w, h, sx, sy, gap, columns = (width - 88) // 5, 38, x + 28, y + 158, 8, 5
        elif kind == "Roster":
            w, h, sx, sy, gap, columns = (width - 376) // 3, min(64, (height - 288) // 5), x + 328, y + 190, 10, 3
        elif kind == "InventoryItem":
            columns = 4 if inventory_count > 10 else 2
            w, h, sx, sy, gap = (width - 56 - (columns - 1) * 10) // columns, min(58, (height - 342) // 5), x + 28, y + 246, 10
        elif kind == "ShopItem":
            w, h, sx, sy, gap, columns = (width - 68) // 2, min(58, (height - 314) // 5), x + 28, y + 206, 12, 2
        elif kind == "Unequip":
            return (x + width // 2 + 20, y + 246 + index * 82, right - 28, y + 316 + index * 82)
        elif kind in ("SkillSlot", "LearnedSkill"):
            w, h, sx, sy, gap, columns = (width - 80) // 3, min(86, max(64, (height - 420) // 2)), x + 28, y + 304, 12, 3
            if kind == "LearnedSkill":
                sy += h + 46 + (index // 3) * (h + 10)
                index %= 3
        else:
            raise ValueError(kind)
        sx += (index % columns) * (w + gap)
        sy += (index // columns) * (h + 8)
        return (sx, sy, sx + w, sy + h)

    def action_rect(self, state, event):
        action, arg = event["action"], event["argument"]
        if action == "Roster":
            arg -= 1
        elif action == "Member":
            arg = state.party.index(arg)
        elif action == "InventoryItem":
            arg = list(state.bag).index(arg)
        elif action == "ShopItem":
            arg = state.catalog.stock.index(arg)
        elif action == "LearnedSkill":
            arg = state.character["learned"].index(arg)
        return self.card(action, arg, len(state.bag))

    def regions(self, kind, state):
        x, y, right, bottom = self.panel
        if kind == "body":
            return [self.body]
        if kind == "character":
            return [(self.body[0], self.body[1] + 52, self.body[2], self.body[3])]
        if kind == "party":
            first = self.card("Roster", 0)
            h = first[3] - first[1]
            return [(x + 28, first[1], x + 304, first[1] + 4 * (h + 8) + h)] + [self.card("Roster", i) for i in range(15)] + [(x + 28, bottom - 34, right - 28, bottom - 10)]
        if kind == "skill":
            return [self.card("SkillSlot", i) for i in range(3)] + [self.card("LearnedSkill", i) for i in range(len(state.character["learned"]))] + [(x + 28, bottom - 51, right - 28, bottom - 17)]
        if kind == "shop":
            return [(x + 28, bottom - 46, right - 28, bottom - 16)]
        return [(x + 28, bottom - 39, right - 28, bottom - 12)]


class Presentation:
    """Independent model of the extracted C++ feedback objects and snapshots."""

    def __init__(self, catalog):
        self.catalog = catalog
        self.body = None
        self.local = None
        self.notice = None
        self.equipment = None
        self.skill_pulse = None
        self.skill_message = ""
        self.skill_message_start = -100.
        self.displayed_bars = None
        self.bar_target = None
        self.bar_from = (0., 0.)

    def pulse(self, seconds):
        if not self.notice:
            return 0.
        age = seconds - self.notice["time"]
        prefix = self.notice["prefix"]
        d = self.catalog.durations
        if not 0 <= age < d[prefix + "FeedbackDuration"]:
            return 0.
        enter = d["PartyPresentationDuration" if prefix == "Party" else prefix + "NoticeTransitionDuration"]
        return (self.notice["pulseFrom"] + (1 - self.notice["pulseFrom"]) * smooth(age / enter)) * (1 - smooth((age - d[prefix + "PulseHoldEnd"]) / d[prefix + "PulseExitDuration"]))

    def clear_actions(self):
        self.notice = self.equipment = self.skill_pulse = None
        self.displayed_bars = self.bar_target = None
        if self.local:
            self.local["image"].close()
        self.local = None

    def transition(self, which, displayed, kind, seconds, duration):
        previous = getattr(self, which)
        if previous:
            previous["image"].close()
        setattr(self, which, dict(image=displayed.copy(), kind=kind, time=seconds, duration=duration))

    def record(self, result, displayed):
        action, seconds, d = result["action"], result["time"], self.catalog.durations
        before, after = result["before"], result["after"]
        changed_tab = before["tab"] != after["tab"]
        changed_member = before["selectedIndex"] != after["selectedIndex"] and action == "Member"
        if changed_tab or changed_member:
            if changed_tab or before["tab"] in (3, 4):
                self.transition("body", displayed, "body" if changed_tab else "character", seconds,
                                d["TabPresentationDuration" if changed_tab else "CharacterPresentationDuration"])
            self.clear_actions()
        if result.get("kind") == "equipment":
            old = self.equipment
            baseline = old["beforeStats"] if old and old["member"] == result["member"] and seconds - old["time"] < d["EquipmentFeedbackDuration"] else result["beforeStats"]
            self.equipment = dict(result, beforeStats=baseline)
        if action in ("Roster", "ShopItem") or result.get("kind") == "potion":
            prefix = "Party" if action == "Roster" else "Shop" if action == "ShopItem" else "Inventory"
            identity = (prefix, result["argument"], result["member"] if prefix == "Inventory" else 0)
            previous = self.pulse(seconds) if self.notice and self.notice["identity"] == identity else 0.
            self.notice = dict(result, prefix=prefix, identity=identity, pulseFrom=previous)
            if prefix == "Inventory":
                s = result["beforeStats"]
                self.bar_from = self.displayed_bars if self.bar_target == result["member"] else (s["CurHP"] / s["MaxHP"], s["CurMP"] / s["MaxMP"] if s["MaxMP"] else 0.)
            duration = d["PartyPresentationDuration" if prefix == "Party" else prefix + "NoticeTransitionDuration"]
            self.transition("local", displayed, prefix.lower(), seconds, duration)
        if action in ("SkillSlot", "LearnedSkill"):
            self.transition("local", displayed, "skill", seconds, d["SkillSelectionDuration" if action == "SkillSlot" else "SkillReplacementDuration"])
            if action == "LearnedSkill":
                self.skill_pulse = dict(time=seconds, slot=after["selectedSlot"], skill=result["argument"])
                self.skill_message, self.skill_message_start = result["message"], seconds

    def compose(self, image, state, layout, seconds):
        for name in ("local", "body"):
            transition = getattr(self, name)
            if not transition:
                continue
            progress = (seconds - transition["time"]) / transition["duration"]
            if progress >= 1:
                transition["image"].close()
                setattr(self, name, None)
                continue
            for rect in layout.regions(transition["kind"], state):
                old, new = transition["image"].crop(rect), image.crop(rect)
                blended = Image.blend(old, new, smooth(progress))
                image.paste(blended, rect[:2])
                old.close(); new.close(); blended.close()

    def close(self):
        for transition in (self.body, self.local):
            if transition:
                transition["image"].close()


class Renderer:
    """Draw current model data using production geometry and unmodified assets."""

    def __init__(self, catalog):
        self.catalog = catalog
        self.fonts = {size: ImageFont.truetype(str(FONT_PATH), size) for size in range(12, 24)}
        self.templates, self.portraits, self.visual_sources = {}, {}, []
        for viewport in ((1280, 720), (1264, 644)):
            path = catalog.root / f"tests/bin/maintenance-map-ui-checks/maintenance-party-{viewport[0]}x{viewport[1]}.bmp"
            with Image.open(path) as opened:
                self.templates[viewport] = opened.convert("RGB")
            self.visual_sources.append(dict(path=str(path), sha256=sha256(path), use="Prior GDI backdrop/chrome only; not new execution evidence"))
        for kind in ("Warrior", "Archer", "Mage"):
            path = catalog.source / f"Resource/{kind}_Portrait.bmp"
            with Image.open(path) as opened:
                image = opened.convert("RGBA")
            image.putdata([(r, g, b, 0 if (r, g, b) == (255, 0, 255) else 255) for r, g, b, _ in image.get_flattened_data()])
            self.portraits[kind] = image
            self.visual_sources.append(dict(path=str(path), sha256=sha256(path), use="Unchanged approved runtime portrait, magenta color key"))
        path = catalog.source / "Source/MouseCursor/MouseCusor.bmp"
        data = path.read_bytes()
        offset = struct.unpack_from("<I", data, 10)[0]
        width, height = struct.unpack_from("<ii", data, 18)
        assert width == 40 and abs(height) == 40 and struct.unpack_from("<H", data, 28)[0] == 32
        # Bitmap bytes are premultiplied BGRA; Pillow composites straight RGBA.
        pixels = []
        for y in range(40):
            row = y if height < 0 else 39 - y
            for x in range(40):
                b, g, r, a = data[offset + (row * 40 + x) * 4:offset + (row * 40 + x + 1) * 4]
                pixels.append(tuple(min(255, round(c * 255 / a)) if a else 0 for c in (r, g, b)) + (a,))
        self.quill = Image.new("RGBA", (40, 40)); self.quill.putdata(pixels)
        self.visual_sources.append(dict(path=str(path), sha256=sha256(path), use="Actual 40px runtime quill; alpha interpretation only, not edited artwork"))
        self.pointer = (-1000, -1000)

    def text(self, image, value, x, y, size=18, color=INK, align="left", width=0):
        draw = ImageDraw.Draw(image)
        if align != "left":
            x += (width - draw.textlength(value, font=self.fonts[size])) / (2 if align == "center" else 1)
        draw.text((x, y), value, font=self.fonts[size], fill=color, anchor="lt")

    def hovered(self, rect):
        return rect[0] <= self.pointer[0] < rect[2] and rect[1] <= self.pointer[1] < rect[3]

    def surface(self, image, rect, fill=NORMAL, border=BORDER):
        ImageDraw.Draw(image).rounded_rectangle((rect[0], rect[1], rect[2] - 1, rect[3] - 1), radius=3, fill=fill, outline=border)

    def box(self, image, rect, value, size=18, fill=NORMAL, border=BORDER, centered=False):
        hovered = self.hovered(rect)
        self.surface(image, rect, HOVER if hovered else fill, GOLD if hovered else border)
        lines = []
        for line in value.split("\n"):
            current = ""
            for word in line.split(" "):
                candidate = current + (" " if current else "") + word
                if current and self.fonts[size].getlength(candidate) > rect[2] - rect[0] - 24:
                    lines.append(current); current = word
                else:
                    current = candidate
            lines.append(current)
        y = rect[1] + (rect[3] - rect[1] - len(lines) * (size + 4)) / 2
        for line in lines:
            self.text(image, line, rect[0] + 12, y, size, INK, "center" if centered else "left", rect[2] - rect[0] - 24)
            y += size + 4

    def portrait(self, image, member, bounds):
        source = self.portraits[self.catalog.roster[member]["kind"]]
        scale = min((bounds[2] - bounds[0]) / source.width, (bounds[3] - bounds[1]) / source.height)
        resized = source.resize((max(1, int(source.width * scale)), max(1, int(source.height * scale))), Image.Resampling.NEAREST)
        image.paste(resized, ((bounds[0] + bounds[2] - resized.width) // 2, (bounds[1] + bounds[3] - resized.height) // 2), resized)
        resized.close()

    def draw(self, state, presentation, seconds, layout):
        image = self.templates[layout.viewport].copy()
        x, y, right, bottom = layout.panel
        draw = ImageDraw.Draw(image)
        draw.rectangle((x + 16, y + 99, right - 17, y + 147), fill=NAVY)
        draw.rectangle((layout.body[0], layout.body[1], layout.body[2] - 1, layout.body[3] - 1), fill=NAVY)
        draw.rectangle((right - 356, y + 30, right - 176, y + 70), fill=NAVY)
        self.text(image, f"GOLD {state.gold}", right - 356, y + 42, 19, GOLD, "right", 180)
        for i, label in enumerate(("1. 보상", "2. 파티", "3. 인벤토리", "4. 장비", "5. 스킬", "6. 상점")):
            self.box(image, layout.card("Tab", i), label, 18, SELECTED if i == state.tab else NORMAL, GOLD if i == state.tab else BORDER, True)
        if state.tab in (2, 3, 4):
            for i, member in enumerate(state.party):
                self.box(image, layout.card("Member", i), self.catalog.roster[member]["name"], 17,
                         SELECTED if i == state.selected else NORMAL, GOLD if i == state.selected else BORDER, True)
        methods = {1: self.party, 2: self.inventory, 3: self.equipment, 4: self.skill, 5: self.shop}
        methods[state.tab](image, state, presentation, seconds, layout)
        return image

    def notice_values(self, presentation, seconds, prefix):
        notice = presentation.notice
        if not notice or notice["prefix"] != prefix or not 0 <= seconds - notice["time"] < self.catalog.durations[prefix + "FeedbackDuration"]:
            return None, 0., 0.
        age = seconds - notice["time"]
        opacity = 1 - smooth((age - self.catalog.durations[prefix + "NoticeHoldEnd"]) / self.catalog.durations[prefix + "NoticeExitDuration"])
        return notice, presentation.pulse(seconds), opacity

    def party(self, image, state, p, seconds, layout):
        x, y, right, bottom = layout.panel
        notice, pulse, opacity = self.notice_values(p, seconds, "Party")
        accent = (GOLD if notice["wasActive"] else GREEN) if notice and notice["success"] else RED
        self.text(image, f"ACTIVE PARTY  {len(state.party)} / {self.catalog.maximum}", x + 28, y + 158, 18, GOLD)
        if notice and notice["countDelta"]:
            self.text(image, f'{notice["countDelta"]:+d}', x + 244, y + 158, 18, mix(NAVY, accent, pulse), "right", 60)
        self.text(image, "ROSTER", x + 328, y + 158, 18, GOLD)
        first = layout.card("Roster", 0); h = first[3] - first[1]
        for i, member in enumerate(state.party):
            top = y + 190 + i * (h + 8)
            self.portrait(image, member, (x + 28, top + 2, x + 64, top + h - 2))
            self.text(image, f'{i + 1}. {self.catalog.roster[member]["name"]}', x + 74, top + 4, 17)
            self.text(image, "/ " + self.catalog.roster[member]["element"], x + 74, top + 29, 15, (188, 167, 203))
        for i, (member, info) in enumerate(self.catalog.roster.items()):
            rect = layout.card("Roster", i); active = member in state.party; hovered = self.hovered(rect)
            emphasis = pulse if notice and notice["argument"] == member else 0.
            self.surface(image, rect, mix(HOVER if hovered else SELECTED if active else NORMAL, accent, emphasis * .14),
                         mix(GOLD if hovered or active else BORDER, accent, emphasis * .85))
            self.portrait(image, member, (rect[0] + 7, rect[1] + 5, rect[0] + 43, rect[3] - 5))
            self.text(image, info["name"], rect[0] + 52, rect[1] + 7, 16)
            if active:
                self.text(image, "ACTIVE", rect[0] + 52, rect[1] + 31, 13, (188, 167, 203))
        self.text(image, "Click a roster character to add/remove. At least 1 and up to 5 characters can be active.", x + 28, bottom - 57, 14, MUTED)
        if notice:
            self.text(image, notice["message"], x + 28, bottom - 34, 17, mix(NAVY, accent, opacity))

    def equipment(self, image, state, p, seconds, layout):
        x, y, right, bottom = layout.panel
        stat, gear = state.character["stats"], state.character["equipment"]
        self.portrait(image, state.member, (x + 28, y + 210, x + 106, y + 305))
        self.text(image, self.catalog.roster[state.member]["name"], x + 124, y + 221, 23)
        self.text(image, "Current Stats", x + 124, y + 264, 18, GOLD)
        event = p.equipment
        age = seconds - event["time"] if event else 100.
        active = event and event["member"] == state.member and age < self.catalog.durations["EquipmentFeedbackDuration"]
        strength = smooth(age / .12) * (1 - smooth((age - 1.1) / .5)) if active else 0.
        w, h = ((right - x) // 2 - 68) // 2, min(54, (bottom - y - 406) // 3)
        values = [f'{stat["CurHP"]} / {stat["MaxHP"]}', f'{stat["CurMP"]} / {stat["MaxMP"]}'] + [str(stat[key]) for key in STAT_KEYS[2:]]
        for i, label in enumerate(("HP", "MP", "STR", "INT", "DEF", "SPD")):
            left, top = x + 28 + (i % 2) * (w + 10), y + 320 + (i // 2) * (h + 8)
            delta = event["afterStats"][STAT_KEYS[i]] - event["beforeStats"][STAT_KEYS[i]] if active else 0
            suffix = " MAX" if i < 2 else ""
            if active and not delta and i < 2:
                key = ("CurHP", "CurMP")[i]
                delta = event["afterStats"][key] - event["beforeStats"][key]; suffix = " NOW"
            accent = GREEN if delta > 0 else RED
            fill = mix((26, 28, 52), accent, strength * .13 if delta else 0.)
            self.surface(image, (left, top, left + w, top + h), fill, mix((100, 84, 70), accent, strength * .8 if delta else 0.))
            self.text(image, label, left + 10, top + 3, 13, MUTED)
            self.text(image, values[i], left + 10, top + 20, 18, GOLD)
            if delta:
                self.text(image, f"{delta:+d}" + suffix, left + 45, top + 3, 13, mix(fill, accent, strength), "right", w - 55)
        self.text(image, "Equipped Items", layout.card("Unequip", 0)[0], y + 210, 20, GOLD)
        for i, item in enumerate(gear):
            label = PARTS[i] + ": " + (self.catalog.items[item]["name"] + "\n[Click to Unequip]" if item >= 0 else "Empty")
            emphasis = strength if active and event["part"] == i else 0.
            self.box(image, layout.card("Unequip", i), label, 18, mix(NORMAL, (70, 59, 65), emphasis),
                     mix(GOLD if item >= 0 else BORDER, (246, 215, 157), emphasis))
        self.text(image, "Use the Inventory tab to equip new items. Click an equipped slot here to unequip it.", x + 28, bottom - 52, 17, MUTED)

    def skill(self, image, state, p, seconds, layout):
        x, y, right, bottom = layout.panel
        info = self.catalog.roster[state.member]
        self.portrait(image, state.member, (x + 28, y + 207, x + 82, y + 275))
        self.text(image, "SKILL SETTING", x + 96, y + 210, 15, GOLD)
        self.text(image, info["name"] + " / " + info["element"], x + 96, y + 239, 21)
        self.text(image, "Equipped Skills", x + 28, y + 276, 17, GOLD)
        pulse = 1 - smooth((seconds - p.skill_pulse["time"]) / self.catalog.durations["SkillPulseDuration"]) if p.skill_pulse else 0.
        for i, sid in enumerate(state.character["skills"]):
            skill = self.catalog.skills[sid]
            emphasis = pulse if p.skill_pulse and p.skill_pulse["slot"] == i else 0.
            self.box(image, layout.card("SkillSlot", i), f'Slot {i + 1}: {skill["name"]}\nMP {skill["mp"]}', 17,
                     mix(SELECTED if i == state.slot else NORMAL, (85, 69, 69), emphasis), mix(GOLD if i == state.slot else BORDER, (255, 229, 171), emphasis))
        self.text(image, "Learned Skills - click a skill to equip it into the selected slot.", x + 28, layout.card("LearnedSkill", 0)[1] - 35, 17, MUTED)
        for i, sid in enumerate(state.character["learned"]):
            skill = self.catalog.skills[sid]
            emphasis = pulse if p.skill_pulse and p.skill_pulse["skill"] == sid else 0.
            selected = sid == state.character["skills"][state.slot]
            self.box(image, layout.card("LearnedSkill", i), f'{skill["name"]}\nMP {skill["mp"]} / R{skill["range"]}', 18,
                     mix(SELECTED if selected else NORMAL, (85, 69, 69), emphasis), mix(GOLD if selected else skill["color"], (255, 229, 171), emphasis))
        elapsed_updates = int(round((seconds - p.skill_message_start) * self.catalog.update_hz))
        if p.skill_message and elapsed_updates < 180:
            self.text(image, p.skill_message, x + 28, bottom - 51, 18, GOLD)

    def shop(self, image, state, p, seconds, layout):
        x, y, right, bottom = layout.panel
        notice, pulse, opacity = self.notice_values(p, seconds, "Shop")
        accent = GREEN if notice and notice["success"] else RED
        self.text(image, f"SHOP     GOLD: {state.gold}     Shop Tier: {self.catalog.tier}", x + 28, y + 158, 20, GOLD)
        if notice and notice["goldDelta"]:
            self.text(image, f'{notice["goldDelta"]}G', right - 268, y + 158, 20, mix(NAVY, RED, pulse), "right", 240)
        for i, iid in enumerate(self.catalog.stock):
            item, rect = self.catalog.items[iid], layout.card("ShopItem", i)
            affordable, hovered = state.gold >= item["price"], self.hovered(rect)
            emphasis = pulse if notice and notice["argument"] == iid else 0.
            fill = mix(HOVER if hovered else NORMAL if affordable else (23, 27, 48), accent, emphasis * .14)
            border = mix(GOLD if hovered else (128, 107, 72) if affordable else (81, 73, 91), accent, emphasis * .85)
            self.surface(image, rect, fill, border)
            color = INK if affordable else (178, 168, 192)
            self.text(image, item["name"], rect[0] + 12, rect[1] + 4, 18, color)
            self.text(image, f'{item["price"]}G', rect[2] - 108, rect[1] + 4, 18, color, "right", 96)
            self.text(image, ("[Use]" if item["kind"] == "Consumable" else "[Equip]") + f'    Bag: {state.bag.get(iid, 0)}', rect[0] + 12, rect[3] - 22, 15, MUTED)
            if emphasis and notice["quantity"]:
                self.text(image, f'+{notice["quantity"]}', rect[2] - 76, rect[3] - 22, 15, mix(fill, accent, emphasis), "right", 64)
        self.text(image, "Click an item to buy one. More items appear as later stages are unlocked. Bag excludes equipped items.", x + 28, bottom - 76, 16, MUTED)
        if notice:
            color = mix(NAVY, accent, opacity)
            self.text(image, notice["message"], x + 28, bottom - 46, 17, color)
            if notice["missing"]:
                self.text(image, f'Need {notice["missing"]}G more.', right - 268, bottom - 46, 17, color, "right", 240)

    def inventory(self, image, state, p, seconds, layout):
        x, y, right, bottom = layout.panel
        notice, pulse, opacity = self.notice_values(p, seconds, "Inventory")
        no_recovery = notice and notice["hp"] == notice["mp"] == 0
        accent = GOLD if no_recovery else GREEN
        status_w = min(220, (right - x - 56) // 4); hp_left = right - 28 - 2 * status_w - 16
        self.text(image, f'INVENTORY     GOLD: {state.gold}     Target: {self.catalog.roster[state.member]["name"]}', x + 28, y + 207, 17, GOLD)
        stat = state.character["stats"]
        displayed = []
        for i, resource in enumerate(("HP", "MP")):
            left = hp_left + i * (status_w + 16)
            color = GREEN if i == 0 else (139, 181, 247)
            current, maximum = stat["Cur" + resource], stat["Max" + resource]
            self.text(image, f"{resource} {current} / {maximum}", left, y + 207, 17)
            relevant = notice and self.catalog.items[notice["argument"]][resource.lower()] > 0
            if relevant:
                self.text(image, f'{notice[resource.lower()]:+d}', left + status_w - 48, y + 207, 15, mix(NAVY, GOLD if no_recovery else color, pulse), "right", 48)
            actual = current / maximum if maximum else 0.
            progress = smooth((seconds - notice["time"]) / self.catalog.durations["InventoryBarDuration"]) if notice else 1.
            displayed.append(p.bar_from[i] + (actual - p.bar_from[i]) * progress)
            draw = ImageDraw.Draw(image)
            draw.rectangle((left, y + 233, left + status_w - 1, y + 236), fill=(42, 43, 65))
            if displayed[i] > 0:
                draw.rectangle((left, y + 233, left + int(status_w * displayed[i] + .5) - 1, y + 236), fill=mix(color, INK, pulse * .25 if relevant else 0.))
        if layout.viewport == (1280, 720):
            p.displayed_bars, p.bar_target = tuple(displayed), state.member
        if not state.bag:
            self.text(image, "No Items Found.", x + 28, y + 270, 20, MUTED)
        for i, (iid, quantity) in enumerate(state.bag.items()):
            rect, item = layout.card("InventoryItem", i, len(state.bag)), self.catalog.items[iid]
            compact, hovered = rect[3] - rect[1] < 50, self.hovered(rect)
            emphasis = pulse if notice and notice["argument"] == iid else 0.
            fill = mix(HOVER if hovered else NORMAL, accent, emphasis * .14)
            self.surface(image, rect, fill, mix(GOLD if hovered else BORDER, accent, emphasis * .85))
            self.text(image, item["name"], rect[0] + 12, rect[1] + 3, 15 if len(state.bag) > 10 else 16 if compact else 17)
            top = rect[1] + 24 if compact else rect[3] - 22
            self.text(image, f"x{quantity}  " + ("[Use]" if item["kind"] == "Consumable" else "[Equip]"), rect[0] + 12, top, 12 if compact else 14, MUTED)
            if emphasis:
                self.text(image, "-1", rect[2] - 50, top, 12 if compact else 14, mix(fill, accent, emphasis), "right", 38)
        self.text(image, "Click equipment to equip it to the selected character. Click potion to use it.", x + 28, bottom - 62, 14, MUTED)
        if notice:
            self.text(image, notice["message"], x + 28, bottom - 39, 16, mix(NAVY, accent, opacity))

    def cursor(self, image, seconds, point):
        surface = Image.new("RGBA", (48, 48))
        surface.paste(self.quill, (4, 4))
        angle = 3 * math.sin(seconds * 2 * math.pi / 4.8)
        rotated = surface.rotate(-angle, Image.Resampling.BICUBIC, center=(6, 41))
        image.paste(rotated, (round(point[0]) - 6, round(point[1]) - 41), rotated)
        rotated.close(); surface.close()

    def close(self):
        for image in list(self.templates.values()) + list(self.portraits.values()) + [self.quill]:
            image.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", type=Path, default=Path("tests/bin/maintenance-full-flow-model"))
    parser.add_argument("--state-only", action="store_true", help="Validate the shared scenario without rendering media")
    args = parser.parse_args()
    started = time.monotonic()
    root = Path(__file__).resolve().parent.parent
    catalog = Catalog(root)
    if args.state_only:
        state = GameState(catalog)
        results = [state.apply(event) for event in catalog.events]
        state.validate_final()
        print(json.dumps(dict(modelStateValidated=True, nativeExecutionVerified=False, events=len(results),
                              finalState=state.snapshot(), inventoryOrders=[list(result["after"]["bag"]) for result in results])), flush=True)
        return
    state, presentation, renderer = GameState(catalog), Presentation(catalog), Renderer(catalog)
    layout = Layout()
    initial = state.snapshot()
    args.output_dir.mkdir(parents=True, exist_ok=True)
    width, height, count = 960, 540, round(catalog.duration * catalog.fps)
    raw_path, trace_path = args.output_dir / "frames.bgr", args.output_dir / "frame-trace.csv"
    proof_times = (1.6, 7.8, 14.2, 22.2, 28.4, 31.8, 34.0, 38.0)
    proofs = [round(t * catalog.fps) for t in proof_times]
    keys = sorted(set(proofs + [0, count - 1] + [round(e["time"] * catalog.fps) + offset for e in catalog.events for offset in (0, 2)]))
    displayed = renderer.draw(state, presentation, 0., layout)
    event_index, transactions, keyframes, native_proofs, compact_proofs = 0, [], [], [], []
    last_point, last_click_time = (1145, 710), 0.
    with raw_path.open("wb") as raw, trace_path.open("w", encoding="utf-8", newline="") as trace_file:
        trace = csv.writer(trace_file)
        trace.writerow(("frame", "seconds", "tab", "party", "selected_member", "selected_slot", "gold", "bag", "warrior_hp", "warrior_mp", "warrior_armor", "warrior_skills"))
        for index in range(count):
            seconds = index / catalog.fps
            if event_index < len(catalog.events) and index == round(catalog.events[event_index]["time"] * catalog.fps):
                event = catalog.events[event_index]
                last_point = center(layout.action_rect(state, event)); last_click_time = seconds
                result = state.apply(event)
                presentation.record(result, displayed)
                transactions.append(result)
                event_index += 1
            point = last_point
            if event_index < len(catalog.events):
                upcoming = catalog.events[event_index]
                target = center(layout.action_rect(state, upcoming))
                start = max(last_click_time, upcoming["time"] - .45)
                amount = smooth((seconds - start) / max(.001, upcoming["time"] - start))
                point = tuple(a + (b - a) * amount for a, b in zip(last_point, target))
            renderer.pointer = point
            displayed.close()
            displayed = renderer.draw(state, presentation, seconds, layout)
            presentation.compose(displayed, state, layout, seconds)
            # Snapshot cache never contains the pointer, matching the production stage.
            output = displayed.copy(); renderer.cursor(output, seconds, point)
            if index in proofs:
                name = f"native-size-{index:04d}-{TABS[state.tab].lower()}.png"
                output.save(args.output_dir / name)
                native_proofs.append(dict(file=name, index=index, viewport=[1280, 720], independentModel=True))
            if index in (round(22.2 * catalog.fps), round(31.8 * catalog.fps)):
                compact = renderer.draw(state, presentation, seconds, Layout((1264, 644)))
                name = f"compact-{TABS[state.tab].lower()}-1264x644.png"
                compact.save(args.output_dir / name); compact.close()
                compact_proofs.append(dict(file=name, index=index, independentModel=True))
            scaled = output.resize((width, height), Image.Resampling.LANCZOS)
            raw.write(scaled.tobytes("raw", "BGR"))
            if index in keys:
                name = f"{index:04d}-{TABS[state.tab].lower()}.png"
                scaled.save(args.output_dir / name)
                keyframes.append(dict(index=index, file=name, label=TABS[state.tab] + " continuous independent model", independentModel=True))
            scaled.close(); output.close()
            warrior = state.characters[1]
            trace.writerow((index, seconds, TABS[state.tab], state.party, state.member, state.slot, state.gold, state.bag,
                            warrior["stats"]["CurHP"], warrior["stats"]["CurMP"], warrior["equipment"][1], warrior["skills"]))
    state.validate_final()
    assert event_index == len(catalog.events) and raw_path.stat().st_size == count * width * height * 3
    assert all(sha256(record["path"]) == record["sha256"] for record in catalog.source_hashes + renderer.visual_sources)
    scope = ("Independent source-based Python/Pillow UI timing model of ONE continuous fixture, not a native renderer, live game recording, input replay or runtime verification. "
             "No game/test executable was launched; no policy bypass or saved-game access. Existing GDI pixels supply backdrop/chrome only. "
             "Runtime portraits/quill remain unchanged. Gameplay data is modeled separately from presentation and rendering; all events share one persistent state. "
             "Skill notices model 180 Update ticks at the shared scenario's 60Hz, independently of 30fps media. Loop boundary restarts only the demonstration fixture.")
    manifest = dict(format="bgr24", rawFile=raw_path.name, width=width, height=height, stride=width * 3, fps=catalog.fps,
                    frameCount=count, durationSeconds=count / catalog.fps, rawSha256=sha256(raw_path), scope=scope,
                    encodingScope=scope + " Media verification is not game behavior verification.",
                    captureMethod="independent-source-based-continuous-maintenance-ui-model", nativeExecutionVerified=False,
                    modeledViewport=[1280, 720], fixture=initial, finalState=state.snapshot(), transactions=transactions,
                    timeline=[dict(start=0., scene="Initial party fixture")] + [dict(start=e["time"], scene=e["action"] + " " + str(e["argument"])) for e in catalog.events],
                    sharedScenario=dict(path=str(catalog.scenario_path), sha256=sha256(catalog.scenario_path)),
                    updateTicksPerSecond=catalog.update_hz, presentationDurations=catalog.durations,
                    keyframes=keyframes, stillFrameIndices=proofs, nativeSizeModelProofs=native_proofs, compactProofs=compact_proofs,
                    visualSources=renderer.visual_sources, productionSources=catalog.source_hashes,
                    font=dict(path=str(FONT_PATH), sha256=sha256(FONT_PATH)),
                    checks=dict(singleStateContinuity=True, finalFixtureMatchesSharedScenario=True, visibleUIActionsOnly=True,
                                sourceHashesUnchanged=True, nativeExecutionVerified=False, newNativeCaptures=0, executableLaunches=0),
                    renderWallSeconds=time.monotonic() - started)
    (args.output_dir / "frames.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    displayed.close(); presentation.close(); renderer.close()
    print(json.dumps(dict(modelRendered=True, nativeExecutionVerified=False, frames=count, seconds=catalog.duration,
                          manifest=str((args.output_dir / "frames.json").resolve()))), flush=True)


if __name__ == "__main__":
    main()
