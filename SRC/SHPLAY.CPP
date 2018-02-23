/*******************************************************************************
	FILE:			SHPLAY.CPP

	DESCRIPTION:	Sonic Holography 3D sound player

	AUTHOR:			Peter M. Freese
	CREATED:		11-09-95
	COPYRIGHT:		Copyright (c) 1995 Q Studios Corporation
*******************************************************************************/
#include <conio.h>
#include <dos.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>

#include "debug4g.h"
#include "error.h"
#include "fx_man.h"
#include "getopt.h"
#include "inifile.h"
#include "key.h"
#include "misc.h"
#include "music.h"
#include "resource.h"
#include "task_man.h"
#include "textio.h"
#include "typedefs.h"
#include "usrhooks.h"
#include "helix.h"
#include "gfx.h"
#include "pcx.h"


#define kAspectAdjustX	1.2

#define kPi				3.141592654
#define kPi2			(kPi * 2)

#define kAngle360		kPi2
#define kAngle180		(kAngle360 / 2)
#define kAngle90		(kAngle360 / 4)
#define kAngle60		(kAngle360 / 6)
#define kAngle45		(kAngle360 / 8)
#define kAngle30		(kAngle360 / 12)
#define kAngle20		(kAngle360 / 18)
#define kAngle15		(kAngle360 / 24)
#define kAngle10		(kAngle360 / 36)
#define kAngle5			(kAngle360 / 72)

#define kTimerRate		30
#define kSampleRate		11110
#define kIntraAuralTime	0.0006
#define kVolScale		10

#define kMaxPlaySounds	255


struct MENU_LINE
{
	int id;
	char *name;
	char *hint;
};

Resource gResource;

IniFile config("SHPLAY.INI");

MENU_LINE SoundCardMenu[] =
{
	{ -1,				"None",					"No sound card" },
	{ SoundBlaster,		"SoundBlaster",			"Sound Blaster, SB Pro, SB 16, or compatible" },
	{ ProAudioSpectrum,	"Pro Audio Spectrum",	"Media Vision Pro Audio Sprectrum" },
	{ SoundMan16,		"Sound Man 16",			"Logitech Sound Man 16" },
	{ Awe32,			"Awe 32",				"Sound Blaster Awe 32" },
	{ SoundScape,		"SoundScape",			"Ensoniq Sound Scape" },
	{ UltraSound,		"Gravis UltraSound",	"Advanced Gravis UltraSound" },
	{ SoundSource,		"Disney Sound Source",	"Disney Sound Source" },
	{ TandySoundSource,	"Tandy Sound Source",	"Tandy Sound Source" },
	{ 0, NULL, NULL },
};

MENU_LINE BlasterTypeMenu[] =
{
	{ fx_SB,		"Sound Blaster or compatible",	"Vanilla Sound Blaster Card" },
	{ fx_SB20,		"Sound Blaster 2.0",			"Sound Blaster 2.0 Card" },
	{ fx_SBPro,		"Sound Blaster Pro (old)",		"Original Sound Blaster Pro" },
	{ fx_SBPro2,	"Sound Blaster Pro (new)",		"Newer Sound Blaster Pro" },
	{ fx_SB16,		"Sound Blaster 16 or AWE32",	"Sound Blaster 16 or AWE 32" },
	{ 0, NULL, NULL },
};

MENU_LINE BlasterAddressMenu[] =
{
	{ 0x210,	"210h",				"Sound Blaster Port Address at 210h" },
	{ 0x220,	"220h (default)",	"Sound Blaster Port Address at 220h" },
	{ 0x230,	"230h",				"Sound Blaster Port Address at 230h" },
	{ 0x240,	"240h",				"Sound Blaster Port Address at 240h" },
	{ 0x250,	"250h",				"Sound Blaster Port Address at 250h" },
	{ 0x260,	"260h",				"Sound Blaster Port Address at 260h" },
	{ 0x270,	"270h",				"Sound Blaster Port Address at 270h" },
	{ 0x280,	"280h",				"Sound Blaster Port Address at 280h" },
	{ 0, NULL, NULL },
};

MENU_LINE BlasterIRQMenu[] =
{
	{ 2,	"IRQ 2",			"Sound Blaster Set to IRQ 2" },
	{ 3,	"IRQ 3",			"Sound Blaster Set to IRQ 3" },
	{ 5,	"IRQ 5 (default",	"Sound Blaster Set to IRQ 5" },
	{ 7,	"IRQ 7",			"Sound Blaster Set to IRQ 7" },
	{ 10,	"IRQ 10",			"Sound Blaster Set to IRQ 10" },
	{ 11,	"IRQ 11",			"Sound Blaster Set to IRQ 11" },
	{ 12,	"IRQ 12",			"Sound Blaster Set to IRQ 12" },
	{ 15,	"IRQ 15",			"Sound Blaster Set to IRQ 15" },
	{ 0, NULL, NULL },
};

MENU_LINE BlasterDma8Menu[] =
{
	{ 0,	"Channel 0",			"8 bit DMA on channel 0" },
	{ 1,	"Channel 1 (default)",	"8 bit DMA on channel 1" },
	{ 3,	"Channel 3",			"8 bit DMA on channel 3" },
	{ 0, NULL, NULL },
};

MENU_LINE BlasterDma16Menu[] =
{
	{ 5,	"Channel 5 (default)",	"16 bit DMA on channel 5" },
	{ 6,	"Channel 6",			"16 bit DMA on channel 6" },
	{ 7,	"Channel 7",			"16 bit DMA on channel 7" },
	{ 0, NULL, NULL },
};

MENU_LINE BitMenu[] =
{
	{ 8,	"8-bit",	"8-bit low quality mixing" },
	{ 16,	"16-bit",	"16-bit high quality mixing" },
	{ 0, NULL, NULL },
};

MENU_LINE ChannelMenu[] =
{
	{ 1,	"Mono",		"Mono sound, no stereo imaging" },
	{ 2,	"Stereo",	"Stereo sound, full stereo imaging" },
	{ 0, NULL, NULL },
};

MENU_LINE VoiceMenu[] =
{
	{ 8,	"8",		"8 voice mixing" },
	{ 16,	"16",		"16 voice mixing" },
	{ 24,	"24",		"24 voice mixing" },
	{ 32,	"32",		"32 voice mixing" },
	{ 0, NULL, NULL },
};

MENU_LINE MixRateMenu[] =
{
	{ 11110,	"11 KHz",	"Mix at 11 KHz" },
	{ 22220,	"22 KHz",	"Mix at 22 KHz" },
	{ 44440,	"44 KHz",	"Mix at 44 KHz" },
	{ 0, NULL, NULL },
};

BOOL		bSoundWildcard = FALSE;
RESHANDLE	hSound[kMaxPlaySounds];
char		*soundName[kMaxPlaySounds];
int			soundCount = 0;
int			soundCursor = 0;

BYTE *background = NULL, *stage = NULL, *video = (BYTE *)0xA0000;

#define kNumBuffers 34
BYTE *buffers[kNumBuffers];

int gFXDevice;
int gMixBits;
int gChannels;
int gMixVoices;
int gMixRate;
int gFXVolume;
fx_blaster_config gSBConfig;

int fHolography = TRUE;

double gPinnaAngle = kAngle45;
double nSat = 0.8;

static int lVoice, rVoice;
static int lPhase, rPhase, lVel, rVel, lVol, rVol;
int x, y, xold, yold;

volatile short gMouseX, gMouseY;
BOOL gMouseDragging = FALSE;
int gMouseShown = FALSE;

EHF prevErrorHandler;

/*---------------------------------------------------------------------
** Function Prototypes
---------------------------------------------------------------------*/
void ProcessArgument(char *s);


/*---------------------------------------------------------------------
   Function: USRHOOKS_GetMem

   Allocates the requested amount of memory and returns a pointer to
   its location, or NULL if an error occurs.  NOTE: pointer is assumed
   to be dword aligned.
---------------------------------------------------------------------*/

int USRHOOKS_GetMem( void **ptr, unsigned long size )
{
	void *memory;

	memory = Resource::Alloc( size );
	if ( memory == NULL )
	{
		return( USRHOOKS_Error );
	}

	*ptr = memory;
	return( USRHOOKS_Ok );
}


/*---------------------------------------------------------------------
   Function: USRHOOKS_FreeMem

   Deallocates the memory associated with the specified pointer.
---------------------------------------------------------------------*/

int USRHOOKS_FreeMem( void *ptr )
{
	if ( ptr == NULL )
    {
		return( USRHOOKS_Error );
    }

	Resource::Free( ptr );
	return( USRHOOKS_Ok );
}


void mouGetPos( void );
#pragma aux mouseGetPos =\
	"mov	ax,03h",\
	"int 	33h",\
	"shr	cx,1",\
	"mov	[gMouseX],cx",\
	"mov	[gMouseY],dx",\
	modify [eax ecx edx ebx]\

char mouseGetButtons( void );
#pragma aux mouseGetButtons =\
	"mov ax,03h",\
	"int 33h",\
	"xor eax,eax",\
	"mov al,bl",\
	modify [eax ebx ecx edx]


inline void pixel( BYTE *buffer, int x, int y, char c )
{
   	*(char *)(buffer + x + gYLookup[y]) = c;
}


/*---------------------------------------------------------------------
   Function: plot

   XORs a 4x4 block onto the screen.
---------------------------------------------------------------------*/

void plot( BYTE *buffer, int x, int y, char c )
{
   	int i;
	long d = (c << 24) | (c << 16) | (c << 8) | c;
	long *v = (long *)(buffer + x + y * 320);

	for( i = 0; i < 4; i++)
	{
		*v = d;
		v += 80;
	}
}


double Vol3d(double angle, double vol)
{
	if (fHolography)
		return vol * (1.0 - (1.0 - cos(angle)) * nSat / 2.0);
	else
		return vol * (cos(angle) * 0.5 + 0.5);
}


int Dist3d(int x, int y)
{
	return sqrt(x * x + y * y);
}


double CalcAngle( int x, int y )
{
	if (y < 0)
		return atan((double)x / (double)y);

	if (y > 0)
		return kPi + atan((double)x / (double)y);

	if (x < 0)
		return kPi / 2;

	return -kPi / 2;
}

/*******************************************************************************

Here is the equation for determining the frequency shift of a sound:

	            Ú        ¿
	            ³ v + vL ³
	    fL = fS ³ ÄÄÄÄÄÄ ³
	            ³ v - vS ³
	            À        Ù

Where:

	fL	= frequency as heard by the listener
	fS	= frequency of the source
	v	= velocity of sound
	vL	= velocity of listener
	vS	= velocity of source

Speed of sound = 343 m/s = 1126 ft/s.

*******************************************************************************/


#define kSoundVelocity	((double)kSampleRate / kTimerRate)
int Freq( int velocity )
{
	return kSampleRate * kSoundVelocity / (ClipLow(kSoundVelocity - velocity, 1));
}


// ear distance (in samples)
#define kEarLX (int)(-kIntraAuralTime / 2 * kSampleRate)
#define kEarRX (int)(+kIntraAuralTime / 2 * kSampleRate)

void MoveSource( void )
{
	int lDist, rDist;
	double angle, vol;

	xold = x;
	yold = y;
	x = gMouseX - 160;
	y = gMouseY - 100;

	angle = CalcAngle(x, y);

	if (fHolography)
	{
		lDist = Dist3d(x - kEarLX, y);
		lVel = Dist3d(xold - kEarLX, yold) - lDist;
		vol = kVolScale * 255 / ClipLow(lDist, 1);
		lVol = Vol3d(angle - gPinnaAngle, vol);
		lPhase = lDist;

		rDist = Dist3d(x - kEarRX, y);
		rVel = Dist3d(xold - kEarRX, yold) - rDist;
		vol = kVolScale * 255 / ClipLow(rDist, 1);
		rVol = Vol3d(angle + gPinnaAngle, vol);
		rPhase = rDist;
	}
	else
	{
		vol = kVolScale * 255 / ClipLow(Dist3d(x, y), 1);
		if (vol > 255) vol = 255;

		lVol = Vol3d(angle - kAngle90, vol);
		rVol = Vol3d(angle + kAngle90, vol);

		lVel = rVel = 0;
		lPhase = rPhase = 0;
	}
}


/*---------------------------------------------------------------------
   Function: PlaySound

   Starts playback of a sound.
---------------------------------------------------------------------*/

void PlaySound( int num )
{
	int nLength;
	BYTE *vl, *vr;
	int id;

	BYTE *soundData = (BYTE *)gResource.Lock(hSound[num]);
	dassert(soundData != NULL);

	if ( mouseGetButtons() )
		gMouseDragging = TRUE;

	nLength = gResource.Size(hSound[num]);

	vl = (BYTE *)Resource::Alloc(nLength + lPhase);
	dassert (vl != NULL);

	memset(vl, 128, lPhase);
	memcpy(&vl[lPhase], soundData, nLength);

	vr = (BYTE *)Resource::Alloc(nLength + rPhase);
	dassert(vr != NULL);

	memset(vr, 128, rPhase);
	memcpy(&vr[rPhase], soundData, nLength);

	gResource.Unlock(hSound[num]);

	_disable();	// make sure the samples start in the same interval

	int nPriority = 1;	// ASS seems to choke if this is less than 1, e.g., 0
	if ( lVol > nPriority )
		nPriority = lVol;
	if ( rVol > nPriority )
		nPriority = rVol;

	for (id = 0; buffers[id] != 0; id++);
	lVoice = FX_PlayRaw( vl, nLength, Freq(lVel), 0, lVol, lVol, 0, nPriority, id);

	if ( lVoice > FX_Ok )
		buffers[id] = vl;
	else
		Resource::Free(vl);

	for (id = 0; buffers[id] != 0; id++);
	rVoice = FX_PlayRaw( vr, nLength, Freq(rVel), 0, rVol, 0, rVol, nPriority, id);

	if ( rVoice > FX_Ok )
		buffers[id] = vr;
	else
		Resource::Free(vr);

	_enable();	// allow them to start
}


void UpdateSound( void )
{
	if (lVoice > FX_Ok)
	{
		FX_SetPan(lVoice, lVol, lVol, 0);
		FX_SetFrequency(lVoice, Freq(lVel));
	}

	if (rVoice > FX_Ok)
	{
		FX_SetPan(rVoice, rVol, 0, rVol);
		FX_SetFrequency(rVoice, Freq(rVel));
	}
}


/*---------------------------------------------------------------------
   Function: DrawMark

   Displays the position the voice will appear in relation to the
   listener.
---------------------------------------------------------------------*/

void DrawMark( int fErase )
{
	static int  lastx = 0;
	static int  lasty = 0;
	static char c = 16;
	int newx, newy;
	BYTE *v;
	int i, j;

	newx = gMouseX - 1;
	if (newx < 0) newx = 0;
	if (newx > 317) newx = 317;

	newy = gMouseY - 1;
	if (newy < 0) newy = 0;
	if (newy > 197) newy = 197;

	// clear the old one
	if (fErase)
	{
		v = video + lastx + lasty * 320;
		for( i = 0; i < 3; i++)
		{
			for (j = 0; j < 3; j++)
				*v++ ^= c;
			v += 320 - 3;
		}
	}

	if (++c == 32)
		c = 16;

	// draw the new one
	v = video + newx + newy * 320;
	for( i = 0; i < 3; i++)
	{
		for (j = 0; j < 3; j++)
			*v++ ^= c;
		v += 320 - 3;
	}

	lastx = newx;
	lasty = newy;
}



/*---------------------------------------------------------------------
   Function: DrawDemoScreen

   Sets up our information screen for the user.  Displays last error,
   what sounds are playing, and the volume levels.
---------------------------------------------------------------------*/

void DrawDemoScreen( void )
{
	int x;
	double a;
	double lSat, rSat, nCos, nSin;

	// make sure the mouse doesn't get drawn while we're setting up
	gMouseShown = FALSE;

	// clear screen
	memcpy(stage, background, 320 * 200);

	for ( a = 0.0; a < kAngle360; a += kAngle5 )
	{
		if (fHolography)
		{
			lSat = Vol3d(a - gPinnaAngle, 25);
			rSat = Vol3d(a + gPinnaAngle, 25);
		}
		else
		{
			lSat = Vol3d(a - kAngle90, 25);
			rSat = Vol3d(a + kAngle90, 25);
		}

		nSin = sin(a);
		nCos = cos(a);

		pixel(stage, 160 - nSin * lSat, 100 - nCos * lSat, 8);
		pixel(stage, 160 - nSin * rSat, 100 - nCos * rSat, 8);
		pixel(stage, 160 - nSin * (lSat + rSat) * kAspectAdjustX, 100 - nCos * (lSat + rSat), 14);
	}

	// draw the holography toggle
	plot( stage, 135, 6, fHolography ? 14 : 8 );

	// Draw volume display
   	for( x = 0; x < 10; x++ )
	{
    	plot( stage, 240 + x * 7, 6, gFXVolume > x ? 10 : 8 );
	}

	// draw the stage
	memcpy(video, stage, 320 * 200);

	// draw the initial cursor
	DrawMark(0);

	// okay, we're ready for the mouse
	gMouseShown = TRUE;
}



/*---------------------------------------------------------------------
   Function: TimerFunc
---------------------------------------------------------------------*/

void TimerFunc( TASK * /* task */ )
{
	mouseGetPos();
	if ( gMouseDragging && !mouseGetButtons() )
		gMouseDragging = FALSE;

	if (gMouseShown)
	{
		DrawMark(1);
		MoveSource();

		if ( gMouseDragging )
		{
			UpdateSound();
		}
	}
}


/*---------------------------------------------------------------------
   Function: SoundCallback

   When a sound fx stops playing, this function is called.
---------------------------------------------------------------------*/

void SoundCallback(unsigned long id)
{
	if (buffers[id] != NULL)
	{
		Resource::Free(buffers[id]);
		buffers[id] = NULL;
	}
	else
		dprintf("SoundCallback() called for non-playing sample\n");
}


void ShowSoundName( int n )
{
	char zbuffer[60];
	sprintf(zbuffer,"%-12s : %i of %i", soundName[n], soundCursor + 1, soundCount );

	Video.SetColor(0);
	gfxFillBox(0, 192, 320, 200);
	gfxDrawText(0, 192, 15, zbuffer);
}


/*---------------------------------------------------------------------
   Function: CheckKeys

   Checks which keys are hit and responds appropriately.
---------------------------------------------------------------------*/

BYTE CheckKeys( void )
{
	BYTE key;

	key = keyGet();
	switch( key )
	{
		case KEY_SPACE:
	        PlaySound(soundCursor);
	        break;

		case KEY_1:
		case KEY_2:
		case KEY_3:
		case KEY_4:
		case KEY_5:
		case KEY_6:
		case KEY_7:
		case KEY_8:
		case KEY_9:
		case KEY_0:
		{
			int nSound = soundCursor + key - KEY_1;
			if (nSound < soundCount)
	        	PlaySound(nSound);
	        break;
		}

		case KEY_UP:
		case KEY_PADUP:
			soundCursor = ClipLow(soundCursor - 1, 0);
			ShowSoundName(soundCursor);
			break;

		case KEY_DOWN:
		case KEY_PADDOWN:
			soundCursor = ClipHigh(soundCursor + 1, soundCount - 1);
			ShowSoundName(soundCursor);
			break;

		case KEY_PAGEUP:
		case KEY_PADPAGEUP:
			soundCursor = ClipLow(soundCursor - 10, 0);
			ShowSoundName(soundCursor);
			break;

		case KEY_PAGEDN:
		case KEY_PADPAGEDN:
			soundCursor = ClipHigh(soundCursor + 10, soundCount - 1);
			ShowSoundName(soundCursor);
			break;

		case KEY_MINUS:
			if (gFXVolume > 0)
			{
				gFXVolume--;
				FX_SetVolume(gFXVolume * 255 / 10);
				plot( video, 240 + gFXVolume * 7, 6, 8 );
			}
			break;

		case KEY_EQUALS:
			if (gFXVolume < 10)
			{
				plot( video, 240 + gFXVolume * 7, 6, 10 );
				gFXVolume++;
				FX_SetVolume(gFXVolume * 255 / 10);
			}
			break;

		case KEY_LBRACE:
			if ( gPinnaAngle > 0 )
			{
				gPinnaAngle -= kAngle5;
				DrawDemoScreen();
			}
			break;

		case KEY_RBRACE:
			if ( gPinnaAngle < kAngle90 )
			{
				gPinnaAngle += kAngle5;
				DrawDemoScreen();
			}
			break;

		case KEY_COMMA:
			if ( nSat > 0 )
			{
				nSat -= 0.05;
				DrawDemoScreen();
			}
			break;

		case KEY_PERIOD:
			if ( nSat < 1.0 )
			{
				nSat += 0.05;
				DrawDemoScreen();
			}
			break;

		case KEY_K:
        	 FX_StopAllSounds();
         	break;

      	case KEY_S:
        	fHolography = !fHolography;
         	DrawDemoScreen();
         	break;
	}
	return key;
}


/*---------------------------------------------------------------------
   Function: DoDemo

   Main loop for our demo.  Sets up a timer so that marker moves at
   same rate on any computer.
---------------------------------------------------------------------*/
void DoDemo( void )
{
	TASK *taskPtr = NULL;
	int   Timer;
	int   i;
	int nOldMode;
	int width = 320, height = 200;
	PALETTE palette;

	background = (BYTE *)Resource::Alloc(320 * 200);
	dassert(background != NULL);

	if (PCX_OKAY != ReadPCX("screen.pcx", palette, &width, &height, &background))
		ThrowError("Error reading screen.pcx\n", ES_ERROR);

	stage = (BYTE *)Resource::Alloc(320 * 200);
	dassert(stage != NULL);

	nOldMode = getvmode();

	if (!gFindMode(320, 200, 256, CHAIN256))
		ThrowError("Video driver not linked\n", ES_ERROR);

	Video.SetMode();

	gSetDACRange(0, 256, palette);

	taskPtr = TS_ScheduleTask( TimerFunc, kTimerRate, 1, &Timer );
	TS_Dispatch();
	Timer = 0;

	// set the current volume of the sound fx
	FX_SetVolume( gFXVolume * 255 / 10 );

	// Set up my keyboard handler.
	keyInstall();

	for (i = 0; i < kNumBuffers; i++)
		buffers[i] = NULL;

	// Set up our fx callback so we can display the sounds that are playing
	FX_SetCallBack(SoundCallback);

	soundCursor = 0;
	DrawDemoScreen();
	ShowSoundName(soundCursor);

	while ( CheckKeys() != KEY_ESC );

	// Stop the sound fx engine.
	FX_StopAllSounds();

	// Restore the system keyboard handler
	keyRemove();

	setvmode(nOldMode);

	// Terminate my timer.  Failure to do so could be fatal!!!
	TS_Terminate(taskPtr);
	TS_Shutdown();

	Resource::Free(stage);
}


int SelectMenu( char *title, MENU_LINE list[], int nDefault )
{
	int cols = 0, rows = 0;
	int pos = 0;
	int nListItems;

//	cols = strlen(title);
	(void)title;

	// find widest menu item text
	for ( int i = 0; list[i].name != NULL; i++)
	{
		int n = strlen(list[i].name);
		if (n > cols)
			cols = n;
	}
	nListItems = i;

	cols += 5;
	rows = nListItems + 3;

	char *saveUnder = (char *)Resource::Alloc(rows * cols * 2);
	dassert(saveUnder != NULL);

	int left = (tioScreenCols - cols) / 2;
	int top = (tioScreenRows - rows) / 2;
	tioSaveWindow(saveUnder, top, left, rows, cols);

	tioFrame(top, left, rows - 1, cols - 1, kFrameSingle, kColorBlue * 16 | kColorCyan);
	tioFill(top + 1, left + 1, rows - 3, cols - 3, ' ', kColorBlue * 16 | kColorYellow);
	tioFillShadow(top + rows - 1, left + 1, 1, cols - 1);
	tioFillShadow(top + 1, left + cols - 1, rows - 2, 1);

	for( i = 0; i < nListItems; i++ )
	{
		tioLeftString(top + i + 1, left + 2, list[i].name, kColorBlue * 16 | kColorYellow);

		if ( list[i].id == nDefault )
			pos = i;
	}

	int ch = 0;
	while( ch != 0x0D && ch != 0x20 )
	{
		// highlight current line and display hint
		tioFillAttr(top + pos + 1, left + 1, 1, cols - 3, kColorLightGray * 16 | kColorBlack);
		tioCenterString(tioScreenRows - 1, 0, tioScreenCols - 1,
			list[pos].hint, kColorRed * 16 | kColorYellow);

		ch = getch();
		if ( ch == 0 )
		{
			ch = 0x100 | getch();
		}
		// Up arrow
		if ( ch == 0x148 )
		{
			tioFillAttr(top + pos + 1, left + 1, 1, cols - 3, kColorBlue * 16 | kColorYellow);
			pos = DecRotate(pos, nListItems);
		}
		// Down arrow
		if ( ch == 0x150 )
		{
			tioFillAttr(top + pos + 1, left + 1, 1, cols - 3, kColorBlue * 16 | kColorYellow);
			pos = IncRotate(pos, nListItems);
		}
		// Esc
		if ( ch == 0x1B )
		{
			tioRestoreWindow(saveUnder, top, left, rows, cols);
			Resource::Free(saveUnder);
			return -1;
		}
	}
	tioRestoreWindow(saveUnder, top, left, rows, cols);
	Resource::Free(saveUnder);
	return list[pos].id;
}


/*---------------------------------------------------------------------
   Function: ReadSounds

   Sets up sound resources. Returns number of sounds available.
---------------------------------------------------------------------*/

int ReadSounds( void )
{
	int i;

	for ( i = 0; i < kMaxPlaySounds; i++)
		hSound[i] = NULL;	// safer than a memset(soundHandle,0,sizeof(soundHandle)); ?

	gResource.Init(NULL, "*.RAW");

	if (!bSoundWildcard)
		ProcessArgument("*.RAW");	// default to local .RAW files

	for ( i = 0; i < soundCount; i++)
	{
		char zNameBuffer[_MAX_PATH2];
		char *name, *ext;
		_splitpath2(soundName[i], zNameBuffer, NULL, NULL, &name, &ext);
		hSound[i] = gResource.Lookup(name, ext);
	}

	return soundCount;
}


void GetSoundSettings( void )
{
	FX_GetBlasterSettings(&gSBConfig);

	// Read current setup
	gFXDevice		= config.GetKeyInt("Sound Setup", "FXDevice", SoundBlaster);
	gMixBits		= config.GetKeyInt("Sound Setup", "Bits", 16);
	gChannels		= config.GetKeyInt("Sound Setup", "Channels", 2);
	gMixVoices		= config.GetKeyInt("Sound Setup", "Voices", 16);
	gMixRate		= config.GetKeyInt("Sound Setup", "MixRate", 22050);
	gFXVolume		= config.GetKeyInt("Sound Setup", "FXVolume", 8);

	gSBConfig.Address	= config.GetKeyInt("Sound Setup", "BlasterAddress", gSBConfig.Address);
	gSBConfig.Type		= config.GetKeyInt("Sound Setup", "BlasterType", gSBConfig.Type);
	gSBConfig.Interrupt	= config.GetKeyInt("Sound Setup", "BlasterInterrupt", gSBConfig.Interrupt);
	gSBConfig.Dma8		= config.GetKeyInt("Sound Setup", "BlasterDma8", gSBConfig.Dma8);
	gSBConfig.Dma16		= config.GetKeyInt("Sound Setup", "BlasterDma16", gSBConfig.Dma16);
	gSBConfig.Emu		= config.GetKeyInt("Sound Setup", "BlasterEmu", gSBConfig.Emu);
}


/*---------------------------------------------------------------------
   Function: GetSoundFXCard

   Asks the user which card to use for sound fx and initializes the
   sound fx routines.
---------------------------------------------------------------------*/
BOOL SetupSoundCard( void )
{
	// Draw our config screen
	tioCenterString(0, 0, tioScreenCols - 1, "Q Studios Sonic Holography Setup", kColorBlue * 16 | kColorYellow);
	tioFill( 1, 0, tioScreenRows - 2, tioScreenCols, '°', kColorBlack * 16 | kColorDarkGray);

	// Get Sound fx card
	gFXDevice = SelectMenu("Select a device for digital sound",
		SoundCardMenu, gFXDevice);
	if ( gFXDevice < 0 )
		return FALSE;

	switch ( gFXDevice )
	{
		case SoundBlaster:
		case ProAudioSpectrum:
		case SoundMan16:
		case Awe32:
		case WaveBlaster:
		case SoundScape:
			gMixBits = SelectMenu("Select the number of bits to use for mixing",
				BitMenu, gMixBits);
			if ( gMixBits < 0 )
				return FALSE;

			gChannels = SelectMenu("Select the channels mode",
				ChannelMenu, gChannels);
			if ( gChannels < 0 )
				return FALSE;
			gMixVoices = SelectMenu("Select the number of voices to use for mixing",
				VoiceMenu, gMixVoices);


			break;

		default:
			gMixBits = 8;
			gChannels = 1;
			gMixVoices = 2;
			break;
	}

	if ( gFXDevice == SoundBlaster || gFXDevice == Awe32 )
	{
		gSBConfig.Address = SelectMenu("Base I/O address",
			BlasterAddressMenu, gSBConfig.Address);

		gSBConfig.Type = SelectMenu("Type of Sound Blaster card",
			BlasterTypeMenu, gSBConfig.Type);

		gSBConfig.Interrupt = SelectMenu("Interrupt",
			BlasterIRQMenu, gSBConfig.Interrupt);

		gSBConfig.Dma8 = SelectMenu("8 bit DMA channel",
			BlasterDma8Menu, gSBConfig.Dma8);

		gSBConfig.Dma16 = SelectMenu("16 bit DMA channel",
			BlasterDma16Menu, gSBConfig.Dma16);
	}

	gMixRate = SelectMenu("Select the rate to use for mixing",
		MixRateMenu, gMixRate);
	if ( gMixRate < 0 )
		return FALSE;

	config.PutKeyInt("Sound Setup", "FXDevice", gFXDevice);
	config.PutKeyInt("Sound Setup", "Bits", gMixBits);
	config.PutKeyInt("Sound Setup", "Channels", gChannels);
	config.PutKeyInt("Sound Setup", "Voices", gMixVoices);
	config.PutKeyInt("Sound Setup", "MixRate", gMixRate);

	config.PutKeyHex("Sound Setup", "BlasterAddress", gSBConfig.Address);
	config.PutKeyInt("Sound Setup", "BlasterType", gSBConfig.Type);
	config.PutKeyInt("Sound Setup", "BlasterInterrupt", gSBConfig.Interrupt);
	config.PutKeyInt("Sound Setup", "BlasterDma8", gSBConfig.Dma8);
	config.PutKeyInt("Sound Setup", "BlasterDma16", gSBConfig.Dma16);
	config.PutKeyInt("Sound Setup", "BlasterEmu", gSBConfig.Emu);

	return TRUE;
}


void InitSoundDevice(void)
{
	int status;

	// Do special Sound Blaster, AWE32 stuff
	if ( gFXDevice == SoundBlaster || gFXDevice == Awe32 )
	{
		int MaxVoices;
		int MaxBits;
		int MaxChannels;

		status = FX_SetupSoundBlaster(gSBConfig, &MaxVoices, &MaxBits, &MaxChannels);
	}
	else
	{
		fx_device device;
		status = FX_SetupCard( gFXDevice, &device );
	}

	if ( status != FX_Ok )
		ThrowError(FX_ErrorString(status), ES_ERROR);

	status = FX_Init( gFXDevice, gMixVoices, gChannels, gMixBits, gMixRate );
	if ( status != FX_Ok )
		ThrowError(FX_ErrorString(status), ES_ERROR);

	FX_SetVolume(gFXVolume);
}


/*******************************************************************************
	FUNCTION:		ShowUsage()

	DESCRIPTION:	Display command-line parameter usage, then exit
*******************************************************************************/
void ShowUsage(void)
{
	tioPrint("Syntax: SHPLAY [options] wildcard[.RAW]");
	tioPrint("  /setup    Reconfigure sound card");
	tioPrint("  /?        This help");
	tioPrint("");
	exit(0);
}


int compare(const void *p1, const void *p2 )
{
	const char **pp1 = (const char **)p1;
	const char **pp2 = (const char **)p2;
	return strcmp(*pp1, *pp2);


}
/***********************************************************************
 * Convert wildcard parameter to sound list
 **********************************************************************/
void ProcessArgument(char *s)
{
	char filespec[_MAX_PATH];
	char buffer[_MAX_PATH2];
	char path[_MAX_PATH];

	strcpy(filespec, s);
	AddExtension(filespec, ".RAW");

	char *drive, *dir;

	// separate the path from the filespec
	_splitpath2(s, buffer, &drive, &dir, NULL, NULL);
	_makepath(path, drive, dir, NULL, NULL);

	struct find_t fileinfo;

	unsigned r = _dos_findfirst(s, _A_NORMAL, &fileinfo);
	if (r != 0)
		printf("%s not found\n", s);

	while ( r == 0 && soundCount < kMaxPlaySounds )
	{
		strcpy(filespec, path);
		strcat(filespec, fileinfo.name);

		soundName[soundCount] = strdup(filespec);
		dassert( soundName[soundCount] != NULL );	// low memory condition
		soundCount++;

		r = _dos_findnext( &fileinfo );
	}

	_dos_findclose(&fileinfo);

	qsort(soundName, soundCount, sizeof(soundName[0]), compare);
}


/***********************************************************************
 * Process command line arguments
 **********************************************************************/
void ParseOptions( void )
{
	enum {
		kSwitchHelp,
		kSwitchSetup,
		kSwitchMap,
		kSwitchOmit,
	};
	static SWITCH switches[] = {
		{ "?", kSwitchHelp, FALSE },
		{ "SETUP", kSwitchSetup, FALSE },
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
				tioPrint(buffer);
				exit(1);

			case GO_FULL:
				ProcessArgument(OptArgument);
				bSoundWildcard = TRUE;
				break;

			case kSwitchSetup:
				if ( !SetupSoundCard() )
					exit(0);
				break;

			case kSwitchHelp:
				ShowUsage();
				break;
		}
	}
}


/*---------------------------------------------------------------------
   Function: GameErrorHandler

   Application error handler
---------------------------------------------------------------------*/
ErrorResult GameErrorHandler( const Error& error )
{
	setvmode(3);
	FX_Shutdown();
	tioTerm();

	// chain to the default error handler
	return prevErrorHandler(error);
};


/*---------------------------------------------------------------------
   Function: main

   Sets up sound cards, calls the demo, and then cleans up.
---------------------------------------------------------------------*/
void main( void )
{
	tioInit(0);

	GetSoundSettings();

	Resource::heap = new QHeap(dpmiDetermineMaxRealAlloc());

	if ( !config.KeyExists("Sound Setup", "FXDevice") )
	{
		if ( !SetupSoundCard() )
		{
			tioClearWindow();
			exit(0);
		}
	}

	ParseOptions();

	if ( !ReadSounds() )
		ThrowError("No sounds available",ES_ERROR);

	InitSoundDevice();

	// install our error handler
	prevErrorHandler = errSetHandler(GameErrorHandler);

	DoDemo();

	FX_Shutdown();

	tioClearWindow();
	tioPrint("Sonic Holography Player Version 2.0   Copyright (c) 1995 Q Studios Corporation");
	tioPrint("Written by Peter M. Freese");
	tioPrint("");
	tioPrint("Pinna angle: %3.0f degrees", gPinnaAngle * 360.0 / kAngle360);
	tioPrint("Saturation:  %0.2f", nSat);

	tioTerm();

	config.PutKeyInt("Setup", "FXVolume", gFXVolume);
	config.Save();
}
