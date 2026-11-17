#!/usr/bin/env python3
"""Check production dialogue-event pointer boundaries."""
# P710 native snapshot update: tests/native-core/check_dialogue_event_guards.py
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
        print("usage: check_dialogue_event_guards.py ENGINE_ROOT", file=sys.stderr)
        return 2

    source = (Path(sys.argv[1]) / "Tactical" / "Dialogue Control.cpp").read_text(
        encoding="latin-1"
    )
    match = re.search(
        r"void\s+HandleDialogue\s*\([^)]*\)\s*\{(?P<body>.*?)\n\}",
        source,
        re.DOTALL,
    )
    if not match:
        print("FAIL: HandleDialogue body was not found", file=sys.stderr)
        return 1
    body = match.group("body")

    checks = (
        (
            bool(re.search(
                r"DIALOGUE_SPECIAL_EVENT_SHOW_CONTRACT_MENU\s*\)\s*\{\s*"
                r"if\s*\(\s*pSoldier\s*!=\s*NULL\s*\)\s*\{.*?"
                r"SetInfoChar\s*\(\s*pSoldier->ubID\s*\)",
                body,
                re.DOTALL,
            )),
            "contract-menu event can read a missing soldier",
        ),
        (
            bool(re.search(
                r"DIALOGUE_SPECIAL_EVENT_PCTRIGGERNPC.*?"
                r"if\s*\(\s*ExecuteCharacterDialogue\s*\(.*?\)\s*&&\s*"
                r"gpCurrentTalkingFace\s*!=\s*NULL\s*\)\s*\{.*?"
                r"FACE_PCTRIGGER_NPC",
                body,
                re.DOTALL,
            )),
            "NPC-trigger event can write a missing talking face",
        ),
        (
            bool(re.search(
                r"DIALOGUE_SPECIAL_EVENT_BEGINPREBATTLEINTERFACE.*?"
                r"if\s*\(\s*ExecuteCharacterDialogue\s*\(.*?\)\s*&&\s*"
                r"gpCurrentTalkingFace\s*!=\s*NULL\s*\)\s*\{.*?"
                r"FACE_TRIGGER_PREBATTLE_INT",
                body,
                re.DOTALL,
            )),
            "prebattle event can write a missing talking face",
        ),
        (
            bool(re.search(
                r"DIALOGUE_Q_STRUCT\s+QItem\s*\{\s*\}\s*;\s*"
                r"if\s*\(\s*!PopFromDialogueQueue\s*\(\s*&QItem\s*\)\s*\)"
                r"\s*\{\s*return\s*;",
                body,
                re.DOTALL,
            )),
            "an emptied dialogue queue can expose an uninitialized event",
        ),
    )

    failures = [message for passed, message in checks if not passed]
    if failures:
        for failure in failures:
            print(f"FAIL: {failure}", file=sys.stderr)
        print(f"FAIL: {len(failures)} of {len(checks)} dialogue event checks failed", file=sys.stderr)
        return 1

    print(f"PASS: {len(checks)} dialogue event pointer guards")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
