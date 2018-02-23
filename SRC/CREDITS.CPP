/*******************************************************************************
	FILE:			CREDITS.CPP

	DESCRIPTION:

	AUTHOR:			Peter M. Freese
	CREATED:		12-05-95
	COPYRIGHT:		Copyright (c) 1995 Q Studios Corporation
*******************************************************************************/

#include "credits.h"
#include "key.h"
#include "sound.h"
#include "screen.h"
#include "globals.h"
#include "misc.h"
#include "view.h"
#include "gfx.h"
#include "trig.h"
#include "debug4g.h"

#define kDACWaitTicks	2	// ticks between DAC updates

char *credits[] =
{
	"Game By",
	"Q Studios",
	"",
	"Project Director",
	"Nick Newhard",
	"",
	"Executive Producer",
	"George Broussard",
	"",
	"Lead Programmer",
	"Peter Freese",
	"",
	"3D Engine",
	"Ken Silverman",
	NULL,
	"Level Design",
	"Jay Wilson",
	"Nick Newhard",
	"Terry Hamel",
	"Kevin Kilstrom",
	"Richard Gray",
	"Vance Andrew Blevins",
	NULL,
	"Artists",
	"Kevin Kilstrom",
	"Terry Hamel",
	"",
	"Creature Models",
	"Kevin Kilstrom",
	"",
	"Additional Art",
	"Rick Welsh",
	"David Brinkley",
	NULL,
	"Music and Sound",
	"LoudMouth",
	"",
	"3D Sound System",
	"Peter Freese",
	"",
	"Audio Technology",
	"Jim Dose",
	"",
	"Network Programming",
	"Mark Dochterman",
	NULL,
	"Special Thanks",
	"Chris Rush",
	"Erik Klokstad",
	"Helen Newhard",
	"Cynthia Freese",
	"Francine Kilstrom",
};

static BOOL Wait( int nTicks )
{
	gGameClock = 0;
	while ( gGameClock < nTicks )
		if ( keyGet() != 0 )
			return FALSE;
	return TRUE;
}


static BOOL DoFade( BYTE r, BYTE g, BYTE b, int nTicks )
{
	dassert(nTicks > 0);
	scrSetupFade(r, g, b);
	gGameClock = gFrameClock = 0;
	do
	{
		while (gGameClock < gFrameClock);
		gFrameClock += kDACWaitTicks;

		WaitVSync();
		scrFadeAmount(divscale16(ClipHigh(gGameClock, nTicks), nTicks));

		if ( keyGet() != 0 )
			return FALSE;
	} while ( gGameClock <= nTicks );
	return TRUE;
}

static BOOL DoUnFade( int nTicks )
{
	dassert(nTicks > 0);
	scrSetupUnfade();
	gGameClock = gFrameClock = 0;
	do
	{
		while (gGameClock < gFrameClock);
		gFrameClock += kDACWaitTicks;

		WaitVSync();
		scrFadeAmount(0x10000 - divscale16(ClipHigh(gGameClock, nTicks), nTicks));

		if ( keyGet() != 0 )
			return FALSE;
	} while ( gGameClock <= nTicks );
	return TRUE;
}

// show logos
void credLogos( void )
{
	int nEffect = 0;

//	sndPlaySong("INTRO", FALSE);

	// black out dac
	if ( !DoFade(0, 0, 0, 1) ) return;

	// castle background
	rotatesprite(320 << 15, 200 << 15, 0x10000, 0, 2046, 0, 0,
		kRotateScale | kRotateNoMask, 0, 0, 319, 199);
	scrNextPage();

	if ( !DoUnFade(2 * kTimerRate) ) return;
	if ( !Wait(1 * kTimerRate) ) return;

	if ( !DoFade(255, 255, 255, 2) ) return;

/*
	// flash the subliminal babe
	rotatesprite(320 << 15, 200 << 15, 0x10000, 0, 2323, 0, 0,
		kRotateScale | kRotateNoMask, 0, 0, 319, 199);
	scrNextPage();
	DoUnFade(1);
	DoFade(255, 255, 255, 1);
*/

	// castle again
	rotatesprite(320 << 15, 200 << 15, 0x10000, 0, 2046, 0, 0,
		kRotateScale | kRotateNoMask, 0, 0, 319, 199);
	// Q Studios logo
	rotatesprite(320 << 15, 200 << 15, 0x10000, 0, 2047, 0, 0,
		kRotateScale | kRotateNoMask, 0, 0, 319, 199);
	scrNextPage();

	sndStartSample("THUNDER2", 255);

	if ( !DoUnFade(60) ) return;

	if ( !Wait(1 * kTimerRate) ) return;
	sndStartSample("THUNDER", 255);
	if ( !Wait(1 * kTimerRate) ) return;
	if ( !DoFade(0, 0, 0, 60) ) return;

	// menu background
	rotatesprite(320 << 15, 200 << 15, 0x10000, 0, 2049, 0, 0,
		kRotateScale | kRotateNoMask, 0, 0, 319, 199);
	scrNextPage();

	if ( !DoUnFade(60) ) return;
	sndStartSample("DOORSLID", 255);
	if ( !Wait(2 * kTimerRate) ) return;
	if ( !DoFade(0, 0, 0, 60) ) return;

	while ( sndIsSongPlaying() )
	{
		if (keyGet() != 0 )
			return;
	}
}


static QFONT *pFont;
static int fuseX, fuseY;


static void DrawCreditText( char *string, int x, int y, int nAlign )
{
	char *s;
	dassert(string != NULL);

	setupmvlineasm(16);

	if ( nAlign != TA_LEFT )
	{
		int nWidth = -pFont->charSpace;
		for ( s = string; *s; s++ )
			nWidth += pFont->info[*s].hspace + pFont->charSpace;

		if ( nAlign == TA_CENTER )
			nWidth >>= 1;

		x -= nWidth;
	}

	for ( s = string; *s; s++ )
	{
		int nShade = ClipRange(qdist(x - fuseX, y - fuseY) - 16, 0, 56);
		BYTE *pPalookup = palookup[kPLUNormal] + (getpalookup(0, nShade) << 8);
		viewDrawChar(pFont, *s, x, y, pPalookup);
		x += pFont->info[*s].hspace + pFont->charSpace;
	}
}


#define kLineSpace	14


// show credits
void credCredits( void )
{
	int n = 0;
	int x, y;

	fuseX = -256;
	fuseY = -256;

	RESHANDLE hFont = gSysRes.Lookup((long)kFontMessage, ".QFN");
	dassert(hFont != NULL);
	pFont = (QFONT *)gSysRes.Lock(hFont);

	sndPlaySong("BLOOD5", FALSE);

	// black out dac
	if ( !DoFade(0, 0, 0, 1) ) return;

	while ( n < LENGTH(credits) )
	{
		// calculate number of lines to display
		int nLines = 0;
		while ( credits[n + nLines] != NULL )
			nLines++;

		// draw the screen
		clearview(0);
		y = (200 - nLines * kLineSpace) / 2;
		for ( int i = 0; i < nLines; i++ )
		{
			DrawCreditText(credits[n + i], 160, y, TA_CENTER);
			y += kLineSpace;
		}
		scrNextPage();

		DoUnFade(60);

		int y0 = Random(200);
		int y1 = Random(200);
		int nFrame = 0;
		for ( x = -32; x < 320 + 32; x += 1 )
		{
			fuseX = x;
			fuseY = y0 + muldiv(y1 - y0, x, 320);

			clearview(0);
			y = (200 - nLines * kLineSpace) / 2;
			for ( int i = 0; i < nLines; i++ )
			{
				DrawCreditText(credits[n + i], 160, y, TA_CENTER);
				y += kLineSpace;
			}

/*
			nFrame = IncRotate(nFrame, 3);

			rotatesprite(fuseX << 16, fuseY << 16, 0x10000 + BiRandom(0x1000),
			(short)BiRandom(kAngle30), (short)(3195 + nFrame),
			-128, kPLUNormal, kRotateStatus, 0, 0, xdim - 1, ydim - 1);
*/
			scrNextPage();

			Wait(3);
		}

		DoFade(0, 0, 0, 60);

		n += nLines + 1;
	}
	sndFadeSong(400);

	gSysRes.Unlock(hFont);
}




