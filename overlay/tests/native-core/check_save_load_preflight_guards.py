#!/usr/bin/env python3
"""Check save-list paging and non-destructive header-preflight handling."""
# P710 native snapshot update: tests/native-core/check_save_load_preflight_guards.py
# Native Linux source/build/test adaptations captured in the accepted review correction checkpoint dated 2026-11-17.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-17.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.


from pathlib import Path
import re
import sys


def function_body(source: str, signature: str, next_signature: str) -> str:
    start = source.find(signature)
    end = source.find(next_signature, start + len(signature))
    return source[start:end] if start >= 0 and end >= 0 else ""


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: check_save_load_preflight_guards.py ENGINE_ROOT", file=sys.stderr)
        return 2

    source = (Path(sys.argv[1]) / "Ja2" / "SaveLoadScreen.cpp").read_text(
        encoding="latin-1"
    )
    init = function_body(source, "BOOLEAN InitSaveGameArray()", "BOOLEAN DisplaySaveGameList()")
    callback = function_body(
        source,
        "void SaveHeaderLoadFailedCallBack( UINT8 bExitValue )",
        "void FailedLoadingGameCallBack( UINT8 bExitValue )",
    )
    failed_loading = function_body(
        source,
        "void FailedLoadingGameCallBack( UINT8 bExitValue )",
        "#ifdef _WIN32",
    )

    checks = [
        (
            "paged save listing passes the page-relative slot to the header loader",
            "LoadSavedGameHeader( cnt, &SaveGameHeader )" in init
            and "LoadSavedGameHeader( cnt2, &SaveGameHeader )" not in init,
        ),
        (
            "header preflight failure uses the non-destructive callback",
            bool(
                re.search(
                    r"ubRetVal\s*==\s*SLS_HEADER_LOAD_FAILED[\s\S]{0,260}"
                    r"SaveHeaderLoadFailedCallBack",
                    source,
                )
            ),
        ),
        (
            "preflight callback invalidates only the selected absolute slot",
            bool(callback)
            and "VAL_SLOT_START" in callback
            and "gbSelectedSaveLocation" in callback
            and "gbSaveGameArray[ uiArrayEntry ] = FALSE" in callback,
        ),
        (
            "preflight callback clears selection and requests a redraw",
            "gbSelectedSaveLocation = -1" in callback
            and "gfRedrawSaveLoadScreen = TRUE" in callback,
        ),
        (
            "preflight callback cannot restart or begin loading",
            bool(callback)
            and all(
                forbidden not in callback
                for forbidden in (
                    "ReStartingGame",
                    "LoadSavedGame(",
                    "StartFadeOutForSaveLoadScreen",
                    "MAINMENU_SCREEN",
                )
            ),
        ),
        (
            "post-load failure recovery retains the restart callback",
            bool(failed_loading)
            and "ReStartingGame" in failed_loading
            and source.count("FailedLoadingGameCallBack );") >= 5,
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
        print(f"FAIL: {failures} save/load preflight checks failed", file=sys.stderr)
        return 1
    print(f"PASS: {len(checks)} save/load preflight checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
