#!/usr/bin/env python3
"""Check the production boxing-disqualification pointer boundary."""
# P710 native snapshot update: tests/native-core/check_boxing_guards.py
# Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
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
        print("usage: check_boxing_guards.py ENGINE_ROOT", file=sys.stderr)
        return 2

    source = (Path(sys.argv[1]) / "Tactical" / "Boxing.cpp").read_text(
        encoding="latin-1"
    )
    match = re.search(
        r"void\s+BoxingPlayerDisqualified\s*\([^)]*\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    )
    if not match:
        print("FAIL: BoxingPlayerDisqualified body was not found", file=sys.stderr)
        return 1
    body = match.group("body")

    guarded = bool(re.search(
        r"if\s*\(\s*pOffender\s*!=\s*NULL\s*&&\s*"
        r"\(\s*bReason\s*==\s*BOXER_OUT_OF_RING\s*\|\|\s*"
        r"bReason\s*==\s*NON_BOXER_IN_RING\s*\)\s*\)",
        body,
    ))
    if not guarded:
        print("FAIL: disqualification can stop a missing offender", file=sys.stderr)
        return 1

    print("PASS: boxing disqualification offender pointer is guarded")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
