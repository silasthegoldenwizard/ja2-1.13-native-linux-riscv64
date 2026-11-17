#!/usr/bin/env python3
"""Check multiplayer start and death RPC boundaries remain guarded."""
# P710 native snapshot update: tests/native-core/check_multiplayer_start_and_death_guards.py
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
        print("usage: check_multiplayer_start_and_death_guards.py ENGINE_ROOT", file=sys.stderr)
        return 2

    root = Path(sys.argv[1])
    client_source = (root / "Multiplayer" / "client.cpp").read_text(encoding="latin-1")
    server_source = (root / "Multiplayer" / "server.cpp").read_text(encoding="latin-1")
    start = section(client_source, "void start_battle (", "void DropOffItemsInSector(")
    receive = section(client_source, "void recieveDEATH (", "void send_hitstruct(")
    relay = section(server_source, "void sendDEATH(", "void sendhitSTRUCT(")
    if not start or not receive or not relay:
        print("FAIL: one or more multiplayer boundaries were not found", file=sys.stderr)
        return 1

    checks = (
        (
            bool(re.search(
                r"SOLDIERTYPE\s*\*\s*pSoldier\s*=\s*NULL\s*;.*?"
                r"if\s*\(\s*pSoldier\s*==\s*NULL\s*\)\s*\{.*?return\s*;.*?"
                r"UINT8\s+ubGroupID\s*=\s*pSoldier->ubGroupID",
                start,
                re.DOTALL,
            )),
            "start_battle uses an uninitialized or missing active soldier",
        ),
        (
            bool(re.search(
                r"GROUP\s*\*\s*pGroup\s*=\s*GetGroup\s*\(\s*ubGroupID\s*\)\s*;\s*"
                r"if\s*\(\s*pGroup\s*==\s*NULL\s*\)\s*\{?\s*return\s*;.*?"
                r"gpBattleGroup\s*=\s*pGroup",
                start,
                re.DOTALL,
            )),
            "start_battle dereferences a missing movement group",
        ),
        (
            bool(re.search(r"CHAR16\s+name\[30\]\s*=\s*\{\s*0\s*\}\s*;", start))
            and bool(re.search(r"size_t\s+nm\s*=\s*mbstowcs\s*\(\s*name\s*,\s*client_names\[i\]\s*,\s*29\s*\)", start))
            and bool(re.search(r"nm\s*!=\s*\(size_t\)-1\s*&&\s*nm\s*>\s*0", start))
            and "name[nm] = L'\\0';" in start,
            "start_battle accepts failed or unterminated client-name conversion",
        ),
        (
            bool(re.search(
                r"if\s*\(\s*rpcParameters\s*==\s*NULL\s*\|\|\s*"
                r"rpcParameters->input\s*==\s*NULL\s*\|\|\s*"
                r"rpcParameters->numberOfBitsOfData\s*<\s*sizeof\s*\(\s*death_struct\s*\)\s*\*\s*8\s*\)",
                receive,
            )),
            "recieveDEATH reads a missing or truncated RPC payload",
        ),
        (
            bool(re.search(
                r"if\s*\(\s*rpcParameters\s*==\s*NULL\s*\|\|\s*"
                r"rpcParameters->input\s*==\s*NULL\s*\|\|\s*"
                r"rpcParameters->numberOfBitsOfData\s*<\s*sizeof\s*\(\s*death_struct\s*\)\s*\*\s*8\s*\)",
                relay,
            )),
            "sendDEATH reads a missing or truncated RPC payload",
        ),
        (
            bool(re.search(
                r"nDeath->soldier_team\s*<\s*1\s*\|\|\s*nDeath->soldier_team\s*>\s*5\s*\|\|\s*"
                r"nDeath->attacker_team\s*<\s*1\s*\|\|\s*nDeath->attacker_team\s*>\s*5.*?return\s*;.*?"
                r"gMPPlayerStats\[nDeath->soldier_team-1\]",
                relay,
                re.DOTALL,
            )),
            "sendDEATH indexes scoreboard arrays with untrusted team IDs",
        ),
    )

    failures = [message for passed, message in checks if not passed]
    if failures:
        for failure in failures:
            print(f"FAIL: {failure}", file=sys.stderr)
        print(f"FAIL: {len(failures)} of {len(checks)} multiplayer start/death checks failed", file=sys.stderr)
        return 1

    print(f"PASS: {len(checks)} multiplayer start and death guards")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
