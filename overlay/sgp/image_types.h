/*
 * P710 native notice preparation: sgp/image_types.h
 * New path extracting/adapting inherited engine material.
 * Native adaptation recorded in the 2026-11-12 checkpoint.
 * Exact first-edit/creation date is not established; this is a checkpoint-record date.
 * Scope: extract original image layouts and flags into portable header.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Inherited extraction/translation source: sgp/himage.h, sgp/types.h.
 * Notice inserted: 2026-11-12.
 * Existing source bytes and original notices retained; no additional rights are granted.
 * This record does not establish license applicability, compliance or distribution permission.
 */

/*
 * P710 native snapshot update: sgp/image_types.h
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#ifndef SGP_IMAGE_TYPES_H
#define SGP_IMAGE_TYPES_H

#include "core_types.h"

// Original v1.13 image data and flags, without allocation/Win32 dependencies.
// Defines for type of file readers
#define PCX_FILE_READER					0x1
#define TGA_FILE_READER					0x2
#define STCI_FILE_READER				0x4
#define TRLE_FILE_READER				0x8
#define PNG_FILE_READER					0x10
#define JPC_FILE_READER					0x20
#define UNKNOWN_FILE_READER			0x200

// Defines for buffer bit depth
#define BUFFER_8BPP							0x1
#define BUFFER_16BPP						0x2

// Defines for image charactoristics
#define IMAGE_COMPRESSED				0x0001
#define IMAGE_TRLECOMPRESSED		0x0002
#define IMAGE_PALETTE						0x0004
#define IMAGE_BITMAPDATA				0x0008
#define IMAGE_APPDATA						0x0010
#define IMAGE_ALLIMAGEDATA			0x000C
#define IMAGE_ALLDATA						0x001C

// Palette structure, mimics that of Win32
typedef struct tagSGPPaletteEntry
{  
  UINT8 peRed; 
  UINT8 peGreen; 
  UINT8 peBlue; 
  UINT8 peFlags; 

} SGPPaletteEntry; 

#define AUX_FULL_TILE					0x01
#define AUX_ANIMATED_TILE			0x02
#define AUX_DYNAMIC_TILE			0x04
#define AUX_INTERACTIVE_TILE	0x08
#define AUX_IGNORES_HEIGHT		0x10
#define AUX_USES_LAND_Z				0x20

typedef struct
{
	UINT8		ubWallOrientation;
	UINT8		ubNumberOfTiles;
	UINT16	usTileLocIndex;
	UINT8		ubUnused1[3];
	UINT8		ubCurrentFrame;
	UINT8		ubNumberOfFrames;
	UINT8		fFlags;
	UINT8		ubUnused[6];
} AuxObjectData;

typedef struct
{
	INT8		bTileOffsetX;
	INT8		bTileOffsetY;
} RelTileLoc; // relative tile location

// TRLE subimage structure, mirroring that of ST(C)I
typedef struct tagETRLEObject
{	
	UINT32			uiDataOffset;
	UINT32			uiDataLength;
	INT16				sOffsetX;
	INT16				sOffsetY;
	UINT16			usHeight;
	UINT16			usWidth;
} ETRLEObject;

typedef struct tagETRLEData
{
	PTR								pPixData;
	UINT32						uiSizePixData;
	ETRLEObject *			pETRLEObject;
	UINT16						usNumberOfObjects;
} ETRLEData;

// Image header structure
typedef struct
{
	UINT16					usWidth;
	UINT16					usHeight;
	UINT8					ubBitDepth;
	UINT16					fFlags;
	SGPFILENAME				ImageFile;
	UINT32					iFileLoader;
	SGPPaletteEntry*		pPalette;
	UINT16*					pui16BPPPalette;
	UINT8*					pAppData;
	UINT32					uiAppDataSize;
	// This union is used to describe each data type and is flexible to include the
	// data strucutre of the compresssed format, once developed.
	union 
	{
		PTR				pImageData;
		PTR				pCompressedImageData;
		UINT8*			p8BPPData;
		UINT16*			p16BPPData;
		UINT32*			p32BPPData;
		struct
		{
			UINT8*			pPixData8;
			UINT32			uiSizePixData;
			ETRLEObject*	pETRLEObject;
			UINT16			usNumberOfObjects;
		};
	};

} image_type, *HIMAGE;


#define SGPGetRValue(rgb)   ((BYTE) (rgb))  
#define SGPGetBValue(rgb)   ((BYTE) ((rgb) >> 16))  
#define SGPGetGValue(rgb)   ((BYTE) (((UINT16) (rgb)) >> 8))  


#endif
