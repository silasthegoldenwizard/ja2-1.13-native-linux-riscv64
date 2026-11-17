#!/usr/bin/env python3
"""Check credits-node rollback and text-buffer initialization."""
# P710 native snapshot update: tests/native-core/check_credits_state_ownership.py
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


def failure_body(function: str, condition: str) -> str:
    match = re.search(
        rf"if\s*\(\s*{condition}\s*\)\s*\{{(?P<body>.*?)\}}",
        function,
        re.DOTALL,
    )
    return match.group("body") if match else ""


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: check_credits_state_ownership.py ENGINE_ROOT", file=sys.stderr)
        return 2

    source = (Path(sys.argv[1]) / "Ja2" / "Credits.cpp").read_text(
        encoding="latin-1"
    )
    add_node = extract(source, "BOOLEAN\tAddCreditNode(", "void HandleCreditNodes(")
    next_credit = extract(
        source,
        "BOOLEAN\tGetNextCreditFromTextFile()\n{",
        "UINT32\tGetAndHandleCreditCodeFromCodeString(",
    )
    if not add_node or not next_credit:
        print("FAIL: one or more credits functions were not found", file=sys.stderr)
        return 1

    string_failure = failure_body(
        add_node, r"pNodeToAdd->pString\s*==\s*NULL"
    )
    surface_failure = failure_body(
        add_node, r"AddVideoSurface\s*\(\s*&vs_desc\s*,\s*&pNodeToAdd->uiVideoSurfaceImage\s*\)\s*==\s*0"
    )
    checks = [
        (
            "failed credits string allocation releases the node",
            bool(re.search(r"\bMemFree\s*\(\s*pNodeToAdd\s*\)", string_failure)),
        ),
        (
            "failed credits surface creation releases string and node",
            bool(
                re.search(r"\bMemFree\s*\(\s*pNodeToAdd->pString\s*\)", surface_failure)
                and re.search(r"\bMemFree\s*\(\s*pNodeToAdd\s*\)", surface_failure)
            ),
        ),
        (
            "credits input buffer starts empty",
            bool(
                re.search(
                    r"CHAR16\s+zOriginalString\s*\[\s*512\s*\]\s*=\s*\{\s*0\s*\}\s*;",
                    next_credit,
                )
            ),
        ),
    ]

    failures = 0
    for label, passed in checks:
        if passed:
            print(f"PASS: {label}")
        else:
            failures += 1
            print(f"FAIL: {label}", file=sys.stderr)

    if failures:
        print(f"FAIL: {failures} credits state/ownership checks failed", file=sys.stderr)
        return 1

    print(f"PASS: {len(checks)} credits state/ownership checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
