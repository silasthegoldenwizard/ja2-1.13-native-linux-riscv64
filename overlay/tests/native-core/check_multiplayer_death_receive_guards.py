#!/usr/bin/env python3
"""Check multiplayer death receipt rejects missing and invalid owners safely."""
# P710 native snapshot update: tests/native-core/check_multiplayer_death_receive_guards.py
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


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: check_multiplayer_death_receive_guards.py ENGINE_ROOT", file=sys.stderr)
        return 2

    root = Path(sys.argv[1])
    source = (root / "Multiplayer" / "client.cpp").read_text(encoding="latin-1")
    receive = section(source, "void recieveDEATH (", "void send_hitstruct(")
    if not receive:
        print("FAIL: recieveDEATH was not found", file=sys.stderr)
        return 1

    checks = (
        (
            bool(re.search(
                r"SOLDIERTYPE\s*\*\s*pSoldier\s*=\s*"
                r"ResolveMultiplayerSoldier\s*\(\s*nDeath->soldier_id\s*\)\s*;\s*"
                r"if\s*\(\s*pSoldier\s*==\s*NULL\s*\)\s*\{?\s*return\s*;",
                receive,
                re.DOTALL,
            )),
            "recieveDEATH dereferences a missing victim",
        ),
        (
            bool(re.search(r"INT8\s+pA_bTeam\s*=\s*0\s*;", receive))
            and bool(re.search(r"CHAR16\s+pA_name\[\s*10\s*\]\s*=\s*\{\s*0\s*\}\s*;", receive)),
            "recieveDEATH leaves attacker display state uninitialized",
        ),
        (
            bool(re.search(
                r"static\s+const\s+char\s*\*\s*GetMultiplayerClientName\s*\(\s*INT8\s+bTeam\s*\).*?"
                r"bTeam\s*>=\s*1.*?bTeam\s*<=\s*4.*?client_names\[\s*bTeam\s*-\s*1\s*\].*?"
                r'return\s+"<\?>"\s*;',
                source,
                re.DOTALL,
            )),
            "multiplayer death messages can index outside the four client names",
        ),
        (
            bool(re.search(
                r"else\s+if\s*\(\s*pAttacker\s*!=\s*NULL\s*\)\s*"
                r"ScreenMsg\s*\(\s*FONT_LTGREEN.*?MPClientMessage\[28\]",
                receive,
                re.DOTALL,
            )),
            "recieveDEATH formats attacker details without an attacker",
        ),
        (
            "client_names[(pS_bTeam-1)]" not in receive
            and "client_names[(pA_bTeam-1)]" not in receive
            and "client_names[pA_bTeam-1]" not in receive
            and receive.count("GetMultiplayerClientName(") >= 2,
            "recieveDEATH still contains unchecked client-name indexing",
        ),
        (
            bool(re.search(
                r"else\s+if\s*\(\s*pAttacker\s*!=\s*NULL\s*&&\s*pAttacker->bTeam\s*==\s*1\s*\)",
                receive,
            )),
            "beta death diagnostics dereference a missing attacker",
        ),
    )

    failures = [message for passed, message in checks if not passed]
    if failures:
        for failure in failures:
            print(f"FAIL: {failure}", file=sys.stderr)
        print(f"FAIL: {len(failures)} of {len(checks)} multiplayer death checks failed", file=sys.stderr)
        return 1

    print(f"PASS: {len(checks)} multiplayer death receive guards")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
