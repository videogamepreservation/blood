/*******************************************************************************
	FILE:			SEQEDIT.CPP

	DESCRIPTION:	Allows editing of sprite sequences

	AUTHOR:			Peter M. Freese
	CREATED:		06-15-95
	COPYRIGHT:		Copyright (c) 1995 Q Studios Corporation
*******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <helix.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>

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
#include "seq.h"
#include "trig.h"
#include "mouse.h"

#include <memcheck.h>

#define kMaxFrames	1024

#define kAttrTitle      (kColorLightGray * 16 + kColorYellow)

BOOL looping = TRUE;
char buffer[256];
static BOOL seqModified;
static BOOL tilesModified;
EHF prevErrorHandler;
POINT origin = { 160, 100 };
schar gShade = 20;
char gSeqName[128];
Seq *pSeq = NULL;

int nOctant = 0;

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
	origin.x = ClipRange(x, 0, 319);
	origin.y = ClipRange(y, 0, 199);
}


void DrawOrigin( int nColor )
{
	Video.SetColor(nColor);
	gfxHLine(origin.y, origin.x - 6, origin.x - 1);
	gfxHLine(origin.y, origin.x + 1, origin.x + 6);
	gfxVLine(origin.x, origin.y - 5, origin.y - 1);
	gfxVLine(origin.x, origin.y + 1, origin.y + 5);
}


BOOL SaveSeq( void )
{
	char filename[_MAX_PATH];
	int hFile;

	strcpy(filename, gSeqName);
	ChangeExtension(filename, ".SEQ");

	// see if file exists
	if ( access(filename, F_OK) == 0 )
	{
		// create the dialog
		Window dialog(59, 80, 202, 46, "Overwrite file?");

		dialog.Insert(new TextButton(4, 4, 60,  20, "&Yes", mrYes));
		dialog.Insert(new TextButton(68, 4, 60, 20, "&No", mrNo));
	 	dialog.Insert(new TextButton(132, 4, 60, 20, "&Cancel", mrCancel));

		ShowModal(&dialog);
		switch ( dialog.endState )
		{
			case mrCancel:
				return FALSE;

			case mrNo:
				return TRUE;
		}

		char bakname[_MAX_PATH];
		strcpy(bakname, filename);
		ChangeExtension(bakname, ".BAK");
		unlink(bakname);
		rename(filename, bakname);
	}

	hFile = open(filename, O_CREAT | O_WRONLY | O_BINARY | O_TRUNC, S_IWUSR);
	if ( hFile == -1 )
		ThrowError("Error creating SEQ file", ES_ERROR);

	memcpy(pSeq->signature, kSEQSig, sizeof(pSeq->signature));
	pSeq->version = kSEQVersion;

	if ( !FileWrite(hFile, pSeq, sizeof(Seq) + pSeq->nFrames * sizeof(SEQFRAME)) )
	{
		close(hFile);
		ThrowError("Error writing SEQ file", ES_ERROR);
	}

	close(hFile);
	seqModified = FALSE;

	scrSetMessage("Sequence saved.");
	return TRUE;
}


BOOL LoadSeq( void )
{
	char filename[128];
	int hFile;
	int nSize;

	strcpy(filename, gSeqName);
	ChangeExtension(filename, ".SEQ");

	dprintf("Attempting to load %s\n", filename);
	hFile = open(filename, O_RDONLY | O_BINARY);
	if ( hFile == -1 )
	{
		dprintf("Couldn't open file, errno=%d\n", errno);
		return FALSE;
	}

	nSize = filelength(hFile);

	if ( !FileRead(hFile, pSeq, nSize) )
	{
		close(hFile);
		ThrowError("Error reading SEQ file", ES_ERROR);
	}

	close(hFile);

	// check signature
	if (memcmp(pSeq->signature, kSEQSig, sizeof(pSeq->signature)) != 0)
		ThrowError("SEQ file corrupted", ES_ERROR);

	// check version
	if ( (pSeq->version & 0xFF00) != (kSEQVersion & 0xFF00) )
		ThrowError("Obsolete SEQ version", ES_ERROR);

	seqModified = FALSE;

	return TRUE;
}


BOOL SaveAs( void )
{
	if ( GetStringBox("Save As", gSeqName) )
	{
		// save it if it seems valid
		if (strlen(gSeqName) > 0)
			return SaveSeq();
	}
	return FALSE;
}


BOOL LoadAs( void )
{
	if ( GetStringBox("Load Anim", gSeqName) )
	{
		// load it if it seems valid
		if (strlen(gSeqName) > 0)
		{
			LoadSeq();
			return TRUE;
		}

	}
	return FALSE;
}


/*******************************************************************************
** ChangeAcknowledge()
**
** Returns TRUE if the YES/NO selected, or FALSE if aborted or escape pressed.
*******************************************************************************/
BOOL AcknowledgeSeqChange(void)
{
	// create the dialog
	Window dialog(59, 80, 202, 46, "Save Sequence?");

	dialog.Insert(new TextButton(4, 4, 60,  20, "&Yes", mrYes));
	dialog.Insert(new TextButton(68, 4, 60, 20, "&No", mrNo));
 	dialog.Insert(new TextButton(132, 4, 60, 20, "&Cancel", mrCancel));

	ShowModal(&dialog);
	switch ( dialog.endState )
	{
		case mrCancel:
			return FALSE;

		case mrOk:
		case mrYes:
			return SaveAs();
	}

	return TRUE;
}


BOOL AcknowledgeTileChange(void)
{
	// create the dialog
	Window dialog(59, 80, 202, 46, "Save Tile Changes?");

	dialog.Insert(new TextButton(4, 4, 60,  20, "&Yes", mrYes));
	dialog.Insert(new TextButton(68, 4, 60, 20, "&No", mrNo));
	dialog.Insert(new TextButton(132, 4, 60, 20, "&Cancel", mrCancel));

	ShowModal(&dialog);
	switch ( dialog.endState )
	{
		case mrCancel:
			return FALSE;

		case mrOk:
		case mrYes:
			tileSaveArtInfo();
	}

	return TRUE;
}


int DrawTile( SEQFRAME *pFrame )
{
	schar nShade = (schar)ClipRange(gShade + pFrame->shade, -128, 127);
	char flags = 0;	// default position by origin
	short nTile = (short)pFrame->nTile;
	uchar nPal = (uchar)pFrame->pal;
	long zoom = pFrame->xrepeat << 10;
	short ang = 0;

	if ( keystatus[KEY_CAPSLOCK] )
		flags |= kRotateNoMask;

	if (pFrame->translucent)
		flags |= kRotateTranslucent;

	if (pFrame->translucentR)
		flags |= kRotateTranslucentR;

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
				// rotate 180 degrees + y-flip is equivalent to x-flip
				ang += kAngle180;
				flags ^= kRotateYFlip;
			}

			break;

		case kSpriteView8Full:
			nTile += nOctant;
			break;

		case kSpriteView5Half:
			break;
	}

	origin.x = ClipLow(origin.x, tilesizx[nTile] / 2 + picanm[nTile].xcenter);
	origin.x = ClipHigh(origin.x, 319 - tilesizx[nTile] / 2 + picanm[nTile].xcenter);
	origin.y = ClipLow(origin.y, tilesizy[nTile] / 2 + picanm[nTile].ycenter);
	origin.y = ClipHigh(origin.y, 180 - tilesizy[nTile] / 2 + picanm[nTile].ycenter);

	rotatesprite(origin.x << 16, origin.y << 16, 0x10000, ang, nTile, nShade,
		nPal, flags, windowx1, windowy1, windowx2, windowy2);

	return nTile;
}


void EditLoop( void )
{
	BYTE key, shift, alt, ctrl, pad5;
	int nbuttons;
	static int obuttons = 0;
	char buffer[256];
	int timeCounter = 0, frameIndex = 0;
	int nTile = 0, nTileView = 0;
	BOOL playing = FALSE;
	BOOL looping = FALSE;

	while (1)
	{
		gFrameTicks = gGameClock - gFrameClock;
		gFrameClock += gFrameTicks;
		UpdateBlinkClock(gFrameTicks);

		clearview(0);

		if (playing)
		{
			timeCounter -= gFrameTicks;
			while (timeCounter < 0)
			{
				timeCounter += pSeq->ticksPerFrame;
				if (++frameIndex == pSeq->nFrames)
				{
					if ( looping )
						frameIndex = 0;
					else
					{
						frameIndex--;
						playing = FALSE;
					}
				}
			}
		}

		if (frameIndex < 0)
			frameIndex = 0;

		if (frameIndex < pSeq->nFrames)
		{
			SEQFRAME *pFrame = &pSeq->frame[frameIndex];
			nTileView = DrawTile(pFrame);
			sprintf(buffer,"TILE=%4d  VIEW=%6s  SH=%+3d  PAL=%2d  XR=%3d  YR=%3d",
				nTile,
				viewNames[picanm[nTile].view],
				pFrame->shade,
				pFrame->pal,
				pFrame->xrepeat,
				pFrame->yrepeat
			);
			printext256(0, 180, gStdColor[14], -1, buffer, 1);

			if ( pFrame->trigger )
				printext256(275, 180, gStdColor[11], gStdColor[8], "F", 1);

			if ( pFrame->smoke )
				printext256(280, 180, gStdColor[11], gStdColor[8], "S", 1);

			if ( pFrame->translucent )
				printext256(285, 180, gStdColor[11], gStdColor[8], "T", 1);

			if ( pFrame->translucentR )
				printext256(285, 180, gStdColor[11], gStdColor[8], "t", 1);

			if ( pFrame->blocking )
				printext256(290, 180, gStdColor[11], gStdColor[8], "B", 1);

			if ( pFrame->hitscan )
				printext256(295, 180, gStdColor[11], gStdColor[8], "H", 1);

			if ( pSeq->flags & kSeqRemove )
				printext256(300, 180, gStdColor[12], gStdColor[8], "R", 1);

			if ( pSeq->flags & kSeqLoop )
				printext256(300, 180, gStdColor[12], gStdColor[8], "L", 1);

			nTile = pFrame->nTile;
		}

		sprintf(buffer,"GLOBAL SH=%4d  ANGLE=%3d  FRAME %2d/%2d  TPF=%3d",
			gShade, nOctant * 45, frameIndex + 1, pSeq->nFrames, pSeq->ticksPerFrame);
		printext256(0, 190, gStdColor[7], -1, buffer, 1);

		if ( !playing )
			DrawOrigin(gStdColor[keystatus[KEY_O] ? 15 : 8]);

		scrDisplayMessage(gStdColor[kColorWhite]);

		if (vidoption != 1)
			WaitVSync();

		scrNextPage();

		key = keyGet();
		shift = keystatus[KEY_LSHIFT] | keystatus[KEY_RSHIFT];
		ctrl = keystatus[KEY_LCTRL] | keystatus[KEY_RCTRL];
		alt = keystatus[KEY_LALT] | keystatus[KEY_RALT];
		pad5 = keystatus[KEY_PAD5];

		switch (key)
		{
			case KEY_ESC:
				if ( playing )
				{
					playing = FALSE;
					break;
				}

				if (!seqModified || AcknowledgeSeqChange())
					return;
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

			case KEY_PLUS:
				pSeq->ticksPerFrame++;
				seqModified = TRUE;
				break;

			case KEY_MINUS:
				pSeq->ticksPerFrame = (short)ClipLow(pSeq->ticksPerFrame - 1, 1);
				seqModified = TRUE;
				break;
		}

		if ( !playing )
		{
			switch ( key )
			{
				case KEY_F2:
					if (keystatus[KEY_LCTRL] || keystatus[KEY_RCTRL] || strlen(gSeqName) == 0)
						SaveAs();
					else
						SaveSeq();
					break;

				case KEY_F3:
					if (!seqModified || AcknowledgeSeqChange())
					{
						LoadAs();
						frameIndex = 0;
					}
					break;

				case KEY_ENTER:
					if (pSeq->nFrames > 0)
					{
						playing = TRUE;
						looping = TRUE;
						frameIndex = 0;
						timeCounter = pSeq->ticksPerFrame;
					}
					break;

				case KEY_SPACE:
					if (pSeq->nFrames > 0)
					{
						playing = TRUE;
						looping = FALSE;
						frameIndex = 0;
						timeCounter = pSeq->ticksPerFrame;
					}
					break;

				case KEY_1:
					if (frameIndex > 0)
						frameIndex--;
					break;

				case KEY_2:
					if (frameIndex < pSeq->nFrames - 1)
						frameIndex++;
					break;

				case KEY_HOME:
					frameIndex = 0;
					break;

				case KEY_END:
					frameIndex = pSeq->nFrames - 1;
					break;

				case KEY_INSERT:
					frameIndex++;
					if (ctrl)
						frameIndex--;

					if (frameIndex > pSeq->nFrames)
						frameIndex = pSeq->nFrames;

					if (ctrl)
						memmove(&pSeq->frame[frameIndex + 1], &pSeq->frame[frameIndex],
							sizeof(SEQFRAME) * (pSeq->nFrames - frameIndex));
					else
						memmove(&pSeq->frame[frameIndex], &pSeq->frame[frameIndex - 1],
							sizeof(SEQFRAME) * (pSeq->nFrames - frameIndex + 1));

					if (pSeq->nFrames == 0)
					{
						pSeq->frame[frameIndex].shade = -4;
						pSeq->frame[frameIndex].translucentR = 0;
						pSeq->frame[frameIndex].translucent = 0;
						pSeq->frame[frameIndex].blocking = 1;
						pSeq->frame[frameIndex].hitscan = 1;
						pSeq->frame[frameIndex].pal = 0;
					}

					pSeq->frame[frameIndex].nTile = tilePick(nTile, -1, SS_ALL);

					pSeq->nFrames++;
					seqModified = TRUE;
					break;

				case KEY_DELETE:
					if (pSeq->nFrames > 0)
					{
						memmove(&pSeq->frame[frameIndex], &pSeq->frame[frameIndex + 1],
							sizeof(SEQFRAME) * (pSeq->nFrames - frameIndex - 1));
						pSeq->nFrames--;
						if (frameIndex == pSeq->nFrames)
							frameIndex--;
					}
					seqModified = TRUE;
					break;

				case KEY_B:
					if (frameIndex < pSeq->nFrames)
						pSeq->frame[frameIndex].blocking = !pSeq->frame[frameIndex].blocking;
					seqModified = TRUE;
					break;

				case KEY_F:
					if (frameIndex < pSeq->nFrames)
						pSeq->frame[frameIndex].trigger = !pSeq->frame[frameIndex].trigger;
					seqModified = TRUE;
					break;

				case KEY_H:
					if (frameIndex < pSeq->nFrames)
						pSeq->frame[frameIndex].hitscan = !pSeq->frame[frameIndex].hitscan;
					seqModified = TRUE;
					break;

				case KEY_L:
					if (frameIndex < pSeq->nFrames)
					{
						pSeq->flags ^= kSeqLoop;
						pSeq->flags &= ~kSeqRemove;
						seqModified = TRUE;
					}
					break;

				case KEY_S:
					if (frameIndex < pSeq->nFrames)
						pSeq->frame[frameIndex].smoke = !pSeq->frame[frameIndex].smoke;
					seqModified = TRUE;
					break;

				case KEY_T:
					if (frameIndex < pSeq->nFrames)
					{
						if ( pSeq->frame[frameIndex].translucent )
						{
							if ( pSeq->frame[frameIndex].translucentR )
							{
								pSeq->frame[frameIndex].translucent = 0;
								pSeq->frame[frameIndex].translucentR = 0;
							}
							else
								pSeq->frame[frameIndex].translucentR = 1;
						}
						else
							pSeq->frame[frameIndex].translucent = 1;
					}
					seqModified = TRUE;
					break;

				case KEY_P:
					if (frameIndex < pSeq->nFrames)
					{
						if ( alt )
							pSeq->frame[frameIndex].pal = GetNumberBox("Palookup", pSeq->frame[frameIndex].pal, pSeq->frame[frameIndex].pal );
						else
							pSeq->frame[frameIndex].pal = IncRotate(pSeq->frame[frameIndex].pal, kMaxPLU);
						seqModified = TRUE;
					}
					break;

				case KEY_R:
					if (frameIndex < pSeq->nFrames)
					{
						pSeq->flags ^= kSeqRemove;
						pSeq->flags &= ~kSeqLoop;
						seqModified = TRUE;
					}
					break;

				case KEY_V:
					if (frameIndex < pSeq->nFrames)
						pSeq->frame[frameIndex].nTile = tilePick(nTile, nTile, SS_ALL);
					seqModified = TRUE;
					break;

				case KEY_W:
					if (frameIndex < pSeq->nFrames)
					{
						picanm[nTile].view = IncRotate(picanm[nTile].view, LENGTH(viewNames));
						tileMarkDirty(nTile);
						tilesModified = TRUE;
					}
					break;

				case KEY_PADPLUS:
					if ( alt )
						gShade = (schar)ClipLow(gShade - 1, 0);
					else if (frameIndex < pSeq->nFrames)
					{
						if ( ctrl )
							pSeq->frame[frameIndex].shade = -128;
						else
							pSeq->frame[frameIndex].shade = ClipLow(pSeq->frame[frameIndex].shade - 1, -128);
						seqModified = TRUE;
					}
					break;

				case KEY_PADMINUS:
					if ( alt )
						gShade = (schar)ClipHigh(gShade + 1, 127);
					else if (frameIndex < pSeq->nFrames)
					{
						if ( ctrl )
							pSeq->frame[frameIndex].shade = 127;
						else
							pSeq->frame[frameIndex].shade = ClipHigh(pSeq->frame[frameIndex].shade + 1, 127);
						seqModified = TRUE;
					}
					break;

				case KEY_PAD0:
					if (frameIndex < pSeq->nFrames)
					{
						pSeq->frame[frameIndex].shade = 0;
						seqModified = TRUE;
					}
					break;

				case KEY_PAGEUP:
					while (pSeq->frame[frameIndex].nTile > 0)
					{
						pSeq->frame[frameIndex].nTile--;
						if (tilesizx[pSeq->frame[frameIndex].nTile] && tilesizx[pSeq->frame[frameIndex].nTile])
							break;
					}
					seqModified = TRUE;
					break;

				case KEY_PAGEDN:
					while (pSeq->frame[frameIndex].nTile < kMaxTiles - 1)
					{
						pSeq->frame[frameIndex].nTile++;
						if (tilesizx[pSeq->frame[frameIndex].nTile] && tilesizx[pSeq->frame[frameIndex].nTile])
							break;
					}
					seqModified = TRUE;
					break;

				case KEY_UP:
					if ( keystatus[KEY_O] )
					{
						SetOrigin(origin.x, origin.y - 1);
						break;
					}
					else if (frameIndex < pSeq->nFrames)
					{
						if (shift)
							picanm[nTileView].ycenter = -tilesizy[nTileView] / 2;
						else
							picanm[nTileView].ycenter++;
						tileMarkDirty(nTileView);
						tilesModified = TRUE;
					}
					break;

				case KEY_LEFT:
					if ( keystatus[KEY_O] )
					{
						SetOrigin(origin.x - 1, origin.y);
						break;
					}
					else if (frameIndex < pSeq->nFrames)
					{
						if (shift)
							picanm[nTileView].xcenter = -tilesizx[nTileView] / 2;
						else
							picanm[nTileView].xcenter++;
						tileMarkDirty(nTileView);
						tilesModified = TRUE;
					}
					break;

				case KEY_PAD5:
					if ( frameIndex < pSeq->nFrames && shift )
					{
						picanm[nTileView].xcenter = 0;
						picanm[nTileView].ycenter = 0;
						tileMarkDirty(nTileView);
						tilesModified = TRUE;
					}
					break;

				case KEY_RIGHT:
					if ( keystatus[KEY_O] )
					{
						SetOrigin(origin.x + 1, origin.y);
						break;
					}
					else if (frameIndex < pSeq->nFrames)
					{
						if (shift)
							picanm[nTileView].xcenter = tilesizx[nTileView] - tilesizx[nTileView] / 2;
						else
							picanm[nTileView].xcenter--;
						tileMarkDirty(nTileView);
						tilesModified = TRUE;
					}
					break;

				case KEY_DOWN:
					if ( keystatus[KEY_O] )
					{
						SetOrigin(origin.x, origin.y + 1);
						break;
					}
					else if (frameIndex < pSeq->nFrames)
					{
						if (shift)
							picanm[nTileView].ycenter = tilesizy[nTileView] - tilesizy[nTileView] / 2;
						else
							picanm[nTileView].ycenter--;
						tileMarkDirty(nTileView);
						tilesModified = TRUE;
					}
					break;

				case KEY_PADUP:
					if (frameIndex < pSeq->nFrames)
						pSeq->frame[frameIndex].yrepeat = IncNext(pSeq->frame[frameIndex].yrepeat,
							pad5 ? 8 : 1);
					seqModified = TRUE;
					break;

				case KEY_PADDOWN:
					if (frameIndex < pSeq->nFrames)
						pSeq->frame[frameIndex].yrepeat = DecNext(pSeq->frame[frameIndex].yrepeat,
							pad5 ? 8 : 1);
					seqModified = TRUE;
					break;

				case KEY_PADLEFT:
					if (frameIndex < pSeq->nFrames)
						pSeq->frame[frameIndex].xrepeat = DecNext(pSeq->frame[frameIndex].xrepeat,
							pad5 ? 8 : 1);
					seqModified = TRUE;
					break;

				case KEY_PADRIGHT:
					if (frameIndex < pSeq->nFrames)
						pSeq->frame[frameIndex].xrepeat = IncNext(pSeq->frame[frameIndex].xrepeat,
							pad5 ? 8 : 1);
					seqModified = TRUE;
					break;
			}
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
	}
}


void main( int argc, char *argv[] )
{
	char title[256];

	gOldDisplayMode = getvmode();

	sprintf(title, "Sequence Editor [%s] -- DO NOT DISTRIBUTE", gBuildDate);
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

	tioPrint("Initializing mouse");
	if ( !initmouse() )
		tioPrint("Mouse not detected");

	// install our error handler
	prevErrorHandler = errSetHandler(EditorErrorHandler);

	InitEngine();

	Mouse::SetRange(xdim, ydim);

	tioPrint("Loading tiles");
	if (tileInit() == 0)
		ThrowError("ART files not found", ES_ERROR);

	tioPrint("Initializing screen");
	scrInit();

	tioPrint("Installing keyboard handler");
	keyInstall();

	scrCreateStdColors();

	tioPrint("Installing timer");
	timerRegisterClient(ClockStrobe, kTimerRate);
	timerInstall();

	tioPrint("Engaging graphics subsystem...");

	scrSetGameMode();

	scrSetGamma(gGamma);
	scrSetDac(0);

	strcpy(gSeqName, "");

	pSeq = (Seq *)Resource::Alloc(sizeof(Seq) + kMaxFrames * sizeof(SEQFRAME));
	memset(pSeq, 0, sizeof(Seq) + kMaxFrames * sizeof(SEQFRAME));

	pSeq->nFrames = 0;
	pSeq->ticksPerFrame = 12;

	seqModified = FALSE;
	tilesModified = FALSE;

	if ( argc > 1 )
	{
 		strcpy(gSeqName, argv[1]);
		LoadSeq();
	}

	do {
		EditLoop();
	} while (tilesModified && !AcknowledgeTileChange());

	setvmode(gOldDisplayMode);

	dprintf("Removing timer\n");
	timerRemove();

 	dprintf("uninitengine()\n");
	uninitengine();

	dprintf("All subsystems shut down.  Processing exit functions\n");

	errSetHandler(prevErrorHandler);
}
