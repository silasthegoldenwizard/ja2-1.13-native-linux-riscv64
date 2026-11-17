#!/usr/bin/env python3
"""Check legacy NPC quote loading and movement pointer boundaries."""
# P710 native snapshot update: tests/native-core/check_npc_quote_guards.py
# Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-17.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.


from pathlib import Path
import re
import sys


def section(source: str, start_text: str, end_text: str) -> str:
    start = source.find(start_text)
    end = source.find(end_text, start)
    return source[start:end] if start >= 0 and end >= 0 else ""


def legacy_loader_guarded(body: str) -> bool:
    return bool(re.search(
        r"FileClose\s*\(\s*hFile\s*\)\s*;\s*"
        r"if\s*\(\s*pFileData_old_\s*==\s*NULL\s*\)\s*\{\s*"
        r"return\s+NULL\s*;",
        body,
        re.DOTALL,
    ))


def conversion_allocations_guarded(body: str) -> bool:
    english = bool(re.search(
        r"pEnglishScript\s*=.*?MemAlloc\s*\(\s*uiFileSize\s*\)\s*;\s*"
        r"if\s*\(\s*pEnglishScript\s*==\s*NULL\s*\)\s*\{\s*"
        r"MemFree\s*\(\s*pFileData_old_\s*\)\s*;\s*return\s+NULL\s*;",
        body,
        re.DOTALL,
    ))
    converted = bool(re.search(
        r"pFileData\s*=.*?MemAlloc\s*\(\s*uiFileSize\s*\)\s*;\s*"
        r"if\s*\(\s*pFileData\s*==\s*NULL\s*\)\s*\{\s*"
        r"MemFree\s*\(\s*pFileData_old_\s*\)\s*;\s*return\s+NULL\s*;",
        body,
        re.DOTALL,
    ))
    return english and converted


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: check_npc_quote_guards.py ENGINE_ROOT", file=sys.stderr)
        return 2

    source = (Path(sys.argv[1]) / "TacticalAI" / "NPC.cpp").read_text(
        encoding="latin-1"
    )
    quote = section(source, "NPCQuoteInfo * LoadQuoteFile(", "NPCQuoteInfo& NPCQuoteInfo::operator=")
    civ = section(source, "NPCQuoteInfo * LoadCivQuoteFile(", "BOOLEAN EnsureCivQuoteFileLoaded(")
    converse = section(source, "void Converse(", "INT32 NPCConsiderInitiatingConv(")
    if not quote or not civ or not converse:
        print("FAIL: one or more NPC function bodies were not found", file=sys.stderr)
        return 1

    checks = (
        (legacy_loader_guarded(quote), "LoadQuoteFile can convert a missing legacy buffer"),
        (conversion_allocations_guarded(quote), "LoadQuoteFile does not guard conversion allocations"),
        (legacy_loader_guarded(civ), "LoadCivQuoteFile can convert a missing legacy buffer"),
        (conversion_allocations_guarded(civ), "LoadCivQuoteFile does not guard conversion allocations"),
        (
            bool(re.search(
                r"if\s*\(\s*pSoldier->flags\.uiStatusFlags\s*&\s*SOLDIER_COWERING\s*\).*?"
                r"pSoldier->EVENT_InitNewSoldierAnim",
                converse,
                re.DOTALL,
            )),
            "movement handling can reuse a missing initial NPC pointer",
        ),
    )

    failures = [message for passed, message in checks if not passed]
    if failures:
        for failure in failures:
            print(f"FAIL: {failure}", file=sys.stderr)
        print(f"FAIL: {len(failures)} of {len(checks)} NPC checks failed", file=sys.stderr)
        return 1

    print(f"PASS: {len(checks)} NPC quote and movement pointer guards")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
