#!/usr/bin/env python3
"""Check default initialization at the mini-event Lua money boundary."""
# P710 native snapshot update: tests/native-core/check_minievent_lua_guards.py
# Native Linux source/build/test adaptations captured in the accepted schedule, mini-event, and facility checkpoint dated 2026-11-17.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-17.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.


from pathlib import Path
import re
import sys


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: check_minievent_lua_guards.py ENGINE_ROOT", file=sys.stderr)
        return 2

    root = Path(sys.argv[1])
    source = (root / "Strategic" / "MiniEvents.cpp").read_text(encoding="latin-1")
    start = source.find("static int l_AddMoneyToPlayerAccount(lua_State* LS)")
    end = source.find("static int l_AddIntel(lua_State* LS)", start)
    body = source[start:end] if start >= 0 and end >= 0 else ""
    if not body:
        print("FAIL: mini-event money function was not found", file=sys.stderr)
        return 1

    initialized = bool(re.search(r"bool\s+forceToZero\s*=\s*false\s*;", body))
    if not initialized:
        print(
            "FAIL: one-argument mini-event withdrawal uses an undefined force-to-zero flag",
            file=sys.stderr,
        )
        return 1

    print("PASS: mini-event money default is defined")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
