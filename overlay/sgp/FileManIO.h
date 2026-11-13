/*
 * P710 native notice preparation: sgp/FileManIO.h
 * New project path.
 * Native adaptation recorded in the 2026-11-12 checkpoint.
 * Exact first-edit/creation date is not established; this is a checkpoint-record date.
 * Scope: declare portable backend file I/O.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Notice inserted: 2026-11-12.
 * Existing source bytes and original notices retained; no additional rights are granted.
 * This record does not establish license applicability, compliance or distribution permission.
 */

#ifndef SGP_FILEMAN_IO_H
#define SGP_FILEMAN_IO_H

#include "core_types.h"
#include <limits>
#include <string>

#define FILE_ACCESS_READ 0x01
#define FILE_ACCESS_WRITE 0x02
#define FILE_ACCESS_READWRITE 0x03
// Preserve the legacy option values for callers; the VFS adapter's existing
// create=true/truncate=false policy remains authoritative for writes.
#define FILE_CREATE_NEW 0x0010
#define FILE_CREATE_ALWAYS 0x0020
#define FILE_OPEN_EXISTING 0x0040
#define FILE_OPEN_ALWAYS 0x0080
#define FILE_TRUNCATE_EXISTING 0x0100

// Keep these origin values in sync with dbman.h.
#define FILE_SEEK_FROM_START 0x01
#define FILE_SEEK_FROM_END 0x02
#define FILE_SEEK_FROM_CURRENT 0x04

HWFILE FileOpen(STR filename, UINT32 options, BOOLEAN deleteOnClose = FALSE,
                STR profile = nullptr);
void FileClose(HWFILE file);
BOOLEAN FileRead(HWFILE file, PTR dest, UINT32 bytes, UINT32* bytesRead);
BOOLEAN FileReadLine(HWFILE file, std::string* dest);
BOOLEAN FileWrite(HWFILE file, const void* source, UINT32 bytes, UINT32* bytesWritten);
BOOLEAN FileSeek(HWFILE file, UINT32 distance, UINT8 how);
INT32 FileGetPos(HWFILE file);
UINT32 FileGetSize(HWFILE file);
BOOLEAN FileCheckEndOfFile(HWFILE file);

// FMOD carries an opaque user token in void*. It never dereferences this value.
// Keep the conversion in one place and reject values that cannot be HWFILE IDs.
inline void* FileHandleToCallbackToken(HWFILE file)
{
    if (file == (std::numeric_limits<HWFILE>::max)()) return nullptr;
    return reinterpret_cast<void*>(static_cast<std::uintptr_t>(file));
}

inline HWFILE FileHandleFromCallbackToken(void* token)
{
    const auto value = reinterpret_cast<std::uintptr_t>(token);
    if (value >= (std::numeric_limits<HWFILE>::max)()) return 0;
    return static_cast<HWFILE>(value);
}

#endif
