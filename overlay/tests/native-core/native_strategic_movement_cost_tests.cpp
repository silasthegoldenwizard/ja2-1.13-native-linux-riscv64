/*
 * P710 native snapshot update: tests/native-core/native_strategic_movement_cost_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-17.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Production-linked failure checks for strategic movement-cost export.
#include "XML.h"
#include <iostream>
#include <stdexcept>

namespace {
unsigned checks = 0;

void check(bool condition, const char* why) {
    ++checks;
    if (!condition) throw std::runtime_error(why);
}
}

int main() {
    try {
        check(!WriteInStrategicMovementCosts(nullptr),
              "a null movement-cost output path is rejected");

        char missingParent[] =
            "/tmp/p710-movement-cost-regression/missing-parent/StrategicMovementCosts.xml";
        check(!WriteInStrategicMovementCosts(missingParent),
              "a failed movement-cost output open is reported");

        std::cout << "PASS: " << checks
                  << " production strategic movement-cost failure assertions\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL after " << checks << " assertions: " << error.what() << '\n';
        return 1;
    }
}
