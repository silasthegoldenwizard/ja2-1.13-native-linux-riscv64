#!/usr/bin/env python3
"""Lock analyzer-proven inventory and map lifecycle guards."""
# P710 native snapshot update: tests/native-core/check_inventory_map_lifecycle_guards.py
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


def mask_cpp(text: str) -> str:
    return re.sub(
        r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
        lambda match: "".join("\n" if char == "\n" else " " for char in match.group()),
        text,
    )


def function(text: str, signature: str) -> str:
    clean = mask_cpp(text)
    search_from = 0
    while True:
        start = clean.find(signature, search_from)
        if start < 0:
            raise ValueError(f"missing function: {signature}")
        opening = clean.find("{", start)
        semicolon = clean.find(";", start)
        if opening >= 0 and (semicolon < 0 or opening < semicolon):
            break
        search_from = start + len(signature)
    depth = 0
    for index in range(opening, len(clean)):
        if clean[index] == "{":
            depth += 1
        elif clean[index] == "}":
            depth -= 1
            if depth == 0:
                return text[start : index + 1]
    raise ValueError(f"unterminated function: {signature}")


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: check_inventory_map_lifecycle_guards.py <engine-root>", file=sys.stderr)
        return 2

    root = Path(sys.argv[1])
    map_text = (root / "Strategic" / "mapscreen.cpp").read_text(encoding="utf-8")
    input_text = (root / "Tactical" / "Turn Based Input.cpp").read_text(
        encoding="utf-8"
    )
    failures: list[str] = []
    checks = 0

    def require(condition: bool, label: str) -> None:
        nonlocal checks
        checks += 1
        if not condition:
            failures.append(label)

    key_ring = function(map_text, "void MAPBeginKeyRingItemPointer")
    require(
        "BeginKeyRingItemPointer( pSoldier, uiKeySlot );" in key_ring,
        "map key-ring entry does not delegate to the owning cursor routine",
    )
    require(
        "gpItemPointer->" not in key_ring,
        "map key-ring entry dereferences the empty cursor",
    )

    sort = function(map_text, "void SortListOfMercsInTeamPanel")
    require(
        bool(re.search(
            r"pSelectedSoldier\s*\[\s*CODE_MAXIMUM_NUMBER_OF_PLAYER_SLOTS\s*\]"
            r"\s*=\s*\{\s*\}\s*;",
            sort,
        )),
        "team-panel retained-selection storage is not value-initialized",
    )

    route = function(map_text, "void CancelOrShortenPlottedPath")
    require(
        bool(re.search(
            r"if\s*\(\s*!\s*GetMouseMapXY\s*\(\s*&sMapX\s*,\s*&sMapY\s*\)\s*\)"
            r"\s*\{\s*return\s*;\s*\}",
            route,
            re.DOTALL,
        )),
        "route cancellation consumes coordinates after a failed mouse-map lookup",
    )

    reload_all = function(input_text, "void HandleTBReloadAll")
    second_pass = reload_all.find("MM: loop thru the soldiers again")
    attached = reload_all.find("IsWeaponAttached( pGun, IC_GUN )")
    require(second_pass >= 0, "reload-all second pass marker is missing")
    require(
        0 <= attached < second_pass,
        "attached-weapon reload uses a stale gun during the ammunition pass",
    )
    require(
        bool(re.search(
            r"OBJECTTYPE\s*\*\s*pGun\s*=\s*&\s*\(\s*"
            r"pTeamSoldier->inv\s*\[\s*bLoop2\s*\]\s*\)\s*;",
            reload_all[:second_pass] if second_pass >= 0 else reload_all,
        )),
        "reload-all gun ownership is not local to the gun slot",
    )

    crates = function(input_text, "void HandleTacticalAmmoCrates")
    while_pos = crates.find("while (ammoPresent")
    for_pos = crates.find("for(unsigned int wItem", while_pos)
    pass_prefix = crates[while_pos:for_pos] if while_pos >= 0 and for_pos >= 0 else ""
    require(
        "mergeSuccessful = false;" in pass_prefix,
        "ammo-crate pass does not initialize its merge result",
    )
    require(
        crates.count("mergeSuccessful = false;") == 1,
        "ammo-crate merge result is reset per item instead of per pass",
    )

    if failures:
        for failure in failures:
            print(f"FAIL: {failure}", file=sys.stderr)
        print(f"FAIL: {len(failures)} of {checks} lifecycle checks failed", file=sys.stderr)
        return 1

    print(f"PASS: {checks} inventory and map lifecycle guard checks")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
