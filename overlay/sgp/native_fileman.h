/*
 * P710 native snapshot update: sgp/native_fileman.h
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Native public FileMan metadata/services; original VFS policy is authoritative.
#ifndef SGP_NATIVE_FILEMAN_H
#define SGP_NATIVE_FILEMAN_H
#include "FileManIO.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

typedef CHAR8 STRING512[512];
#define MAX_FILENAME_LEN 48
#define FILE_IS_READONLY 1
#define FILE_IS_DIRECTORY 2
#define FILE_IS_HIDDEN 4
#define FILE_IS_NORMAL 8
#define FILE_IS_ARCHIVE 16
#define FILE_IS_SYSTEM 32
#define FILE_IS_TEMPORARY 64
#define FILE_IS_COMPRESSED 128
#define FILE_IS_OFFLINE 256

BOOLEAN InitializeFileManager(STR);
void ShutdownFileManager();
BOOLEAN FileExists(const char*);
BOOLEAN FileExistsNoDB(const char*);

// Native metadata selected by the same SF_TOP VFS policy as FileExists/FileOpen.
// Calendar fields are local time, matching the save list's original display.
// epochSeconds/nanosecond retain actual ordering independently of local DST.
// Missing/unmounted/library/nonregular files or stat/calendar conversion errors
// return false and preserve output; a zero record is never a successful fallback.
struct NativeFileDateTime {
    int year{}, month{}, day{}, hour{}, minute{}, second{};
    std::int64_t epochSeconds{};
    INT32 nanosecond{};
};
bool GetNativeFileModificationTime(const char* filename, NativeFileDateTime& output) noexcept;

BOOLEAN FileDelete(const char*);
BOOLEAN FileLoad(const char*, PTR, UINT32, UINT32*);
BOOLEAN FilePrintf(HWFILE, const char*, ...);
UINT32 FileSize(const char*);

// These three APIs address the host OS, not the virtual filesystem. Fixed
// STRING512 outputs require512bytes and fail unchanged if the path will not fit.
BOOLEAN SetFileManCurrentDirectory(const char*);
BOOLEAN GetFileManCurrentDirectory(STRING512);
BOOLEAN GetExecutableDirectory(STRING512);

// Original VFS behavior: ignore recursive, delete writable matching files,
// retain directories and read-only profile contents. These are not OS removals.
BOOLEAN RemoveFileManDirectory(const char*, BOOLEAN);
BOOLEAN EraseDirectory(const char*);
typedef struct _GETFILESTRUCT_TAG {
    INT32 iFindHandle;
    CHAR8 zFileName[260];
    UINT32 uiFileSize;
    UINT32 uiFileAttribs;
} GETFILESTRUCT;
// Original singleton cursor; first replaces an earlier search, close is safe
// repeatedly. Metadata that cannot fit the public record fails without writing
// that record. Mutating VFS profiles during iteration is unsupported.
BOOLEAN GetFileFirst(const char*, GETFILESTRUCT*);
BOOLEAN GetFileNext(GETFILESTRUCT*);
void GetFileClose(GETFILESTRUCT*);

// Host-directory boundary used after VFS has selected a writable real path.
// Enumerate non-directory children only, without recursion or ordering promise.
// Empty directories succeed; path/iteration/status failures preserve output.
bool EnumerateHostDirectoryFiles(const std::string& directory, std::vector<std::string>& output) noexcept;
// Concatenate original VFS path components using validated native-wide UTF8.
// Name is a terminated string as in the original template API; no guessed bound,
// normalization, profile selection or filename filtering is introduced.
// Null/invalid Unicode and allocation failure preserve output.
bool MakeNativeWideFilePath(const std::string& prefix, const wchar_t* name,
                            const std::string& suffix, std::string& output) noexcept;

// Preserve mp3/ogg/wav preference and missing-file wav output. Native pointer
// callers must state capacity; legacy fixed arrays infer it without overflow.
BOOLEAN SoundFileExists(const char*, char*, std::size_t);
template<std::size_t N>
BOOLEAN SoundFileExists(const char* filename, char (&found)[N]) {
    return SoundFileExists(filename, found, N);
}
#endif
