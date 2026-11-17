#!/usr/bin/env python3
"""Keep tactical pickup-menu indexes wide enough for the world-item pool."""
# P710 native snapshot update: tests/native-core/check_item_pickup_count_width.py
# Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-17.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.


from __future__ import annotations

import re
import sys
from pathlib import Path


def pickup_hover_checks(interface: str) -> list[tuple[bool, str]]:
    """Check hover selection without engine types or rendering dependencies."""
    menu = re.search(
        r"class\s+ITEM_PICKUP_MENU_STRUCT\s*\{(.*?)\n\};", interface, re.DOTALL,
    )
    callback = re.search(
        r"void\s+ItemPickMenuMouseMoveCallback\s*\([^)]*\)\s*\{(.*?)\n\}",
        interface,
        re.DOTALL,
    )
    # The disabled compatible-ammo block contains an inactive array subscript.
    body = re.sub(r"/\*.*?\*/|//[^\n]*", "", callback.group(1), flags=re.DOTALL) if callback else ""
    return [
        (
            menu is not None and bool(re.search(r"\bINT32\s+bCurSelect\s*;", menu.group(1))),
            "pickup hover selection must remain signed INT32",
        ),
        (
            bool(re.search(
                r"uiItemPos\s*=\s*MSYS_GetRegionUserData\s*\(\s*pRegion\s*,\s*0\s*\)\s*;",
                body,
            )),
            "pickup hover must read the row from region user data slot 0",
        ),
        (
            bool(re.search(r"\bINT32\s+bPos\s*;", body)),
            "pickup hover absolute index must remain INT32",
        ),
        (
            bool(re.search(
                r"if\s*\(\s*iReason\s*&\s*MSYS_CALLBACK_REASON_MOVE\s*\)\s*\{\s*"
                r"bPos\s*=\s*\(\s*uiItemPos\s*\+\s*gItemPickupMenu\.ubScrollAnchor\s*\)\s*;\s*"
                r"if\s*\(\s*bPos\s*<\s*gItemPickupMenu\.ubTotalItems\s*\)\s*\{\s*"
                r"gItemPickupMenu\.bCurSelect\s*=\s*bPos\s*;",
                body,
            )),
            "pickup hover must select the exact in-range absolute index on every page",
        ),
        (
            bool(re.search(
                r"else\s+if\s*\(\s*iReason\s*&\s*MSYS_CALLBACK_REASON_LOST_MOUSE\s*\)\s*\{\s*"
                r"gItemPickupMenu\.bCurSelect\s*=\s*-1\s*;",
                body,
            )),
            "pickup lost-mouse hover sentinel must be -1, outside all nonnegative item indexes",
        ),
        (
            bool(re.search(
                r"if\s*\(\s*gItemPickupMenu\.bCurSelect\s*==\s*"
                r"\(\s*cnt\s*\+\s*gItemPickupMenu\.ubScrollAnchor\s*\)\s*\)",
                interface,
            )),
            "pickup rendering must compare hover selection with the absolute row index",
        ),
    ]


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: check_item_pickup_count_width.py <engine-root>", file=sys.stderr)
        return 2

    root = Path(sys.argv[1])
    interface = (root / "Tactical" / "Interface Items.cpp").read_text(encoding="utf-8")
    handle = (root / "Tactical" / "Handle Items.cpp").read_text(encoding="utf-8")
    failures: list[str] = []
    checks = 0

    def require(condition: bool, label: str) -> None:
        nonlocal checks
        checks += 1
        if not condition:
            failures.append(label)

    for member in ("bScrollPage", "ubScrollAnchor", "ubTotalItems"):
        require(
            bool(re.search(rf"INT32\s+{member}\s*;", interface)),
            f"pickup menu {member} is narrower than the pool count",
        )

    require(
        "gItemPickupMenu.ubTotalItems = cnt;" in interface,
        "displayable pickup count is narrowed before allocation",
    )
    require(
        "gItemPickupMenu.ubScrollAnchor\t= iStart;" in interface,
        "pickup-page anchor is narrowed before selection indexing",
    )
    require(
        interface.count("void SetupPickupPage( INT32 bPage )") == 2,
        "pickup-page declaration or definition uses a narrow page type",
    )
    require(
        not re.search(r"SetupPickupPage\s*\(\s*\(UINT8\)", interface),
        "pickup-page navigation narrows the page number",
    )

    callback = re.search(
        r"void\s+ItemPickMenuMouseClickCallback\s*\([^)]*\)\s*\{(.*?)\n\}",
        interface,
        re.DOTALL,
    )
    require(callback is not None, "pickup click callback is missing")
    if callback is not None:
        require(
            bool(re.search(r"INT32\s+cnt\s*;", callback.group(1))),
            "pickup selection scan wraps before the full item count",
        )
        require(
            "cnt < gItemPickupMenu.ubTotalItems" in callback.group(1),
            "pickup selection scan no longer covers the full allocation",
        )

    require(
        bool(re.search(
            r"SoldierGetItemFromWorld\s*\(.*?ITEM_PICKUP_SELECTION.*?"
            r"while\s*\(\s*pItemPool\s*\).*?pfSelectionList\s*\[\s*cnt\s*\]",
            handle,
            re.DOTALL,
        )),
        "selection consumer no longer indexes the full visible item pool",
    )

    for condition, label in pickup_hover_checks(interface):
        require(condition, label)

    if failures:
        for failure in failures:
            print(f"FAIL: {failure}", file=sys.stderr)
        print(f"FAIL: {len(failures)} of {checks} pickup count-width checks failed", file=sys.stderr)
        return 1

    print(f"PASS: {checks} pickup count-width checks")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
