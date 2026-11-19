"""Read-only checks for startup asset-root selection; never starts native code."""

import json
import ntpath
from pathlib import Path
import re


REPOSITORY = Path(__file__).resolve().parents[1]
PROJECT = REPOSITORY / "Tower_Triumph" / "Tower_Triumph"
SOURCE = (PROJECT / "Framework.cpp").read_text(encoding="utf-8-sig")


def literals(name):
    match = re.search(rf"\b{name}\[\]\s*=\s*\{{(.*?)\}};", SOURCE, re.S)
    assert match, f"Missing startup path contract: {name}"
    return [json.loads('"' + value + '"') for value in
            re.findall(r'L"((?:\\.|[^"\\])*)"', match.group(1))]


EXE_SUFFIXES = literals("kExecutableAssetSuffixes")
CWD_SUFFIXES = literals("kWorkingAssetSuffixes")
MARKERS = literals("kAssetRootMarkers")


def candidates(executable_directory, working_directory):
    for base, suffixes in ((executable_directory, EXE_SUFFIXES),
                           (working_directory, CWD_SUFFIXES)):
        if base:
            for suffix in suffixes:
                yield ntpath.normpath(base + suffix)


def select(executable_directory, working_directory, is_file):
    for root in candidates(executable_directory, working_directory):
        if all(is_file(ntpath.normpath(root + marker)) for marker in MARKERS):
            return root
    return working_directory


def portable_files(root):
    return {ntpath.normcase(ntpath.normpath(root + marker)) for marker in MARKERS}


def check():
    checks = 0
    assert len(MARKERS) >= 2 and all("IdlePose.bmp" in marker for marker in MARKERS)
    assert EXE_SUFFIXES[0] == "" and CWD_SUFFIXES[0] == ""
    checks += 1

    # Check the actual repository layout without requiring a built executable.
    build_dir = REPOSITORY / "Tower_Triumph" / "x64" / "Debug"
    for cwd in (PROJECT, REPOSITORY, build_dir, REPOSITORY / "Tower_Triumph"):
        chosen = select(str(build_dir), str(cwd), lambda path: Path(path).is_file())
        assert Path(chosen).resolve() == PROJECT, (cwd, chosen)
        checks += 1

    # A source checkout remains discoverable when the executable lives elsewhere.
    for cwd in (PROJECT, REPOSITORY, REPOSITORY / "Tower_Triumph"):
        chosen = select(r"Z:\unrelated\bin", str(cwd), lambda path: Path(path).is_file())
        assert Path(chosen).resolve() == PROJECT, (cwd, chosen)
        checks += 1

    # Shipping next to Resource has priority over every development/CWD candidate.
    portable = r"Z:\portable-game"
    known = portable_files(portable) | portable_files(str(PROJECT))
    is_virtual_file = lambda path: ntpath.normcase(path) in known
    assert select(portable, str(PROJECT), is_virtual_file) == portable
    checks += 1

    drive_root = "Z:\\"
    known = portable_files(drive_root)
    assert select(drive_root, str(PROJECT), lambda path: ntpath.normcase(path) in known) == drive_root
    checks += 1

    # A single marker, or an unrelated higher ancestor, must not select a root.
    one_marker = {ntpath.normcase(portable + MARKERS[0])}
    untouched_cwd = r"Z:\unrelated\launch"
    assert select(portable, untouched_cwd, lambda path: ntpath.normcase(path) in one_marker) == untouched_cwd
    known = portable_files(r"Z:\unrelated")
    assert select(r"Z:\unrelated\one\two\three\bin", untouched_cwd,
                  lambda path: ntpath.normcase(path) in known) == untouched_cwd
    assert select(portable, untouched_cwd, lambda path: False) == untouched_cwd
    checks += 3

    # The production function keeps the slash even for an executable at C:\.
    assert "executablePath.substr(0, separator + 1)" in SOURCE
    assert SOURCE.count("SetCurrentDirectoryW(") == 1
    win_main = SOURCE[SOURCE.index("int WINAPI WinMain("):]
    assert win_main.index("InitializeRuntimeAssetRoot();") < win_main.index("CreateWindow(")
    assert "FindFirstFile" not in SOURCE and "recursive_directory_iterator" not in SOURCE
    checks += 1
    print(f"Runtime asset-root checks: {checks} passed (read-only; no native execution).")


if __name__ == "__main__":
    check()
