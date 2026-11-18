#!/usr/bin/env python3
"""Check inclusive team ranges and empty-slot guards in tactical scans."""
# P710 native snapshot update: tests/native-core/check_tactical_team_iteration_bounds.py
# Native Linux source/build/test adaptations captured in the accepted native checkpoint dated 2026-11-18.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-18.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.


from pathlib import Path
import re
import sys


def function_body(source: str, marker: str) -> str:
    start = source.rfind(marker)
    if start < 0:
        return ""
    opening = source.find("{", start)
    if opening < 0:
        return ""
    depth = 0
    for pos in range(opening, len(source)):
        if source[pos] == "{":
            depth += 1
        elif source[pos] == "}":
            depth -= 1
            if depth == 0:
                return re.sub(r"\s+", "", source[opening + 1 : pos])
    return ""


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: check_tactical_team_iteration_bounds.py ENGINE_ROOT", file=sys.stderr)
        return 2

    root = Path(sys.argv[1])
    sources = {
        "control": (root / "Tactical" / "Soldier Control.cpp").read_text(encoding="latin-1"),
        "items": (root / "Tactical" / "Items.cpp").read_text(encoding="latin-1"),
        "init": (root / "Tactical" / "Soldier Init List.cpp").read_text(encoding="latin-1"),
        "iso": (root / "TileEngine" / "Isometric Utils.cpp").read_text(encoding="latin-1"),
    }
    inclusive = "cnt<=lastid"
    checks = (
        ("SectorAddAssassins", "init", "void SectorAddAssassins(", 1),
        ("SectorAddPrisonersofWar", "init", "void SectorAddPrisonersofWar(", 1),
        ("IsFeedingExternal", "control", "SOLDIERTYPE::IsFeedingExternal(", 1),
        ("OrderArtilleryStrike", "control", "BOOLEAN SOLDIERTYPE::OrderArtilleryStrike(", 2),
        ("GetRadioOperatorSignal", "control", "BOOLEAN GetRadioOperatorSignal(", 1),
        ("IsValidArtilleryOrderSector", "control", "BOOLEAN IsValidArtilleryOrderSector(", 1),
        ("PlayerTeamIsScanning", "control", "BOOLEAN PlayerTeamIsScanning(", 1),
        ("GridNoSpotterCTHBonus", "control", "UINT16\tGridNoSpotterCTHBonus(", 1),
        ("GetExternalFeedingObject", "items", "OBJECTTYPE* GetExternalFeedingObject(", 1),
        ("GridNoNearPlayerMercs", "iso", "BOOLEAN GridNoNearPlayerMercs(", 1),
    )

    failures = []
    bodies = {}
    for name, source_name, marker, required in checks:
        body = function_body(sources[source_name], marker)
        bodies[name] = body
        if not body or body.count(inclusive) < required:
            failures.append(f"{name} must include the final team slot")

    sector_jammed = function_body(sources["control"], "BOOLEAN SectorJammed(")
    bodies["SectorJammed"] = sector_jammed
    for name in ("SectorJammed", "PlayerTeamIsScanning", "GridNoSpotterCTHBonus"):
        body = bodies.get(name, "")
        if "pSoldier&&pSoldier->bActive" not in body:
            failures.append(f"{name} must reject empty and inactive soldier slots")

    if failures:
        for failure in failures:
            print(f"FAIL: {failure}", file=sys.stderr)
        print(f"FAIL: {len(failures)} tactical team-iteration checks failed", file=sys.stderr)
        return 1

    print(f"PASS: {len(checks)} inclusive team scans and 3 empty-slot guards")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
