/*
 * P710 native notice preparation: sgp/core_types.h
 * New path extracting/adapting inherited engine material.
 * Native adaptation recorded in the 2026-11-12 checkpoint.
 * Exact first-edit/creation date is not established; this is a checkpoint-record date.
 * Scope: isolate portable fixed-width and geometry types from Win32.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Inherited extraction/translation source: sgp/types.h.
 * Notice inserted: 2026-11-12.
 * Existing source bytes and original notices retained; no additional rights are granted.
 * This record does not establish license applicability, compliance or distribution permission.
 */

/*
 * P710 native snapshot update: sgp/core_types.h
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#ifndef SGP_CORE_TYPES_H
#define SGP_CORE_TYPES_H

#include <cstdint>

// Primitive types shared by the raster and FileMan layers, without Win32/VFS.
// types.h includes this before defining the remaining legacy Sir-Tech types.
#ifndef _SIRTECH_TYPES_
typedef std::int32_t INT32;
typedef std::uint32_t UINT32;
typedef std::uint16_t UINT16;
typedef std::uint8_t UINT8;
typedef std::int8_t INT8;
typedef std::int16_t INT16;
typedef float FLOAT;
typedef unsigned char BOOLEAN;
typedef char CHAR8;
typedef CHAR8* STR;
typedef CHAR8* STR8;
typedef void* PTR;
// Opaque process-local ID; never a native pointer or an on-disk field.
typedef UINT32 HWFILE;
typedef UINT8 BYTE;
#define SGPFILENAME_LEN 100
typedef CHAR8 SGPFILENAME[SGPFILENAME_LEN];
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

typedef struct
{
	INT32 x;
	INT32 y;
	INT32 width;
	INT32 height;
} SGPRectangle;

// Preserve the original signed, 32-bit rectangle coordinates.
typedef struct
{
	INT32 iLeft;
	INT32 iTop;
	INT32 iRight;
	INT32 iBottom;
} SGPRect;

#endif
