/*******************************************************************************
	FILE:			FONTEDIT.CPP

	DESCRIPTION:	Allows editing of 2D animation sequences

	AUTHOR:			Nicholas C. Newhard
	CREATED:		11-30-95
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
#include "qav.h"
#include "screen.h"
#include "textio.h"
#include "inifile.h"
#include "options.h"
#include "timer.h"
#include "mouse.h"
#include "font.h"

#include <memcheck.h>

#define kAttrTitle      (kColorGreen * 16 + kColorWhite)


/***********************************************************************
 * CLASS FontEdit
 **********************************************************************/
class FontEdit
{
private:
	Font	theFont;
	int		theCharacter;

public:
	FontEdit( void );
	Load( char *zName = NULL );
	Draw( void );
};


/***********************************************************************
 * FontEdit::FontEdit
 **********************************************************************/
FontEdit::FontEdit( void )
{
	theCharacter = 'A';
	unlink("UNNAMED.FNT");
	theFont.Load("UNNAMED");
}


/***********************************************************************
 * FontEdit::Load
 **********************************************************************/
FontEdit::Load( char *zName )
{
	// allocate and initialize a font
	if (theFont.Load( zName ) == FALSE)
	{
		pFont = (FONTDATA *)Resource::Alloc( sizeof(FONTDATA) );
		if (pFont == NULL)
			ThrowError("Error allocating FNT resource", ES_ERROR );

		memcpy(pFont->header.signature, kFontSig, sizeof(pFont->header.signature));
		pFont->header.version = kFontVersion;

		// create and null terminate the internal font name
		memset(pFont->name, 0, kFontNameSize);
		strncpy(pFont->name, zName, kFontNameSize);
		*(pFont->name + kFontNameSize) = '\0';

		pFont->charSpace = 0;
		pFont->lineSpace = 0;
		pFont->wordSpace = 0;
		pFont->firstTile = -1;

		// clear the offset and width tables
		for (int i = 0; i < SCHAR_MAX; i++ )
		{
			pFont->tileOffset[i] = -1;
			pFont->tileWidth[i] = 0;
		}
	}
}


//			// overlay the tile number
//			if ( !keystatus[KEY_CAPSLOCK] )
//			{
//				sprintf(buffer, "%d", nTile);
//				Video.FillBox(0, x, y, x + strlen(buffer) * 4 + 1, y + 7);
//				printext256(x + 1, y, gStdColor[14], -1, buffer, 1);
//			}


/***********************************************************************
 * GLOBALS
 **********************************************************************/
char gFontName[128];
char buffer[256];
EHF prevErrorHandler;
static BOOL isModified;

FontEdit theFontEditor;


/***********************************************************************
 *
 **********************************************************************/
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


void SaveFont( void )
{
	char filename[128];
	int hFile;

	strcpy(filename, gFontName);
	ChangeExtension(filename, ".FNT");

	hFile = open(filename, O_CREAT | O_WRONLY | O_BINARY | O_TRUNC, S_IWUSR);
	if ( hFile == -1 )
		ThrowError("Error creating FNT file", ES_ERROR);

//	memcpy(gFont->signature, kFNTSig, sizeof(gFont->signature));
//	gFont->version = kFNTVersion;

//	if ( !FileWrite(hFile, gFont, sizeof(FONT) + gFont->frames * sizeof(FRAME)) )
//		goto WriteError;

	close(hFile);
	isModified = FALSE;

	scrSetMessage("Font saved.");
	return;

//WriteError:
//	close(hFile);
//	ThrowError("Error writing FNT file", ES_ERROR);
}


BOOL LoadFont( void )
{
	char filename[128];
	int hFile;
	int nSize;

	strcpy(filename, gFontName);
	ChangeExtension(filename, ".FNT");

	hFile = open(filename, O_RDONLY | O_BINARY);
	if ( hFile == -1 )
		return FALSE;

	nSize = filelength(hFile);

//	if ( !FileRead(hFile, gFont, nSize) )
//		goto ReadError;

//	if (memcmp(gFont->signature, kFNTSig, sizeof(gFont->signature)) != 0)
//	{
//		close(hFile);
//		ThrowError("FNT file corrupted", ES_ERROR);
//	}

/*******************************************************************************
I'm ignoring version information for now.  We'll probably have to support
more versions as we make enhancements to the font editor.
*******************************************************************************/

	close(hFile);
	isModified = FALSE;
	return TRUE;

//ReadError:
//	close(hFile);
//	ThrowError("Error reading FNT file", ES_ERROR);
//	return FALSE;
}


BOOL SaveAs( void )
{
	if ( GetStringBox("Save As", gFontName) )
	{
		// load it if it seems valid
		if (strlen(gFontName) > 0)
		{
			SaveFont();
			return TRUE;
		}
	}
	return FALSE;
}


BOOL LoadAs( void )
{
	if ( GetStringBox("Load Font", gFontName) )
	{
		// load it if it seems valid
		if (strlen(gFontName) > 0)
		{
			LoadFont();
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


void EditLoop( void )
{
	BYTE key, shift, alt, ctrl, pad5;
//	int i;
//	int t = 0;
	int nbuttons;
	static int obuttons = 0;
//	char buffer[256];

	while (1)
	{
		gFrameTicks = gGameClock - gFrameClock;
		gFrameClock += gFrameTicks;
		UpdateBlinkClock(gFrameTicks);

		clearview(0);

//		gFont->DrawTiles();

//		sprintf(buffer,"%2i:%03i [%3i]", t / kTimerRate, t % kTimerRate, step);
//		printext256(270, 190, gStdColor[15], -1, buffer, 1);

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
				keystatus[KEY_ESC] = 0;
				if (!isModified || ChangeAcknowledge())
				{
//					free(gFont);
//					gFont = NULL;
					return;
				}
				break;

			case KEY_SPACE:
//				PlayIt();
				break;

			case KEY_F2:
				if (keystatus[KEY_LCTRL] || keystatus[KEY_RCTRL] || strlen(gFontName) == 0)
					SaveAs();
				else
					SaveFont();
				break;

			case KEY_F3:
				if (!isModified || ChangeAcknowledge())
				{
					LoadAs();
//					t = 0;
//					GetFrameTables(t);
				}
				break;

			case KEY_INSERT:
			{
//				// find first frame starting after t
//				for (i = 0; i < gFont->frames; i++)
//				{
//					if (gAnim->frame[i].timeStart > t)
//						break;
//				}
//
//				if (i < gAnim->frames)
//					memmove(&gAnim->frame[i+1], &gAnim->frame[i], (gAnim->frames - i) * sizeof(FRAME));
//
//				gAnim->frames++;
//
//				memset(&gAnim->frame[i], 0, sizeof(FRAME));
//				gAnim->frame[i].timeStart = t;
//				gAnim->frame[i].timeStop = t + step;
//				gAnim->frame[i].type = kQFrameTile;
//				gAnim->frame[i].flags = 0;
//				gAnim->frame[i].x = 0;
//				gAnim->frame[i].y = 0;
//
//				lastTile = gAnim->frame[i].id = tilePick((short)lastTile, SS_ALL);
//
//				GetFrameTables(t);
				isModified = TRUE;
				break;
			}

			case KEY_DELETE:
				// ***
//				--gAnim->frames;
//				if (editFrame[edit] < &gAnim->frame[gAnim->frames])
//					memmove(editFrame[edit], editFrame[edit] + 1, (&gAnim->frame[gAnim->frames] - editFrame[edit]) * sizeof(FRAME));
//				GetFrameTables(t);
				isModified = TRUE;
				break;

			case KEY_G:
				GetNumberBox("Go To Character (0..126)", 0, 0 );
				break;

			case KEY_P:
//				if (edit >= 0 && editFrame[edit]->type == kQFrameTile)
//				{
//					// set the palette for the highlighted tile frame
//					if ( alt )
//						editFrame[edit]->pal = GetNumberBox("Palookup", editFrame[edit]->pal, editFrame[edit]->pal );
//					else
//						editFrame[edit]->pal = IncRotate(editFrame[edit]->pal, kMaxPLU);
//					isModified = TRUE;
//				}
				break;

			case KEY_T:
//				if (edit >= 0 && editFrame[edit]->type == kQFrameTile)
//				{
//					// set translucency flags for the highlighted tile frame
//					int flags = editFrame[edit]->flags;
//					int level = 0;
//
//					if ( flags & kQFrameTranslucent )
//					{
//						level = 1;
//						if ( flags & kQFrameTranslucentR )
//							level = 2;
//					}
//					level = IncRotate(level, 3);
//
//					switch ( level )
//					{
//						case 0:
//							flags &= ~kQFrameTranslucent & ~kQFrameTranslucentR;
//							break;
//
//						case 1:
//							flags &= ~kQFrameTranslucentR;
//							flags |= kQFrameTranslucent;
//							break;
//
//						case 2:
//							flags |= kQFrameTranslucent | kQFrameTranslucentR;
//							break;
//					}
//					editFrame[edit]->flags = flags;
//					isModified = TRUE;
//				}
				break;

			case KEY_V:
//				if (edit >= 0 && editFrame[edit]->type == kQFrameTile)
//				{
//					// pick a tile for the tile frame
//					lastTile = editFrame[edit]->id = tilePick((short)editFrame[edit]->id, SS_ALL);
//					isModified = TRUE;
//				}
				break;

			case KEY_PADPLUS:
//				if (edit >= 0 && editFrame[edit]->type == kQFrameTile)
//				{
//					editFrame[edit]->shade = ClipLow( editFrame[edit]->shade - 1, -128 );
//					isModified = TRUE;
//				}
				break;

			case KEY_PADMINUS:
//				if (edit >= 0 && editFrame[edit]->type == kQFrameTile)
//				{
//					editFrame[edit]->shade = ClipHigh( editFrame[edit]->shade + 1, 127 );
//					isModified = TRUE;
//				}
				break;

			case KEY_PAD0:
//				if (edit >= 0 && editFrame[edit]->type == kQFrameTile)
//				{
//					editFrame[edit]->shade = 0;
//					isModified = TRUE;
//				}
				break;

			case KEY_TAB:
				SetBlinkOff();
				isModified = TRUE;
				break;

			case KEY_UP:
				SetBlinkOff();
				isModified = TRUE;
				break;

			case KEY_DOWN:
				SetBlinkOff();
				isModified = TRUE;
				break;

			case KEY_LEFT:
				SetBlinkOff();
				isModified = TRUE;
				break;

			case KEY_RIGHT:
				SetBlinkOff();
				isModified = TRUE;
				break;
		}

		Mouse::Read(gFrameTicks);

		// which buttons just got pressed
		nbuttons = (short)(~obuttons & Mouse::buttons);
		obuttons = Mouse::buttons;

//		if ( (nbuttons & 2) && editFrames > 1)
//		{
//			edit = IncRotate(edit, editFrames);
//			SetBlinkOn();
//		}

		if ( Mouse::buttons & 1 )
		{
//			if ( keystatus[KEY_O] )
//			{
//				gAnim->SetOrigin(gAnim->origin.x + Mouse::dX2, gAnim->origin.y + Mouse::dY2);
//				isModified = TRUE;
//			}
//			else
//			{
//				SetBlinkOff();
//
//				if (edit >= 0)
//				{
//					editFrame[edit]->x += Mouse::dX2;
//					editFrame[edit]->y += Mouse::dY2;
//					isModified = TRUE;
//				}
//			}
		}

		// make sure x and y don't cause the tile to go off screen
//		if (edit >= 0)
//			gAnim->ClipFrameTile(editFrame[edit]);
	}
}


void main( int argc, char *argv[] )
{
	char title[256];

	gOldDisplayMode = getvmode();

	sprintf(title, "Ye Olde Font Editor [%s] -- DO NOT DISTRIBUTE", gBuildDate);
	tioInit();
	tioCenterString(0, 0, tioScreenCols - 1, title, kAttrTitle);
	tioCenterString(tioScreenRows - 1, 0, tioScreenCols - 1,
		"Copyright (c) 1995 Q Studios Corporation", kAttrTitle);

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

	strcpy(gFontName, "");

	isModified = FALSE;

//	gAnim = (EditAnim *)malloc(sizeof(EditAnim) + kMaxFrames * sizeof(FRAME));
//	dassert(gAnim != NULL);

	if ( argc > 1 )
	{
 		strcpy(gFontName, argv[1]);
		theFontEditor.Load(argv[1]);
	}
	else
	{
//		gAnim->frames = 0;
//		gAnim->duration = 0;
//		gAnim->origin.x = 160;
//		gAnim->origin.y = 100;
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
