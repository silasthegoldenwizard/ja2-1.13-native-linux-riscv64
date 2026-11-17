/*
 * P710 native snapshot update: tests/native-core/native_rebel_mission_selection_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted review correction checkpoint dated 2026-11-17.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-17.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Production-linked Rebel Command agent-selection boundary tests.
#include "types.h"

#include <cstddef>
#include <iostream>
#include <stdexcept>

namespace RebelCommand {
BOOLEAN ResolveRebelCommandAgentSelection(
    INT8 selectedAgent, std::size_t rosterSize,
    std::size_t* selectedIndex, BOOLEAN* isGenericAgent);
}

using RebelCommand::ResolveRebelCommandAgentSelection;

namespace {
unsigned checks = 0;

void check(bool condition, const char* why) {
    ++checks;
    if (!condition) throw std::runtime_error(why);
}

void expectSelection(INT8 selectedAgent, std::size_t rosterSize,
                     std::size_t expectedIndex, BOOLEAN expectedGeneric,
                     const char* why) {
    std::size_t selectedIndex = 999;
    BOOLEAN isGenericAgent = static_cast<BOOLEAN>(!expectedGeneric);
    check(ResolveRebelCommandAgentSelection(selectedAgent, rosterSize,
                                            &selectedIndex, &isGenericAgent),
          why);
    check(selectedIndex == expectedIndex, "resolved agent index is exact");
    check(isGenericAgent == expectedGeneric, "generic-agent result is exact");
}
}

int main() {
    try {
        expectSelection(0, 127, 0, FALSE,
                        "index zero remains valid with 127 mercs");
        expectSelection(0, 128, 0, FALSE,
                        "index zero remains valid with 128 mercs");
        expectSelection(127, 128, 127, FALSE,
                        "highest INT8 index remains a real merc with 128 mercs");
        expectSelection(127, 127, 127, TRUE,
                        "representable roster-size sentinel selects generic agent");

        std::size_t selectedIndex = 999;
        BOOLEAN isGenericAgent = TRUE;
        check(!ResolveRebelCommandAgentSelection(-1, 128, &selectedIndex,
                                                 &isGenericAgent),
              "negative selection is rejected");
        check(!ResolveRebelCommandAgentSelection(1, 0, &selectedIndex,
                                                 &isGenericAgent),
              "selection beyond the roster is rejected");
        check(!ResolveRebelCommandAgentSelection(0, 1, nullptr,
                                                 &isGenericAgent),
              "null index output is rejected");
        check(!ResolveRebelCommandAgentSelection(0, 1, &selectedIndex, nullptr),
              "null generic output is rejected");

        std::cout << "PASS: " << checks
                  << " production Rebel Command selection assertions\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL after " << checks << " assertions: "
                  << error.what() << '\n';
        return 1;
    }
}
