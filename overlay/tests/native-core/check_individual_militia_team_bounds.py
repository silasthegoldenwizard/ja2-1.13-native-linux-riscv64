#!/usr/bin/env python3
"""Check that individual militia loops include the final team slot."""
# P710 native snapshot update: tests/native-core/check_individual_militia_team_bounds.py
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
        print("usage: check_individual_militia_team_bounds.py ENGINE_ROOT", file=sys.stderr)
        return 2

    source = (Path(sys.argv[1]) / "Strategic" / "MilitiaIndividual.cpp").read_text(
        encoding="latin-1"
    )
    functions = (
        "ApplyTacticalLifeRatioToMilitia",
        "ApplyMilitiaHealthRatioToTactical",
        "GetIdOfUnusedIndividualMilitia",
        "PromoteIndividualMilitiaInSector",
    )
    inclusive_loop = (
        "SoldierIDcnt=gTacticalStatus.Team[MILITIA_TEAM].bFirstID;"
        "SoldierIDlastid=gTacticalStatus.Team[MILITIA_TEAM].bLastID;"
        "for(;cnt<=lastid;++cnt)"
    )
    failures = []
    for name in functions:
        match = re.search(
            rf"^\w+\s+{name}\s*\([^)]*\)\s*\{{(.*?)^\}}",
            source,
            re.MULTILINE | re.DOTALL,
        )
        if match is None or inclusive_loop not in "".join(match[1].split()):
            failures.append(name)

    if failures:
        for name in failures:
            print(f"FAIL: {name} must include the final militia team slot", file=sys.stderr)
        return 1

    print(f"PASS: {len(functions)} individual militia loops include the final team slot")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
