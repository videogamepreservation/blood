/*******************************************************************************
	FILE:			QAVEDIT.CPP

	DESCRIPTION:	Allows editing of 2D animation sequences

	AUTHOR:			Peter M. Freese
	CREATED:		02-26-95
	COPYRIGHT:		Copyright (c) 1995 Q Studios Corporation
*******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <helix.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <dos.h>

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
#include "qav.h"
#include "screen.h"
#include "textio.h"
#include "inifile.h"
#include "options.h"
#include "timer.h"
#include "mouse.h"
#include "trig.h"
#include "getopt.h"

#define kMaxFrames	1024

#define kAttrTitle      (kColorGreen * 16 + kColorWhite)

struct FRAME1
{
	long			timeStart;
	long			timeStop;
	int				type;
	int				id;
	int				flags;
	union
	{
		// type = kQFrameTile
		struct
		{
			int		x, y;
			int		layer;
			int		shade;
			int		pal;
		};

		// type = kQFrameSound
		struct
		{
			int		priority;
			int		volume;
			int		soundID;
		};

		// type = kQFrameTrigger
		struct {
			int		trigger;
		};
	};
};

struct QAV1
{
	char 	signature[4];
	short 	version;
	char	dummy[2];

	int		frames;
	long	duration;
	int		backTile;
	Point	origin;
	FRAME1	frame[1];
};


struct EditQAV : public QAV
{
	void 	ClipTileFrame( TILE_FRAME *f );
	void	SetOrigin( int x, int y );
	void 	DrawOrigin( int nColor );
	void 	HighlightFrame( TILE_FRAME *f );
};


char gQAVName[128] = "";

int lastTile = 2052;
char buffer[256];
EditQAV *gQAV = NULL;
static BOOL isModified;
EHF prevErrorHandler;

void faketimerhandler( void )
{ }


struct FNODE
{
	FNODE *next;
	char name[1];
};

FNODE head = { &head, "" };
FNODE *tail = &head;


/*******************************************************************************
	FUNCTION:		ShowBanner()

	DESCRIPTION:	Show application banner
*******************************************************************************/
void ShowBanner( void )
{
    printf("컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�\n");
	printf("       QAV Editor  Version 2.0  Copyright (c) 1995 Q Studios Corporation\n");
    printf("컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�\n");
}


/*******************************************************************************
	FUNCTION:		ShowUsage()

	DESCRIPTION:	Display command-line parameter usage, then exit
*******************************************************************************/
void ShowUsage(void)
{
	printf("Syntax:  QAVEDIT [options] files[.qav]\n");
	printf("-c            Batch convert files to current verson\n");
	printf("-?            This help\n");
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
	printf(msg);
	exit(1);
}


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


/***********************************************************************
 * Ensure that the tile location isn't off the screen
 **********************************************************************/
void EditQAV::ClipTileFrame( TILE_FRAME *f )
{
	int xOffset, yOffset;

	if ( f->flags & kQFrameCorner )
		xOffset = yOffset = 0;
	else
	{
		xOffset = picanm[f->id].xcenter + tilesizx[f->id] / 2 - origin.x;
		yOffset = picanm[f->id].ycenter + tilesizy[f->id] / 2 - origin.y;
	}

	if (f->x < xOffset - tilesizx[f->id] + 1)
		f->x = xOffset - tilesizx[f->id] + 1;

	if (f->x > 319 + xOffset)
		f->x = 319 + xOffset;

	if (f->y < yOffset - tilesizy[f->id] + 1)
		f->y = yOffset - tilesizy[f->id] + 1;

	if (f->y > 199 + yOffset)
		f->y = 199 + yOffset;
}


void EditQAV::SetOrigin( int x, int y )
{
	if (keystatus[KEY_LCTRL] || keystatus[KEY_RCTRL])
	{
		x = ClipRange(x, 0, 319);
		y = ClipRange(y, 0, 199);
		int dx = x - origin.x;
		int dy = y - origin.y;

		for (int i = 0; i < nFrames; i++)
		{
			for (int j = 0; j < kMaxLayers; j++)
			{
				frame[i].layer[j].x -= dx;
				frame[i].layer[j].y -= dy;
			}
		}
		origin.x = x;
		origin.y = y;
	}
	else
	{
		origin.x = ClipRange(x, 0, 319);
		origin.y = ClipRange(y, 0, 199);

		// ensure that no frame are offscreen
		for (int i = 0; i < nFrames; i++)
		{
			for (int j = 0; j < kMaxLayers; j++)
			{
				ClipTileFrame(&frame[i].layer[j]);
			}
		}
	}
}


void EditQAV::DrawOrigin( int nColor )
{
	Video.SetColor(nColor);
	gfxHLine(origin.y << 1, origin.x - 6 << 1, origin.x - 1 << 1);
	gfxHLine(origin.y << 1, origin.x + 1 << 1, origin.x + 6 << 1);
	gfxVLine(origin.x << 1, origin.y - 5 << 1, origin.y - 1 << 1);
	gfxVLine(origin.x << 1, origin.y + 1 << 1, origin.y + 5 << 1);
}


void EditQAV::HighlightFrame( TILE_FRAME *f )
{
	int xOffset, yOffset;

	dassert(f->id > 0);

	if ( f->flags & kQFrameCorner )
		xOffset = yOffset = 0;
	else
	{
		xOffset = picanm[f->id].xcenter + tilesizx[f->id] / 2;
		yOffset = picanm[f->id].ycenter + tilesizy[f->id] / 2;
	}

	int x0 = (origin.x + f->x - mulscale16(xOffset, f->zoom) << 1) - 1;
	int y0 = (origin.y + f->y - mulscale16(yOffset, f->zoom) << 1) - 1;
	int x1 = x0 + (mulscale16(tilesizx[f->id], f->zoom) << 1) + 1;
	int y1 = y0 + (mulscale16(tilesizy[f->id], f->zoom) << 1) + 1;

	Video.SetColor(gStdColor[8]);
	gfxHLine(y0, x0, x1);
	gfxHLine(y1, x0, x1);
	gfxVLine(x0, y0, y1);
	gfxVLine(x1, y0, y1);
}


void SaveQAV( void )
{
	char filename[128];
	int hFile;

	strcpy(filename, gQAVName);
	ChangeExtension(filename, ".QAV");

	// see if file exists
	if ( access(filename, F_OK) == 0 )
	{
		char bakname[_MAX_PATH];
		strcpy(bakname, filename);
		ChangeExtension(bakname, ".BAK");
		unlink(bakname);
		rename(filename, bakname);
	}

	hFile = open(filename, O_CREAT | O_WRONLY | O_BINARY | O_TRUNC, S_IWUSR);
	if ( hFile == -1 )
		ThrowError("Error creating QAV file", ES_ERROR);

	memcpy(gQAV->signature, kQAVSig, sizeof(gQAV->signature));
	gQAV->version = kQAVVersion;

	gQAV->duration = gQAV->nFrames * gQAV->ticksPerFrame;

	if ( !FileWrite(hFile, gQAV, sizeof(QAV) + (gQAV->nFrames) * sizeof(FRAME)) )
		goto WriteError;

	close(hFile);
	isModified = FALSE;

	scrSetMessage("QAV saved.");
	return;

WriteError:
	close(hFile);
	ThrowError("Error writing QAV file", ES_ERROR);
}


BOOL LoadQAV( void )
{
	char filename[128];
	int hFile;
	int nSize;

	strcpy(filename, gQAVName);
	ChangeExtension(filename, ".QAV");

	hFile = open(filename, O_RDONLY | O_BINARY);
	if ( hFile == -1 )
		return FALSE;

	nSize = filelength(hFile);

	QAV *pTemp = (QAV *)Resource::Alloc(nSize);

	if ( !FileRead(hFile, pTemp, nSize) )
		goto ReadError;

	close(hFile);

	if (memcmp(pTemp->signature, kQAVSig, sizeof(pTemp->signature)) != 0)
	{
		close(hFile);
		ThrowError("QAV file corrupted", ES_ERROR);
	}

	gQAV->nFrames = 0;
	gQAV->ticksPerFrame = 5;
	gQAV->duration = 0;
	gQAV->origin.x = 160;
	gQAV->origin.y = 100;

	memset(&gQAV->frame[0], 0, sizeof(FRAME) * kMaxFrames);

	switch ( pTemp->version >> 8 )
	{
		case 1:
		{
			QAV1 *pQAV1 = (QAV1 *)pTemp;
			gQAV->nFrames = pQAV1->duration / 5;
			gQAV->ticksPerFrame = 5;
			gQAV->duration = pQAV1->duration;
			gQAV->origin = pQAV1->origin;

			FRAME1 *pFrame1 = &pQAV1->frame[0];
			for (int i = 0; i < pQAV1->frames; i++, pFrame1++)
			{
				if ( pFrame1->type != 0 )
					continue;

				int j = pFrame1->timeStart / 5;
				FRAME *pFrame = &gQAV->frame[j];
				int nLayer = kMaxLayers - 1;

				// find highest used layer
				while ( nLayer >= 0 && pFrame->layer[nLayer].id == 0 )
					nLayer--;

				// use next layer up
				nLayer++;
				if ( nLayer == kMaxLayers )
				{
					dprintf("Out of layers at time index %d\n", pFrame1->timeStart);
					nLayer = 0;
				}

				for ( ; j < pFrame1->timeStop / 5; j++, pFrame++)
				{
					pFrame->layer[nLayer].id = pFrame1->id;
					pFrame->layer[nLayer].x = pFrame1->x;
					pFrame->layer[nLayer].y = pFrame1->y;
					pFrame->layer[nLayer].zoom = 0x10000;
					pFrame->layer[nLayer].flags = pFrame1->flags;
					pFrame->layer[nLayer].shade = (schar)pFrame1->shade;
					pFrame->layer[nLayer].pal = (uchar)pFrame1->pal;
					pFrame->layer[nLayer].angle = 0;
				}
			}
			break;
		}

		case 2:
			memcpy(gQAV, pTemp, nSize);
			break;
	}

	isModified = FALSE;

	return TRUE;

ReadError:
	close(hFile);
	ThrowError("Error reading QAV file", ES_ERROR);
	return FALSE;
}


BOOL SaveAs( void )
{
	if ( GetStringBox("Save As", gQAVName) )
	{
		// load it if it seems valid
		if (strlen(gQAVName) > 0)
		{
			SaveQAV();
			return TRUE;
		}
	}
	return FALSE;
}


BOOL LoadAs( void )
{
	if ( GetStringBox("Load QAV", gQAVName) )
	{
		// load it if it seems valid
		if (strlen(gQAVName) > 0)
		{
			LoadQAV();
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
BOOL ChangeAcknowledge(void)
{
	// create the dialog
	Window dialog(59, 80, 202, 46, "Save Changes?");

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
			return SaveAs();
	}

	return TRUE;
}


void PlayIt( BOOL looping )
{
	ulong timeBase = gGameClock;
	ulong t = 0;
	int timeFactor;

	gQAV->duration = gQAV->nFrames * gQAV->ticksPerFrame;

	while (1)
	{
		BYTE key = keyGet();
		BYTE shift = keystatus[KEY_LSHIFT] | keystatus[KEY_RSHIFT];
		BYTE ctrl = keystatus[KEY_LCTRL] | keystatus[KEY_RCTRL];

		switch (key)
		{
			case KEY_ESC:
				keystatus[KEY_ESC] = 0;
				return;

			case KEY_PLUS:
				gQAV->ticksPerFrame++;
				gQAV->duration = gQAV->nFrames * gQAV->ticksPerFrame;
				isModified = TRUE;
				break;

			case KEY_MINUS:
				gQAV->ticksPerFrame = ClipLow(gQAV->ticksPerFrame - 1, 1);
				gQAV->duration = gQAV->nFrames * gQAV->ticksPerFrame;
				isModified = TRUE;
				break;
		}


		if ((t >> 16) >= gQAV->duration)
		{
			if (!looping)
				break;

			t -= (gQAV->duration << 16);
			continue;
		}

		clearview(0);

		gQAV->Draw(t >> 16, 0, kQFrameScale);

		sprintf(buffer, "%2d:%03d  TPF=%2d",
			(t >> 16) / kTimerRate, (t >> 16) % kTimerRate, gQAV->ticksPerFrame);
		gfxDrawText(512, 8, gStdColor[15], buffer);

		if (vidoption != 1)
			WaitVSync();

		scrNextPage();

		gFrameTicks = gGameClock - gFrameClock;
		gFrameClock += gFrameTicks;

		timeFactor = 16;
		if ( ctrl )
			timeFactor = 15;
		if ( shift )
			timeFactor = 14;

		t += gFrameTicks << timeFactor;
	}
}


void DrawEditFrame( FRAME *f )
{
	// show the trigger info
	TRIGGER_FRAME *tf = &f->trigger;
	if ( tf->id > 0 )
	{
		sprintf( buffer, "Trigger: %6i", tf->id );
		gfxDrawText(0, 16, gStdColor[15], buffer);
	}

	// show the sound info
	SOUND_FRAME *sf = &f->sound;
	if ( sf->id > 0 )
	{
		sprintf(buffer, "Snd: %6d  Vol: %4d  Pri: %4d", sf->id, sf->volume, sf->priority );
		gfxDrawText(400, 16, gStdColor[15], buffer);
	}
}


void EditLoop( void )
{
	int nbuttons;
	static int obuttons = 0;
	char buffer[256];
	int nFrame = 0;
	int nLayer = 0;

	while (1)
	{
		FRAME *pFrame = &gQAV->frame[nFrame];
		TILE_FRAME *pTFrame = &pFrame->layer[nLayer];

		gFrameTicks = gGameClock - gFrameClock;
		gFrameClock += gFrameTicks;
		UpdateBlinkClock(gFrameTicks);

		gQAV->duration = gQAV->nFrames * gQAV->ticksPerFrame;

		clearview(0);

		if ( nFrame < gQAV->nFrames )
			gQAV->Draw(nFrame * gQAV->ticksPerFrame, 0, kQFrameScale);
		DrawEditFrame(pFrame);

		sprintf(buffer,"%2d/%2d  Layer=%d  TPF=%2d",
			nFrame, gQAV->nFrames, nLayer, gQAV->ticksPerFrame);
		gfxDrawText(464, 8, gStdColor[15], buffer);

		if (keystatus[KEY_O])
		{
			gQAV->DrawOrigin(gStdColor[15]);
			sprintf(buffer,"%3i,%3i", gQAV->origin.x, gQAV->origin.y);
			gfxDrawText(0, 8, gStdColor[15], buffer);
		}
		else
		{
	 		gQAV->DrawOrigin(gStdColor[8]);
			if ( pTFrame->id > 0 )
			{
				if ( IsBlinkOn() )
					gQAV->HighlightFrame(pTFrame);

				sprintf(buffer,"Tile=%4d %+4d,%+4d Sh=%+4d Pal=%2d Zoom=%8X Ang=%4d",
					pTFrame->id,
					pTFrame->x,
					pTFrame->y,
					pTFrame->shade,
					pTFrame->pal,
					pTFrame->zoom,
					pTFrame->angle
				);
				gfxDrawText(0, 8, gStdColor[15], buffer);
			}
		}

		scrDisplayMessage(gStdColor[kColorWhite]);

		if (vidoption != 1)
			WaitVSync();

		scrNextPage();

		BYTE key = keyGet();
		BYTE shift = keystatus[KEY_LSHIFT] | keystatus[KEY_RSHIFT];
		BYTE ctrl = keystatus[KEY_LCTRL] | keystatus[KEY_RCTRL];
		BYTE alt = keystatus[KEY_LALT] | keystatus[KEY_RALT];
		BYTE pad5 = keystatus[KEY_PAD5];

		switch (key)
		{
			case KEY_ESC:
				keystatus[KEY_ESC] = 0;
				if (!isModified || ChangeAcknowledge())
				{
					free(gQAV);
					gQAV = NULL;
					return;
				}
				break;

			case KEY_SPACE:
				PlayIt(FALSE);
				break;

			case KEY_ENTER:
				PlayIt(TRUE);
				break;

			case KEY_F2:
				if (keystatus[KEY_LCTRL] || keystatus[KEY_RCTRL] || strlen(gQAVName) == 0)
					SaveAs();
				else
					SaveQAV();
				break;

			case KEY_F3:
				if (!isModified || ChangeAcknowledge())
				{
					LoadAs();
					nFrame = 0;
					nLayer = 0;
				}
				break;

			case KEY_2:
				SetBlinkOn();
				nFrame = ClipHigh(nFrame + 1, gQAV->nFrames);
				break;

			case KEY_1:
				SetBlinkOn();
				nFrame = ClipLow(nFrame - 1, 0);
				break;

			case KEY_HOME:
				SetBlinkOn();
				nFrame = 0;
				break;

			case KEY_END:
				SetBlinkOn();
				nFrame = gQAV->nFrames;
				break;

			case KEY_PLUS:
				gQAV->ticksPerFrame++;
				isModified = TRUE;
				break;

			case KEY_MINUS:
				gQAV->ticksPerFrame = ClipLow(gQAV->ticksPerFrame - 1, 1);
				isModified = TRUE;
				break;

			case KEY_INSERT:
			{
				if ( ctrl )
				{
					// insert a layer
					for (int i = 0; i < gQAV->nFrames; i++)
					{
						for (int j = kMaxLayers - 1; j > nLayer; j--)
						{
							gQAV->frame[i].layer[j] = gQAV->frame[i].layer[j - 1];
						}
						memset(&gQAV->frame[i].layer[j], 0, sizeof(TILE_FRAME));
					}
					isModified = TRUE;
					break;
				}

				if ( alt )
				{
					// insert a frame
					memmove(pFrame + 1, pFrame, (gQAV->nFrames - nFrame) * sizeof(FRAME));
					memset(pFrame, 0, sizeof(FRAME));
					gQAV->nFrames++;
					isModified = TRUE;
					break;
				}

				// just duplicate a tile
				if ( pTFrame->id <= 0 && nFrame > 0 )
				{
					*pTFrame = gQAV->frame[nFrame - 1].layer[nLayer];
					isModified = TRUE;

					if ( nFrame == gQAV->nFrames )
						gQAV->nFrames++;
					break;
				}
				break;
			}

			case KEY_DELETE:
				if ( ctrl )
				{
					// delete a layer
					for (int i = 0; i < gQAV->nFrames; i++)
					{
						for (int j = nLayer; j < kMaxLayers - 1; j++)
						{
							gQAV->frame[i].layer[j] = gQAV->frame[i].layer[j + 1];
						}
						memset(&gQAV->frame[i].layer[j], 0, sizeof(TILE_FRAME));
					}
					isModified = TRUE;
					break;
				}

				if ( alt )
				{
					// delete a frame
					if ( nFrame < gQAV->nFrames )
					{
						gQAV->nFrames--;
						memmove(pFrame, pFrame + 1, (gQAV->nFrames - nFrame) * sizeof(FRAME));
						memset(&gQAV->frame[gQAV->nFrames], 0, sizeof(FRAME));
						isModified = TRUE;
					}
					break;
				}

				// just delete a tile
				if ( pTFrame->id > 0 )
				{
					memset(pTFrame, 0, sizeof(TILE_FRAME));
					isModified = TRUE;
					break;
				}
				break;

			case KEY_PAGEUP:
				SetBlinkOn();
				if ( nLayer < kMaxLayers - 1 )
				{
					if ( shift )
					{
						if ( ctrl )
						{
							// raise entire layer
							for (int i = 0; i < gQAV->nFrames; i++)
							{
								TILE_FRAME temp = gQAV->frame[i].layer[nLayer];
								gQAV->frame[i].layer[nLayer] = gQAV->frame[i].layer[nLayer + 1];
								gQAV->frame[i].layer[nLayer + 1] = temp;
							}
							nLayer++;
							isModified = TRUE;
							break;
						}

						// raise tile
						TILE_FRAME temp = *pTFrame;
						*pTFrame = pFrame->layer[nLayer + 1];
						pFrame->layer[nLayer + 1] = temp;
						nLayer++;
						isModified = TRUE;
						break;
					}
					nLayer++;
				}
				break;

			case KEY_PAGEDN:
				SetBlinkOn();
				if ( nLayer > 0 )
				{
					if ( shift )
					{
						if ( ctrl )
						{
							// lower entire layer
							for (int i = 0; i < gQAV->nFrames; i++)
							{
								TILE_FRAME temp = gQAV->frame[i].layer[nLayer];
								gQAV->frame[i].layer[nLayer] = gQAV->frame[i].layer[nLayer - 1];
								gQAV->frame[i].layer[nLayer - 1] = temp;
							}
							nLayer--;
							isModified = TRUE;
							break;
						}

						// lower tile
						TILE_FRAME temp = *pTFrame;
						*pTFrame = pFrame->layer[nLayer - 1];
						pFrame->layer[nLayer - 1] = temp;
						nLayer--;
						isModified = TRUE;
						break;
					}
					nLayer--;
				}
				break;

			case KEY_F:
				pFrame->trigger.id = GetNumberBox("Trigger Id", pFrame->trigger.id, pFrame->trigger.id);

				if ( pFrame->trigger.id == 0 )
					break;

				if ( nFrame == gQAV->nFrames )
					gQAV->nFrames++;

				isModified = TRUE;
				break;

			case KEY_P:
				if ( pTFrame->id > 0 )
				{
					// set the palette for the highlighted tile frame
					if ( alt )
						pTFrame->pal = (uchar)GetNumberBox("Palookup", pTFrame->pal, pTFrame->pal );
					else
						pTFrame->pal = (uchar)IncRotate(pTFrame->pal, kMaxPLU);
					isModified = TRUE;
				}
				break;

			case KEY_R:
				if ( pTFrame->id > 0 )
				{
					pTFrame->flags ^= kQFrameCorner;
					isModified = TRUE;
				}
				break;

			case KEY_T:
				if ( pTFrame->id > 0 )
				{
					// set translucency flags for the highlighted tile frame
					int flags = pTFrame->flags;
					int level = 0;

					if ( flags & kQFrameTranslucent )
					{
						level = 1;
						if ( flags & kQFrameTranslucentR )
							level = 2;
					}
					level = IncRotate(level, 3);

					switch ( level )
					{
						case 0:
							flags &= ~kQFrameTranslucent & ~kQFrameTranslucentR;
							break;

						case 1:
							flags &= ~kQFrameTranslucentR;
							flags |= kQFrameTranslucent;
							break;

						case 2:
							flags |= kQFrameTranslucent | kQFrameTranslucentR;
							break;
					}
					pTFrame->flags = flags;
					isModified = TRUE;
				}
				break;

			case KEY_V:
			{
				int nTile = pTFrame->id;
				if ( nTile == 0 )
				{
					if ( nFrame > 0 )
					{
						*pTFrame = gQAV->frame[nFrame - 1].layer[nLayer];
						nTile = pTFrame->id;
						pTFrame->id = 0;
					}
					else
						pTFrame->zoom = 0x10000;
				}
				pTFrame->id = tilePick(nTile, pTFrame->id, SS_ALL);

				if ( pTFrame->id == 0 )
					break;

				if ( nFrame == gQAV->nFrames )
					gQAV->nFrames++;

				isModified = TRUE;
				break;
			}

			case KEY_X:
				if ( pTFrame->id > 0 )
				{
					pTFrame->flags ^= kQFrameXFlip;
					isModified = TRUE;
				}
				break;

			case KEY_Y:
				if ( pTFrame->id > 0 )
				{
					pTFrame->flags ^= kQFrameYFlip;
					isModified = TRUE;
				}
				break;

			case KEY_COMMA:
				if ( pTFrame->id > 0 )
				{
					int step = shift ? 4 : 128;
					pTFrame->angle = (short)(DecNext(pTFrame->angle, step) & kAngleMask);
					isModified = TRUE;
				}
				break;

			case KEY_PERIOD:
				if ( pTFrame->id > 0 )
				{
					int step = shift ? 4 : 128;
					pTFrame->angle = (short)(IncNext(pTFrame->angle, step) & kAngleMask);
					isModified = TRUE;
				}
				break;

			case KEY_PADSLASH:
				if ( pTFrame->id > 0 )
				{
					int step = shift ? 0x100 : 0x1000;
					pTFrame->zoom = pTFrame->zoom + step;
					isModified = TRUE;
				}
				break;

			case KEY_PADSTAR:
				if ( pTFrame->id > 0 )
				{
					int step = shift ? 0x100 : 0x1000;
					pTFrame->zoom = ClipLow(pTFrame->zoom - step, step);
					isModified = TRUE;
				}
				break;

			case KEY_PADPLUS:
				if ( pTFrame->id > 0 )
				{
					if ( ctrl )
						pTFrame->shade = -128;
					else
						pTFrame->shade = (schar)ClipLow( pTFrame->shade - 1, -128 );
					isModified = TRUE;
				}
				break;

			case KEY_PADMINUS:
				if ( pTFrame->id > 0 )
				{
					if ( ctrl )
						pTFrame->shade = 127;
					else
						pTFrame->shade = (schar)ClipHigh( pTFrame->shade + 1, 127 );
					isModified = TRUE;
				}
				break;

			case KEY_PAD0:
				if ( pTFrame->id > 0 )
				{
					pTFrame->shade = 0;
					isModified = TRUE;
				}
				break;

			case KEY_UP:
				SetBlinkOff();
				if ( keystatus[KEY_O] )
				{
					gQAV->SetOrigin(gQAV->origin.x, gQAV->origin.y - 1);
					isModified = TRUE;
					break;
				}

				if ( pTFrame->id > 0 )
				{
					pTFrame->y--;
					isModified = TRUE;
				}
				break;

			case KEY_DOWN:
				SetBlinkOff();
				if ( keystatus[KEY_O] )
				{
					gQAV->SetOrigin(gQAV->origin.x, gQAV->origin.y + 1);
					isModified = TRUE;
					break;
				}

				if ( pTFrame->id > 0 )
				{
					pTFrame->y++;
					isModified = TRUE;
				}
				break;

			case KEY_LEFT:
				SetBlinkOff();
				if ( keystatus[KEY_O] )
				{
					gQAV->SetOrigin(gQAV->origin.x - 1, gQAV->origin.y);
					isModified = TRUE;
					break;
				}

				if ( pTFrame->id > 0 )
				{
					pTFrame->x--;
 					isModified = TRUE;
				}
				break;

			case KEY_RIGHT:
				SetBlinkOff();
				if ( keystatus[KEY_O] )
				{
					gQAV->SetOrigin(gQAV->origin.x + 1, gQAV->origin.y);
					isModified = TRUE;
					break;
				}

				if ( pTFrame->id > 0 )
				{
					pTFrame->x++;
					isModified = TRUE;
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
			{
				gQAV->SetOrigin(gQAV->origin.x + Mouse::dX2, gQAV->origin.y + Mouse::dY2);
				isModified = TRUE;
			}
			else
			{
				SetBlinkOff();

				if ( pTFrame->id > 0 )
				{
					pTFrame->x += Mouse::dX2;
					pTFrame->y += Mouse::dY2;
					isModified = TRUE;
				}
			}
		}

		// make sure x and y don't cause the tile to go off screen
		if ( pTFrame->id > 0 )
			gQAV->ClipTileFrame(pTFrame);
	}
}


void InsertFilename( char *fname )
{
	FNODE *n = (FNODE *)malloc(sizeof(FNODE) + strlen(fname));
	strcpy(n->name, fname);

	// insert the node at the tail, so it stays in order
	n->next = tail->next;
	tail->next = n;
	tail = n;
}


void ConvertWildcards(char *s)
{
	char filespec[_MAX_PATH];
	char buffer[_MAX_PATH2];
	char path[_MAX_PATH];

	strcpy(filespec, s);
	AddExtension(filespec, ".SEQ");

	char *drive, *dir;

	// separate the path from the filespec
	_splitpath2(s, buffer, &drive, &dir, NULL, NULL);
	_makepath(path, drive, dir, NULL, NULL);

	struct find_t fileinfo;

	unsigned r = _dos_findfirst(s, _A_NORMAL, &fileinfo);
	if (r != 0)
		printf("%s not found\n", s);

	while ( r == 0 )
	{
		strcpy(filespec, path);
		strcat(filespec, fileinfo.name);
		InsertFilename(filespec);

		r = _dos_findnext( &fileinfo );
	}

	_dos_findclose(&fileinfo);

	// process the file list
	for (FNODE *n = head.next; n != &head; n = n->next)
	{
		strcpy(gQAVName, n->name);
		printf("Converting: %s\n", n->name);
		LoadQAV();
		SaveQAV();
	}
}


/***********************************************************************
 * Process command line arguments
 **********************************************************************/
void ParseOptions( void )
{
	enum {
		kSwitchHelp,
		kSwitchConvert,
	};
	static SWITCH switches[] = {
		{ "?", kSwitchHelp, FALSE },
		{ "C", kSwitchConvert, TRUE },
		{ NULL, 0, FALSE },

	};
	int r;
	while ( (r = GetOptions(switches)) != GO_EOF )
	{
		switch (r)
		{
			case GO_INVALID:
				QuitMessage("Invalid argument: %s", OptArgument);

			case GO_FULL:
		 		strcpy(gQAVName, OptArgument);
				break;

			case kSwitchHelp:
				ShowUsage();
				break;

			case kSwitchConvert:
				ConvertWildcards(OptArgument);
				exit(0);
		}
	}
}


void main( void )
{
	ShowBanner();

	gOldDisplayMode = getvmode();

	if ( _grow_handles(kRequiredFiles) < kRequiredFiles )
		ThrowError("Not enough file handles available", ES_ERROR);

	gGamma = BloodINI.GetKeyInt("View", "Gamma", 0);

	Resource::heap = new QHeap(dpmiDetermineMaxRealAlloc());

	gQAV = (EditQAV *)Resource::Alloc(sizeof(EditQAV) + kMaxFrames * sizeof(FRAME));
	dassert(gQAV != NULL);

	ParseOptions();

	gSysRes.Init("BLOOD.RFF", "*.*");
	gGuiRes.Init("GUI.RFF", NULL);

	if ( !initmouse() )
		QuitMessage("Mouse not detected");

	// install our error handler
	prevErrorHandler = errSetHandler(EditorErrorHandler);

	InitEngine(1, 640, 400);

	Mouse::SetRange(xdim, ydim);

	if (tileInit() == 0)
		ThrowError("ART files not found", ES_ERROR);

	scrInit();

	keyInstall();

	scrCreateStdColors();

	timerRegisterClient(ClockStrobe, kTimerRate);
	timerInstall();

	scrSetGameMode();

	scrSetGamma(gGamma);
	scrSetDac(0);

	isModified = FALSE;

	if ( strlen(gQAVName) > 0 )
		LoadQAV();
	else
	{
		gQAV->nFrames = 0;
		gQAV->ticksPerFrame = 5;
		gQAV->duration = 0;
		gQAV->origin.x = 160;
		gQAV->origin.y = 100;

		memset(&gQAV->frame[0], 0, sizeof(FRAME) * kMaxFrames);
	}

	EditLoop();

	setvmode(gOldDisplayMode);

	dprintf("Removing timer\n");
	timerRemove();

 	dprintf("uninitengine()\n");
	uninitengine();

	dprintf("All subsystems shut down.  Processing exit functions\n");

	errSetHandler(prevErrorHandler);
}



