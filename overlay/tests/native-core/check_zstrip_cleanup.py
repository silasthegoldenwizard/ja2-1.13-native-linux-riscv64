#!/usr/bin/env python3
"""Check production Z-strip allocation-failure ownership."""
# P710 native snapshot update: tests/native-core/check_zstrip_cleanup.py
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
        print("usage: check_zstrip_cleanup.py ENGINE_ROOT", file=sys.stderr)
        return 2

    source = (Path(sys.argv[1]) / "TileEngine" / "structure.cpp").read_text(
        encoding="latin-1"
    )
    function = re.search(
        r"BOOLEAN\s+AddZStripInfoToVObject\s*\([^)]*\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    )
    if not function:
        print("FAIL: AddZStripInfoToVObject body was not found", file=sys.stderr)
        return 1
    body = function.group("body")

    helper = re.search(
        r"static\s+void\s+FreeZStripInfo\s*\(\s*HVOBJECT\s+hVObject\s*\)\s*\{"
        r"(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    )
    helper_body = helper.group("body") if helper else ""

    checks = (
        (
            bool(helper)
            and "ppZStripInfo[uiLoop]->pbZChange" in helper_body
            and "MemFree(hVObject->ppZStripInfo[uiLoop])" in helper_body
            and "MemFree(hVObject->ppZStripInfo);" in helper_body
            and "hVObject->ppZStripInfo = NULL;" in helper_body,
            "a complete Z-strip cleanup helper is missing",
        ),
        (
            body.count("FreeZStripInfo(hVObject);") == 3,
            "not every Z-strip allocation failure uses complete cleanup",
        ),
        (
            "MemFree(hVObject->ppZStripInfo[uiLoop])" not in body,
            "allocation-failure cleanup repeatedly frees the current loop slot",
        ),
    )

    failures = [message for passed, message in checks if not passed]
    if failures:
        for failure in failures:
            print(f"FAIL: {failure}", file=sys.stderr)
        print(f"FAIL: {len(failures)} of {len(checks)} Z-strip ownership checks failed", file=sys.stderr)
        return 1

    print(f"PASS: {len(checks)} Z-strip allocation-failure ownership checks")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
