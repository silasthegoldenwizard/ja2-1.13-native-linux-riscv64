#!/usr/bin/env python3
"""Keep weapon-description tab changes free of stale transformation popups."""
# P710 native snapshot update: tests/native-core/check_item_desc_tab_redraw.py
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


def function_body(source: str, name: str) -> str | None:
    match = re.search(rf"(?:void|BOOLEAN)\s+{name}\s*\([^)]*\)\s*\{{(.*?)\n\}}", source, re.DOTALL)
    return None if match is None else match.group(1)


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: check_item_desc_tab_redraw.py <engine-root>", file=sys.stderr)
        return 2

    source = (Path(sys.argv[1]) / "Tactical" / "Interface Items.cpp").read_text(encoding="utf-8")
    tab = function_body(source, "HandleItemDescTabButton")
    dismiss = function_body(source, "DismissItemDescTransformPopup")
    if tab is None or dismiss is None:
        print("FAIL: item-description tab redraw functions are missing", file=sys.stderr)
        return 1

    close = tab.find("DismissItemDescTransformPopup();")
    render = tab.find("RenderItemDescriptionBox();")
    checks = (
        (close >= 0 and render >= 0 and close < render,
         "tab switch does not retire the transformation popup before repainting"),
        ("gfSkipDestroyTransformPopup" in dismiss,
         "popup retirement ignores protected transformation ownership"),
        ("delete gItemDescTransformPopup;" in dismiss,
         "popup retirement does not release the popup owner"),
        ("gItemDescTransformPopup = NULL;" in dismiss and
         "gfItemDescTransformPopupInitialized = FALSE;" in dismiss and
         "gfItemDescTransformPopupVisible = FALSE;" in dismiss,
         "popup retirement leaves stale transformation state"),
    )
    failures = [message for passed, message in checks if not passed]
    if failures:
        for failure in failures:
            print(f"FAIL: {failure}", file=sys.stderr)
        return 1

    print(f"PASS: {len(checks)} weapon-description tab redraw checks")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
