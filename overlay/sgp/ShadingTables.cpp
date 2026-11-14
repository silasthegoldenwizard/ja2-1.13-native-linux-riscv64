/*
 * P710 native notice preparation: sgp/ShadingTables.cpp
 * New path extracting/adapting inherited engine material.
 * Native adaptation recorded in the 2026-11-12 checkpoint.
 * Exact first-edit/creation date is not established; this is a checkpoint-record date.
 * Scope: extract original shading state and translate CPU color math.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Inherited extraction/translation source: sgp/shading.cpp.
 * Notice inserted: 2026-11-12.
 * Existing source bytes and original notices retained; no additional rights are granted.
 * This record does not establish license applicability, compliance or distribution permission.
 */

/*
 * P710 native snapshot update: sgp/ShadingTables.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Original v1.13 shading state and CPU math, shared by game and native resource paths.
#include "shading.h"
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>

BOOLEAN ShadesCalculateTables(SGPPaletteEntry *p8BPPPalette);
BOOLEAN ShadesCalculatePalette(SGPPaletteEntry *pSrcPalette, SGPPaletteEntry *pDestPalette, UINT16 usRed, UINT16 usGreen, UINT16 usBlue, BOOLEAN fMono);
void FindIndecies(SGPPaletteEntry *pSrcPalette, SGPPaletteEntry *pMapPalette, UINT8 *pTable);
void FindMaskIndecies(UINT8 *, UINT8 *, UINT8 *);

SGPPaletteEntry Shaded8BPPPalettes[HVOBJECT_SHADE_TABLES+3][256];
UINT8 ubColorTables[HVOBJECT_SHADE_TABLES+3][256];

UINT16	IntensityTable[65536];
UINT16	ShadeTable[65536];
UINT16	White16BPPPalette[ 256 ];
FLOAT	guiShadePercent = (FLOAT)0.48;
FLOAT	guiBrightPercent = (FLOAT)1.1;

BOOLEAN ShadesCalculateTables(SGPPaletteEntry *p8BPPPalette)
{
UINT32	uiCount;

		// Green palette
		ShadesCalculatePalette(p8BPPPalette, Shaded8BPPPalettes[0], 0, 255, 0, TRUE);
		// Blue palette
		ShadesCalculatePalette(p8BPPPalette, Shaded8BPPPalettes[HVOBJECT_SHADE_TABLES],	0, 0, 255, TRUE);
		// Yellow palette
		ShadesCalculatePalette(p8BPPPalette, Shaded8BPPPalettes[HVOBJECT_SHADE_TABLES+1], 255, 255, 0, TRUE);
		// Red palette
		ShadesCalculatePalette(p8BPPPalette, Shaded8BPPPalettes[HVOBJECT_SHADE_TABLES+2], 255, 0, 0, TRUE);

		// these are the brightening tables, 115%-150% brighter than original
		ShadesCalculatePalette(p8BPPPalette, Shaded8BPPPalettes[1], 293, 293, 293, FALSE);
		ShadesCalculatePalette(p8BPPPalette, Shaded8BPPPalettes[2], 281, 281, 281, FALSE);
		ShadesCalculatePalette(p8BPPPalette, Shaded8BPPPalettes[3], 268, 268, 268, FALSE);

		// palette 4 is the non-modified palette.
		ShadesCalculatePalette(p8BPPPalette, Shaded8BPPPalettes[4], 255, 255, 255, FALSE);

		// the rest are darkening tables, right down to all-black.
		ShadesCalculatePalette(p8BPPPalette, Shaded8BPPPalettes[5], 195, 195, 195, FALSE);
		ShadesCalculatePalette(p8BPPPalette, Shaded8BPPPalettes[6], 165, 165, 165, FALSE);
		ShadesCalculatePalette(p8BPPPalette, Shaded8BPPPalettes[7], 135, 135, 135, FALSE);
		ShadesCalculatePalette(p8BPPPalette, Shaded8BPPPalettes[8], 105, 105, 105, FALSE);
		ShadesCalculatePalette(p8BPPPalette, Shaded8BPPPalettes[9], 75, 75, 75, FALSE);
		ShadesCalculatePalette(p8BPPPalette, Shaded8BPPPalettes[10], 45, 45, 45, FALSE);
		ShadesCalculatePalette(p8BPPPalette, Shaded8BPPPalettes[11], 36, 36, 36, FALSE);
		ShadesCalculatePalette(p8BPPPalette, Shaded8BPPPalettes[12], 27, 27, 27, FALSE);
		ShadesCalculatePalette(p8BPPPalette, Shaded8BPPPalettes[13], 18, 18, 18, FALSE);
		ShadesCalculatePalette(p8BPPPalette, Shaded8BPPPalettes[14], 9, 9, 9, FALSE);
		ShadesCalculatePalette(p8BPPPalette, Shaded8BPPPalettes[15], 0, 0, 0, FALSE);


	// Remap the shade colors to the original palette
	for(uiCount=0; uiCount < (HVOBJECT_SHADE_TABLES+3); uiCount++)
	{
		FindIndecies(Shaded8BPPPalettes[uiCount], p8BPPPalette, ubColorTables[uiCount]);
		ubColorTables[uiCount][0]=0;
	}

	return(TRUE);
}

BOOLEAN ShadesCalculatePalette(SGPPaletteEntry *pSrcPalette, SGPPaletteEntry *pDestPalette, UINT16 usRed, UINT16 usGreen, UINT16 usBlue, BOOLEAN fMono)
{
UINT32 cnt, lumin;
UINT32 rmod, gmod, bmod;

	assert( pSrcPalette != NULL );
	assert( pDestPalette != NULL );

	for ( cnt = 0; cnt < 256; cnt++ )
	{
		if(fMono)
		{
			lumin=(pSrcPalette[ cnt ].peRed*299/1000)+ (pSrcPalette[ cnt ].peGreen*587/1000)+(pSrcPalette[ cnt ].peBlue*114/1000);
			rmod=usRed*lumin/255;
			gmod=usGreen*lumin/255;
			bmod=usBlue*lumin/255;
		}
		else
		{
			rmod = (usRed*pSrcPalette[ cnt ].peRed/255);
			gmod = (usGreen*pSrcPalette[ cnt ].peGreen/255);
			bmod = (usBlue*pSrcPalette[ cnt ].peBlue/255);
		}

		pDestPalette[ cnt ].peRed = (UINT8)std::min<UINT32>(rmod, 255);
		pDestPalette[ cnt ].peGreen = (UINT8)std::min<UINT32>(gmod, 255);
		pDestPalette[ cnt ].peBlue = (UINT8)std::min<UINT32>(bmod, 255);
	}

	return(TRUE);
}




// Exact scalar translation of the original x86 Manhattan-distance search.
// Palette zero is reserved; equal-distance ties keep the first index.
void FindIndecies(SGPPaletteEntry *source, SGPPaletteEntry *map, UINT8 *table)
{
    table[0] = 0;
    for (unsigned i = 1; i < 256; ++i)
    {
        unsigned best = 65535;
        UINT8 index = 0;
        for (unsigned j = 1; j < 256; ++j)
        {
            const unsigned delta = static_cast<unsigned>(
                std::abs(int(source[i].peRed) - int(map[j].peRed)) +
                std::abs(int(source[i].peGreen) - int(map[j].peGreen)) +
                std::abs(int(source[i].peBlue) - int(map[j].peBlue)));
            if (delta < best) { best = delta; index = static_cast<UINT8>(j); }
        }
        table[i] = index;
    }
}

/**********************************************************************************************
 BuildShadeTable

	Builds a 16-bit color shading table. This function should be called only after the current
	video adapter's pixel format is known (IE: GetRgbDistribution() has been called, and the
	globals for masks and shifts have been initialized by that function), and before any
	blitting is done.

	Using the table is a straight lookup. The pixel to be shaded down is used as the index into
	the table and the entry at that point will be a pixel that is 25% darker.

**********************************************************************************************/
void BuildShadeTable(void)
{
	UINT16 red, green, blue;
	UINT16 index = 0;


	for(red=0; red < 256; red+=4)
		for(green=0; green < 256; green+=4)
			for(blue=0; blue < 256; blue+=4)
			{
				index=Get16BPPColor(FROMRGB(red, green, blue));
				ShadeTable[index]=Get16BPPColor(FROMRGB(red*guiShadePercent, green*guiShadePercent, blue*guiShadePercent));
			}

	memset( White16BPPPalette, 65535, sizeof( White16BPPPalette ) );
}


/**********************************************************************************************
 BuildIntensityTable

	Builds a 16-bit color shading table. This function should be called only after the current
	video adapter's pixel format is known (IE: GetRgbDistribution() has been called, and the
	globals for masks and shifts have been initialized by that function), and before any
	blitting is done.



**********************************************************************************************/
void BuildIntensityTable(void)
{
	UINT16 red, green, blue = 0;
	UINT16 index = 0;
	FLOAT	dShadedPercent = (FLOAT)0.80;






	for(red=0; red < 256; red+=4)
		for(green=0; green < 256; green+=4)
			for(blue=0; blue < 256; blue+=4)
			{
				index=Get16BPPColor(FROMRGB(red, green, blue));
				IntensityTable[index]=Get16BPPColor(FROMRGB(red*dShadedPercent, green*dShadedPercent, blue*dShadedPercent));
			}

}



void SetShadeTablePercent( FLOAT uiShadePercent )
{
	guiShadePercent = uiShadePercent;
	BuildShadeTable( );
}


