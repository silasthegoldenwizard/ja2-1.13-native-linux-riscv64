/*
 * P710 native notice preparation: sgp/vobject_types.h
 * New path extracting/adapting inherited engine material.
 * Native adaptation recorded in the 2026-11-12 checkpoint.
 * Exact first-edit/creation date is not established; this is a checkpoint-record date.
 * Scope: extract original runtime object layout and flags.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Inherited extraction/translation source: sgp/vobject.h.
 * Notice inserted: 2026-11-12.
 * Existing source bytes and original notices retained; no additional rights are granted.
 * This record does not establish license applicability, compliance or distribution permission.
 */

/*
 * P710 native snapshot update: sgp/vobject_types.h
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#ifndef SGP_VOBJECT_TYPES_H
#define SGP_VOBJECT_TYPES_H

#include "image_types.h"

// Original v1.13 runtime object layout and flags for CPU sprite kernels.
// This definition mimics what is found in WINDOWS.H ( for Direct Draw compatiblity )
typedef UINT32 COLORVAL;

// Defines for blitting
#define VO_BLT_SRCTRANSPARENCY		0x000000002
#define VO_BLT_DESTTRANSPARENCY		0x000000120
#define VO_BLT_SHADOW				0x000000200
#define VO_BLT_TRANSSHADOW			0x000000003
#define VO_BLT_UNCOMPRESSED			0x000004000
#define VO_BLT_CLIP					0x000000001
#define VO_BLT_MIRROR_Y				0x000001000 // must be the same as VS_BLT_MIRROR_Y for Wiz!!!

// Defines for HVOBJECT limits
#define HVOBJECT_SHADE_TABLES		48

#define HVOBJECT_GLOW_GREEN			0
#define HVOBJECT_GLOW_BLUE			1
#define HVOBJECT_GLOW_YELLOW		2
#define HVOBJECT_GLOW_RED			3

// Effects structure for specialized blitting
typedef struct 
{
	UINT32	uiShadowLevel;
	SGPRect	ClipRect;

} blt_fx;


// Z-buffer info structure for properly assigning Z values
typedef struct
{
	INT8			bInitialZChange;		// difference in Z value between the leftmost and base strips
	UINT8			ubFirstZStripWidth;	// # of pixels in the leftmost strip
	UINT8			ubNumberOfZChanges;	// number of strips (after the first)
	INT8 *		pbZChange;			// change to the Z value in each strip (after the first)
} ZStripInfo;

typedef struct
{
	UINT16 *		p16BPPData;
	UINT16			usRegionIndex;
	UINT8			ubShadeLevel;
	UINT16			usWidth;
	UINT16			usHeight;
	INT16			sOffsetX;
	INT16			sOffsetY;
} SixteenBPPObjectInfo;

// This definition mimics what is found in WINDOWS.H ( for Direct Draw compatiblity )
// From RGB to COLORVAL
#define FROMRGB(r, g ,b)  ((UINT32) (((UINT8) (r) | ((UINT16) (g) << 8)) | (((UINT32) (UINT8) (b)) << 16))) 


// Video object creation flags
// Used in the VOBJECT_DESC structure to describe creation flags

#define VOBJECT_CREATE_DEFAULT			  0x00000020		// Creates and empty object of given width, height and BPP
#define VOBJECT_CREATE_FROMFILE			  0x00000040		// Creates a video object from a file ( using HIMAGE )
#define VOBJECT_CREATE_FROMHIMAGE		  0x00000080		// Creates a video object from a pre-loaded hImage
#define VOBJECT_CREATE_FROMJPC            0x00010000		// replace file extension : sti -> jpc.7z
#define VOBJECT_CREATE_FROMPNG            0x00020000		// replace file extension : sti -> png
#define VOBJECT_CREATE_FROMJPC_FALLBACK   0x00100000		// replace file extension : sti -> jpc.7z, fall back to sti when it doesn't exist
#define VOBJECT_CREATE_FROMPNG_FALLBACK   0x00200000		// replace file extension : sti -> png, fall back to sti when it doesn't exist

// VOBJECT FLAGS
#define	VOBJECT_FLAG_SHADETABLE_SHARED 0x00000100

// This structure is a video object. 
// The video object contains different data based on it's type, compressed or not
typedef struct TAG_HVOBJECT
{
	UINT32					fFlags;								// Special flags
	UINT32					uiSizePixData;						// ETRLE data size
	SGPPaletteEntry			*pPaletteEntry;						// 8BPP Palette						  
	COLORVAL				TransparentColor;					// Defaults to 0,0,0
	UINT16					*p16BPPPalette;						// A 16BPP palette used for 8->16 blits
	
	PTR						pPixData;							// ETRLE pixel data
	ETRLEObject				*pETRLEObject;						// Object offset data etc
	SixteenBPPObjectInfo	*p16BPPObject;
	UINT16					*pShades[HVOBJECT_SHADE_TABLES];	// Shading tables
	UINT16					*pShadeCurrent;
	UINT16					*pGlow;								// glow highlight table
	UINT8					*pShade8;							// 8-bit shading index table
	UINT8					*pGlow8;							// 8-bit glow table
	ZStripInfo				**ppZStripInfo;						// Z-value strip info arrays

	UINT16					usNumberOf16BPPObjects;
	UINT16					usNumberOfObjects;					// Total number of objects
	UINT8					ubBitDepth;							// BPP 
	SGPFILENAME				ImageFile;
} SGPVObject, *HVOBJECT;


// This structure describes the creation parameters for a Video Object
typedef struct
{
	UINT32				fCreateFlags;						// Specifies creation flags like from file or not
	union
	{
		//struct
		//{
			SGPFILENAME		ImageFile;							// Filename of image data to use
		//};
		//struct
		//{
			HIMAGE				hImage;
		//};
	};
	UINT8					ubBitDepth;							// BPP, ignored if given from file
} VOBJECT_DESC;


#endif
