#!/usr/bin/env python3
"""Check popup-string allocation rollback in the production source."""
# P710 native snapshot update: tests/native-core/check_popup_string_ownership.py
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


def frees_structure_after_string_failure(function: str) -> bool:
    match = re.search(
        r"if\s*\(\s*pLocalString\s*==\s*NULL\s*\)\s*"
        r"\{(?P<body>.*?)\}",
        function,
        re.DOTALL,
    )
    return bool(match and re.search(r"\bMemFree\s*\(\s*pStringSt\s*\)", match.group("body")))


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: check_popup_string_ownership.py ENGINE_ROOT", file=sys.stderr)
        return 2

    source = (Path(sys.argv[1]) / "Utils" / "PopUpBox.cpp").read_text(
        encoding="latin-1"
    )
    functions = {
        "AddMonoString": extract(source, "void AddMonoString(", "void AddSecondColumnMonoString("),
        "AddSecondColumnMonoString": extract(
            source, "void AddSecondColumnMonoString(", "void AddColorString("
        ),
        "AddColorString": extract(source, "void AddColorString(", "void ResizeBoxForSecondStrings("),
    }
    if any(not body for body in functions.values()):
        print("FAIL: one or more popup-string functions were not found", file=sys.stderr)
        return 1

    checks = [
        (
            f"{name} releases the popup structure when string allocation fails",
            frees_structure_after_string_failure(body),
        )
        for name, body in functions.items()
    ]

    failures = 0
    for label, passed in checks:
        if passed:
            print(f"PASS: {label}")
        else:
            failures += 1
            print(f"FAIL: {label}", file=sys.stderr)

    if failures:
        print(f"FAIL: {failures} popup-string ownership checks failed", file=sys.stderr)
        return 1

    print(f"PASS: {len(checks)} popup-string ownership checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
