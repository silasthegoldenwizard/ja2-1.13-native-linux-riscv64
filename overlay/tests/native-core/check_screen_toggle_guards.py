#!/usr/bin/env python3
"""Lock the Options and Features checkbox-ID safety contract."""
# P710 native snapshot update: tests/native-core/check_screen_toggle_guards.py
# Native Linux source/build/test adaptations captured in the accepted native development checkpoint dated 2026-11-16.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-16.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.


from __future__ import annotations

import re
import sys
from pathlib import Path


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: check_screen_toggle_guards.py <engine-root>", file=sys.stderr)
        return 2

    root = Path(sys.argv[1])
    screens = (
        (root / "Ja2" / "Options Screen.cpp", "guiOptionsToggles", "GetOptionsToggleButton"),
        (root / "Ja2" / "FeaturesScreen.cpp", "guiFeaturesToggles", "GetFeaturesToggleButton"),
    )
    failures: list[str] = []
    checks = 0

    def require(condition: bool, label: str) -> None:
        nonlocal checks
        checks += 1
        if not condition:
            failures.append(label)

    for path, array_name, helper_name in screens:
        text = path.read_text(encoding="utf-8")
        label = path.name

        helper_pattern = re.compile(
            rf"static\s+GUI_BUTTON\s*\*\s*{helper_name}\s*\(UINT8\s+index\)\s*"
            rf"\{{.*?index\s*>=\s*MAX_NUMBER_OF_OPTION_TOGGLES.*?"
            rf"{array_name}\s*\[\s*index\s*\]\s*>=\s*MAX_BUTTONS.*?"
            rf"return\s+NULL\s*;.*?"
            rf"return\s+GetButtonPtr\s*\(\s*static_cast<INT32>\s*\(\s*"
            rf"{array_name}\s*\[\s*index\s*\]\s*\)\s*\)\s*;.*?\}}",
            re.DOTALL,
        )
        require(bool(helper_pattern.search(text)), f"{label}: bounded lookup helper changed or missing")

        sentinel = f"{array_name}[counter1] = static_cast<UINT32>(-1);"
        require(sentinel in text, f"{label}: toggle IDs are not initialized to an invalid sentinel")

        create = "const INT32 toggleButton = CreateCheckBoxButton"
        failure_check = "if (toggleButton < 0)"
        assignment = f"{array_name}[counter1] = static_cast<UINT32>(toggleButton);"
        create_pos = text.find(create)
        failure_pos = text.find(failure_check, create_pos + 1)
        assignment_pos = text.find(assignment, failure_pos + 1)
        require(create_pos >= 0, f"{label}: checkbox creation site changed or missing")
        require(
            create_pos < failure_pos < assignment_pos,
            f"{label}: checkbox result is stored before its failure check",
        )
        require(
            failure_pos >= 0 and "continue;" in text[failure_pos:assignment_pos],
            f"{label}: failed checkbox creation does not skip all ID consumers",
        )

        direct_index = re.search(rf"ButtonList\s*\[\s*{array_name}\s*\[", text)
        require(not direct_index, f"{label}: toggle ID directly indexes ButtonList")

        helper_reads = re.findall(rf"GetButtonPtr\s*\([^;\n]*{array_name}\s*\[", text)
        require(
            len(helper_reads) == 1,
            f"{label}: toggle IDs reach GetButtonPtr outside the bounded helper",
        )

        require(
            text.count(f"{helper_name}(") >= 6,
            f"{label}: toggle lifecycle no longer consistently uses the bounded helper",
        )

        for draw_name in ("DrawCheckBoxButtonOn", "DrawCheckBoxButtonOff"):
            draw = re.search(
                rf"{draw_name}\s*\(\s*(?:static_cast<INT32>\s*\(\s*)?"
                rf"{array_name}\s*\[\s*Button_UserData_0\s*\]",
                text,
            )
            guarded_prefix = text[max(0, draw.start() - 500):draw.start()] if draw else ""
            require(
                bool(draw)
                and bool(re.search(
                    rf"GUI_BUTTON\s*\*\s*toggleButton\s*=\s*{helper_name}\s*\("
                    rf"Button_UserData_0\s*\)",
                    guarded_prefix,
                ))
                and bool(re.search(
                    r"if\s*\(\s*fDown\s*&&\s*toggleButton\s*\)\s*$",
                    guarded_prefix,
                )),
                f"{label}: {draw_name} is not guarded by bounded lookup",
            )

    if failures:
        for failure in failures:
            print(f"FAIL: {failure}", file=sys.stderr)
        print(f"FAIL: {len(failures)} of {checks} screen-toggle guard checks failed", file=sys.stderr)
        return 1

    print(f"PASS: {checks} screen-toggle guard checks across {len(screens)} screens")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
