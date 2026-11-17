#!/usr/bin/env python3
"""Check town-distance and message-box cursor initialization boundaries."""
# P710 native snapshot update: tests/native-core/check_loyalty_messagebox_state_guards.py
# Native Linux source/build/test adaptations captured in the accepted strategic runtime-state checkpoint dated 2026-11-17.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-17.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.


from pathlib import Path
import re
import sys


def extract(source: str, start_marker: str, end_marker: str) -> str:
    start = source.find(start_marker)
    end = source.find(end_marker, start + len(start_marker))
    return source[start:end] if start >= 0 and end >= 0 else ""


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: check_loyalty_messagebox_state_guards.py ENGINE_ROOT", file=sys.stderr)
        return 2

    root = Path(sys.argv[1])
    loyalty_source = (root / "Strategic" / "Strategic Town Loyalty.cpp").read_text(
        encoding="latin-1"
    )
    message_source = (root / "Ja2" / "MessageBoxScreen.cpp").read_text(
        encoding="latin-1"
    )

    all_towns = extract(
        loyalty_source,
        "void AffectAllTownsLoyaltyByDistanceFrom(",
        "void AffectClosestTownLoyaltyByDistanceFrom(",
    )
    closest_town = extract(
        loyalty_source,
        "void AffectClosestTownLoyaltyByDistanceFrom(",
        "void CheckIfEntireTownHasBeenLiberated(",
    )
    message_box = extract(
        message_source,
        "INT32 DoMessageBox(",
        "void MsgBoxClickCallback(",
    )
    if not all_towns or not closest_town or not message_box:
        print("FAIL: one or more guarded production functions were not found", file=sys.stderr)
        return 1

    full_init = re.compile(
        r"for\s*\(\s*UINT8\s+townIndex\s*=\s*0\s*;\s*"
        r"townIndex\s*<\s*MAX_TOWNS\s*;\s*\+\+townIndex\s*\)"
        r"[\s\S]{0,120}iShortestDistance\s*\[\s*townIndex\s*\]\s*=\s*999999"
    )
    bounded_list = re.compile(
        r"while\s*\(\s*uiIndex\s*<\s*MAX_TOWN_SECTORS\s*&&\s*"
        r"pTownNamesList\s*\[\s*uiIndex\s*\]\s*!=\s*0\s*\)"
    )
    town_guard = re.compile(
        r"const\s+INT32\s+townId\s*=\s*pTownNamesList\s*\[\s*uiIndex\s*\]\s*;"
        r"[\s\S]{0,180}if\s*\(\s*townId\s*<\s*FIRST_TOWN\s*\|\|\s*"
        r"townId\s*>=\s*MAX_TOWNS\s*\)"
        r"[\s\S]{0,220}bTownId\s*=\s*static_cast<INT8>\(townId\)"
    )

    default_positions = bool(
        re.search(
            r"else\s+if\s*\(\s*ubDefaultButton\s*!=\s*"
            r"MSG_BOX_DEFAULT_BUTTON_NONE\s*\)\s*\{"
            r"[\s\S]{0,220}sButtonX\s*=\s*usTextBoxWidth\s*/\s*2\s*;"
            r"[\s\S]{0,100}sButtonY\s*=\s*usTextBoxHeight\s*-\s*4\s*;",
            message_box,
        )
    )
    bounded_defaults = all(
        re.search(pattern, message_box)
        for pattern in (
            r"MSG_BOX_FLAG_GENERIC_FOUR_BUTTONS\s*\)\s*&&\s*"
            r"ubDefaultButton\s*<=\s*MSG_BOX_DEFAULT_BUTTON_4",
            r"MSG_BOX_FLAG_GENERIC_EIGHT_BUTTONS\s*\)\s*&&\s*"
            r"ubDefaultButton\s*<=\s*MSG_BOX_DEFAULT_BUTTON_8",
            r"MSG_BOX_FLAG_GENERIC_SIXTEEN_BUTTONS\s*\)\s*&&\s*"
            r"ubDefaultButton\s*<=\s*MSG_BOX_DEFAULT_BUTTON_16",
        )
    )

    checks = [
        ("all-town distance table initializes its complete extent", bool(full_init.search(all_towns))),
        ("closest-town distance table initializes its complete extent", bool(full_init.search(closest_town))),
        ("all-town sector list traversal is bounded", bool(bounded_list.search(all_towns))),
        ("closest-town sector list traversal is bounded", bool(bounded_list.search(closest_town))),
        ("all-town list values are validated before indexing", bool(town_guard.search(all_towns))),
        ("closest-town list values are validated before indexing", bool(town_guard.search(closest_town))),
        ("message-box cursor coordinates have a safe fallback", default_positions),
        ("message-box specialized default buttons are range-checked", bounded_defaults),
    ]

    failures = 0
    for label, passed in checks:
        if passed:
            print(f"PASS: {label}")
        else:
            failures += 1
            print(f"FAIL: {label}", file=sys.stderr)

    if failures:
        print(f"FAIL: {failures} loyalty/message-box state checks failed", file=sys.stderr)
        return 1

    print(f"PASS: {len(checks)} loyalty/message-box state checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
