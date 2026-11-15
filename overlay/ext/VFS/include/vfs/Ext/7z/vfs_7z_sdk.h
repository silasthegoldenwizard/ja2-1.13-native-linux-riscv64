/*
 * P710 native snapshot update: ext/VFS/include/vfs/Ext/7z/vfs_7z_sdk.h
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Native adapter for the bundled public 7z C SDK. Include its declarations in
// the global namespace, exactly as the compiled C implementation sees them.
// Re-declaring the headers inside a C++ namespace creates distinct enum/struct
// types and an incompatible Seek callback, detected by Clang's function UBSan.
#ifndef VFS_NATIVE_7Z_SDK_H
#define VFS_NATIVE_7Z_SDK_H
extern "C" {
#include <7z.h>
#include <7zCrc.h>
#include <7zAlloc.h>
}

// Keep the adapters' existing qualification without inventing new SDK types.
namespace sz {
using ::Byte;
using ::UInt32;
using ::UInt64;
using ::Int64;
using ::SRes;
using ::ESzSeek;
using ::SZ_SEEK_SET;
using ::SZ_SEEK_CUR;
using ::SZ_SEEK_END;
using ::ISeekInStream;
using ::CLookToRead;
using ::CSzArEx;
using ::CSzFileItem;
using ::CSzFolder;
using ::ISzAlloc;
using ::CrcCalc;
using ::CrcGenerateTable;
using ::LookToRead_CreateVTable;
using ::LookToRead_Init;
using ::SzAlloc;
using ::SzAllocTemp;
using ::SzFree;
using ::SzFreeTemp;
using ::SzArEx_Free;
using ::SzArEx_GetFolderStreamPos;
using ::SzArEx_Init;
using ::SzArEx_Open;
using ::k7zSignature;
using ::k7zIdATime;
using ::k7zIdCRC;
using ::k7zIdCTime;
using ::k7zIdCodersUnpackSize;
using ::k7zIdEmptyFile;
using ::k7zIdEmptyStream;
using ::k7zIdEnd;
using ::k7zIdFilesInfo;
using ::k7zIdFolder;
using ::k7zIdHeader;
using ::k7zIdMTime;
using ::k7zIdMainStreamsInfo;
using ::k7zIdName;
using ::k7zIdPackInfo;
using ::k7zIdSize;
using ::k7zIdSubStreamsInfo;
using ::k7zIdUnpackInfo;
using ::k7zIdWinAttributes;
}
#endif
