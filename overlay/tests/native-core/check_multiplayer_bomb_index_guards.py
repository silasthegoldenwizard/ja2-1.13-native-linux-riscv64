#!/usr/bin/env python3
"""Check multiplayer bomb lookup does not use an unsigned not-found index."""
# P710 native snapshot update: tests/native-core/check_multiplayer_bomb_index_guards.py
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


def guarded_lookup(source: str) -> bool:
    return bool(re.search(
        r"UINT32\s+uiBombIndex\s*=\s*guiNumWorldBombs\s*;.*?"
        r"uiBombIndex\s*=\s*uiCount\s*;.*?"
        r"if\s*\(\s*uiBombIndex\s*<\s*guiNumWorldBombs\s*\).*?"
        r"gWorldBombs\[\s*uiBombIndex\s*\]",
        source,
        re.DOTALL,
    )) and "uiBombIndex > -1" not in source


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: check_multiplayer_bomb_index_guards.py ENGINE_ROOT", file=sys.stderr)
        return 2

    root = Path(sys.argv[1])
    source = (root / "Multiplayer" / "client.cpp").read_text(encoding="latin-1")
    detonate = section(source, "void send_detonate_explosive (", "void recieveDETONATEEXPLOSIVE (")
    disarm = section(source, "void send_disarm_explosive(", "void recieveDISARMEXPLOSIVE (")
    if not detonate or not disarm:
        print("FAIL: one or more multiplayer bomb functions were not found", file=sys.stderr)
        return 1

    checks = (
        (guarded_lookup(detonate), "detonation uses an always-valid unsigned not-found bomb index"),
        (guarded_lookup(disarm), "disarm uses an always-valid unsigned not-found bomb index"),
    )

    failures = [message for passed, message in checks if not passed]
    if failures:
        for failure in failures:
            print(f"FAIL: {failure}", file=sys.stderr)
        print(f"FAIL: {len(failures)} of {len(checks)} multiplayer bomb-index checks failed", file=sys.stderr)
        return 1

    print(f"PASS: {len(checks)} multiplayer bomb-index guards")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
