#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include "typedefs.h"
#include "misc.h"
#include "debug4g.h"
#include "getopt.h"
#include "error.h"

#define kMaxTiles 4096

struct PICANM {
	unsigned frames : 5;	// number of frames - 1
	unsigned update : 1;	// this came from upper bit of frames
	unsigned type : 2;		// 0 = none, 1 = Oscil, 2 = Frwd, 3 = Bkwd
	signed xcenter : 8;
	signed ycenter : 8;
  	unsigned speed : 4;		// (clock >> speed) determines rate
	unsigned view : 2;
	unsigned registered : 1;
} picanm[kMaxTiles];

long artversion;
short tilesizx[kMaxTiles];
short tilesizy[kMaxTiles];
int checksum[kMaxTiles];


/***********************************************************************
 * ShowBanner
 *
 * Display application banner.
 **********************************************************************/
void ShowBanner( void )
{
    printf("컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�\n");
	printf(" APE - Art Patch Engine  Version 1.1  Copyright (c) 1995 Q Studios Corporation\n");
    printf("컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�\n");
}


/***********************************************************************
 * ShowUsage
 *
 * Display command-line parameter usage, then exit.
 **********************************************************************/
void ShowUsage(void)
{
	printf("Syntax:  APE [commands]\n");
	printf("-a filename   Apply delta file\n");
	printf("-b filename   Build delta file from checksum\n");
	printf("-c filename   Create checksum file\n");
	printf("-?            This help\n");
	printf("\n");
	exit(0);
}


void CreateChecksumFile( char *argument )
{
	char filename[_MAX_PATH];
	int i;
	int nTiles;
	int nFile, hFile;
	int nSize;
	int tileStart, tileEnd;
	long numtiles;

	printf("Processing tile     ");
	nFile = 0;
	while (1)
	{
		sprintf(filename, "TILES%03i.ART", nFile);
		hFile = open(filename, O_BINARY | O_RDONLY);

		if ( hFile == -1 )
			break;

		FileRead(hFile, &artversion, sizeof(artversion));
		dassert(artversion == 1);

		FileRead(hFile, &numtiles, sizeof(numtiles));
		FileRead(hFile, &tileStart, sizeof(tileStart));
		FileRead(hFile, &tileEnd, sizeof(tileEnd));

		nTiles = tileEnd - tileStart + 1;
		FileRead(hFile, &tilesizx[tileStart], nTiles * sizeof(tilesizx[0]));
		FileRead(hFile, &tilesizy[tileStart], nTiles * sizeof(tilesizy[0]));
		FileRead(hFile, &picanm[tileStart], nTiles * sizeof(picanm[0]));

		for (i = tileStart; i <= tileEnd; i++)
		{
			nSize = tilesizx[i] * tilesizy[i];

			if ( nSize > 0 )
			{
				printf("\b\b\b\b%4d", i);
				BYTE *p = (BYTE *)malloc(nSize);
				dassert(p != NULL);

				if ( !FileRead(hFile, p, nSize) )
					ThrowError("Error FileReading tile file", ES_ERROR);

				checksum[i] = CRC32(p, nSize);
				free(p);
			}
		}
		close(hFile);
		nFile++;
	}
	printf("\n");

	strcpy(filename, argument);
	AddExtension(filename, "APC");
	if ( !FileSave(filename, checksum, sizeof(checksum)) )
		ThrowError("Error creating output file", ES_ERROR);

	printf("Checksum file %s created.\n", filename);
}


void BuildDeltaFile( char *argument )
{
	char filename[_MAX_PATH];
	int i;
	int nTiles;
	int nFile, hTileFile, hDeltaFile;
	int nSize;
	int tileStart, tileEnd;
	long numtiles;
	int crc;
	BYTE *p = NULL;

	strcpy(filename, argument);
	ChangeExtension(filename, "APC");
	if ( !FileLoad(filename, checksum, sizeof(checksum)) )
		ThrowError("Error FileReading checksum file", ES_ERROR);

	ChangeExtension(filename, "APD");
	hDeltaFile = open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IWUSR);
	if (hDeltaFile == -1)
		ThrowError("Error creating delta file", ES_ERROR);

	printf("Processing tile     ");
	nFile = 0;
	while (1)
	{
		sprintf(filename, "TILES%03i.ART", nFile);
		hTileFile = open(filename, O_BINARY | O_RDONLY);

		if ( hTileFile == -1 )
			break;

		FileRead(hTileFile, &artversion, sizeof(artversion));
		dassert(artversion == 1);

		FileRead(hTileFile, &numtiles, sizeof(numtiles));
		FileRead(hTileFile, &tileStart, sizeof(tileStart));
		FileRead(hTileFile, &tileEnd, sizeof(tileEnd));

		nTiles = tileEnd - tileStart + 1;
		FileRead(hTileFile, &tilesizx[tileStart], nTiles * sizeof(tilesizx[0]));
		FileRead(hTileFile, &tilesizy[tileStart], nTiles * sizeof(tilesizy[0]));
		FileRead(hTileFile, &picanm[tileStart], nTiles * sizeof(picanm[0]));

		for (i = tileStart; i <= tileEnd; i++)
		{
			printf("\b\b\b\b%4d", i);
			crc = 0;

			nSize = tilesizx[i] * tilesizy[i];
			if ( nSize > 0 )
			{
				p = (BYTE *)malloc(nSize);
				dassert(p != NULL);

				FileRead(hTileFile, p, nSize);
				crc = CRC32(p, nSize);
			}

			if ( crc != checksum[i] )
			{
				picanm[i].update = 1;
				FileWrite(hDeltaFile, &picanm[i], sizeof(picanm[i]));
				FileWrite(hDeltaFile, &tilesizx[i], sizeof(tilesizx[i]));
				FileWrite(hDeltaFile, &tilesizy[i], sizeof(tilesizy[i]));
				FileWrite(hDeltaFile, p, nSize);
			}
			else
			{
				picanm[i].update = 0;
				FileWrite(hDeltaFile, &picanm[i], sizeof(picanm[i]));
			}

			if ( p != NULL )
				free(p);
		}
		close(hTileFile);
		nFile++;
	}
	printf("\nDelta file created\n");
	close(hDeltaFile);
}


void ApplyDeltaFile( char *argument )
{
	char tileName[_MAX_PATH], bakName[_MAX_PATH], tempName[_MAX_PATH], deltaName[_MAX_PATH];
	long offset;
	int i;
	int nTiles = 0;	// tiles per art file
	int nFile, hTileFile, hDeltaFile, hTemp;
	int nSize;
	int tileStart, tileEnd;
	long numtiles;
	BYTE *p = NULL;
	int nChanged = 0;
	FILE *fp;	// use stream so we can write formatted text more easily

	fp = fopen("ape.rpt", "wt");
	if ( !fp )
		ThrowError("Error opening report file", ES_ERROR);

	strcpy(deltaName, argument);
	ChangeExtension(deltaName, "APD");
	hDeltaFile = open(deltaName, O_RDONLY | O_BINARY);
	if (hDeltaFile == -1)
		ThrowError("Error opening delta file", ES_ERROR);

	memset(picanm, 0, sizeof(picanm));
	memset(tilesizx, 0, sizeof(tilesizx));
	memset(tilesizy, 0, sizeof(tilesizy));

	printf("Processing tile     ");
	nFile = 0;
	while ( !eof(hDeltaFile) )
	{
		hTemp = -1;

		tmpnam(tempName);
		sprintf(tileName, "TILES%03i.ART", nFile);
		hTileFile = open(tileName, O_BINARY | O_RDONLY);
		if ( hTileFile != -1 )
		{
			FileRead(hTileFile, &artversion, sizeof(artversion));

			FileRead(hTileFile, &numtiles, sizeof(numtiles));
			FileRead(hTileFile, &tileStart, sizeof(tileStart));
			FileRead(hTileFile, &tileEnd, sizeof(tileEnd));

			nTiles = tileEnd - tileStart + 1;
			FileRead(hTileFile, &tilesizx[tileStart], nTiles * sizeof(tilesizx[0]));
			FileRead(hTileFile, &tilesizy[tileStart], nTiles * sizeof(tilesizy[0]));
			FileRead(hTileFile, &picanm[tileStart], nTiles * sizeof(picanm[0]));

			// get current position
			offset = lseek(hTileFile, 0, SEEK_CUR);

			// create the output file
			hTemp = open(tempName, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IWUSR);

			// position the output file to the start of the tile data
			lseek(hTemp, offset, SEEK_SET);
		}

		// merge all the changed tiles
		for (i = tileStart; i <= tileEnd; i++)
		{
			printf("\b\b\b\b%4d", i);

			FileRead(hDeltaFile, &picanm[i], sizeof(picanm[i]));
			nSize = tilesizx[i] * tilesizy[i];
			if ( picanm[i].update )
			{
				// we may need to create a new art file
				if (hTemp == -1)
				{
					// we've got to have opened at least one previous file...
					dassert(nTiles != 0);

					// create the output file
					hTemp = open(tempName, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IWUSR);

					// position the output file to the start of the tile data
					lseek(hTemp, offset, SEEK_SET);
				}

				picanm[i].update = 0;

				// skip past the tile we're replacing
				if ( hTileFile != -1 )
					lseek(hTileFile, nSize, SEEK_CUR);

				FileRead(hDeltaFile, &tilesizx[i], sizeof(tilesizx[i]));
				FileRead(hDeltaFile, &tilesizy[i], sizeof(tilesizy[i]));
				nSize = tilesizx[i] * tilesizy[i];

				if ( nSize > 0 )
				{
					p = (BYTE *)malloc(nSize);
					dassert(p != NULL);

					FileRead(hDeltaFile, p, nSize);
					FileWrite(hTemp, p, nSize);

					fprintf(fp, "Tile %4d replaced or added\n", i);
				}
				else
					fprintf(fp, "Tile %4d removed\n", i);

				nChanged++;
			}
			else
			{
				if ( nSize > 0 )
				{
					// if we're not updating, it must exist
					dassert(hTileFile != -1);

					p = (BYTE *)malloc(nSize);
					dassert(p != NULL);

					FileRead(hTileFile, p, nSize);
					FileWrite(hTemp, p, nSize);
				}
			}

			if ( p != NULL )
				free(p);
		}

		if ( hTileFile != -1 )
			close(hTileFile);

		if ( hTemp != -1 )
		{
			// now write out the art file header
			lseek(hTemp, 0, SEEK_SET);
			FileWrite(hTemp, &artversion, sizeof(artversion));
			FileWrite(hTemp, &numtiles, sizeof(numtiles));
			FileWrite(hTemp, &tileStart, sizeof(tileStart));
			FileWrite(hTemp, &tileEnd, sizeof(tileEnd));
			FileWrite(hTemp, &tilesizx[tileStart], nTiles * sizeof(tilesizx[0]));
			FileWrite(hTemp, &tilesizy[tileStart], nTiles * sizeof(tilesizy[0]));
			FileWrite(hTemp, &picanm[tileStart], nTiles * sizeof(picanm[0]));
			close(hTemp);

			// make the current tile file into a backup
			strcpy(bakName, tileName);
			ChangeExtension(bakName, ".BAK");
			rename(tileName, bakName);
			rename(tempName, tileName);
		}

		nFile++;

		// increment these just in case the next tile file doesn't exist yet
		tileStart += nTiles;
		tileEnd += nTiles;
	}
	close(hDeltaFile);
	fclose(fp);
	printf("\nUpdate complete.  %d tiles modified.  See APE.RPT for details.\n", nChanged);
}


/***********************************************************************
 * Process command line arguments
 **********************************************************************/
void ParseOptions( void )
{
	enum {
		kSwitchHelp,
		kSwitchApply,
		kSwitchBuild,
		kSwitchCreate,
	};
	static SWITCH switches[] = {
		{ "?", kSwitchHelp, FALSE },
		{ "A", kSwitchApply, TRUE },
		{ "B", kSwitchBuild, TRUE },
		{ "C", kSwitchCreate, TRUE },
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

			case GO_MISSING:
				ThrowError("Missing argument", ES_ERROR);
				break;

			case kSwitchHelp:
				ShowUsage();
				break;

			case GO_FULL:
				ShowUsage();
				break;

			case kSwitchApply:
				ApplyDeltaFile(OptArgument);
				break;

			case kSwitchBuild:
				BuildDeltaFile(OptArgument);
				break;

			case kSwitchCreate:
				CreateChecksumFile(OptArgument);
				break;

		}
	}
}


/***********************************************************************
 * Main
 **********************************************************************/
int main( int argc )
{
	ShowBanner();

	if ( argc == 1 )
		ShowUsage();

	ParseOptions();

	return 0;
}
