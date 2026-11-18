#!/usr/bin/env python3
"""Check per-widget video-object ownership in laptop UI redraw paths."""
# P710 native snapshot update: tests/native-core/check_laptop_video_object_ownership.py
# Native Linux source/build/test adaptations captured in the accepted native checkpoint dated 2026-11-18.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-18.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.


from pathlib import Path
import re
import sys


def function_body(source: str, marker: str) -> str:
    start = source.rfind(marker)
    if start < 0:
        return ""
    opening = source.find("{", start)
    if opening < 0:
        return ""
    depth = 0
    for pos in range(opening, len(source)):
        if source[pos] == "{":
            depth += 1
        elif source[pos] == "}":
            depth -= 1
            if depth == 0:
                return source[opening + 1 : pos]
    return ""


def compact(text: str) -> str:
    return re.sub(r"\s+", "", text)


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: check_laptop_video_object_ownership.py ENGINE_ROOT", file=sys.stderr)
        return 2

    root = Path(sys.argv[1])
    base_h = (root / "Laptop" / "BaseTable.h").read_text(encoding="latin-1")
    base_cpp = (root / "Laptop" / "BaseTable.cpp").read_text(encoding="latin-1")
    drop_h = (root / "Laptop" / "DropDown.h").read_text(encoding="latin-1")
    drop_cpp = (root / "Laptop" / "DropDown.cpp").read_text(encoding="latin-1")
    compare = (root / "Laptop" / "merccompare.cpp").read_text(encoding="latin-1")

    table_create = compact(function_body(base_cpp, "TestTable::Create("))
    table_destroy = compact(function_body(base_cpp, "TestTable::Destroy("))
    dropdown_create = compact(function_body(drop_cpp, "DropDownBase::Create("))
    dropdown_destroy = compact(function_body(drop_cpp, "DropDownBase::Destroy("))
    display_merc = compact(function_body(compare, "BOOLEAN DisplayMercData("))

    failures = []
    if "UINT32mBackgroundImage" not in compact(base_h):
        failures.append("TestTable must own its background video object")
    if (
        "Destroy();" not in table_create
        or "AddVideoObject(&VObjectDesc,&mBackgroundImage)" not in table_create
        or table_create.find("Destroy();") > table_create.find("AddVideoObject(&VObjectDesc,&mBackgroundImage)")
        or "DeleteVideoObjectFromIndex(mBackgroundImage)" not in table_destroy
        or "guiHelpScreenBackGround" in base_cpp
    ):
        failures.append("TestTable rebuild must retire the old instance image before loading its replacement")

    if "UINT32mGoldArrowImages" not in compact(drop_h):
        failures.append("each DropDownBase must own its arrow video object")
    if (
        "AddVideoObject(&VObjectDesc,&mGoldArrowImages)" not in dropdown_create
        or "DeleteVideoObjectFromIndex(mGoldArrowImages)" not in dropdown_destroy
        or "guGoldArrowImages" in drop_cpp
    ):
        failures.append("dropdown creation and destruction must use the instance arrow image")

    first_delete = display_merc.find("DeleteVideoObjectFromIndex(uiInsMercFaceImageA)")
    second_load = display_merc.find("AddVideoObject(&VObjectDesc,&uiInsMercFaceImageB)")
    second_delete = display_merc.find("DeleteVideoObjectFromIndex(uiInsMercFaceImageB)")
    if (
        display_merc.count("AddVideoObject(&VObjectDesc,&uiInsMercFaceImageA)") != 2
        or display_merc.count("AddVideoObject(&VObjectDesc,&uiInsMercFaceImageB)") != 2
        or first_delete < 0
        or second_load < 0
        or second_delete < 0
        or not first_delete < second_load < second_delete
    ):
        failures.append("Merc Compare must release both portrait video objects, including second-load failure")

    if failures:
        for failure in failures:
            print(f"FAIL: {failure}", file=sys.stderr)
        print(f"FAIL: {len(failures)} laptop video-object ownership checks failed", file=sys.stderr)
        return 1

    print("PASS: table, dropdown, and Merc Compare video objects have stable owners")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
