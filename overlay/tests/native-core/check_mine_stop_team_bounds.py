#!/usr/bin/env python3
"""Check the player-team loop bounds after a mine is spotted outside combat."""
# P710 native snapshot update: tests/native-core/check_mine_stop_team_bounds.py
# Native Linux source/build/test adaptations captured in the accepted native checkpoint dated 2026-11-18.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-18.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.


from pathlib import Path
import re
import sys


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: check_mine_stop_team_bounds.py ENGINE_ROOT", file=sys.stderr)
        return 2

    source = (Path(sys.argv[1]) / "Tactical" / "Overhead.cpp").read_text(
        encoding="latin-1"
    )
    team = r"gTacticalStatus\.Team\[\s*gbPlayerNum\s*\]"
    flags = re.MULTILINE | re.DOTALL
    stop_loop = re.compile(
        r"^\s*if\s*\(\s*NearbyGroundSeemsWrong\([^\n]+\)\s*\)"
        r".*?^\s*if\s*\(\s*!\(\s*gTacticalStatus\.uiFlags\s*&\s*INCOMBAT\s*\)\s*\)"
        r"\s*\{[^{}]*?"
        rf"\bSoldierID\s+(?P<id>\w+)\s*=\s*{team}\.bFirstID\s*;"
        r"[^{}]*?^\s*for\s*\(\s*;\s*(?P=id)\s*(?P<bound><=?)\s*"
        rf"{team}\.bLastID\s*;\s*\+\+(?P=id)\s*\)",
        flags,
    )

    failed = False
    for name in ("HandleGotoNewGridNo", "HandleAtNewGridNo"):
        function = re.search(
            rf"^BOOLEAN\s+{name}\s*\([^)]*\)\s*\{{(?P<body>.*?)^\}}",
            source,
            flags,
        )
        loop = stop_loop.search(function["body"]) if function else None
        if loop is None or loop["bound"] != "<=":
            print(f"FAIL: {name} mine-stop loop must include bLastID", file=sys.stderr)
            failed = True

        if loop is not None:
            safe_lookup = re.compile(
                rf"\b(?P<soldier>\w+)\s*=\s*NULL\s*;\s*"
                rf"if\s*\(\s*GetSoldier\s*\(\s*&(?P=soldier)\s*,\s*{loop['id']}\s*\)\s*\)\s*"
                rf"\{{.*?(?P=soldier)->EVENT_StopMerc",
                flags,
            )
            loop_tail = function["body"][loop.end() : loop.end() + 500]
            if safe_lookup.search(loop_tail) is None:
                print(
                    f"FAIL: {name} mine-stop loop must resolve each active soldier safely",
                    file=sys.stderr,
                )
                failed = True

    if failed:
        return 1
    print("PASS: both mine-stop loops include and safely resolve the final player-team slot")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
