/*
 * P710 native snapshot update: sgp/native_text.h
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#ifndef SGP_NATIVE_TEXT_H
#define SGP_NATIVE_TEXT_H
#include <cstddef>
#include <string>
#include <vector>
namespace sgp {
// Resource/script text uses UTF-8 and native wchar_t. -1 includes the source
// terminator in the result count; explicit lengths preserve embedded NULs.
// A null destination with zero capacity queries the required output units.
// Invalid Unicode or insufficient capacity returns0 and writes no output.
int Utf8ToNativeWide(const char*, int sourceBytes, wchar_t*, std::size_t capacity) noexcept;
int NativeWideToUtf8(const wchar_t*, int sourceUnits, char*, std::size_t capacity) noexcept;
// Fixed-width resource fields need not contain NUL. Always terminate a valid
// destination, leaving it empty on rejected UTF8 or insufficient capacity.
bool Utf8FieldToNativeWide(const char*, std::size_t fieldBytes,
                           wchar_t*, std::size_t capacity) noexcept;
// NUL-terminated save fields use UTF16LE. maxWireUnits includes the NUL.
// Reject embedded NULs, invalid Unicode, and fields beyond that bound.
// On failure the output remains unchanged. These functions do not perform I/O.
bool NativeWideToUtf16LE(const wchar_t*, std::size_t maxWireUnits,
                          std::vector<unsigned char>& output) noexcept;
bool Utf16LEToNativeWide(const unsigned char*, std::size_t bytes,
                          std::size_t maxWireUnits, std::wstring& output) noexcept;
}
#endif
