/*
 * P710 native notice preparation: sgp/ImagePalette.cpp
 * New path extracting/adapting inherited engine material.
 * Native adaptation recorded in the 2026-11-12 checkpoint.
 * Exact first-edit/creation date is not established; this is a checkpoint-record date.
 * Scope: extract original palette and pixel-format helpers.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Inherited extraction/translation source: sgp/himage.cpp.
 * Notice inserted: 2026-11-12.
 * Existing source bytes and original notices retained; no additional rights are granted.
 * This record does not establish license applicability, compliance or distribution permission.
 */

/*
 * P710 native snapshot update: sgp/ImagePalette.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Original v1.13 palette/format helpers, shared by native image and game paths.
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include "himage.h"
#include "vobject_types.h"
#ifdef _WIN32
#include "DEBUG.H"
#else
#define Assert assert
#endif

#define BLACK_SUBSTITUTE 0x0001

UINT16 gusAlphaMask = 0;
UINT16 gusRedMask = 0;
UINT16 gusGreenMask = 0;
UINT16 gusBlueMask = 0;
INT16	gusRedShift = 0;
INT16	gusBlueShift = 0;
INT16	gusGreenShift = 0;

void ConfigureRGB565ImageFormat()
{
    gusAlphaMask = 0;
    gusRedMask = 0xf800;
    gusGreenMask = 0x07e0;
    gusBlueMask = 0x001f;
    gusRedShift = 8;
    gusGreenShift = 3;
    gusBlueShift = -3;
}


UINT16 *Create16BPPPalette( SGPPaletteEntry *pPalette )
{
	UINT16 *p16BPPPalette, r16, g16, b16, usColor;
	UINT32 cnt;
	UINT8	r,g,b;

	Assert( pPalette != NULL );

	p16BPPPalette = (UINT16 *) MemAlloc( sizeof( UINT16 ) * 256 );
	if (!p16BPPPalette) return nullptr;
	memset( p16BPPPalette, 0, sizeof( UINT16 ) * 256 );

	for ( cnt = 0; cnt < 256; cnt++ )
	{
		r = pPalette[ cnt ].peRed;
		g = pPalette[ cnt ].peGreen;
		b = pPalette[ cnt ].peBlue;

		if(gusRedShift < 0)
			r16=((UINT16)r>>abs(gusRedShift));
		else
			r16=((UINT16)r<<gusRedShift);

		if(gusGreenShift < 0)
			g16=((UINT16)g>>abs(gusGreenShift));
		else
			g16=((UINT16)g<<gusGreenShift);


		if(gusBlueShift < 0)
			b16=((UINT16)b>>abs(gusBlueShift));
		else
			b16=((UINT16)b<<gusBlueShift);

		usColor = (r16&gusRedMask)|(g16&gusGreenMask)|(b16&gusBlueMask);

		if(usColor==0)
		{
			if((r+g+b)!=0)
				usColor=BLACK_SUBSTITUTE | gusAlphaMask;
		}
		else
			usColor |= gusAlphaMask;

		p16BPPPalette[ cnt ] = usColor;
	}

	return( p16BPPPalette );
}

UINT16 *Create16BPPPaletteShaded( SGPPaletteEntry *pPalette, UINT32 rscale, UINT32 gscale, UINT32 bscale, BOOLEAN mono)
{
	UINT16 *p16BPPPalette, r16, g16, b16, usColor;
	UINT32 cnt, lumin;
	UINT32 rmod, gmod, bmod;
	UINT8	r,g,b;

	Assert( pPalette != NULL );

	p16BPPPalette = (UINT16 *) MemAlloc( sizeof( UINT16 ) * 256 );
	if (!p16BPPPalette) return nullptr;
	memset( p16BPPPalette, 0, sizeof( UINT16 ) * 256 );

	for ( cnt = 0; cnt < 256; cnt++ )
	{
		if(mono)
		{
			lumin=(pPalette[ cnt ].peRed*299/1000)+ (pPalette[ cnt ].peGreen*587/1000)+(pPalette[ cnt ].peBlue*114/1000);
			rmod=(rscale*lumin)/256;
			gmod=(gscale*lumin)/256;
			bmod=(bscale*lumin)/256;
		}
		else
		{
			rmod = (rscale*pPalette[ cnt ].peRed/256);
			gmod = (gscale*pPalette[ cnt ].peGreen/256);
			bmod = (bscale*pPalette[ cnt ].peBlue/256);
		}

		r = (UINT8)(std::min)( rmod, UINT32{255} );
		g = (UINT8)(std::min)( gmod, UINT32{255} );
		b = (UINT8)(std::min)( bmod, UINT32{255} );

		if(gusRedShift < 0)
			r16=((UINT16)r>>(-gusRedShift));
		else
			r16=((UINT16)r<<gusRedShift);

		if(gusGreenShift < 0)
			g16=((UINT16)g>>(-gusGreenShift));
		else
			g16=((UINT16)g<<gusGreenShift);


		if(gusBlueShift < 0)
			b16=((UINT16)b>>(-gusBlueShift));
		else
			b16=((UINT16)b<<gusBlueShift);

		// Prevent creation of pure black color
		usColor	= (r16&gusRedMask)|(g16&gusGreenMask)|(b16&gusBlueMask);

		if(usColor==0)
		{
			if((r+g+b)!=0)
				usColor=BLACK_SUBSTITUTE | gusAlphaMask;
		}
		else
			usColor |= gusAlphaMask;

		p16BPPPalette[ cnt ] = usColor;
	}
	return( p16BPPPalette );
}

UINT16 Get16BPPColor( UINT32 RGBValue )
{
	UINT16 r16, g16, b16, usColor = 0;
	UINT8	r,g,b;

	r = SGPGetRValue( RGBValue );
	g = SGPGetGValue( RGBValue );
	b = SGPGetBValue( RGBValue );

	if(gusRedShift < 0)
		r16=((UINT16)r>>abs(gusRedShift));
	else
		r16=((UINT16)r<<gusRedShift);

	if(gusGreenShift < 0)
		g16=((UINT16)g>>abs(gusGreenShift));
	else
		g16=((UINT16)g<<gusGreenShift);


	if(gusBlueShift < 0)
		b16=((UINT16)b>>abs(gusBlueShift));
	else
		b16=((UINT16)b<<gusBlueShift);

	usColor=(r16&gusRedMask)|(g16&gusGreenMask)|(b16&gusBlueMask);

	// if our color worked out to absolute black, and the original wasn't
	// absolute black, convert it to a VERY dark grey to avoid transparency
	// problems

	if(usColor==0)
	{
		if(RGBValue!=0)
			usColor=BLACK_SUBSTITUTE | gusAlphaMask;
	}
	else
		usColor	|=	gusAlphaMask;

	return(usColor);
}

UINT32 GetRGBColor( UINT16 Value16BPP )
{
	UINT16 r16, g16, b16;
	UINT32 r,g,b,val;

	r16 = Value16BPP & gusRedMask;
	g16 = Value16BPP & gusGreenMask;
	b16 = Value16BPP & gusBlueMask;

	if(gusRedShift < 0)
		r=((UINT32)r16<<abs(gusRedShift));
	else
		r=((UINT32)r16>>gusRedShift);

	if(gusGreenShift < 0)
		g=((UINT32)g16<<abs(gusGreenShift));
	else
		g=((UINT32)g16>>gusGreenShift);

	if(gusBlueShift < 0)
		b=((UINT32)b16<<abs(gusBlueShift));
	else
		b=((UINT32)b16>>gusBlueShift);

	r &= 0x000000ff;
	g &= 0x000000ff;
	b &= 0x000000ff;

	val = FROMRGB(r,g,b);

	return(val);
}

SGPPaletteEntry *ConvertRGBToPaletteEntry(UINT8 sbStart, UINT8 sbEnd, UINT8 *pOldPalette)
{
	UINT16 Index;
	SGPPaletteEntry *pPalEntry;
	SGPPaletteEntry *pInitEntry;

	pPalEntry = (SGPPaletteEntry *)MemAlloc(sizeof(SGPPaletteEntry) * 256);
	if (!pPalEntry || !pOldPalette || sbEnd < sbStart) { MemFree(pPalEntry); return nullptr; }
	memset( pPalEntry, 0, sizeof(SGPPaletteEntry) * 256 );
	pInitEntry = pPalEntry;

	// Diagnostic output is not required for palette conversion.

	for(Index=0; Index <= (sbEnd-sbStart);Index++)
	{
		pPalEntry->peRed = *(pOldPalette + (Index*3));
		pPalEntry->peGreen = *(pOldPalette + (Index*3) + 1);
		pPalEntry->peBlue = *(pOldPalette + (Index*3) + 2);
		pPalEntry->peFlags = 0;
		pPalEntry++;
	}
	return pInitEntry;
}

void ConvertRGBDistribution565To555(UINT16* pixels, UINT32 count)
{
    for (UINT32 i = 0; i < count; ++i)
        if (pixels[i] != 0)
            pixels[i] = static_cast<UINT16>(((pixels[i] & 0xffc0U) >> 1) | (pixels[i] & 0x001fU) | gusAlphaMask);
}

void ConvertRGBDistribution565To655(UINT16* pixels, UINT32 count)
{
    for (UINT32 i = 0; i < count; ++i)
        pixels[i] = static_cast<UINT16>((pixels[i] & 0xf800U) | ((pixels[i] & 0x07c0U) >> 1) | (pixels[i] & 0x001fU));
}

void ConvertRGBDistribution565To556(UINT16* pixels, UINT32 count)
{
    for (UINT32 i = 0; i < count; ++i)
        pixels[i] = static_cast<UINT16>((pixels[i] & 0xffc0U) | ((pixels[i] & 0x001fU) << 1));
}

void ConvertRGBDistribution565ToAny( UINT16 * p16BPPData, UINT32 uiNumberOfPixels )
{
	UINT16 *	pPixel;
	UINT32		uiRed, uiGreen, uiBlue, uiTemp, uiLoop;

	pPixel = p16BPPData;
	for (uiLoop = 0; uiLoop < uiNumberOfPixels; uiLoop++)
	{
		// put the 565 RGB 16-bit value into a 32-bit RGB value
		uiRed = (*pPixel) >> 11;
		uiGreen = (*pPixel & 0x07E0) >> 5;
		uiBlue = (*pPixel & 0x001F);
		uiRed = (uiRed << 3) | (uiRed >> 2);
		uiGreen = (uiGreen << 2) | (uiGreen >> 4);
		uiBlue = (uiBlue << 3) | (uiBlue >> 2);
		uiTemp = FROMRGB(uiRed,uiGreen,uiBlue);
		// then convert the 32-bit RGB value to whatever 16 bit format is used
		*pPixel = Get16BPPColor( uiTemp );
		pPixel++;
	}
}
