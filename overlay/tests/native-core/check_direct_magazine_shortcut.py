#!/usr/bin/env python3
"""Check the direct inventory magazine shortcut's stack and cursor contract."""
# P710 native snapshot update: tests/native-core/check_direct_magazine_shortcut.py
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
        print("usage: check_direct_magazine_shortcut.py ENGINE_ROOT", file=sys.stderr)
        return 2

    source = (Path(sys.argv[1]) / "Tactical" / "Interface Panels.cpp").read_text(
        encoding="latin-1"
    )
    start = source.find("void SMInvClickCallback(")
    end = source.find("BOOLEAN  ChangeZipperStatus(", start)
    callback = re.sub(r"\s+", " ", source[start:end]) if start >= 0 and end >= 0 else ""
    if not callback:
        print("FAIL: SMInvClickCallback was not found", file=sys.stderr)
        return 1

    checks = [
        (
            "the direct shortcut names one explicit stack entry",
            "const UINT32 objectIndex = 0;" in callback,
        ),
        (
            "round detection and ejection use the same stack entry",
            ")[objectIndex]->data.gun.ubGunShotsLeft > 0" in callback
            and "EmptyWeaponMagazine( &(gpSMCurrentMerc->inv[ uiHandPos ]), &gItemPointer, objectIndex )" in callback,
        ),
        (
            "cursor ownership is published only after successful ejection",
            re.search(
                r"if \( EmptyWeaponMagazine\(.*?objectIndex \) \) \{ gpItemPointer = &gItemPointer; gpItemPointerSoldier = gpSMCurrentMerc; \}",
                callback,
            )
            is not None,
        ),
    ]

    failures = 0
    for label, passed in checks:
        print(f"{'PASS' if passed else 'FAIL'}: {label}")
        failures += not passed
    if failures:
        print(f"FAIL: {failures} direct magazine-shortcut checks failed", file=sys.stderr)
        return 1
    print(f"PASS: {len(checks)} direct magazine-shortcut checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
