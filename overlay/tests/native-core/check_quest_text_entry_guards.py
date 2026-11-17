#!/usr/bin/env python3
"""Check quest-debug text entry input guards in the production source."""
# P710 native snapshot update: tests/native-core/check_quest_text_entry_guards.py
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
        print("usage: check_quest_text_entry_guards.py ENGINE_ROOT", file=sys.stderr)
        return 2

    root = Path(sys.argv[1])
    source = (root / "Strategic" / "Quest Debug System.cpp").read_text(
        encoding="latin-1"
    )
    start = source.find("BOOLEAN\tCreateDestroyDisplayTextEntryBox(")
    end = source.find("void InitQuestDebugTextInputBoxes()", start)
    body = source[start:end] if start >= 0 and end >= 0 else ""
    if not body:
        print("FAIL: quest-debug text entry function was not found", file=sys.stderr)
        return 1

    create_start = body.find("case QD_DROP_DOWN_CREATE:")
    created = body.find("fMouseRegionCreated = TRUE;", create_start)
    create_prefix = body[create_start:created] if create_start >= 0 and created >= 0 else ""

    checks = (
        (
            bool(re.search(r"if\s*\(\s*pString\s*==\s*NULL\s*\)\s*return\s+FALSE\s*;", create_prefix)),
            "quest text-entry creation accepts a null source string",
        ),
        (
            bool(re.search(
                r"wcsncpy\s*\(\s*zString\s*,\s*pString\s*,\s*255\s*\)\s*;\s*"
                r"zString\s*\[\s*255\s*\]\s*=\s*L?'\\0'\s*;",
                body,
                re.DOTALL,
            )),
            "quest text-entry creation performs an unbounded fixed-buffer copy",
        ),
    )

    failures = [message for passed, message in checks if not passed]
    if failures:
        for failure in failures:
            print(f"FAIL: {failure}", file=sys.stderr)
        print(f"FAIL: {len(failures)} of {len(checks)} quest text-entry guards failed", file=sys.stderr)
        return 1

    print(f"PASS: {len(checks)} quest text-entry input guards")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
