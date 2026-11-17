#!/usr/bin/env python3
"""Check analyzer-proven runtime values before they reach production consumers."""
# P710 native snapshot update: tests/native-core/check_uninitialized_runtime_guards.py
# Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-17.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.


from pathlib import Path
import re
import sys


def section(source: str, start_text: str, end_text: str) -> str:
    start = source.find(start_text)
    end = source.find(end_text, start)
    return source[start:end] if start >= 0 and end >= 0 else ""


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: check_uninitialized_runtime_guards.py ENGINE_ROOT", file=sys.stderr)
        return 2

    root = Path(sys.argv[1])
    handle_ui = (root / "Tactical" / "Handle UI.cpp").read_text(encoding="latin-1")
    lua_npcs = (root / "Strategic" / "LuaInitNPCs.cpp").read_text(encoding="latin-1")
    points = (root / "Tactical" / "Points.cpp").read_text(encoding="latin-1")
    soldier_ani = (root / "Tactical" / "Soldier Ani.cpp").read_text(encoding="latin-1")
    soldier_create = (root / "Tactical" / "Soldier Create.cpp").read_text(encoding="latin-1")

    move = section(
        handle_ui,
        "UINT32 UIHandleCMoveMerc( UI_EVENT *pUIEvent )\n{",
        "UINT32 UIHandleCOnTerrain( UI_EVENT *pUIEvent )\n{",
    )
    buddy_lua = section(
        lua_npcs,
        "static int l_WhichBuddy(lua_State* L)\n{",
        "static int l_AnimMercPtsrsAbsoluteFinalDestination(lua_State* L)\n{",
    )
    attack = section(points, "INT16 CalcTotalAPsToAttack(", "INT16 MinAPsToAttack(")
    buddy_quote = section(soldier_ani, "void SayBuddyWitnessedQuoteFromKill(", "void HandleKilledQuote(")
    traits_start = soldier_create.find("BOOLEAN AssignTraitsToSoldier(")
    traits = soldier_create[traits_start:] if traits_start >= 0 else ""
    if not all((move, buddy_lua, attack, buddy_quote, traits)):
        print("FAIL: one or more runtime function bodies were not found", file=sys.stderr)
        return 1

    checks = (
        (
            bool(re.search(
                r"SOLDIERTYPE\s*\*pMovedSoldier\s*=\s*NULL\s*;.*?"
                r"pMovedSoldier\s*=\s*pSoldier\s*;.*?"
                r"if\s*\(\s*pMovedSoldier\s*\)\s*"
                r"SetConfirmMovementModeCursor\s*\(\s*pMovedSoldier\s*,\s*TRUE\s*\)",
                move,
                re.DOTALL,
            )),
            "move-all cursor setup consumes an uninitialized or ineligible soldier",
        ),
        (
            bool(re.search(
                r"INT8\s+bBuddyIndex\s*=\s*-1\s*;.*?"
                r"if\s*\(\s*lua_gettop\(L\)\s*>=\s*2\s*\).*?"
                r"lua_pushinteger\s*\(\s*L\s*,\s*bBuddyIndex\s*\)\s*;\s*"
                r"return\s+1\s*;",
                buddy_lua,
                re.DOTALL,
            )),
            "Lua buddy lookup publishes an undefined result for invalid inputs",
        ),
        (
            bool(re.search(r"INT32\s+sActionGridNo\s*=\s*sGridNo\s*;", attack)),
            "same-tile melee calculation uses an undefined action grid",
        ),
        (
            bool(re.search(r"INT32\s+sAdjustedGridNo\s*=\s*sGridNo\s*;", attack)),
            "same-tile melee calculation uses an undefined adjusted grid",
        ),
        (
            bool(re.search(
                r"if\s*\(\s*bTempBuddyIndex\s*>=\s*0\s*&&\s*"
                r"bTempBuddyIndex\s*<=\s*5\s*\)",
                buddy_quote,
            )),
            "buddy quote selection accepts an index with no quote mapping",
        ),
        (
            bool(re.search(r"INT32\s+iChance\s*=\s*0\s*;", traits)),
            "trait assignment passes an undefined chance for an unknown class",
        ),
    )

    failures = [message for passed, message in checks if not passed]
    if failures:
        for failure in failures:
            print(f"FAIL: {failure}", file=sys.stderr)
        print(
            f"FAIL: {len(failures)} of {len(checks)} undefined-state guards failed",
            file=sys.stderr,
        )
        return 1

    print(f"PASS: {len(checks)} undefined-state runtime guards")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
