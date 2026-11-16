/*
 * P710 native snapshot update: Tactical/map_read_cursor.h
 * Native Linux source/build/test adaptations captured in the accepted native development checkpoint dated 2026-11-16.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-16.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#ifndef P710_MAP_READ_CURSOR_H
#define P710_MAP_READ_CURSOR_H

#include <cstddef>
#include <cstring>

namespace p710 {
// A map reader commits its caller's cursor only after a complete record.
class MapReadCursor {
public:
    INT8* cursor;
    std::size_t remaining;
    MapReadCursor(INT8* data, std::size_t bytes) : cursor(data), remaining(bytes) {}
    bool read(void* destination, std::size_t bytes) {
        if (!cursor || bytes > remaining) return false;
        std::memcpy(destination, cursor, bytes);
        cursor += bytes;
        remaining -= bytes;
        return true;
    }
    template<class Record, class... Args>
    bool load(Record& record, Args... args) {
        auto* start = cursor;
        if (!record.Load(&cursor, args..., remaining)) return false;
        remaining -= static_cast<std::size_t>(cursor - start);
        return true;
    }
};
}
#endif
