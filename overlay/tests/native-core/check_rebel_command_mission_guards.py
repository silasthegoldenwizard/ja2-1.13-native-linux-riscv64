#!/usr/bin/env python3
"""Check mission and agent validation at the Rebel Command launch boundary."""
# P710 native snapshot update: tests/native-core/check_rebel_command_mission_guards.py
# Native Linux source/build/test adaptations captured in the accepted strategic runtime-state checkpoint dated 2026-11-17.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-17.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.

# P710 native snapshot update: tests/native-core/check_rebel_command_mission_guards.py
# Native Linux source/build/test adaptations captured in the accepted review correction checkpoint dated 2026-11-17.
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
        print("usage: check_rebel_command_mission_guards.py ENGINE_ROOT", file=sys.stderr)
        return 2

    root = Path(sys.argv[1])
    source = (root / "Strategic" / "Rebel Command.cpp").read_text(
        encoding="latin-1"
    )
    start = source.find("void PrepareMission(INT8 index)\n{")
    end = source.find("// end website", start)
    body = source[start:end] if start >= 0 and end >= 0 else ""
    helper_start = source.find("BOOLEAN ResolveRebelCommandAgentSelection(")
    helper_end = source.find("void PrepareMission(INT8 index)", helper_start)
    helper = (
        source[helper_start:helper_end]
        if helper_start >= 0 and helper_end >= 0
        else ""
    )
    if not body:
        print("FAIL: Rebel Command mission preparation function was not found", file=sys.stderr)
        return 1

    checks = [
        (
            "mission slot is range-checked before indexed state is read",
            bool(
                re.search(
                    r"index\s*<\s*0\s*\|\|\s*index\s*>=\s*NUM_ARC_AGENT_SLOTS",
                    body,
                )
            )
            and body.find("index < 0") < body.find("rebelCommandSaveInfo.availableMissions[index]"),
        ),
        (
            "mission id is captured and constrained to implemented missions",
            bool(
                re.search(
                    r"mission(?:Id)?\s*=\s*rebelCommandSaveInfo\.availableMissions\[index\]",
                    body,
                )
                and re.search(
                    r"mission(?:Id)?\s*<\s*RCAM_DEEP_DEPLOYMENT\s*\|\|\s*"
                    r"mission(?:Id)?\s*>=\s*RCAM_NUM_MISSIONS",
                    body,
                )
            ),
        ),
        (
            "negative agent selections are rejected before widening",
            bool(helper)
            and "selectedAgent < 0" in helper
            and helper.find("selectedAgent < 0")
            < helper.find("static_cast<std::size_t>( selectedAgent )"),
        ),
        (
            "agent selection is widened and compared without an INT8 size cast",
            bool(helper)
            and "const std::size_t selectedAgentIndex" in helper
            and "selectedAgentIndex > rosterSize" in helper
            and "static_cast<INT8>(mercs.size())" not in helper
            and "static_cast<INT8>(mercs.size())" not in body,
        ),
        (
            "representable roster-size sentinel selects the generic agent",
            "*pIsGenericAgent = selectedAgentIndex == rosterSize" in helper,
        ),
        (
            "mission preparation uses the checked widened selection",
            "std::size_t selectedAgentIndex = 0" in body
            and "ResolveRebelCommandAgentSelection(agentIndex[index], mercs.size()" in body,
        ),
        (
            "agent pointer is resolved only after the selection check",
            bool(
                re.search(
                    r"SOLDIERTYPE\s*\*\s*(?:const\s+)?selectedSoldier\s*=\s*"
                    r"isGenericAgent\s*\?\s*nullptr\s*:\s*mercs\[selectedAgentIndex\]",
                    body,
                )
                and re.search(
                    r"const\s+MERCPROFILESTRUCT\s*\*\s*merc\s*=\s*"
                    r"selectedSoldier\s*\?",
                    body,
                )
            ),
        ),
        (
            "missions requiring a merc reject a generic agent",
            "RCAM_SEND_SUPPLIES_TO_TOWN" in body
            and "RCAM_FORGE_TRANSPORT_ORDERS" in body
            and bool(
                re.search(
                    r"isGenericAgent[\s\S]{0,260}RCAM_SEND_SUPPLIES_TO_TOWN"
                    r"[\s\S]{0,160}RCAM_FORGE_TRANSPORT_ORDERS[\s\S]{0,80}return",
                    body,
                )
            ),
        ),
        (
            "unsupported mission switch exits instead of using undefined state",
            bool(re.search(r"default\s*:\s*return\s*;", body)),
        ),
        (
            "deferred soldier lookup tolerates missing team entries",
            bool(
                re.search(
                    r"if\s*\(\s*pSoldier\s*&&\s*pSoldier->ubProfile\s*==\s*"
                    r"evt\.mercProfileId\s*\)",
                    body,
                )
            ),
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
        print(f"FAIL: {failures} Rebel Command mission guard checks failed", file=sys.stderr)
        return 1

    print(f"PASS: {len(checks)} Rebel Command mission guard checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
