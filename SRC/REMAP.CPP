#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <dos.h>
#include <io.h>
#include <fcntl.h>
#include <conio.h>

#include <helix.h>

#include "error.h"
#include "getopt.h"
#include "palette.h"
#include "inifile.h"
#include "pcx.h"
#include "lbm.h"
#include "misc.h"
#include "gfx.h"
#include "debug4g.h"
#include "textio.h"


#define kMaxTiles 4096

short tilesizx[kMaxTiles];
short tilesizy[kMaxTiles];
long picanm[kMaxTiles];

struct FNODE
{
	FNODE *next;
	char name[1];
};


FNODE head = { &head, "" };
FNODE *tail = &head;


enum FILE_TYPE
{
	FT_NONE,
	FT_PAL,
	FT_PCX,
	FT_LBM,
	FT_ART,
};

static struct
{
	char *ext;
	FILE_TYPE fileType;
} ftTable[] =
{
	{ ".ART", FT_ART },
	{ ".LBM", FT_LBM },
	{ ".PAL", FT_PAL },
	{ ".PCX", FT_PCX },
	{ ".PCC", FT_PCX },
};


int nWeightR, nWeightG, nWeightB;
BOOL ExcludeColor[256];
BYTE RemapColor[256];
int URemapColor[256];
PALETTE srcPal, destPal, thisPal;
IniFile paltoolINI("PALTOOL.INI");
char palName[_MAX_PATH];
char bakname[_MAX_PATH], tempName[_MAX_PATH];
BOOL showImages = FALSE;


/*******************************************************************************
	FUNCTION:		ShowBanner()

	DESCRIPTION:	Show application banner
*******************************************************************************/
void ShowBanner( void )
{
    tioPrint("컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�");
	tioPrint("     Image Remap Tool Version 2.0  Copyright (c) 1995 Q Studios Corporation");
    tioPrint("컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�");
}


/*******************************************************************************
	FUNCTION:		ShowUsage()

	DESCRIPTION:	Display command-line parameter usage, then exit
*******************************************************************************/
void ShowUsage(void)
{
	tioPrint("Syntax: REMAP [options] files");
	tioPrint("  /Fxxx     Load palette from file [.PAL]");
	tioPrint("  /Mx,y     Map color x to color y");
	tioPrint("  /Oa[-b]   Color(s) in dest palette is off limits");
	tioPrint("");
	tioPrint("Supported file types are: PCX, LBM, ART");
	tioPrint("");
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

	NOTES:			Colors marked with a TRUE in ExcludeColor[n] are not
					considered.
*******************************************************************************/
BYTE FindClosestColor( int r, int g, int b )
{
	int i;
	int dr, dg, db, dist, matchDist, match;

	matchDist = 0x7FFFFFFF;

	for ( i = 0; i < 256; i++ )
	{
		if ( ExcludeColor[i] )
			continue;

		dist = 0;

		dg = (int)destPal[i].g - g;
		dist += nWeightG * dg * dg;
		if ( dist >= matchDist )
			continue;

		dr = (int)destPal[i].r - r;
		dist += nWeightR * dr * dr;
		if ( dist >= matchDist )
			continue;

		db = (int)destPal[i].b - b;
		dist += nWeightB * db * db;
		if ( dist >= matchDist )
			continue;

		matchDist = dist;
		match = i;

		if (dist == 0)
			break;
	}

	return (BYTE)match;
}


/*******************************************************************************
	FUNCTION:		BuildRemapTable()

	DESCRIPTION:	Create the lookup table to remap from srcPal to destPal
*******************************************************************************/
void BuildRemapTable(void)
{
	for (int i = 0; i < 256; i++)
	{
		if ( URemapColor[i] < 0 )
			RemapColor[i] = FindClosestColor(srcPal[i].r, srcPal[i].g, srcPal[i].b);
		else
			RemapColor[i] = (BYTE)URemapColor[i];
	}
}


void RemapBuffer( BYTE *buffer, int length, BYTE *remapTable );
#pragma aux RemapBuffer =\
	"	xor		eax,eax",\
	"loop1:",\
	"	mov		al,[edx]",\
	"	mov		al,[ebx+eax]",\
	"	mov		[edx],al",\
	"	inc		edx",\
	"	dec		ecx",\
	"	jnz		loop1",\
	parm [edx][ecx][ebx]\
	modify [eax edx]\


FILE_TYPE GetFileType( char *filename )
{
	char ext[_MAX_EXT];
	_splitpath(filename, NULL, NULL, NULL, ext);

	for ( int i = 0; i < LENGTH(ftTable); i++ )
	{
		if ( stricmp(ext, ftTable[i].ext) == 0 )
			return ftTable[i].fileType;
	}

	return FT_NONE;
}


void GetNewPalette(void)
{
	int width = 0, height = 0;	// not really used, but necessary for ReadXXX call

	switch( GetFileType(palName) )
	{
		case FT_PAL:
			if ( !FileLoad(palName, destPal, sizeof(destPal)) )
				QuitMessage("Couldn't open palette file '%s'", palName);
			break;

		case FT_PCX:
		{
			int r = ReadPCX(palName, destPal, &width, &height, NULL);

			if ( r != PCX_OKAY )
				QuitMessage("Error reading palette from %s: code = %d", palName, r);
			break;
		}

		case FT_LBM:
		{
			int r = ReadLBM(palName, destPal, &width, &height, NULL);

			if ( r != LBM_OKAY )
				QuitMessage("Error reading palette from %s: code = %d", palName, r);
			break;
		}

	}
}


void DisplayImage( BYTE *bits, int width, int height )
{
	int size = width * height;

	QBITMAP *qbmSource = (QBITMAP *)malloc(sizeof(QBITMAP) + size);
	dassert(qbmSource != NULL);

	memcpy(qbmSource->data, bits, size);

	qbmSource->bitModel = BM_RAW;
	qbmSource->tcolor = 0;
	qbmSource->cols = (short)width;
	qbmSource->rows = (short)height;
	qbmSource->stride = (short)width;

	gfxDrawBitmap(qbmSource, 160 - width / 2, 100 - height / 2);

	free(qbmSource);
}


void RemapART( char *filename )
{
	int hInFile, hOutFile;
	int tileStart, tileEnd;
	long artversion, numtiles;
	char artPalName[_MAX_PATH];

	strcpy(artPalName, paltoolINI.GetKeyString(NULL, "GamePal", ""));
	if ( !FileLoad(artPalName, thisPal, sizeof(thisPal)) )
		QuitMessage("Couldn't open palette file '%s'", artPalName);

	if ( memcmp(srcPal, thisPal, sizeof(PALETTE)) != 0 )
	{
		memcpy(srcPal, thisPal, sizeof(PALETTE));
		BuildRemapTable();
	}

	hInFile = open(filename, O_BINARY | O_RDWR);
	if (hInFile == -1)
		QuitMessage("Error opening %s", filename);

	tmpnam(tempName);
	hOutFile = open(tempName, O_CREAT | O_WRONLY | O_BINARY | O_TRUNC, S_IWUSR);
	if ( hOutFile == -1 )
		QuitMessage("Error creating temporary file");

	read(hInFile, &artversion, sizeof(artversion));
	read(hInFile, &numtiles, sizeof(numtiles));
	read(hInFile, &tileStart, sizeof(tileStart));
	read(hInFile, &tileEnd, sizeof(tileEnd));

	int nTiles = tileEnd - tileStart + 1;
	read(hInFile, &tilesizx[tileStart], nTiles * sizeof(tilesizx[0]));
	read(hInFile, &tilesizy[tileStart], nTiles * sizeof(tilesizy[0]));
	read(hInFile, &picanm[tileStart], nTiles * sizeof(picanm[0]));

	write(hOutFile, &artversion, sizeof(artversion));
	write(hOutFile, &numtiles, sizeof(numtiles));
	write(hOutFile, &tileStart, sizeof(tileStart));
	write(hOutFile, &tileEnd, sizeof(tileEnd));
	write(hOutFile, &tilesizx[tileStart], nTiles * sizeof(tilesizx[0]));
	write(hOutFile, &tilesizy[tileStart], nTiles * sizeof(tilesizy[0]));
	write(hOutFile, &picanm[tileStart], nTiles * sizeof(picanm[0]));

	for (int i = tileStart; tioGauge(i - tileStart, nTiles); i++)
	{
		int nSize = tilesizx[i] * tilesizy[i];

		if (nSize == 0)
			continue;

		BYTE *pBits = (BYTE *)malloc(nSize);
		dassert(pBits != NULL);

		read(hInFile, pBits, nSize);
		RemapBuffer(pBits, nSize, RemapColor);
		write(hOutFile, pBits, nSize);

		if ( showImages )
			DisplayImage(pBits, tilesizy[i], tilesizx[i]);
		free(pBits);
	}

	close(hInFile);
	close(hOutFile);

	// backup the existing sequence
	strcpy(bakname, filename);
	ChangeExtension(bakname, ".BAK");
	unlink(bakname);
	rename(filename, bakname);
	rename(tempName, filename);
}


void RemapPCX( char *filename )
{
	int srcWidth = 0, srcHeight = 0;
	int nResult;

	nResult = ReadPCX(filename, NULL, &srcWidth, &srcHeight, NULL);
	if (nResult != 0)
		QuitMessage("Problem reading PCX file %s: error=%d", filename, nResult);

	BYTE *pBits = NULL;

	nResult = ReadPCX(filename, thisPal, &srcWidth, &srcHeight, &pBits);
	if (nResult != 0)
		QuitMessage("Problem reading PCX file %s: error=%d", filename, nResult);

	if ( memcmp(srcPal, thisPal, sizeof(PALETTE)) != 0 )
	{
		memcpy(srcPal, thisPal, sizeof(PALETTE));
		BuildRemapTable();
	}

	RemapBuffer(pBits, srcWidth * srcHeight, RemapColor);

	if ( showImages )
		DisplayImage(pBits, srcWidth, srcHeight);

	tmpnam(tempName);
	nResult = WritePCX(tempName, destPal, srcWidth, srcHeight, pBits);
	if (nResult != 0)
		QuitMessage("Problem writing temporary PCX: error=%d", nResult);

	// backup the existing sequence
	strcpy(bakname, filename);
	ChangeExtension(bakname, ".BAK");
	unlink(bakname);
	rename(filename, bakname);
	rename(tempName, filename);

	free(pBits);
}


void RemapLBM( char *filename )
{
	int srcWidth = 0, srcHeight = 0;
	int nResult;

	nResult = ReadLBM(filename, NULL, &srcWidth, &srcHeight, NULL);
	if (nResult != 0)
		QuitMessage("Problem reading LBM file %s: error=%d", filename, nResult);

	BYTE *pBits = NULL;

	nResult = ReadLBM(filename, thisPal, &srcWidth, &srcHeight, &pBits);
	if (nResult != 0)
		QuitMessage("Problem reading LBM file %s: error=%d", filename, nResult);

	if ( memcmp(srcPal, thisPal, sizeof(PALETTE)) != 0 )
	{
		memcpy(srcPal, thisPal, sizeof(PALETTE));
		BuildRemapTable();
	}

	RemapBuffer(pBits, srcWidth * srcHeight, RemapColor);

	if ( showImages )
		DisplayImage(pBits, srcWidth, srcHeight);

	free(pBits);
}


void ProcessFile( char *filename )
{
	tioPrint("Processing %s", filename);

	FILE_TYPE fileType = GetFileType(filename);

	if ( fileType == FT_NONE )
		QuitMessage("Don't know how to handle file %s", filename);

	// determine the size of the source image
	switch ( fileType )
	{
		case FT_ART:
			RemapART(filename);
			break;

		case FT_LBM:
			RemapLBM(filename);
			break;

		case FT_PCX:
			RemapPCX(filename);
			break;
	}
}


void Initialize(void)
{
	for (int i = 0; i < 256; i++)
	{
		ExcludeColor[i] = FALSE;
		URemapColor[i] = -1;
	}
}


void InsertFilename( char *filename )
{
	FNODE *n = (FNODE *)malloc(sizeof(FNODE) + strlen(filename));
	strcpy(n->name, filename);

	// insert the node at the tail, so it stays in order
	n->next = tail->next;
	tail->next = n;
	tail = n;
}


void ProcessArgument(char *s)
{
	char filespec[_MAX_PATH];
	char buffer[_MAX_PATH2];
	char path[_MAX_PATH];

	strcpy(filespec, s);

	char *drive, *dir;

	// separate the path from the filespec
	_splitpath2(s, buffer, &drive, &dir, NULL, NULL);
	_makepath(path, drive, dir, NULL, NULL);

	struct find_t fileinfo;

	unsigned r = _dos_findfirst(s, _A_NORMAL, &fileinfo);
	if (r != 0)
		tioPrint("%s not found", s);

	while ( r == 0 )
	{
		strcpy(filespec, path);
		strcat(filespec, fileinfo.name);
		InsertFilename(filespec);

		r = _dos_findnext( &fileinfo );
	}

	_dos_findclose(&fileinfo);
}


/***********************************************************************
 * Process command line arguments
 **********************************************************************/
void ParseOptions( void )
{
	enum {
		kSwitchHelp,
		kSwitchFile,
		kSwitchMap,
		kSwitchOmit,
	};
	static SWITCH switches[] = {
		{ "?", kSwitchHelp,	FALSE },
		{ "F", kSwitchFile,	TRUE },
		{ "M", kSwitchMap,	TRUE },
		{ "O", kSwitchOmit,	TRUE },
		{ NULL, 0, FALSE },
	};
	char buffer[256];
	int r;
	while ( (r = GetOptions(switches)) != GO_EOF )
	{
		switch (r)
		{
			case GO_INVALID:
				sprintf(buffer, "Invalid argument: %s", OptArgument);
				ThrowError(buffer, ES_ERROR);
				break;

			case GO_FULL:
				ProcessArgument(OptArgument);
				break;

			case kSwitchHelp:
				ShowUsage();
				break;

			case kSwitchFile:
				strcpy(palName, OptArgument);
				AddExtension(palName, ".PAL");
				break;

			case kSwitchMap:
			{
				int nFrom, nTo;
				if ( sscanf(OptArgument, "%i,%i", &nFrom, &nTo) != 2 )
					QuitMessage("Syntax error: %s", OptArgument);
				URemapColor[nFrom] = nTo;
				break;
			}

			case kSwitchOmit:
			{
				int nFrom, nTo;
				switch ( sscanf(OptArgument, "%i-%i", &nFrom, &nTo) )
				{
					case 1:
						ExcludeColor[nFrom] = TRUE;
						break;

					case 2:
						while (nFrom <= nTo)
							ExcludeColor[nFrom++] = TRUE;
						break;

					default:
						QuitMessage("Syntax error: %s", OptArgument);
				}
				break;
			}
		}
	}
}


void main( int argc	/*, char *argv[]*/)
{
	tioInit(0);

	ShowBanner();

	if (argc < 2)
		ShowUsage();

	nWeightR = paltoolINI.GetKeyInt(NULL, "WeightR", 1);
	nWeightG = paltoolINI.GetKeyInt(NULL, "WeightG", 1);
	nWeightB = paltoolINI.GetKeyInt(NULL, "WeightB", 1);

	Initialize();
	ParseOptions();

	if ( head.next == &head )
		QuitMessage("No source file specified");

	if ( palName[0] == '\0' )
		QuitMessage("You must specify a new palette.");

	GetNewPalette();

	int oldMode = gGetMode();

	if (!gFindMode(320, 200, 256, CHAIN256))
		QuitMessage("Helix driver not found");

	if ( showImages )
	{
		Video.SetMode();
		gfxSetClip(0, 0, 320, 200);
	 	gSetDACRange(0, 256, destPal);
	}

	// process the file list
	for (FNODE *n = head.next; n != &head; n = n->next)
		ProcessFile(n->name);

	if ( showImages )
		gSetMode(oldMode);
}



