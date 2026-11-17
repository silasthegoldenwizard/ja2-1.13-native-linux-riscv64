/*
 * P710 native snapshot update: tests/native-core/native_squad_save_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-17.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "BufferMan.h"
#include "Overhead.h"
#include "Squads.h"

#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

extern INT8 SquadMovementGroups[NUMBER_OF_SQUADS];

namespace {
using Bytes = std::vector<UINT8>;

struct SquadRecord {
    INT16 id;
    INT16 padding[5];
};

static_assert(sizeof(SquadRecord) == 12, "squad save record wire size changed");

unsigned checks = 0;
unsigned failures = 0;

void check(bool condition, const char* message)
{
    ++checks;
    if (condition) return;
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
}

template<class T>
void append(Bytes& bytes, const T& value)
{
    const auto* begin = reinterpret_cast<const UINT8*>(&value);
    bytes.insert(bytes.end(), begin, begin + sizeof(value));
}

Bytes squadData(INT16 firstID, bool includeMovement = true)
{
    Bytes bytes;
    for (INT32 squad = 0; squad < NUMBER_OF_SQUADS; ++squad) {
        for (INT32 slot = 0; slot < NUMBER_OF_SOLDIERS_PER_SQUAD; ++slot) {
            SquadRecord record{};
            record.id = squad == 0 && slot == 0 ? firstID : -1;
            append(bytes, record);
        }
    }
    if (includeMovement) {
        for (INT32 squad = 0; squad < NUMBER_OF_SQUADS; ++squad) {
            const INT8 movement = static_cast<INT8>(squad + 1);
            append(bytes, movement);
        }
    }
    return bytes;
}

BOOLEAN load(Bytes& bytes, std::size_t& consumed)
{
    gpLoadBufferStart = reinterpret_cast<CHAR8*>(bytes.data());
    giLoadBufferSize = static_cast<INT32>(bytes.size());
    gfBufferLoadFailed = FALSE;
    auto* begin = reinterpret_cast<CHAR8*>(bytes.data());
    auto* cursor = begin;
    const BOOLEAN result = LoadSquadInfoFromSavedGameFile(&cursor);
    consumed = static_cast<std::size_t>(cursor - begin);
    gpLoadBufferStart = nullptr;
    giLoadBufferSize = 0;
    return result;
}

bool squadsAreEmpty()
{
    for (INT32 squad = 0; squad < NUMBER_OF_SQUADS; ++squad) {
        for (INT32 slot = 0; slot < NUMBER_OF_SOLDIERS_PER_SQUAD; ++slot) {
            if (Squad[squad][slot] != nullptr) return false;
        }
    }
    return true;
}

bool movementIsEmpty()
{
    for (const auto value : SquadMovementGroups) {
        if (value != 0) return false;
    }
    return true;
}

void clearState()
{
    auto empty = squadData(-1);
    std::size_t consumed = 0;
    check(load(empty, consumed), "empty squad table loads");
    check(consumed == empty.size(), "empty squad table consumes its exact extent");
    check(squadsAreEmpty(), "empty squad table clears all member pointers");
}

void invalidIDRegression(INT16 id)
{
    auto invalid = squadData(id);
    std::size_t consumed = 0;
    check(!load(invalid, consumed), "out-of-range squad member ID is rejected");
    check(!gfBufferLoadFailed,
          "invalid squad member ID is a semantic rejection, not a short read");
    check(consumed <= invalid.size(), "invalid-ID cursor remains in bounds");
    check(squadsAreEmpty(), "invalid member ID leaves no live squad pointers");
    check(movementIsEmpty(), "invalid member ID leaves no movement IDs");
}

int finish()
{
    gpLoadBufferStart = nullptr;
    giLoadBufferSize = 0;
    gfBufferLoadFailed = FALSE;
    if (failures) {
        std::cerr << "FAIL: " << failures << " of " << checks
                  << " squad-save lifecycle assertions\n";
        return 1;
    }
    std::cout << "PASS: " << checks
              << " squad-save lifecycle assertions\n";
    return 0;
}
}

int main(int argc, char** argv)
{
    if (argc == 2 && std::strcmp(argv[1], "--invalid-id") == 0) {
        invalidIDRegression(INT16_MAX);
        return finish();
    }

    clearState();

    auto valid = squadData(0);
    std::size_t consumed = 0;
    check(load(valid, consumed), "valid squad member loads");
    check(consumed == valid.size(), "valid squad data consumes its exact extent");
    check(Squad[0][0] == &Menptr[0], "valid member ID rebuilds the live soldier pointer");
    check(Squad[0][1] == nullptr, "empty member slot remains null");
    check(SquadMovementGroups[0] == 1 &&
              SquadMovementGroups[NUMBER_OF_SQUADS - 1] == NUMBER_OF_SQUADS,
          "movement IDs publish after the squad table");

    auto missingMovement = squadData(0, false);
    consumed = 0;
    check(!load(missingMovement, consumed), "missing movement table is rejected");
    check(gfBufferLoadFailed, "missing movement table marks the bounded load failure");
    check(consumed <= missingMovement.size(), "missing-movement cursor remains in bounds");
    check(squadsAreEmpty(), "missing movement table rolls back squad pointers");
    check(movementIsEmpty(), "missing movement table rolls back movement IDs");

    auto truncatedTable = squadData(-1);
    truncatedTable.resize(truncatedTable.size() / 2);
    consumed = 0;
    check(!load(truncatedTable, consumed), "truncated squad table is rejected");
    check(gfBufferLoadFailed, "truncated squad table marks the bounded load failure");
    check(consumed <= truncatedTable.size(), "truncated-table cursor remains in bounds");
    check(squadsAreEmpty(), "truncated squad table leaves no member pointers");
    check(movementIsEmpty(), "truncated squad table leaves no movement IDs");

    invalidIDRegression(INT16_MAX);
    invalidIDRegression(-2);
    clearState();
    return finish();
}
