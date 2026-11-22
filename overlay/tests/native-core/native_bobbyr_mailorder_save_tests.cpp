/*
 * P710 native snapshot update: tests/native-core/native_bobbyr_mailorder_save_tests.cpp
 * Native Linux source/build/test adaptations captured in the November 21 transactional save-loader checkpoint.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-21.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Genuine original Bobby Ray shipment loader with synthetic save bytes only.
// No game assets or existing saves are used.
#include "BobbyRMailOrder.h"
#include "BufferMan.h"
#include "MemMan.h"

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
using Bytes = std::vector<UINT8>;
unsigned checks = 0;

void check(bool condition, const char* why) {
    ++checks;
    if (!condition) throw std::runtime_error(why);
}

struct BufferBounds {
    explicit BufferBounds(Bytes& bytes) {
        gpLoadBufferStart = reinterpret_cast<char*>(bytes.data());
        giLoadBufferSize = static_cast<INT32>(bytes.size());
        gfBufferLoadFailed = FALSE;
    }
    ~BufferBounds() {
        gpLoadBufferStart = nullptr;
        giLoadBufferSize = 0;
        gfBufferLoadFailed = FALSE;
    }
};

Bytes countHeader(INT32 count) {
    const auto wire = static_cast<UINT32>(count);
    Bytes bytes(4, 0);
    for (unsigned i = 0; i < 4; ++i) bytes[i] = (wire >> (8 * i)) & 255;
    return bytes;
}

void seedPublishedShipment() {
    BobbyRayMailOrderEndGameShutDown();
    gpNewBobbyrShipments = static_cast<NewBobbyRayOrderStruct*>(
        MemAlloc(sizeof(NewBobbyRayOrderStruct)));
    check(gpNewBobbyrShipments != nullptr, "seed shipment allocated");
    std::memset(gpNewBobbyrShipments, 0, sizeof(NewBobbyRayOrderStruct));
    gpNewBobbyrShipments[0].fActive = TRUE;
    gpNewBobbyrShipments[0].uiPackageWeight = 0x12345678u;
    giNumberOfNewBobbyRShipment = 1;
}

void checkPublishedShipmentUnchanged(const char* why) {
    check(giNumberOfNewBobbyRShipment == 1 && gpNewBobbyrShipments != nullptr &&
          gpNewBobbyrShipments[0].fActive &&
          gpNewBobbyrShipments[0].uiPackageWeight == 0x12345678u, why);
}

void rejectTruncatedShipment() {
    auto bytes = countHeader(1);
    seedPublishedShipment();
    auto* const published = gpNewBobbyrShipments;
    BufferBounds bounds(bytes);
    auto* cursor = reinterpret_cast<char*>(bytes.data());
    check(!NewWayOfLoadingBobbyRMailOrdersToSaveGame(&cursor) && gfBufferLoadFailed,
          "truncated Bobby Ray shipment rejected");
    check(cursor == reinterpret_cast<char*>(bytes.data() + bytes.size()),
          "truncated shipment stops after the complete count header");
    check(gpNewBobbyrShipments == published,
          "truncated shipment preserves the published allocation");
    checkPublishedShipmentUnchanged(
        "truncated shipment preserves the published record and count");
}

void rejectInvalidCounts() {
    for (INT32 count : {-1, 0x7fffffff}) {
        auto bytes = countHeader(count);
        seedPublishedShipment();
        auto* const published = gpNewBobbyrShipments;
        BufferBounds bounds(bytes);
        auto* cursor = reinterpret_cast<char*>(bytes.data());
        check(!NewWayOfLoadingBobbyRMailOrdersToSaveGame(&cursor),
              "negative or buffer-impossible shipment count rejected");
        check(cursor == reinterpret_cast<char*>(bytes.data() + bytes.size()),
              "invalid shipment count rejected after its complete header");
        check(gpNewBobbyrShipments == published,
              "invalid shipment count preserves the published allocation");
        checkPublishedShipmentUnchanged(
            "invalid shipment count preserves the published record and count");
    }
}

void acceptOneShipment() {
    NewBobbyRayOrderStruct record = {};
    record.fActive = TRUE;
    record.ubDeliveryLoc = 3;
    record.ubDeliveryMethod = 2;
    record.uiPackageWeight = 9876;
    auto bytes = countHeader(1);
    const auto oldSize = bytes.size();
    bytes.resize(oldSize + sizeof(record));
    std::memcpy(bytes.data() + oldSize, &record, sizeof(record));

    seedPublishedShipment();
    BufferBounds bounds(bytes);
    auto* cursor = reinterpret_cast<char*>(bytes.data());
    check(NewWayOfLoadingBobbyRMailOrdersToSaveGame(&cursor),
          "single complete Bobby Ray shipment accepted");
    check(cursor == reinterpret_cast<char*>(bytes.data() + bytes.size()) &&
          giNumberOfNewBobbyRShipment == 1 && gpNewBobbyrShipments != nullptr,
          "single shipment consumes the exact payload and publishes one record");
    check(gpNewBobbyrShipments[0].fActive &&
          gpNewBobbyrShipments[0].ubDeliveryLoc == 3 &&
          gpNewBobbyrShipments[0].ubDeliveryMethod == 2 &&
          gpNewBobbyrShipments[0].uiPackageWeight == 9876,
          "single shipment fields survive transactional publication");
}

void acceptEmptyTable() {
    auto bytes = countHeader(0);
    seedPublishedShipment();
    BufferBounds bounds(bytes);
    auto* cursor = reinterpret_cast<char*>(bytes.data());
    check(NewWayOfLoadingBobbyRMailOrdersToSaveGame(&cursor),
          "empty Bobby Ray shipment table accepted");
    check(cursor == reinterpret_cast<char*>(bytes.data() + bytes.size()) &&
          giNumberOfNewBobbyRShipment == 0 && gpNewBobbyrShipments == nullptr,
          "empty table atomically clears the published shipment list");
}
}

int main() {
    try {
        rejectTruncatedShipment();
        rejectInvalidCounts();
        acceptOneShipment();
        acceptEmptyTable();
        BobbyRayMailOrderEndGameShutDown();
        std::cout << "Bobby Ray mail-order save load: " << checks
                  << " checks passed; original loader, synthetic bytes only\n";
        return 0;
    } catch (const std::exception& error) {
        BobbyRayMailOrderEndGameShutDown();
        std::cerr << "Bobby Ray mail-order save load failed at check " << checks
                  << ": " << error.what() << '\n';
        return 1;
    }
}
