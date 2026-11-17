#!/usr/bin/env python3
"""Check the first bounded family of failure-capable GUI creation sites."""
# P710 native snapshot update: tests/native-core/check_ui_creation_guards.py
# Native Linux source/build/test adaptations captured in the accepted native development checkpoint dated 2026-11-16.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-16.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.

# P710 native snapshot update: tests/native-core/check_ui_creation_guards.py
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
        print("usage: check_ui_creation_guards.py <engine-root>", file=sys.stderr)
        return 2

    root = Path(sys.argv[1])
    failures: list[str] = []
    checks = 0

    def require(condition: bool, label: str) -> None:
        nonlocal checks
        checks += 1
        if not condition:
            failures.append(label)

    save_screens = (
        (root / "Ja2" / "Options Screen.cpp", "GetOptionsSaveGameButton"),
        (root / "Ja2" / "FeaturesScreen.cpp", "GetFeaturesSaveGameButton"),
    )
    for path, helper in save_screens:
        text = path.read_text(encoding="utf-8")
        label = path.name
        require(
            bool(re.search(
                rf"static\s+GUI_BUTTON\s*\*\s*{helper}\s*\(\s*\)\s*\{{.*?"
                rf"guiOptGotoSaveGameBtn\s*>=\s*MAX_BUTTONS.*?return\s+NULL\s*;.*?"
                rf"return\s+GetButtonPtr\s*\(\s*static_cast<INT32>\s*\(\s*"
                rf"guiOptGotoSaveGameBtn\s*\)\s*\)\s*;.*?\}}",
                text,
                re.DOTALL,
            )),
            f"{label}: save-button lookup is not bounded",
        )
        require(
            "guiOptGotoSaveGameBtn = static_cast<UINT32>(-1);" in text,
            f"{label}: save-button ID has no invalid sentinel",
        )
        create_pos = text.find("const INT32 saveGameButton = CreateIconAndTextButton")
        check_pos = text.find("if (saveGameButton >= 0)", create_pos + 1)
        store_pos = text.find(
            "guiOptGotoSaveGameBtn = static_cast<UINT32>(saveGameButton);",
            check_pos + 1,
        )
        require(
            create_pos >= 0 and create_pos < check_pos < store_pos,
            f"{label}: failed save-button creation can reach the stored ID",
        )
        require(
            not re.search(r"ButtonList\s*\[\s*guiOptGotoSaveGameBtn\s*\]", text),
            f"{label}: save-button ID directly indexes ButtonList",
        )
        require(
            text.count(f"{helper}(") >= 3,
            f"{label}: save-button lifecycle does not use the bounded helper",
        )
        require(
            bool(re.search(
                rf"if\s*\(GUI_BUTTON\s*\*\s*saveGameButton\s*=\s*{helper}\s*\(\s*\)\s*\)"
                rf".*?saveGameButton->uiFlags\s*&\s*BUTTON_ENABLED",
                text,
                re.DOTALL,
            )),
            f"{label}: S-key path does not validate the save-button ID",
        )

    help_path = root / "Ja2" / "HelpScreen.cpp"
    help_text = help_path.read_text(encoding="utf-8")
    require(
        bool(re.search(
            r"static\s+GUI_BUTTON\s*\*\s*GetHelpDontShowAgainButton\s*\(\s*\)\s*\{.*?"
            r"gHelpScreenDontShowHelpAgainToggle\s*>=\s*MAX_BUTTONS.*?return\s+NULL\s*;.*?"
            r"return\s+GetButtonPtr\s*\(\s*static_cast<INT32>\s*\(\s*"
            r"gHelpScreenDontShowHelpAgainToggle\s*\)\s*\)\s*;.*?\}",
            help_text,
            re.DOTALL,
        )),
        "HelpScreen.cpp: help-checkbox lookup is not bounded",
    )
    require(
        "gHelpScreenDontShowHelpAgainToggle = static_cast<UINT32>(-1);" in help_text,
        "HelpScreen.cpp: help-checkbox ID has no invalid sentinel",
    )
    create_pos = help_text.find("const INT32 dontShowAgainButton = CreateCheckBoxButton")
    check_pos = help_text.find("if (dontShowAgainButton >= 0)", create_pos + 1)
    store_pos = help_text.find(
        "gHelpScreenDontShowHelpAgainToggle = static_cast<UINT32>(dontShowAgainButton);",
        check_pos + 1,
    )
    require(
        create_pos >= 0 and create_pos < check_pos < store_pos,
        "HelpScreen.cpp: failed help-checkbox creation can reach the stored ID",
    )
    require(
        not re.search(
            r"ButtonList\s*\[\s*gHelpScreenDontShowHelpAgainToggle\s*\]",
            help_text,
        ),
        "HelpScreen.cpp: help-checkbox ID directly indexes ButtonList",
    )
    require(
        help_text.count("GetHelpDontShowAgainButton(") >= 4,
        "HelpScreen.cpp: help-checkbox lifecycle does not use the bounded helper",
    )

    interface_path = root / "Tactical" / "Interface.cpp"
    interface_text = interface_path.read_text(encoding="utf-8")
    movement_match = re.search(
        r"void\s+PopupMovementMenu\s*\(\s*UI_EVENT\s*\*pUIEvent\s*\)\s*\{"
        r"(?P<body>.*?)\n\}",
        interface_text,
        re.DOTALL,
    )
    movement_body = movement_match.group("body") if movement_match else ""
    nobody_guard = movement_body.find("if ( gusSelectedSoldier == NOBODY )")
    soldier_assign = movement_body.find("pSoldier = gusSelectedSoldier;")
    pointer_guard = movement_body.find("if ( pSoldier == NULL )", soldier_assign)
    first_use = movement_body.find("pSoldier->MercInWater")
    require(bool(movement_match), "Interface.cpp: movement-menu function is missing")
    require(
        0 <= nobody_guard < soldier_assign,
        "Interface.cpp: movement menu converts NOBODY to a soldier pointer",
    )
    require(
        soldier_assign < pointer_guard < first_use,
        "Interface.cpp: movement menu uses a missing selected-soldier pointer",
    )

    if failures:
        for failure in failures:
            print(f"FAIL: {failure}", file=sys.stderr)
        print(f"FAIL: {len(failures)} of {checks} GUI creation checks failed", file=sys.stderr)
        return 1

    print(f"PASS: {checks} guarded GUI creation checks across 4 screens")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
