#!/usr/bin/env python3
"""Check that each reinforcement-count option is published before reuse."""
# P710 native snapshot update: tests/native-core/check_reinforcement_menu_ownership.py
# Native Linux source/build/test adaptations captured in the accepted ownership and bounds checkpoint dated 2026-11-17.
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


def option_is_published(function: str, count: int) -> bool:
    allocation = re.search(
        rf"pOption\s*=\s*new\s+POPUP_OPTION\s*\(.*?Wrapper_Function_ReinforcementNumber\s*,\s*{count}\s*\)\s*\)\s*;",
        function,
        re.DOTALL,
    )
    if not allocation:
        return False
    next_allocation = function.find("pOption = new POPUP_OPTION", allocation.end())
    segment = function[allocation.end() : next_allocation if next_allocation >= 0 else len(function)]
    return bool(re.search(r"GetPopup\s*\(\s*\)\s*->\s*addOption\s*\(\s*\*pOption\s*\)", segment))


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: check_reinforcement_menu_ownership.py ENGINE_ROOT", file=sys.stderr)
        return 2

    source = (Path(sys.argv[1]) / "Tactical" / "SkillMenu.cpp").read_text(
        encoding="latin-1"
    )
    setup = extract(
        source,
        "ReinforcementNumber::Setup( UINT32 aVal )",
        "ReinforcementNumber::Functions( UINT32 aVal )",
    )
    if not setup:
        print("FAIL: ReinforcementNumber::Setup was not found", file=sys.stderr)
        return 1

    checks = [
        (f"{count}-militia option is published", option_is_published(setup, count))
        for count in (5, 10, 15, 20, 30, 40)
    ]
    failures = 0
    for label, passed in checks:
        if passed:
            print(f"PASS: {label}")
        else:
            failures += 1
            print(f"FAIL: {label}", file=sys.stderr)

    if failures:
        print(f"FAIL: {failures} reinforcement menu checks failed", file=sys.stderr)
        return 1

    print(f"PASS: {len(checks)} reinforcement menu checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
