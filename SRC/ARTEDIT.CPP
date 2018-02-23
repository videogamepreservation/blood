/*******************************************************************************
	FILE:			ARTEDIT.CPP

	DESCRIPTION:	Allows rearrangement of art tiles

	AUTHOR:			Peter M. Freese
	CREATED:		06-19-95
	COPYRIGHT:		Copyright (c) 1995 Q Studios Corporation
*******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <helix.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <ctype.h>

#include "typedefs.h"
#include "engine.h"
#include "key.h"
#include "misc.h"
#include "gameutil.h"
#include "gfx.h"
#include "bstub.h"
#include "globals.h"
#include "debug4g.h"
#include "error.h"
#include "gui.h"
#include "tile.h"
#include "screen.h"
#include "textio.h"
#include "inifile.h"
#include "options.h"
#include "timer.h"
#include "getopt.h"
#include "mouse.h"
#include "db.h"
#include "trig.h"

#include <memcheck.h>

#define kAttrTitle      (kColorGreen * 16 + kColorYellow)

#define kMaxUndo		100

struct UNDO
{
	int start, length, shift, cursor;
};

struct NAMED_TYPE
{
	short id;
	char *name;
};

int pickSize[] = { 20, 32, 40, 64, 80, 128, 160 };
BOOL tilesModified = FALSE;
BOOL tilesMoved = FALSE;
POINT origin;
int nOctant = 0;
char buffer[256];
EHF prevErrorHandler;
UNDO undo[kMaxUndo];
int undoCount = 0, undoCountMax = 0;
int nCursor;
BOOL bForceVersion = FALSE;
int nVersion;
int nSelect1 = -1, nSelect2 = -1;
int nSelectBegin = 0, nSelectEnd = -1;
PICANM oldpicanm[kMaxTiles];
static BYTE *inverseCLU = NULL;

#define kMaxNameLength	17	// this is ken's lame-o size that can't be changed...
static char oldNames[ kMaxTiles ][ kMaxNameLength ];
static char tileNames[ kMaxTiles ][ kMaxNameLength ];
BOOL namesModified = FALSE;


char *viewNames[] =
{
	"SINGLE",
	"5 FULL",
	"8 FULL",
	"5 HALF",
	"3 FLAT",
	"4 FLAT",
};

char *animNames[] =
{
	"NoAnm",
	"Oscil",
	"AnmFw",
	"AnmBk",
};


char *surfNames[256];

NAMED_TYPE surfNTNames[] =
{
	kSurfNone,		"NONE",
	kSurfStone,		"STONE",
	kSurfMetal,		"METAL",
	kSurfWood,		"WOOD",
	kSurfFlesh,		"FLESH",
	kSurfWater,		"WATER",
	kSurfDirt,		"DIRT",
	kSurfClay,		"CLAY",
	kSurfSnow,		"SNOW",
	kSurfIce,		"ICE",
	kSurfLeaves,	"LEAVES",
	kSurfCloth,		"CLOTH",
	kSurfPlant,		"PLANT",
	kSurfGoo,		"GOO",
	kSurfLava,		"LAVA",
};


/***********************************************************************
 * LoadTileNames
 *
 **********************************************************************/
int LoadTileNames( void )
{
	char buffer[160], ch;
	int fil, i, num, buffercnt;

	if ((fil = open("names.h", O_TEXT | O_RDWR, S_IREAD)) == -1)
		return(-1);

	i = read(fil,&ch,1);
	while ( ch != '#' && i > 0)
	{
		i = read(fil,&ch,1);
	}

	while ( ch == '#' )
	{
		// find first space
		while ( !isspace(ch) )
			read(fil,&ch,1);

		// find non-space
		while ( isspace(ch) )
			read(fil,&ch,1);

		// get name
		buffercnt = 0;
		while ( !isspace(ch) )
		{
			buffer[buffercnt++] = ch;
			read(fil,&ch,1);
		}

		// find non-space
		while ( isspace(ch) )
			read(fil,&ch,1);

		// get number
		num = 0;
		while ( isdigit(ch) )
		{
			num = num*10+(ch-48);
			read(fil,&ch,1);
		}

		// find end of line
		while ( ch != '\n' )
			read(fil,&ch,1);

		memcpy(tileNames[num], buffer, buffercnt);
		tileNames[num][buffercnt] = 0;

		read(fil,&ch,1);
	}

	close(fil);
	memcpy(oldNames, tileNames, sizeof(oldNames));
	namesModified = FALSE;
	return(0);
}


/***********************************************************************
 * SaveTileNames
 *
 **********************************************************************/
void SaveTileNames( void )
{
	char buffer[160];
	int hFile;
	int len;

	if (namesModified == FALSE)
		return;

	if ((hFile = open("names.h",O_TEXT | O_TRUNC | O_CREAT | O_WRONLY, S_IWRITE)) == -1)
		return;

	strcpy(buffer,"//Be careful when changing this file - it is parsed by Editart and Build.\n");
	write(hFile, buffer, strlen(buffer));

	for (int i = 0; i < kMaxTiles; i++)
	{
		if (tileNames[i][0] != 0)
		{
			len = sprintf(buffer, "#define %s %d\n", tileNames[i], i);
			write(hFile, buffer, len);
		}
	}

	close(hFile);
	namesModified = FALSE;
	return;
}


/***********************************************************************
 * FillNameArray
 *
 **********************************************************************/
void FillNameArray( char **names, NAMED_TYPE *ntNames, int length )
{
	for (int i = 0; i < length; i++)
	{
		names[ntNames->id] = ntNames->name;
		ntNames++;
	}
}


void faketimerhandler( void )
{ }


/***********************************************************************
 * EditorErrorHandler()
 *
 * Terminate from error condition, displaying a message in text mode.
 *
 **********************************************************************/
ErrorResult EditorErrorHandler( const Error& error )
{
	uninitengine();
	keyRemove();
	setvmode(gOldDisplayMode);

	// chain to the default error handler
	return prevErrorHandler(error);
};


void SetOrigin( int x, int y )
{
	origin.x = ClipRange(x, 0, xdim - 1);
	origin.y = ClipRange(y, 0, ydim - 1);
}


void DrawOrigin( int nColor )
{
	Video.SetColor(nColor);
	gfxHLine(origin.y, origin.x - 6, origin.x - 1);
	gfxHLine(origin.y, origin.x + 1, origin.x + 6);
	gfxVLine(origin.x, origin.y - 5, origin.y - 1);
	gfxVLine(origin.x, origin.y + 1, origin.y + 5);
}


void AddUndo( int start, int length, int shift, int cursor)
{
	if (undoCount == kMaxUndo)
	{
		undoCount--;
		memmove(&undo[0], &undo[1], undoCount * sizeof(UNDO));
	}
	undo[undoCount].start = start;
	undo[undoCount].length = length;
	undo[undoCount].shift = shift;
	undo[undoCount].cursor = cursor;
	undoCountMax = ++undoCount;
}


void Undo( void )
{
	if (undoCount > 0)
	{
		undoCount--;
		tileRotateTiles(undo[undoCount].start, undo[undoCount].length, -undo[undoCount].shift);
		scrSetMessage("Move undone");
		nCursor = undo[undoCount].cursor;
	}
	else
		scrSetMessage("Nothing to undo.");
}


void Redo( void )
{
	if (undoCount < undoCountMax)
	{
		tileRotateTiles(undo[undoCount].start, undo[undoCount].length, undo[undoCount].shift);
		scrSetMessage("Move redone");
		nCursor = undo[undoCount].cursor;
		undoCount++;
	}
	else
		scrSetMessage("Nothing to redo.");
}


void MoveTiles( int dest, int source, int count )
{
	int start, length, shift;
	if (source < dest)
	{
		start = source;
		length = dest - source;
		shift = count;
	}
	else
	{
		start = dest;
		length = source - dest + count;
		shift = source - dest;
	}
	tileRotateTiles(start, length, shift);
	AddUndo(start, length, shift, nCursor);
	scrSetMessage("Tiles moved");
	tilesMoved = tilesModified = TRUE;
}


BOOL AcknowledgeTileChange(void)
{
	// create the dialog
	Window dialog(0, 0, 202, 46, "Save Art Changes?");

	TextButton *pbYes    = new TextButton( 4, 4, 60,  20, "&Yes", mrYes );
	TextButton *pbNo     = new TextButton( 68, 4, 60, 20, "&No", mrNo );
	TextButton *pbCancel = new TextButton( 132, 4, 60, 20, "&Cancel", mrCancel );
	dialog.Insert(pbYes);
	dialog.Insert(pbNo);
 	dialog.Insert(pbCancel);

	ShowModal(&dialog);
	switch ( dialog.endState )
	{
		case mrCancel:
			return FALSE;

		case mrOk:
		case mrYes:
			if ( tilesMoved )
				tileSaveArt();
			else
				tileSaveArtInfo();
			if ( namesModified )
				SaveTileNames();
			break;
	}

	return TRUE;
}


MODAL_RESULT KeepTileChanges(void)
{
	// create the dialog
	Window dialog(0, 0, 202, 46, "Keep tile changes?");

	TextButton *pbYes    = new TextButton( 4, 4, 60,  20, "&Yes", mrYes );
	TextButton *pbNo     = new TextButton( 68, 4, 60, 20, "&No", mrNo );
	TextButton *pbCancel = new TextButton( 132, 4, 60, 20, "&Cancel", mrCancel );
	dialog.Insert(pbYes);
	dialog.Insert(pbNo);
 	dialog.Insert(pbCancel);

	ShowModal(&dialog);
	return dialog.endState;
}


BOOL EditSelectedTiles(void)
{
	// create the dialog
	Window dialog(0, 0, 202, 46, "Edit all selected?");

	TextButton *pbYes    = new TextButton( 4, 4, 60,  20, "&Yes", mrYes );
	TextButton *pbNo     = new TextButton( 68, 4, 60, 20, "&No", mrNo );
	TextButton *pbCancel = new TextButton( 132, 4, 60, 20, "&Cancel", mrCancel );
	dialog.Insert(pbYes);
	dialog.Insert(pbNo);
 	dialog.Insert(pbCancel);

	ShowModal(&dialog);
	return dialog.endState;
}


int DrawTile( int nTile )
{
	char nShade = 0;
	char nFlags = kRotateNormal;
	short nAngle = 0;
	switch ( picanm[nTile].view )
	{
		case kSpriteViewSingle:
			break;

		case kSpriteView5Full:
			if (nOctant <= 4)
				nTile += nOctant;
			else
			{
				nTile += 8 - nOctant;
				nAngle = kAngle180;
				nFlags |= kRotateYFlip;
			}

			break;

		case kSpriteView8Full:
			nTile += nOctant;
			break;

		case kSpriteView5Half:
			break;
	}
	rotatesprite(origin.x << 16, origin.y << 16, 0x10000, 0, (short)nTile, nShade, kPLUNormal,
		nFlags, 0, 0, xdim - 1, ydim - 1);
	return nTile;
}


int EditTileLoop( int nTile )
{
	BYTE key, shift, ctrl;
	int nbuttons;
	static int obuttons = 0;
	char buffer[256];
	int nTileView = 0;
	BOOL playing = FALSE;
	BOOL localModified = FALSE;
	int xcenter, ycenter;
	BOOL localNamesModified = FALSE;

	memcpy(oldpicanm, picanm, sizeof(oldpicanm));
	memcpy(oldNames, tileNames, sizeof(oldNames));

	while (1)
	{
		gFrameTicks = gGameClock - gFrameClock;
		gFrameClock += gFrameTicks;
		UpdateBlinkClock(gFrameTicks);

		clearview(0);

		// turn off animation playing if the tile doesn't support it
		if (picanm[nTile].type == 0)
			playing = FALSE;

		if (playing)
		{
			rotatesprite(origin.x << 16, origin.y << 16, 0x10000, 0,
				(short)(nTile + animateoffs((short)nTile, 0)), 0, kPLUNormal,
				kRotateNormal, 0, 0, xdim - 1, ydim - 1);
			nTileView = nTile;
		}
		else
			nTileView = DrawTile(nTile);

		if (tileNames[nTile][0] != '\0')
		{
			sprintf(buffer, "Name: %s", tileNames[nTile]);
			printext256(0, 0, gStdColor[7], -1, buffer, 0);
		}

		sprintf(buffer, "Tile: %4i  Surf: %s", nTile, surfNames[surfType[nTile]]);
		printext256(0, ydim - 20, gStdColor[7], -1, buffer, 0);

		sprintf(buffer, "%s:%2i [%2i]  VIEW: %s", animNames[picanm[nTile].type],
			picanm[nTile].frames, picanm[nTile].speed, viewNames[picanm[nTile].view]);
		printext256(0, ydim - 10, gStdColor[15], -1, buffer, 0);

		DrawOrigin(gStdColor[keystatus[KEY_O] ? 15 : 8]);

		sprintf(buffer,"ANG: %3i", nOctant * 45);
		printext256(xdim - 80, ydim - 20, gStdColor[7], -1, buffer, 0);

		scrDisplayMessage(gStdColor[kColorWhite]);

		WaitVSync();
		scrNextPage();

		xcenter = picanm[nTileView].xcenter;
		ycenter = picanm[nTileView].ycenter;

		key = keyGet();
		shift = keystatus[KEY_LSHIFT] | keystatus[KEY_RSHIFT];
		ctrl = keystatus[KEY_LCTRL] | keystatus[KEY_RCTRL];

		switch (key)
		{
			case KEY_ESC:
				if ( !localModified && !localNamesModified )
					return nTile;

				switch ( KeepTileChanges() )
				{
					case mrOk:
					case mrYes:
						if (localModified)
							tilesModified = TRUE;
						if (localNamesModified)
							namesModified = TRUE;
						return nTile;

					case mrNo:
						if (localModified)
							memcpy(picanm, oldpicanm, sizeof(oldpicanm));
						if (localNamesModified)
							memcpy(tileNames, oldNames, sizeof(tileNames));
						return nTile;
				}
				break;

			case KEY_SPACE:
				playing = !playing;
				break;

			case KEY_COMMA:		// "<"
				nOctant = DecRotate(nOctant, 8);
				break;

			case KEY_PERIOD:	// ">"
				nOctant = IncRotate(nOctant, 8);
				break;

			case KEY_SLASH:
				nOctant = 0;
				break;

			case KEY_A:
				picanm[nTile].type++;
				tileMarkDirty(nTileView);
				localModified = TRUE;
				break;

			case KEY_G:
				nTile = GetNumberBox("Goto tile", 0, nTile);
				break;

			case KEY_N:
			{
				char buffer[80];
				strcpy(buffer, tileNames[nTile]);
				if (GetStringBox( " Tile Name (max. 16 letters) ", buffer) != 0)	// check for cancelled dialog box
				{
					strncpy(tileNames[nTile], buffer, 16);
					tileNames[nTile][16] = '\0';
					localNamesModified = TRUE;
				}
				break;
			}

			case KEY_P:
			{
				int c = 0;
				for ( int i = 0; i < 16; i++)
				{
					for (int j = 0; j < 16; j++)
					{
						Video.SetColor(c++);
						gfxFillBox(i * 8, j * 8, i * 8 + 8, j * 8 + 8);
					}
				}
				scrNextPage();
				while ( keyGet() == 0 );
				break;
			}

			case KEY_W:
				picanm[nTile].view = IncRotate(picanm[nTile].view, LENGTH(viewNames));
				tileMarkDirty(nTile);
				localModified = TRUE;
				break;

			case KEY_PADPLUS:
				if (shift)
				{
					if (picanm[nTile].speed > 0)
					{
						picanm[nTile].speed--;
						tileMarkDirty(nTileView);
						localModified = TRUE;
					}
				}
				else
				{
					if (picanm[nTile].frames < 63)
					{
						picanm[nTile].frames++;
						if (picanm[nTile].type == 0)
							picanm[nTile].type = 2;	// forward
						tileMarkDirty(nTileView);
						localModified = TRUE;
					}
				}
				break;

			case KEY_PADMINUS:
				if (shift)
				{
					if (picanm[nTile].speed < 15)
					{
						picanm[nTile].speed++;
						tileMarkDirty(nTileView);
						localModified = TRUE;
					}
				}
				else
				{
					if (picanm[nTile].frames > 0)
					{
						picanm[nTile].frames--;
						if (picanm[nTile].frames == 0)
							picanm[nTile].type = 0;
						tileMarkDirty(nTileView);
						localModified = TRUE;
					}
				}
				break;

			case KEY_PAD0:
				break;

			case KEY_PAGEUP:
				while (nTile > 0)
				{
					nTile--;
					if (tilesizx[nTile] > 0 && tilesizy[nTile] > 0)
						break;
				}
				break;

			case KEY_PAGEDN:
				while (nTile < kMaxTiles - 1)
				{
					nTile++;
					if (tilesizx[nTile] > 0 && tilesizy[nTile] > 0)
						break;
				}
				break;

			case KEY_UP:
			case KEY_PADUP:
				if ( keystatus[KEY_O] )
				{
					SetOrigin(origin.x, origin.y - 1);
				}
				else if (ctrl)
				{
					picanm[nTileView].ycenter = -tilesizy[nTileView] / 2;
					tileMarkDirty(nTileView);
					localModified = TRUE;
				}
				else
				{
					picanm[nTileView].ycenter++;
					tileMarkDirty(nTileView);
					localModified = TRUE;
				}
				break;

			case KEY_DOWN:
			case KEY_PADDOWN:
				if ( keystatus[KEY_O] )
				{
					SetOrigin(origin.x, origin.y + 1);
				}
				else if (ctrl)
				{
					picanm[nTileView].ycenter = tilesizy[nTileView] - tilesizy[nTileView] / 2;
					tileMarkDirty(nTileView);
					localModified = TRUE;
				}
				else
				{
					picanm[nTileView].ycenter--;
					tileMarkDirty(nTileView);
					localModified = TRUE;
				}
				break;

			case KEY_PAD5:
				if (ctrl)
				{
					picanm[nTileView].xcenter = 0;
					picanm[nTileView].ycenter = 0;
					tileMarkDirty(nTileView);
					localModified = TRUE;
				}
				break;

			case KEY_LEFT:
			case KEY_PADLEFT:
				if ( keystatus[KEY_O] )
				{
					SetOrigin(origin.x - 1, origin.y);
				}
				else if (ctrl)
				{
					picanm[nTileView].xcenter = -tilesizx[nTileView] / 2;
					tileMarkDirty(nTileView);
					localModified = TRUE;
				}
				else
				{
					picanm[nTileView].xcenter++;
					tileMarkDirty(nTileView);
					localModified = TRUE;
				}
				break;

			case KEY_RIGHT:
			case KEY_PADRIGHT:
				if ( keystatus[KEY_O] )
				{
					SetOrigin(origin.x + 1, origin.y);
				}
				else if (ctrl)
				{
					picanm[nTileView].xcenter = tilesizx[nTileView] - tilesizx[nTileView] / 2;
					tileMarkDirty(nTileView);
					localModified = TRUE;
				}
				else
				{
					picanm[nTileView].xcenter--;
					tileMarkDirty(nTileView);
					localModified = TRUE;
				}
				break;
		}

		Mouse::Read(gFrameTicks);

		// which buttons just got pressed
		nbuttons = (short)(~obuttons & Mouse::buttons);
		obuttons = Mouse::buttons;

		if ( Mouse::buttons & 1 )
		{
			if ( keystatus[KEY_O] )
				SetOrigin(origin.x + Mouse::dX2, origin.y + Mouse::dY2);
		}

		if ( nTileView >= nSelectBegin && nTileView <= nSelectEnd )
		{
			for (int i = nSelectBegin; i <= nSelectEnd; i++)
			{
				if ( i != nTileView )
				{
					picanm[i].xcenter += picanm[nTileView].xcenter - xcenter;
					picanm[i].ycenter += picanm[nTileView].ycenter - ycenter;
					tileMarkDirty(i);
				}
			}
		}
	}
}


void DrawTileScreen( long nStart, long nCursor, int size, int nMax )
{
	int n, i, j, k, nTile, uSize, vSize;
	long duv;
	int x, y;
	long u;
	uchar *pTile, *pScreen;
	int height, width;
	int nCols = xdim / size;
	int nRows = ydim / size;
	char buffer[256];

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

				if ( nTile >= nSelectBegin && nTile <= nSelectEnd )
					palookupoffse[0] = palookupoffse[1] = palookupoffse[2] = palookupoffse[3] = inverseCLU;

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

				// overlay the surface type
				if ( !keystatus[KEY_CAPSLOCK] )
				{
					sprintf(buffer, "%s", surfNames[surfType[nTile]]);
					Video.FillBox(0, x, y + size - 7, x + strlen(buffer) * 4 + 1, y + size);
					printext256(x + 1, y + size - 7, gStdColor[surfType[nTile] == kSurfNone ? 4 : 11], -1, buffer, 1);
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



void MainEditLoop( void )
{
	static int nZoom = 3;
	int nStart;
	BYTE key, shift, alt;
	int size = pickSize[nZoom];
	int nCols = xdim / size;
	int nRows = ydim / size;
	BOOL selecting = FALSE;

	for (int i = 0; i < kMaxTiles; i++)
		tileIndex[i] = (short)i;
	nCursor = 0;

	nStart = ClipLow((nCursor - nRows * nCols + nCols) / nCols * nCols, 0);

	while (1)
	{
		DrawTileScreen(nStart, nCursor, pickSize[nZoom], kMaxTiles );
		scrDisplayMessage(gStdColor[15]);

		scrNextPage();

		if (vidoption != 1)
			WaitVSync();

		gFrameTicks = gGameClock - gFrameClock;
		gFrameClock += gFrameTicks;
		UpdateBlinkClock(gFrameTicks);

		key = keyGet();
		shift = keystatus[KEY_LSHIFT] | keystatus[KEY_RSHIFT];
		alt = keystatus[KEY_LALT] | keystatus[KEY_RALT];

		switch (key)
		{
			case KEY_LSHIFT:
			case KEY_RSHIFT:
				nSelect1 = nCursor;
				selecting = TRUE;
				break;

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

			case KEY_BACKSPACE:
				if (alt)
					Undo();
				break;

			case KEY_ENTER:
				if (alt)
					Redo();
				else
				{
					if ( nSelectEnd >= nSelectBegin )
					{
						switch ( EditSelectedTiles() )
						{
							case mrOk:
							case mrYes:
								nCursor = EditTileLoop(nCursor);
								break;

							case mrNo:
								nSelectEnd = nSelectBegin - 1;
								nCursor = EditTileLoop(nCursor);
								break;
						}
					}
					else
						nCursor = EditTileLoop(nCursor);
				}
				break;

			case KEY_S:
			{
				surfType[nCursor] = (BYTE)IncRotate(surfType[nCursor], kSurfMax);
				tilesModified = TRUE;

				if ( nCursor >= nSelectBegin && nCursor <= nSelectEnd )
				{
					for (int i = nSelectBegin; i <= nSelectEnd; i++)
					{
						surfType[i] = surfType[nCursor];
					}
				}
				break;
			}

			case KEY_LEFT:
				if (nCursor - 1 >= 0)
					nCursor--;
				SetBlinkOn();
				break;

			case KEY_RIGHT:
				if (nCursor + 1 < kMaxTiles)
					nCursor++;
				SetBlinkOn();
				break;

			case KEY_UP:
				if (nCursor - nCols >= 0)
				{
					nCursor -= nCols;
				}
				SetBlinkOn();
				break;

			case KEY_DOWN:
				if (nCursor + nCols < kMaxTiles)
				{
					nCursor += nCols;
				}
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
				if (nCursor + nRows * nCols < kMaxTiles)
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

			case KEY_END:
				nCursor = kMaxTiles - 1;
				SetBlinkOn();
				break;

			case KEY_G:
				nCursor = (short)GetNumberBox("Goto tile", 0, tileIndex[nCursor]);
				break;

			case KEY_ESC:
				keystatus[KEY_ESC] = 0;
				return;

			case KEY_INSERT:
				if ( nSelectBegin <= nSelectEnd )
				{
					MoveTiles(nCursor, nSelectBegin, nSelectEnd - nSelectBegin + 1);
					nSelect1 = nSelect2 = nSelectBegin = nSelectEnd = -1;
				}
				break;
		}

		if (selecting)
		{
			nSelect2 = nCursor;
			if (nSelect1 < nSelect2)
			{
				nSelectBegin = nSelect1;
				nSelectEnd = nSelect2 - 1;
			}
			else
			{
				nSelectBegin = nSelect2;
				nSelectEnd = nSelect1 - 1;
			}
		}

		if ( !shift )
			selecting = FALSE;

		while (nCursor < nStart)
			nStart -= nCols;

		while (nStart + nRows * nCols <= nCursor)
			nStart += nCols;

		if (key != 0)
			keyFlushStream();
	}
}


/***********************************************************************
 * ShowUsage
 *
 * Display command-line parameter usage, then exit.
 **********************************************************************/
void ShowUsage(void)
{
	printf("Syntax:  ARTEDIT [options]\n");
	printf("-vN           force art version to N\n");
	printf("-?            This help\n");
	exit(0);
}


void QuitMessage(char * fmt, ...)
{
	char msg[80];
	va_list argptr;
	va_start( argptr, fmt );
	vsprintf( msg, fmt, argptr );
	va_end(argptr);
	printf(msg);
	exit(1);
}


/***********************************************************************
 * Process command line arguments
 **********************************************************************/
void ParseOptions( void )
{
	enum { kSwitchHelp, kSwitchVersion };
	static SWITCH switches[] = {
		{ "?", kSwitchHelp, FALSE },
		{ "V", kSwitchVersion, TRUE },
		{ NULL, 0, FALSE },
	};
	int r;
	while ( (r = GetOptions(switches)) != GO_EOF )
	{
		switch (r) {
			case GO_INVALID:
			case GO_FULL:
				QuitMessage("Invalid argument: %s", OptArgument);
				break;

			case kSwitchVersion:
				nVersion = strtol(OptArgument, NULL, 0);
				bForceVersion = TRUE;
				break;

			case kSwitchHelp:
				ShowUsage();
				break;
		}
	}
}


void main( void )
{
	char title[256];

	FillNameArray(surfNames, surfNTNames, LENGTH(surfNTNames));

	ParseOptions();

	gOldDisplayMode = getvmode();

	sprintf(title, "Art Editor [%s] -- DO NOT DISTRIBUTE", gBuildDate);
	tioInit();
	tioCenterString(0, 0, tioScreenCols - 1, title, kAttrTitle);
	tioCenterString(tioScreenRows - 1, 0, tioScreenCols - 1,
		"Copyright (c) 1994, 1995 Q Studios Corporation", kAttrTitle);

	tioWindow(1, 0, tioScreenRows - 2, tioScreenCols);

	if ( _grow_handles(kRequiredFiles) < kRequiredFiles )
		ThrowError("Not enough file handles available", ES_ERROR);

	gGamma = BloodINI.GetKeyInt("View", "Gamma", 0);

	tioPrint("Initializing heap and resource system");
	Resource::heap = new QHeap(dpmiDetermineMaxRealAlloc());

	tioPrint("Initializing resource archive");
	gSysRes.Init("BLOOD.RFF", "*.*");
	gGuiRes.Init("GUI.RFF", NULL);

	tioPrint("Loading tiles");
	if (tileInit() == 0)
		ThrowError("ART files not found", ES_ERROR);

	if ( bForceVersion )
	{
		artversion = nVersion;
		tioPrint("Updating art version number");
		tileMarkDirtyAll();
		tileSaveArtInfo();
		exit(0);
	}

	LoadTileNames();	//load constants

	tioPrint("Initializing mouse");
	if ( !initmouse() )
		tioPrint("Mouse not detected");

	// install our error handler
	prevErrorHandler = errSetHandler(EditorErrorHandler);

	InitEngine();

	Mouse::SetRange(xdim, ydim);

	tioPrint("Initializing screen");
	scrInit();

	tioPrint("Installing keyboard handler");
	keyInstall();

	scrCreateStdColors();

 	RESHANDLE hInverse = gSysRes.Lookup("INVERSE", ".CLU");
	if ( !hInverse )
		ThrowError("Missing INVERSE.CLU resource", ES_ERROR);

	inverseCLU = (BYTE *)gSysRes.Lock(hInverse);

	tioPrint("Installing timer");
	timerRegisterClient(ClockStrobe, kTimerRate);
	timerInstall();

	tioPrint("Engaging graphics subsystem...");

	scrSetGameMode();

	scrSetGamma(gGamma);
	scrSetDac(0);

	SetOrigin(xdim / 2, ydim / 2);

	do {
		MainEditLoop();
	} while ((tilesModified || namesModified) && !AcknowledgeTileChange());

	setvmode(gOldDisplayMode);

	timerRemove();

	uninitengine();

	errSetHandler(prevErrorHandler);
}
