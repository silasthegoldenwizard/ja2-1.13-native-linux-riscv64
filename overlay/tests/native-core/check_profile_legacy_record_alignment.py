#!/usr/bin/env python3
"""Verify legacy MERCPROFILE record fields preserve the POD byte count."""
# P710 native snapshot update: tests/native-core/check_profile_legacy_record_alignment.py
# Native Linux source/build/test adaptations captured in the accepted native checkpoint dated 2026-11-18.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-18.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.


from __future__ import annotations

import re
import sys
from pathlib import Path


def check(condition: bool, message: str, failures: list[str]) -> None:
    if not condition:
        failures.append(message)


def main() -> int:
    root = Path(sys.argv[1] if len(sys.argv) > 1 else ".").resolve()
    source = (root / "Ja2" / "SaveLoadGame.cpp").read_text(
        encoding="utf-8", errors="strict"
    )
    pattern = re.compile(
        r"if\s*\(\s*guiCurrentSaveGameVersion\s*<\s*"
        r"STOMP12_SAVEGAME_DATATYPE_CHANGE\s*\)"
        r"\s*\{\s*// No longer included but need to read them anyway"
        r"(?P<body>.*?)"
        r"\}\s*"
        r"numBytesRead\s*=\s*ReadFieldByField(?:FromBuffer)?"
        r"\([^;]*usTotalDaysServed[^;]*;",
        re.DOTALL,
    )
    blocks = [match.group("body") for match in pattern.finditer(source)]
    failures: list[str] = []

    check(
        len(blocks) == 2,
        "expected the file and buffer legacy-profile record blocks",
        failures,
    )
    for index, block in enumerate(blocks):
        lane = "file" if index == 0 else "buffer"
        legacy_reads = re.findall(
            r"temp\s*=\s*ReadFieldByField(?:FromBuffer)?\s*\(", block
        )
        check(
            len(legacy_reads) == 6,
            f"{lane} loader must consume all six legacy UINT16 record fields",
            failures,
        )
        check(
            "numBytesRead = temp;" in block,
            f"{lane} loader must carry the complete legacy byte count forward",
            failures,
        )
        check(
            "numBytesRead++;" not in block,
            f"{lane} loader must not collapse twelve consumed bytes to one",
            failures,
        )

    # Six aligned UINT16 fields add twelve bytes.  The former +1 count was
    # eleven bytes short for an aligned start and changed later padding.
    for start in range(8):
        expected = start
        for _ in range(6):
            expected += (-expected) % 2
            expected += 2
        check(
            expected != start + 1,
            f"legacy count simulation unexpectedly accepts +1 at offset {start}",
            failures,
        )

    if failures:
        for failure in failures:
            print(f"FAIL: {failure}", file=sys.stderr)
        return 1

    print(
        "PASS: legacy profile record byte counts stay aligned in file and buffer loaders"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
