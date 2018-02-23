#include <io.h>
#include <fcntl.h>
#include <i86.h>
#include <string.h>
#include <stdio.h>

#include "typedefs.h"
#include "misc.h"
#include "globals.h"
#include "textio.h"
#include "palette.h"
#include "debug4g.h"
#include "resource.h"
#include "error.h"
#include "screen.h"
#include "helix.h"
#include "trig.h"
#include "gfx.h"

typedef uchar GAMMA_SUBTABLE[256];

static GAMMA_SUBTABLE *gammaTable;
static RGB baseDAC[256], curDAC[256], fromDAC[256], toRGB;
static RGB *palTable[kMaxPalettes];
static int effectR = 0, effectG = 0, effectB = 0;
static BOOL DacInvalid = TRUE;
static long messageTime = 0;
static char message[256];
static int curPalette = kPalNormal;
static int curGamma = 0;

static RGB StdPal[32] =
{
	{  0,  0,  0},
	{  0,  0,170},
	{  0,170,  0},
	{  0,170,170},
	{170,  0,  0},
	{170,  0,170},
	{170, 85,  0},
	{170,170,170},
	{ 85, 85, 85},
	{ 85, 85,255},
	{ 85,255, 85},
	{ 85,255,255},
	{255, 85, 85},
	{255, 85,255},
	{255,255, 85},
	{255,255,255},
	{241,241,241},
	{226,226,226},
	{211,211,211},
	{196,196,196},
	{181,181,181},
	{166,166,166},
	{151,151,151},
	{136,136,136},
	{120,120,120},
	{105,105,105},
	{ 90, 90, 90},
	{ 75, 75, 75},
	{ 60, 60, 60},
	{ 45, 45, 45},
	{ 30, 30, 30},
	{ 15, 15, 15},
};

struct LOADITEM {
	int index;
	char *name;
};


static LOADITEM PLU[] =
{
	{ kPLUNormal,	"NORMAL" },
	{ kPLUFlame,	"FLAME" },
	{ kPLUCold,		"COLD" },
	{ kPLURed,		"BEAST" },
	{ kPLUGray,		"GRAY" },
	{ kPLUGrayish,	"GRAYISH" },
	{ kPLUCultist2,	"TOMMY" },
	{ kPLUSpider1,	"SPIDER1" },
	{ kPLUSpider2,	"SPIDER2" },
	{ kPLUSpider3,	"SPIDER3" },
	{ kPLUPlayer1,	"P0" },	// also kPLUPlayer5
	{ kPLUPlayer2,	"P1" },	// also kPLUPlayer6
	{ kPLUPlayer3,	"P2" },	// also kPLUPlayer7
	{ kPLUPlayer4,	"P3" },	// also kPLUPlayer8
};

static LOADITEM PAL[] =
{
	{ kPalNormal,	"BLOOD" },
	{ kPalWater,	"WATER" },
	{ kPalBeast,	"BEAST" },
};

// exported variables
int gGammaLevels;
BOOL gFogMode = FALSE;
BYTE gStdColor[32];	// standard 32 colors


// Weight coefficients for color matching
#define kWeightR		1
#define kWeightG		1
#define kWeightB		1

BYTE scrFindClosestColor( int r, int g, int b )
{
	int i;
	int dr, dg, db, dist, matchDist, match;

	matchDist = 0x7FFFFFFF;

	for ( i = 0; i < 256; i++ )
	{
		dist = 0;

		dg = (int)palette[i].g - g;
		dist += kWeightG * dg * dg;
		if ( dist >= matchDist )
			continue;

		dr = (int)palette[i].r - r;
		dist += kWeightR * dr * dr;
		if ( dist >= matchDist )
			continue;

		db = (int)palette[i].b - b;
		dist += kWeightB * db * db;
		if ( dist >= matchDist )
			continue;

		matchDist = dist;
		match = i;

		if (dist == 0)
			break;
	}

	return (BYTE)match;
}


void scrCreateStdColors( void )
{
	for (int i = 0; i < LENGTH(StdPal); i++)
		gStdColor[i] = scrFindClosestColor(StdPal[i].r, StdPal[i].g, StdPal[i].b);
}


void scrLoadPLUs( void )
{
	int i;

	if ( gFogMode )
	{
		RESHANDLE hRemap;
		hRemap = gSysRes.Lookup("FOG", ".FLU");
		if (hRemap == NULL)
			ThrowError("FOG.FLU not found", ES_ERROR);
		palookup[0] = (BYTE *)gSysRes.Lock(hRemap);

		for ( i = 0; i < LENGTH(PLU); i++ )
		{
			palookup[PLU[i].index] = (BYTE *)palookup[0];
		}

		parallaxvisibility = 8192;
	}
	else
	{
		for ( i = 0; i < LENGTH(PLU); i++ )
		{
			RESHANDLE hRemap;
			hRemap = gSysRes.Lookup(PLU[i].name, ".plu");
			if (hRemap == NULL)
			{
				char buffer[128];
				sprintf(buffer, "%s.PLU not found", PLU[i].name);
				ThrowError(buffer, ES_ERROR);
			}

			if ( gSysRes.Size(hRemap) / 256 != kPalLookups )
				ThrowError("Incorrect PLU size", ES_ERROR);

			palookup[PLU[i].index] = (uchar *)gSysRes.Lock(hRemap);
		}
	}
}


void scrLoadPalette()
{
	int i;
	tioPrint("Loading palettes");
	for ( i = 0; i < LENGTH(PAL); i++ )
	{
		RESHANDLE hRemap;
		hRemap = gSysRes.Lookup(PAL[i].name, ".pal");
		if (hRemap == NULL)
		{
			char buffer[128];
			sprintf(buffer, "%s.PAL not found", PAL[i].name);
			ThrowError(buffer, ES_ERROR);
		}

		palTable[PAL[i].index] = (RGB *)gSysRes.Lock(hRemap);
	}

	// copy the default palette into ken's variable so Std color stuff still works
	memcpy(palette, palTable[kPalNormal], sizeof(palette));

	paletteloaded = 1;
	numpalookups = kPalLookups;

	scrLoadPLUs();

	RESHANDLE hTransluc;
	tioPrint("Loading translucency table");

	hTransluc = gSysRes.Lookup("trans", ".tlu");
	if (hTransluc == NULL)
		ThrowError("TRANS.TLU not found", ES_ERROR);
	transluc = (uchar *)gSysRes.Lock(hTransluc);

	fixtransluscence(transluc);
}


void scrSetMessage(char *s)
{
	messageTime = gFrameClock;
	strcpy(message, s);
}

void scrDisplayMessage( int nColor )
{

	if (gFrameClock < messageTime + kTimerRate * 2)
		gfxDrawText(windowx1, windowy1, nColor, message, 0);
}


void scrSetPalette( int nPalette )
{
	curPalette = nPalette;
	scrSetGamma(curGamma);	// reset baseline colors
}


void scrSetGamma( int nGamma )
{
	int i;

	dassert(nGamma < gGammaLevels);
	curGamma = nGamma;

	for ( i = 0; i < 256; i++ )
	{
		baseDAC[i].r = (uchar)(gammaTable[nGamma][palTable[curPalette][i].r]);
		baseDAC[i].g = (uchar)(gammaTable[nGamma][palTable[curPalette][i].g]);
		baseDAC[i].b = (uchar)(gammaTable[nGamma][palTable[curPalette][i].b]);
	}

	// force a DAC update
	DacInvalid = TRUE;
}


void scrDacRelEffect( int r, int g, int b )
{
	effectR = ClipHigh(effectR + r, 384);
	effectG = ClipHigh(effectG + g, 384);
	effectB = ClipHigh(effectB + b, 384);
//	effectR += r;
//	effectG += g;
//	effectB += b;

	DacInvalid = TRUE;
}


void scrDacAbsEffect( int r, int g, int b )
{
	effectR = r;
	effectG = g;
	effectB = b;
	DacInvalid = TRUE;
}


void scrSetupFade( BYTE r, BYTE g, BYTE b )
{
	memcpy(fromDAC, curDAC, sizeof(fromDAC));
	toRGB.r = r;
	toRGB.g = g;
	toRGB.b = b;
}


void scrSetupUnfade( void )
{
	memcpy(fromDAC, baseDAC, sizeof(fromDAC));
}


void scrFadeAmount( int nFrac )
{
	for ( int i = 0; i < 256; i++ )
	{
		curDAC[i].r = (BYTE)dmulscale16(0x10000 - nFrac, fromDAC[i].r, nFrac, toRGB.r);
		curDAC[i].g = (BYTE)dmulscale16(0x10000 - nFrac, fromDAC[i].g, nFrac, toRGB.g);
		curDAC[i].b = (BYTE)dmulscale16(0x10000 - nFrac, fromDAC[i].b, nFrac, toRGB.b);
	}
	gSetDACRange(0, 256, curDAC);
}


void scrSetDac( ulong nTicks )
{
	int i;

	// don't set the palette in stereo red/blue mode
	if ( vidoption == 6 )
		return;

	// don't update DACs if no effect necessary
	if ( DacInvalid )
	{
		for (i = 0; i < 256; i++)
		{
			curDAC[i].r = (uchar)ClipRange(baseDAC[i].r + effectR, 0, 255);
			curDAC[i].g = (uchar)ClipRange(baseDAC[i].g + effectG, 0, 255);
			curDAC[i].b = (uchar)ClipRange(baseDAC[i].b + effectB, 0, 255);
		}

		gSetDACRange(0, 256, curDAC);
	}

	DacInvalid = FALSE;

	if ( effectR | effectG | effectB )
	{
		DacInvalid = TRUE;

		if (effectR > 0)
			effectR = ClipLow(effectR - nTicks, 0);
		else if (effectR < 0)
			effectR = ClipHigh(effectR + nTicks, 0);

		if (effectG > 0)
			effectG = ClipLow(effectG - nTicks, 0);
		else if (effectG < 0)
			effectG = ClipHigh(effectG + nTicks, 0);

		if (effectB > 0)
			effectB = ClipLow(effectB - nTicks, 0);
		else if (effectB < 0)
			effectB = ClipHigh(effectB + nTicks, 0);
	}
}


void scrInit( void )
{
	RESHANDLE hRes;

	curPalette = kPalNormal;
	curGamma = 0;

	tioPrint("Loading gamma correction table");
	hRes = gSysRes.Lookup("gamma", ".dat");
	if ( hRes == NULL )
		ThrowError("Gamma table not found", ES_ERROR);

	gGammaLevels = gSysRes.Size(hRes) / 256;
	gammaTable = (GAMMA_SUBTABLE *)gSysRes.Lock(hRes);
}


void scrSetGameMode( void )
{
	setgamemode();

	// initialize y lookups for helix driver
	for (int i = 0; i < kMaxRows; i++)
		gYLookup[i] = ylookup[i];

	if ( vidoption == 1 || vidoption == 2 )
	{
		gPageTable[0].bytesPerRow = ylookup[1];
		gPageTable[0].begin = (unsigned)frameplace;
	}

	clearview(0);
	scrNextPage();
	scrSetDac(0);
}


void scrNextPage( void )
{
	nextpage();

	switch ( vidoption )
	{
		case 0:	//	non-chained
			if (chainnumpages > 1)
				gPageTable[0].begin = (unsigned)chainplace;
			break;

		case 1:	// VESA 2.0 linear buffer
			gPageTable[0].begin = (unsigned)frameplace;

		case 2:	// memory buffered VESA/13h
			break;

		case 3:	// Tseng ET4000
			break;

		case 4:	// Paradise
			break;

		case 5:	// S3
			break;

		case 6:	// Crystal Eyes
			break;

		case 7:	// Red/Blue Stereogram
			break;
	}
}

