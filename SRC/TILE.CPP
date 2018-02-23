#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <i86.h>
#include <io.h>
#include <fcntl.h>

#include "engine.h"

#include "tile.h"
#include "helix.h"
#include "typedefs.h"
#include "debug4g.h"
#include "gameutil.h"
#include "misc.h"
#include "globals.h"
#include "key.h"
#include "gui.h"
#include "screen.h"
#include "error.h"
#include "db.h"

#define kMaxTileFiles 256

static CACHENODE tileNode[kMaxTiles];
static int tileStart[kMaxTileFiles], tileEnd[kMaxTileFiles];
static int hTileFile[kMaxTileFiles];
static BOOL tileFileDirty[kMaxTileFiles];
static int nTileFiles = 0;
static BOOL artLoaded = FALSE;
static int pickSize[] = { 20, 32, 40, 64, 80, 128, 160 };
static int tileHist[kMaxTiles];

short tileIndex[kMaxTiles];
int tileIndexCount;
BYTE surfType[kMaxTiles];


/*******************************************************************************
	FUNCTION:		tileTerm()

	DESCRIPTION:	Close all open tile files
*******************************************************************************/
void tileTerm( void )
{
	for (int i = 0; i < kMaxTileFiles; i++)
	{
		if (hTileFile[i] != -1)
		{
			close(hTileFile[i]);
			hTileFile[i] = -1;
		}
	}
}


static void CalcPicsiz( int nTile, int x, int y )
{
	int i;
	uchar n = 0;

	for (i = 2; i <= x; i <<= 1)
		n += 0x01;

	for (i = 2; i <= y; i <<= 1)
		n += 0x10;

	picsiz[nTile] = n;
}


int tileInit( void )
{
	char filename[20];
	long offset;
	int i;
	int nTiles;
	int nFile, hFile;

	// already loaded?
	if ( artLoaded )
		return nTileFiles;

	memset(tilesizx, 0, sizeof(tilesizx));
	memset(tilesizy, 0, sizeof(tilesizy));
 	memset(picanm, 0, sizeof(picanm));
	memset(gotpic, 0, sizeof(gotpic));
	memset(hTileFile, -1, sizeof(hTileFile));

	nFile = 0;
	while (1)
	{
		sprintf(filename, "TILES%03i.ART", nFile);
		hFile = open(filename, O_BINARY | O_RDWR);

		if ( hFile == -1 )
			break;

		hTileFile[nFile] = hFile;

		read(hFile, &artversion, sizeof(artversion));

		read(hFile, &numtiles, sizeof(numtiles));
		read(hFile, &tileStart[nFile], sizeof(tileStart[nFile]));
		read(hFile, &tileEnd[nFile], sizeof(tileEnd[nFile]));

		nTiles = tileEnd[nFile] - tileStart[nFile] + 1;
		read(hFile, &tilesizx[tileStart[nFile]], nTiles * sizeof(tilesizx[0]));
		read(hFile, &tilesizy[tileStart[nFile]], nTiles * sizeof(tilesizy[0]));
		read(hFile, &picanm[tileStart[nFile]], nTiles * sizeof(picanm[0]));

		// get current position
		offset = lseek(hFile, 0, SEEK_CUR);

		for (i = tileStart[nFile]; i <= tileEnd[nFile]; i++)
		{
			tilefilenum[i] = (char)nFile;
			tilefileoffs[i] = offset;
			offset += tilesizx[i] * tilesizy[i];
		}
		nFile++;
	}
	nTileFiles = nFile;
	dprintf("%i tile files opened\n", nTileFiles);

	for (i = 0; i < kMaxTiles; i++)
	{
		// setup the tile size log 2 array for internal engine use
		CalcPicsiz(i, tilesizx[i], tilesizy[i]);
		waloff[i] = NULL;
		tileNode[i].ptr = NULL;
	}

	hFile = open("SURFACE.DAT", O_BINARY | O_RDWR);
	if ( hFile != -1 )
	{
		read(hFile, surfType, sizeof(surfType));
		close(hFile);
	}

	artLoaded = TRUE;
	return nTileFiles;
}


void tileSaveArt( void )
{
	int nTiles, hFile, i, j;
	char tempName[20], fileName[20], bakName[20];

	for (i = 0; i < nTileFiles; i++)
	{
		if ( tileFileDirty[i] )
		{
			tmpnam(tempName);
			hFile = open(tempName, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IWUSR);

			write(hFile, &artversion, sizeof(artversion));

			write(hFile, &numtiles, sizeof(numtiles));
			write(hFile, &tileStart[i], sizeof(tileStart[i]));
			write(hFile, &tileEnd[i], sizeof(tileEnd[i]));

			nTiles = tileEnd[i] - tileStart[i] + 1;
			write(hFile, &tilesizx[tileStart[i]], nTiles * sizeof(tilesizx[0]));
			write(hFile, &tilesizy[tileStart[i]], nTiles * sizeof(tilesizy[0]));
			write(hFile, &picanm[tileStart[i]], nTiles * sizeof(picanm[0]));

			for (j = tileStart[i]; j <= tileEnd[i]; j++)
				write(hFile, tileLoadTile(j), tilesizx[j] * tilesizy[j]);

			close(hFile);

			sprintf(bakName, "TILES%03i.BAK", i);
			remove(bakName);
			sprintf(fileName, "TILES%03i.ART", i);
			rename(fileName, bakName);
			rename(tempName, fileName);

			tileFileDirty[i] = FALSE;
		}
	}

	for (i = 0; i < nTileFiles; i++)
	{
		close(hTileFile[i]);
		hTileFile[i] = -1;
	}

	hFile = open("SURFACE.DAT", O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IWUSR);
	write(hFile, surfType, sizeof(surfType));
	close(hFile);

	artLoaded = FALSE;
	tileInit();
}


void tileSaveArtInfo( void )
{
	int nTiles;
	int hFile;

	for (int i = 0; i < nTileFiles; i++)
	{
		if ( tileFileDirty[i] )
		{
			hFile = hTileFile[i];
			dassert(hFile != -1);

			lseek(hFile, 0, SEEK_SET);

			write(hFile, &artversion, sizeof(artversion));

			write(hFile, &numtiles, sizeof(numtiles));
			write(hFile, &tileStart[i], sizeof(tileStart[i]));
			write(hFile, &tileEnd[i], sizeof(tileEnd[i]));

			nTiles = tileEnd[i] - tileStart[i] + 1;
			write(hFile, &tilesizx[tileStart[i]], nTiles * sizeof(tilesizx[0]));
			write(hFile, &tilesizy[tileStart[i]], nTiles * sizeof(tilesizy[0]));
			write(hFile, &picanm[tileStart[i]], nTiles * sizeof(picanm[0]));

			tileFileDirty[i] = FALSE;
		}
	}

	hFile = open("SURFACE.DAT", O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IWUSR);
	write(hFile, surfType, sizeof(surfType));
	close(hFile);
}


void tileMarkDirty( int nTile )
{
	tileFileDirty[tilefilenum[nTile]] = TRUE;
}


void tileMarkDirtyAll( void )
{
	for (int i = 0; i < nTileFiles; i++)
		tileFileDirty[i] = TRUE;
}


// Remove a tile from the resource cache.  This gets called by tileMoveTiles()
void tilePurgeTile( int nTile )
{
	CACHENODE *node = &tileNode[nTile];

	if (node->ptr != NULL )
		Resource::Flush((RESHANDLE)node);
}


void tileRotateTiles( int start, int length, int shift )
{
	int i, j, k;
	int ttilefileoffs;
	short ttilesizx, ttilesizy;
	char ttilefilenum;
	uchar tpicsiz;
	BYTE *twaloff;
	PICANM tpicanm;

	while (shift < 0)
		shift += length;
	i = k = start;

	for (int n = length; n > 0; n--)
	{
		// invalidate cache data for this tile
		tilePurgeTile(i);
		tileMarkDirty(i);

		if (i == k)
		{
			ttilesizx = 	tilesizx[i];
			ttilesizy = 	tilesizy[i];
 			ttilefilenum = 	tilefilenum[i];
 			ttilefileoffs =	tilefileoffs[i];
 			tpicsiz = 		picsiz[i];
 			twaloff = 		waloff[i];
			tpicanm = 		picanm[i];
		}

		j = i + shift;
		while (j >= start + length)
			j -= length;

		if (j == k)
		{
			tilesizx[i] = 		ttilesizx;
			tilesizy[i] = 		ttilesizy;
	 		tilefilenum[i] = 	ttilefilenum;
	 		tilefileoffs[i] = 	ttilefileoffs;
	 		picsiz[i] = 		tpicsiz;
	 		waloff[i] = 		twaloff;
			picanm[i] = 		tpicanm;
			i = ++k;
			continue;
		}
		tilesizx[i] = 		tilesizx[j];
		tilesizy[i] = 		tilesizy[j];
 		tilefilenum[i] = 	tilefilenum[j];
 		tilefileoffs[i] =	tilefileoffs[j];
 		picsiz[i] = 		picsiz[j];
 		waloff[i] = 		waloff[j];
		picanm[i] = 		picanm[j];
		i = j;
	}
}


BYTE *tileLoadTile( int nTile )
{
	static nLastTile = 0;
	int hFile;

	if ( tileNode[nLastTile].lockCount == 0)
		waloff[nLastTile] = NULL;

	nLastTile = nTile;

	dassert(nTile >= 0 && nTile < kMaxTiles);
	CACHENODE *node = &tileNode[nTile];

	// already in memory?
	if (node->ptr != NULL)
	{
		waloff[nTile] = (BYTE *)node->ptr;
		if (node->lockCount == 0)
		{
			Resource::RemoveMRU(node);
			Resource::AddMRU(node);
			nLastTile = nTile;
		}
		else
			dprintf("loadtile() called for locked tile %i\n", nTile);

		return waloff[nTile];
	}

	int nSize = tilesizx[nTile] * tilesizy[nTile];

	if (nSize <= 0)
		return NULL;

	gCacheMiss = 30;

	dassert(node->lockCount == 0);
	node->ptr = Resource::Alloc(nSize);
	waloff[nTile] = (BYTE *)node->ptr;
	Resource::AddMRU(node);

	// load the tile from the art file
	hFile = hTileFile[tilefilenum[nTile]];
	lseek(hFile, tilefileoffs[nTile], SEEK_SET);
	read(hFile, node->ptr, nSize);

	return waloff[nTile];
}


BYTE *tileLockTile( int nTile )
{
	dassert(nTile >= 0 && nTile < kMaxTiles);

	if ( tileLoadTile(nTile) == NULL )
		return NULL;

	CACHENODE *node = &tileNode[nTile];

	if (node->lockCount++ == 0)
		Resource::RemoveMRU(node);

	return waloff[nTile];
}


void tileUnlockTile( int nTile )
{
	dassert(nTile >= 0 && nTile < kMaxTiles);

	CACHENODE *node = &tileNode[nTile];

	waloff[nTile] = NULL;

	if (node->lockCount > 0)
	{
		if ( --node->lockCount == 0 )
			Resource::AddMRU(node);
	}
}


BYTE *tileAllocTile( int nTile, int sizeX, int sizeY )
{
	int nSize;
	void *p;

	dassert(nTile >= 0 && nTile < kMaxTiles);

	if ( sizeX <= 0 || sizeY <= 0 || nTile >= kMaxTiles )
		return NULL;

	nSize = sizeX * sizeY;

	p = Resource::Alloc( nSize );
	dassert( p != NULL );
	tileNode[nTile].lockCount++;
	tileNode[nTile].ptr = p;

	waloff[nTile]   = (BYTE *)p;
	tilesizx[nTile] = (short)sizeX;
	tilesizy[nTile] = (short)sizeY;

	//probably should turn this into a union, or just memset 0 over the struct
 	picanm[nTile].frames  = 0;
	picanm[nTile].type    = 0;
	picanm[nTile].xcenter = 0;
	picanm[nTile].ycenter = 0;
	picanm[nTile].speed   = 0;

	// setup the tile size log 2 array for internal engine use
	CalcPicsiz(nTile, sizeX, sizeY);

	return waloff[nTile];
}


void tileFreeTile( int nTile )
{
	dassert(nTile >= 0 && nTile < kMaxTiles);

	tilePurgeTile(nTile);
	waloff[nTile] = NULL;
	tilesizx[nTile] = 0;
	tilesizy[nTile] = 0;
	picsiz[nTile] = 0;

 	picanm[nTile].frames  = 0;
	picanm[nTile].type    = 0;
	picanm[nTile].xcenter = 0;
	picanm[nTile].ycenter = 0;
	picanm[nTile].speed   = 0;
}


/*******************************************************************************
	FUNCTION:		tilePreloadTile()

	DESCRIPTION:

	PARAMETERS:

	RETURNS:

	NOTES:
*******************************************************************************/
void tilePreloadTile( int nTile )
{
	int i;

	switch (picanm[nTile].view)
	{
		case kSpriteViewSingle:
			if ( picanm[nTile].type != 0 )
			{
				for (i = picanm[nTile].frames; i >= 0; i--)
				{
					if (picanm[nTile].type == 3)	// special case for backward anims
						tileLoadTile(nTile - i);
					else
						tileLoadTile(nTile + i);
				}
			}
			else
				tileLoadTile(nTile);
			break;

		case kSpriteView5Full:
			for (i = 0; i < 5; i++)
				tileLoadTile(nTile + i);
	}
}


int CompareTileFreqs( const void *ref1, const void *ref2 )
{
	return tileHist[*(short *)ref2] - tileHist[*(short *)ref1];
}


/*******************************************************************************
	FUNCTION:		BuildTileHistogram()

	DESCRIPTION:	Fills in the tileHist[] array with the usage count for each
					tile.

	PARAMETERS:		type is one of the searchstat types define in engine.h

	RETURNS:		The index of the most frequently used tile

	NOTES:
*******************************************************************************/
short tileBuildHistogram( int type )
{
	int i;

	memset(tileHist, 0, sizeof(tileHist[0]) * kMaxTiles);

	switch ( type )
	{
		case SS_WALL:
			for (i = 0; i < numwalls; i++)
				tileHist[wall[i].picnum]++;
			break;

		case SS_CEILING:
			for (i = 0; i < numsectors; i++)
				tileHist[sector[i].ceilingpicnum]++;
			break;

		case SS_FLOOR:
			for (i = 0; i < numsectors; i++)
				tileHist[sector[i].floorpicnum]++;
			break;

		case SS_MASKED:
			for (i = 0; i < numwalls; i++)
				tileHist[wall[i].overpicnum]++;
			break;

		case SS_SPRITE:
			for (i = 0; i < kMaxSprites; i++)
			{
				if ( sprite[i].statnum == kMaxStatus )
					continue;

				if ( sprite[i].statnum == kStatMarker)
					continue;

				if ( sprite[i].cstat & kSpriteInvisible )
					continue;

				// only add face sprites
				if ( (sprite[i].cstat & kSpriteRMask) == kSpriteFace )
					tileHist[sprite[i].picnum]++;
			}
			break;

		case SS_FLATSPRITE:
			for (i = 0; i < kMaxSprites; i++)
			{
				if ( sprite[i].statnum == kMaxStatus )
					continue;

				if ( sprite[i].statnum == kStatMarker)
					continue;

				if ( sprite[i].cstat & kSpriteInvisible )
					continue;

				// only add flat sprites
				if ( (sprite[i].cstat & kSpriteRMask) != kSpriteFace )
					tileHist[sprite[i].picnum]++;
			}
			break;
	}

	for (i = 0; i < kMaxTiles; i++)
		tileIndex[i] = (short)i;

	qsort(tileIndex, kMaxTiles, sizeof(tileIndex[0]), &CompareTileFreqs);

	// find out how many used tiles are in the histogram
	tileIndexCount = 0;
	while (tileHist[tileIndex[tileIndexCount]] > 0 && tileIndexCount < kMaxTiles)
		tileIndexCount++;

	return tileIndex[0];
}



void tileDrawTileScreen( long nStart, long nCursor, int size, int nMax )
{
	int n, i, j, k, nTile, uSize, vSize;
	long duv;
	int x, y;
	long u;
	uchar *pTile, *pScreen;
	int height, width;
	int nCols = xdim / size;
	int nRows = ydim / size;
	char buffer[8];

	setupvlineasm(16);
	Video.SetColor(gStdColor[0]);

	clearview(0);

	n = 0;

	y = 0;
	for ( i = 0; i < nRows; i++, y += size )
	{
		x = 0;
		for ( j = 0; j < nCols; j++, n++, x += size )
		{
			if (nStart + n >= nMax)
				break;

			nTile = tileIndex[nStart + n];
			if ( tilesizx[nTile] > 0 && tilesizy[nTile] > 0 )
			{
				palookupoffse[0] = palookupoffse[1] = palookupoffse[2] = palookupoffse[3] = palookup[kPLUNormal];

				pTile = tileLoadTile(nTile);
				uSize = tilesizx[nTile];
				vSize = tilesizy[nTile];

				pScreen = frameplace + ylookup[y] + x;

				// draw actual size
				if ( uSize <= size && vSize <= size )
				{
					vince[0] = vince[1] = vince[2] = vince[3] = 0x00010000;
					bufplce[3] = pTile - vSize;
					for (u = 0; u + 3 < uSize; u += 4)
					{
						bufplce[0] = bufplce[3] + vSize;
						bufplce[1] = bufplce[0] + vSize;
						bufplce[2] = bufplce[1] + vSize;
						bufplce[3] = bufplce[2] + vSize;
						vplce[0] = vplce[1] = vplce[2] = vplce[3] = 0;
						vlineasm4(vSize, pScreen);
						pTile += 4 * vSize;
						pScreen += 4;
					}
					for (; u < uSize; u++)
					{
						vlineasm1(0x00010000, palookupoffse[0], vSize - 1, 0, pTile, pScreen);
						pTile += vSize;
						pScreen++;
					}
				}
				else
				{
					if (uSize > vSize)	// wider
					{
						duv = (uSize << 16) / size;
						height = size * vSize / uSize;
						width = size;
					}
					else				// taller
					{
						duv = (vSize << 16) / size;
						height = size;
						width = size * uSize / vSize;
					}

					// don't draw any that are too small to see
					if (height == 0)
						continue;

					vince[0] = vince[1] = vince[2] = vince[3] = duv;
					u = 0;
					for (k = 0; k + 3 < width; k += 4)
					{
						bufplce[0] = waloff[nTile] + vSize * (u >> 16);
						u += duv;
						bufplce[1] = waloff[nTile] + vSize * (u >> 16);
						u += duv;
						bufplce[2] = waloff[nTile] + vSize * (u >> 16);
						u += duv;
						bufplce[3] = waloff[nTile] + vSize * (u >> 16);
						u += duv;
						vplce[0] = vplce[1] = vplce[2] = vplce[3] = 0;
						vlineasm4(height, pScreen);
						pTile += 4 * vSize;
						pScreen += 4;
					}
					for (; k < width; k++)
					{
						pTile = waloff[nTile] + vSize * (u >> 16);
						vlineasm1(duv, palookupoffse[0], height - 1, 0, pTile, pScreen);
						pScreen++;
						u += duv;
					}
				}

				// overlay the usage count
				if ( nMax < kMaxTiles && !keystatus[KEY_CAPSLOCK] )
				{
					sprintf(buffer, "%d", tileHist[nTile]);
					Video.FillBox(0, x + size - strlen(buffer) * 4 - 2, y, x + size, y + 7);
					printext256(x + size - strlen(buffer) * 4 - 1, y, gStdColor[11], -1, buffer, 1);
				}
			}

			// overlay the tile number
			if ( !keystatus[KEY_CAPSLOCK] )
			{
				sprintf(buffer, "%d", nTile);
				Video.FillBox(0, x, y, x + strlen(buffer) * 4 + 1, y + 7);
				printext256(x + 1, y, gStdColor[14], -1, buffer, 1);
			}
		}
	}

	if ( IsBlinkOn() )
	{
		k = nCursor - nStart;    //draw open white box
		x = (k % nCols) * size;
		y = (k / nCols) * size;

		Video.SetColor(gStdColor[15]);
		Video.HLine(0, y, x, x + size - 1);
		Video.HLine(0, y + size - 1, x, x + size - 1);
		Video.VLine(0, x, y, y + size - 1);
		Video.VLine(0, x + size - 1, y, y + size - 1);
	}
}




/*******************************************************************************
	FUNCTION:		tilePick()

	DESCRIPTION:	Allows visual selection of a tile

	PARAMETERS:		nTile will be the initial highlighted selection

	RETURNS:

	NOTES:
*******************************************************************************/
short tilePick( int nTile, int nDefault, int type )
{
	static int nZoom = 3;
	int i, nStart, nCursor;
	BYTE key;
	int size = pickSize[nZoom];
	int nCols = xdim / size;
	int nRows = ydim / size;

	if ( type != SS_CUSTOM )
 		tileBuildHistogram(type);

	if ( tileIndexCount == 0 )
	{
		tileIndexCount = kMaxTiles;
		for(i = 0; i < kMaxTiles; i++)
			tileIndex[i] = (short)i;
	}

	nCursor = 0;
	for (i = 0; i < tileIndexCount; i++)
	{
		if (tileIndex[i] == nTile)
		{
			nCursor = i;
			break;
		}
	}

	nStart = ClipLow((nCursor - nRows * nCols + nCols) / nCols * nCols, 0);

	while (1)
	{
		tileDrawTileScreen(nStart, nCursor, pickSize[nZoom], tileIndexCount );

		if (vidoption != 1)
			WaitVSync();

		scrNextPage();

		gFrameTicks = gGameClock - gFrameClock;
		gFrameClock += gFrameTicks;
		UpdateBlinkClock(gFrameTicks);

		key = keyGet();

		switch (key)
		{
			case KEY_PADSTAR:
				if (nZoom > 0)
				{
					nZoom--;
					size = pickSize[nZoom];
					nCols = xdim / size;
					nRows = ydim / size;
					nStart = ClipLow((nCursor - nRows * nCols + nCols) / nCols * nCols, 0);
				}
				break;

			case KEY_PADSLASH:
				if (nZoom + 1 < LENGTH(pickSize))
				{
					nZoom++;
					size = pickSize[nZoom];
					nCols = xdim / size;
					nRows = ydim / size;
					nStart = ClipLow((nCursor - nRows * nCols + nCols) / nCols * nCols, 0);
				}
				break;

			case KEY_LEFT:
				if (nCursor - 1 >= 0)
					nCursor--;
				SetBlinkOn();
				break;

			case KEY_RIGHT:
				if (nCursor + 1 < tileIndexCount)
					nCursor++;
				SetBlinkOn();
				break;

			case KEY_UP:
				if (nCursor - nCols >= 0)
					nCursor -= nCols;
				SetBlinkOn();
				break;

			case KEY_DOWN:
				if (nCursor + nCols < tileIndexCount)
					nCursor += nCols;
				SetBlinkOn();
				break;

			case KEY_PAGEUP:
				if (nCursor - nRows * nCols >= 0)
				{
					nCursor -= nRows * nCols;
					nStart -= nRows * nCols;
					if (nStart < 0) nStart = 0;
				}
				SetBlinkOn();
				break;

			case KEY_PAGEDN:
				if (nCursor + nRows * nCols < tileIndexCount)
				{
					nCursor += nRows * nCols;
					nStart += nRows * nCols;
				}
				SetBlinkOn();
				break;

			case KEY_HOME:
				nCursor = 0;
				SetBlinkOn();
				break;

			case KEY_V:
				if ( type != SS_CUSTOM && tileIndexCount < kMaxTiles )
				{
					nCursor = tileIndex[nCursor];
					tileIndexCount = kMaxTiles;
					for(i = 0; i < kMaxTiles; i++)
						tileIndex[i] = (short)i;
				}
				break;

			case KEY_G:
				nTile = (short)GetNumberBox("Goto tile", 0, tileIndex[nCursor]);

				if (nTile >= kMaxTiles)
					break;

				if (nTile != tileIndex[nCursor])
				{
					// switch to all tile mode if not already there
					nCursor = nTile;
					tileIndexCount = kMaxTiles;
					for(i = 0; i < kMaxTiles; i++)
						tileIndex[i] = (short)i;
				}
				break;

			case KEY_ESC:
				clearview(0);	// clear this page so it isn't 'leftover'
				keystatus[KEY_ESC] = 0;
				return (short)nDefault;

			case KEY_ENTER:
				clearview(0);	// clear this page so it isn't 'leftover'

				nTile = tileIndex[nCursor];

				if (tilesizx[nTile] == 0 || tilesizy[nTile] == 0)
					return (short)nDefault;
				else
					return (short)nTile;
		}


		while (nCursor < nStart)
			nStart -= nCols;

		while (nStart + nRows * nCols <= nCursor)
			nStart += nCols;

		// let's see if this fixes the input buffering problem
		if (key != 0)
			keyFlushStream();
	}
}


/*******************************************************************************
	FUNCTION:		tileGetSurfType()

	DESCRIPTION:	Helper function to get the surface type for a game object.

	PARAMETERS:		nHit is the type and index of the hit object.

	RETURNS:		The surface type of the hit object.

	NOTES:
*******************************************************************************/
BYTE tileGetSurfType( int nHit )
{
	int nHitType = nHit & kHitTypeMask;
	int nHitIndex = nHit & kHitIndexMask;
	switch ( nHitType )
	{
		case kHitFloor:
			return surfType[sector[nHitIndex].floorpicnum];

		case kHitCeiling:
			return surfType[sector[nHitIndex].ceilingpicnum];

		case kHitWall:
			return surfType[wall[nHitIndex].picnum];

		case kHitSprite:
			return surfType[sprite[nHitIndex].picnum];
	}
	return 0;
}
