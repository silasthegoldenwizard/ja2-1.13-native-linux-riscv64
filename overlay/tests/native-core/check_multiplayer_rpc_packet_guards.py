#!/usr/bin/env python3
"""Check multiplayer RPC payloads and wire soldier IDs are validated."""
# P710 native snapshot update: tests/native-core/check_multiplayer_rpc_packet_guards.py
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


def has_payload_guard(source: str, packet_type: str) -> bool:
    return bool(re.search(
        rf"rpcParameters\s*==\s*NULL\s*\|\|\s*"
        rf"rpcParameters->input\s*==\s*NULL\s*\|\|\s*"
        rf"rpcParameters->numberOfBitsOfData\s*<\s*sizeof\s*\(\s*{packet_type}\s*\)\s*\*\s*8",
        source,
    ))


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: check_multiplayer_rpc_packet_guards.py ENGINE_ROOT", file=sys.stderr)
        return 2

    root = Path(sys.argv[1])
    source = (root / "Multiplayer" / "client.cpp").read_text(encoding="latin-1")
    detonate = section(source, "void recieveDETONATEEXPLOSIVE (", "void send_disarm_explosive(")
    disarm = section(source, "void recieveDISARMEXPLOSIVE (", "void send_spreadeffect (")
    death = section(source, "void recieveDEATH (", "void send_hitstruct(")
    if not detonate or not disarm or not death:
        print("FAIL: one or more multiplayer receive handlers were not found", file=sys.stderr)
        return 1

    resolver = bool(re.search(
        r"static\s+SOLDIERTYPE\s*\*\s*ResolveMultiplayerSoldier\s*\(\s*SoldierID\s+soldierID\s*\).*?"
        r"soldierID\.i\s*>=\s*TOTAL_SOLDIERS.*?return\s+NULL\s*;.*?"
        r"return\s+MercPtrs\[\s*soldierID\.i\s*\]\s*;",
        source,
        re.DOTALL,
    ))

    checks = (
        (resolver, "wire soldier IDs can reach MercPtrs without a TOTAL_SOLDIERS bound"),
        (has_payload_guard(detonate, "detonate_struct"), "detonation reads a missing or truncated RPC payload"),
        (has_payload_guard(disarm, "disarm_struct"), "disarm reads a missing or truncated RPC payload"),
        (
            bool(re.search(
                r"MPDecodeSoldierID\s*\(\s*det->ubID\s*\).*?"
                r"ResolveMultiplayerSoldier\s*\(\s*det->ubID\s*\)",
                detonate,
                re.DOTALL,
            )),
            "detonation converts an unbounded decoded soldier ID to a pointer",
        ),
        (
            bool(re.search(
                r"MPDecodeSoldierID\s*\(\s*disarm->ubID\s*\).*?"
                r"ResolveMultiplayerSoldier\s*\(\s*disarm->ubID\s*\)",
                disarm,
                re.DOTALL,
            )),
            "disarm converts an unbounded decoded soldier ID to a pointer",
        ),
        (
            bool(re.search(
                r"ResolveMultiplayerSoldier\s*\(\s*nDeath->soldier_id\s*\)",
                death,
            )),
            "death converts an unbounded victim ID to a pointer",
        ),
        (
            bool(re.search(
                r"ResolveMultiplayerSoldier\s*\(\s*ubAttackerID\s*\)",
                death,
            )),
            "death converts an unbounded attacker ID to a pointer",
        ),
    )

    failures = [message for passed, message in checks if not passed]
    if failures:
        for failure in failures:
            print(f"FAIL: {failure}", file=sys.stderr)
        print(f"FAIL: {len(failures)} of {len(checks)} multiplayer RPC packet checks failed", file=sys.stderr)
        return 1

    print(f"PASS: {len(checks)} multiplayer RPC packet guards")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
