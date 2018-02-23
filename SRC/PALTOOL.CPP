#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>

#include "screen.h"
#include "palette.h"
#include "debug4g.h"
#include "textio.h"
#include "inifile.h"
#include "getopt.h"
#include "misc.h"

#define kMistLevels	32
#define kMistValue	180

RGB gamePalette[256];

int rdist[256], gdist[256], bdist[256];

int nPalLookups;
int nWeightR, nWeightG, nWeightB;
int nMaxDark;

IniFile paltoolINI("PALTOOL.INI");


/*******************************************************************************
	FUNCTION:		ShowBanner()

	DESCRIPTION:	Show application banner
*******************************************************************************/
void ShowBanner( void )
{
    tioPrint("컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�");
	tioPrint("          PalTool Version 1.0  Copyright (c) 1995 Q Studios Corporation");
    tioPrint("컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�");
}


/*******************************************************************************
	FUNCTION:		ShowUsage()

	DESCRIPTION:	Display command-line parameter usage, then exit
*******************************************************************************/
void ShowUsage(void)
{
	tioPrint("Syntax:  PALTOOL destfile command");
	tioPrint("-c filename  create color LUT (CLU) using specified palette file");
	tioPrint("-f N[,O]     create fog LUT (FLU) with gray level of N, opacity O");
	tioPrint("-s filename  create shade LUT (PLU) using specified palette file");
	tioPrint("-p filename  create palette (PAL) from specified lookup table (CLU/PLU)");
	tioPrint("-t N         create translucency LUT (TLU) with opacity of N%");
	tioPrint("-?           this help");
	exit(0);
}


/*******************************************************************************
	FUNCTION:		QuitMessage()

	DESCRIPTION:	Display a printf() style message, the exit with code 1
*******************************************************************************/
void QuitMessage(char * fmt, ...)
{
	char msg[80];
	va_list argptr;
	va_start( argptr, fmt );
	vsprintf( msg, fmt, argptr );
	va_end(argptr);
	tioPrint(msg);
	exit(1);
}


/*******************************************************************************
	FUNCTION:		FindClosestColor()

	DESCRIPTION:	Finds the closest color in RGB using the weights in
					nWeightR, nWeightG, and nWeightB.

	RETURNS:		Index of the closest color
*******************************************************************************/
BYTE FindClosestColor( int r, int g, int b )
{
	int i;
	int dist, matchDist, match;

	matchDist = 0x7FFFFFFF;

	for ( i = 0; i < 256; i++ )
	{
		dist = 0;

		dist += gdist[qabs((int)gamePalette[i].g - g)];
		if ( dist >= matchDist )
			continue;

		dist += rdist[qabs((int)gamePalette[i].r - r)];
		if ( dist >= matchDist )
			continue;

		dist += bdist[qabs((int)gamePalette[i].b - b)];
		if ( dist >= matchDist )
			continue;

		matchDist = dist;
		match = i;

		if (dist == 0)
			break;
	}

	return (BYTE)match;
}


void BuildCLU( char *filename, RGB *palette)
{
	int i;
	char cluName[_MAX_PATH];

	BYTE *table = (BYTE *)malloc(256);

	for ( i = 0; tioGauge(i, 256); i++ )
		table[i] = FindClosestColor(palette[i].r, palette[i].g, palette[i].b);

	strcpy(cluName, filename);
	ChangeExtension(cluName, ".CLU");
	FileSave(cluName, table, 256);
	free(table);
}


void BuildPLU( char *filename, RGB *palette, int grayLevel)
{
	int i, j;
	int r, g, b;
	char pluName[_MAX_PATH];

	BYTE *table = (BYTE *)malloc(256 * nPalLookups);
	BYTE *p = table;

	for ( i = 0; tioGauge(i, nPalLookups); i++ )
	{
		for ( j = 0; j < 256; j++ )
		{
			int n = muldiv(i, nMaxDark, nPalLookups);
			r = dmulscale16r(0x10000 - n, palette[j].r, n, grayLevel);
			g = dmulscale16r(0x10000 - n, palette[j].g, n, grayLevel);
			b = dmulscale16r(0x10000 - n, palette[j].b, n, grayLevel);

			p[j] = FindClosestColor(r, g, b);
		}
		p += 256;
	}

	strcpy(pluName, filename);
	ChangeExtension(pluName, ".PLU");
	FileSave(pluName, table, 256 * nPalLookups);
	free(table);
}


void BuildFLU( char *filename, RGB *palette, int grayLevel, int nOpacity)
{
	int i, j, k;
	int r, g, b;
	char fluName[_MAX_PATH];

	int nDepthLevels = 16;
	int nBrightLevels = 16;

	// convert to 16:16 fixed point fraction
	nOpacity = divscale16(nOpacity, 100);

	BYTE *table = (BYTE *)malloc(nDepthLevels * nBrightLevels * 256);
	BYTE *p = table;

	for ( i = 0; tioGauge(i, nDepthLevels); i++ )
	{
		int nDepth = muldiv(i, nOpacity, nDepthLevels);
		for ( j = 0; j < nBrightLevels; j++ )
		{
			int nShade = muldiv(j, 0x0F000, nBrightLevels);
			for ( k = 0; k < 256; k++ )
			{
				r = mulscale16r(0x10000 - nShade, palette[k].r);
				r = dmulscale16r(0x10000 - nDepth, r, nDepth, grayLevel);
				g = mulscale16r(0x10000 - nShade, palette[k].g);
				g = dmulscale16r(0x10000 - nDepth, g, nDepth, grayLevel);
				b = mulscale16r(0x10000 - nShade, palette[k].b);
				b = dmulscale16r(0x10000 - nDepth, b, nDepth, grayLevel);

				p[k] = FindClosestColor(r, g, b);
			}
			p += 256;
		}
	}

	strcpy(fluName, filename);
	ChangeExtension(fluName, ".FLU");
	FileSave(fluName, table, nDepthLevels * nBrightLevels * 256);
	free(table);
}


/*
** lucWeight range = 0 - 256
*/
void BuildTLU( char *filename, RGB *palette, int nPercent)
{
	char tluName[_MAX_PATH];
	int i, j;
	int r, g, b;
	int lucWeight = divscale16(nPercent, 100);

	BYTE *table = (BYTE *)malloc(65536);
	BYTE *p = table;

	for ( i = 0; tioGauge(i, 256); i++ )
	{
		for ( j = 0; j < 256; j++ )
		{
			r = dmulscale16r(0x10000 - lucWeight, palette[i].r, lucWeight, palette[j].r);
			g = dmulscale16r(0x10000 - lucWeight, palette[i].g, lucWeight, palette[j].g);
			b = dmulscale16r(0x10000 - lucWeight, palette[i].b, lucWeight, palette[j].b);

			p[j] = FindClosestColor(r, g, b);
		}
		p += 256;
	}

	strcpy(tluName, filename);
	ChangeExtension(tluName, ".TLU");
	FileSave(tluName, table, 65536);
	free(table);
}


void LUTtoPAL( BYTE *table, RGB *palette )
{
	dassert(table != NULL);
	dassert(palette != NULL);

	for ( int i = 0; i < 256; i++ )
	{
		palette[i] = gamePalette[ table[i] ];
	}
}


/***********************************************************************
 * Process command line arguments
 **********************************************************************/
void ParseOptions( void )
{
	enum {
		kSwitchColor,
		kSwitchFog,
		kSwitchPal,
		kSwitchShade,
		kSwitchTrans,
		kSwitchHelp,
	};
	static SWITCH switches[] = {
		{ "?", kSwitchHelp, FALSE },
		{ "C", kSwitchColor, TRUE },
		{ "F", kSwitchFog, TRUE },
		{ "P", kSwitchPal, TRUE },
		{ "S", kSwitchShade, TRUE },
		{ "T", kSwitchTrans, TRUE },
		{ NULL, 0, FALSE },

	};
	char destName[_MAX_PATH] = "";
	int r;
	while ( (r = GetOptions(switches)) != GO_EOF )
	{
		switch (r)
		{
			case GO_INVALID:
				QuitMessage("Invalid argument: %s", OptArgument);

			case GO_FULL:
				if (destName[0] != '\0')
					QuitMessage("Invalid argument: %s", OptArgument);

				strcpy(destName, OptArgument);
				break;

			case kSwitchColor:
			{
				char palName[_MAX_PATH];
				RGB palette[256];

				strcpy(palName, OptArgument);
				AddExtension(palName, "PAL");

				if ( !FileLoad(palName, palette, sizeof(palette)) )
					QuitMessage("Error loading %s", palName);

				tioPrint("Building color LUT from %s", palName);
				BuildCLU(destName, palette);
				break;
			}

			case kSwitchFog:
			{
				int nGrayLevel = 0, nOpacity = 100;
				sscanf(OptArgument, "%d,%d", &nGrayLevel, &nOpacity);
				tioPrint("Building fog LUT, gray level = %d, opacity = %d%%",
					nGrayLevel, nOpacity);
				BuildFLU(destName, gamePalette, nGrayLevel, nOpacity);
				break;
			}

			case kSwitchPal:
			{
				char zName[_MAX_PATH];
				char *ext;
				char buffer[_MAX_PATH2];
				RGB palette[256];

				strcpy( zName, OptArgument );
				_splitpath2(zName, buffer, NULL, NULL, NULL, &ext);

				if ( strcmpi(ext, ".CLU") == 0 || strcmpi(ext, ".PLU") == 0 )
				{
					BYTE *table = (BYTE *)malloc(256);
					if ( !FileLoad(zName, table, 256) )
						QuitMessage("Error loading %s", zName);
					LUTtoPAL(table, palette);
					ChangeExtension(zName, ".PAL");
					FileSave(zName, palette, sizeof(palette));
					free(table);
				}
				else
					QuitMessage(".PLU or .CLU extension required for parameter %s", zName);
				break;
			}

			case kSwitchShade:
			{
				char palName[_MAX_PATH];
				RGB palette[256];

				strcpy(palName, OptArgument);
				AddExtension(palName, "PAL");

				if ( !FileLoad(palName, palette, sizeof(palette)) )
					QuitMessage("Error loading %s", palName);

				tioPrint("Building shade LUT from %s", palName);
				BuildPLU(destName, palette, 0);
				break;
			}

			case kSwitchTrans:
			{
				int nPercent = strtol(OptArgument, NULL, 0);
				tioPrint("Building translucency LUT, opacity = %d%%", nPercent);
				BuildTLU(destName, gamePalette, nPercent);
				break;
			}

			case kSwitchHelp:
				ShowUsage();
				break;
		}
	}
}


void main( int argc )
{
	char sGamePal[_MAX_PATH];

	tioInit(0);

	ShowBanner();

	nPalLookups = paltoolINI.GetKeyInt(NULL, "NumPalLookups", 32);
	nWeightR = paltoolINI.GetKeyInt(NULL, "WeightR", 1);
	nWeightG = paltoolINI.GetKeyInt(NULL, "WeightG", 1);
	nWeightB = paltoolINI.GetKeyInt(NULL, "WeightB", 1);
	strcpy(sGamePal, paltoolINI.GetKeyString(NULL, "GamePal", ""));
	nMaxDark = paltoolINI.GetKeyInt(NULL, "MaxDark", 0xE000);

	if ( !FileLoad(sGamePal, gamePalette, sizeof(gamePalette)) )
		QuitMessage("Error loading %s", sGamePal);

	if (argc == 1)
		ShowUsage();

	tioPrint("nPalLookups = %d, WeightR = %d, WeightG = %d, WeightB = %d",
		nPalLookups, nWeightR, nWeightG, nWeightB);

	tioPrint("Creating distance lookup table");
	for (int i = 0; tioGauge(i, 256); i++)
	{
		int square = i * i;
		rdist[i] = nWeightR * square;
		gdist[i] = nWeightG * square;
		bdist[i] = nWeightB * square;
	}

	ParseOptions();

	tioPrint("");

	tioTerm();
}


/*
void InverseColor(int *r, int *g, int *b)
{
	int gray = (*r * 2 + *g * 5 + *b) / 8;

	*r = 255 - gray;
	*g = 255 - gray;
	*b = 255 - gray;
}


void BeastColor(int *r, int *g, int *b)
{
	*r = gammaTable[1][*r];
	*g = gammaTable[1][*g / 2];
	*b = gammaTable[1][*b / 2];
}


void ScopeColor(int *r, int *g, int *b)
{
	int i;

	i = ClipLow(*r - *g / 4 - *b / 2, 0);

	*r = gammaTable[3][i / 2];
	*g = gammaTable[3][i];
	*b = gammaTable[3][i / 2];
}


void WaterColor(int *r, int *g, int *b)
{
	*r = gammaTable[1][*r / 2];
	*g = gammaTable[1][*g / 2];
	*b = gammaTable[1][*b];
}

void GrayColor(int *r, int *g, int *b)
{
	int gray = (*r * 2 + *g * 5 + *b) / 8;

	*r = gray;
	*g = gray;
	*b = gray;
}


void GrayishColor(int *r, int *g, int *b)
{
	int gray = (*r * 2 + *g * 5 + *b) / 8;

	*r = (*r + gray) / 2;
	*g = (*g + gray) / 2;
	*b = (*b + gray) / 2;
}
*/
