#!/usr/bin/env python3
"""Keep item-description teardown safe during multi-result transformations."""
# P710 native snapshot update: tests/native-core/check_item_transform_callback_guard.py
# Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-17.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.


from __future__ import annotations

import re
import sys
from pathlib import Path


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: check_item_transform_callback_guard.py <engine-root>", file=sys.stderr)
        return 2

    source = (
        Path(sys.argv[1]) / "Tactical" / "Interface Items.cpp"
    ).read_text(encoding="utf-8")
    function = re.search(
        r"void\s+TransformFromItemDescBox\s*\([^)]*\)\s*\{(.*?)\n\}",
        source,
        re.DOTALL,
    )
    if function is None:
        print("FAIL: TransformFromItemDescBox is missing", file=sys.stderr)
        return 1

    body = function.group(1)
    checks = (
        ("gpItemDescObject->TransformObject(" in body, "production transform call is missing"),
        ("OBJECTTYPE *pTemp = gpItemDescObject;" in body, "post-transform description state is not captured"),
        (
            bool(re.search(r"if\s*\(\s*pTemp\s*!=\s*NULL\s*&&\s*pTemp->usItem\s*>\s*0", body)),
            "closed description state is dereferenced after transformation",
        ),
    )
    failures = [message for passed, message in checks if not passed]
    if failures:
        for failure in failures:
            print(f"FAIL: {failure}", file=sys.stderr)
        print(f"FAIL: {len(failures)} of {len(checks)} transform callback checks failed", file=sys.stderr)
        return 1

    print(f"PASS: {len(checks)} guarded transform callback checks")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
