/*******************************************************************************
	FILE:			SETUP.CPP

	DESCRIPTION:	Blood setup program

	AUTHOR:			Peter M. Freese
	CREATED:		11-18-95
	COPYRIGHT:		Copyright (c) 1995 Q Studios Corporation
*******************************************************************************/
#include <conio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <process.h>

#include "typedefs.h"
#include "debug4g.h"
#include "misc.h"
#include "task_man.h"
#include "fx_man.h"
#include "music.h"
#include "inifile.h"
#include "textio.h"
#include "usrhooks.h"

IniFile config("BLOOD.INI");


struct MENU_LINE
{
	int id;
	char *name;
	char *hint;
};


MENU_LINE MusicCardMenu[] =
{
	{ -1,				"None",					"No music card" },
	{ SoundBlaster,		"SoundBlaster",			"Sound Blaster, SB Pro, SB 16, or compatible" },
	{ ProAudioSpectrum,	"Pro Audio Spectrum",	"Media Vision Pro Audio Sprectrum" },
	{ SoundMan16,		"Sound Man 16",			"Logitech Sound Man 16" },
	{ Adlib,			"Adlib",				"Adlib card" },
	{ GenMidi,			"General Midi",			"General MIDI device through MPU-401" },
	{ SoundCanvas,		"Sound Canvas",			"Sound Canvas SCC-1 or daughterboard" },
	{ Awe32,			"Awe 32",				"Sound Blaster Awe 32" },
	{ WaveBlaster,		"WaveBlaster",			"WaveBlaster daughterboard" },
	{ SoundScape,		"SoundScape",			"Ensoniq Sound Scape" },
	{ UltraSound,		"Gravis UltraSound",	"Advanced Gravis UltraSound" },
	{ SoundSource,		"Disney Sound Source",	"Disney Sound Source" },
	{ TandySoundSource,	"Tandy Sound Source",	"Tandy Sound Source" },
	{ 0, NULL, NULL },
};

MENU_LINE MidiPortMenu[] =
{
	{ 0x300,	"300h",				"MIDI Port Address at 300h" },
	{ 0x310,	"310h",				"MIDI Port Address at 310h" },
	{ 0x320,	"320h",				"MIDI Port Address at 320h" },
	{ 0x330,	"330h (default)",	"MIDI Port Address at 330h" },
	{ 0, NULL, NULL },
};

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

enum
{
	kVModeUnchained = 0,
	kVModeVesa2 = 1,
	kVModeBuffered = 2,
	kVModeTseng = 3,
	kVModeParadise = 4,
	kVModeS3 = 5,
	kVModeCrystal = 6,
	kVModeRedBlue = 7,
};


MENU_LINE VModeMenu[] =
{
	{ kVModeUnchained,	"Unchained",			"Unchained VGA modes (mode X)" },
	{ kVModeVesa2,		"VESA 2.0",				"VESA 2.0 Linear Frame Buffer modes" },
	{ kVModeBuffered,	"Memory buffered",		"Memory buffer VGA and VESA 1.0 modes" },
	{ kVModeTseng,		"Tseng ET4000",			"Specially optimized 320x200 mode for Tseng ET4000 chipsets" },
	{ kVModeParadise,	"Paradise WD90Cxxx",	"Specially optimized 320x200 mode for Paradise WD90Cxx chipsets" },
	{ kVModeS3,			"S3",					"Specially optimized 320x200 mode for S3 chipsets" },
	{ kVModeCrystal,	"Crystal Eyes Stereo",	"320x200 120 Hz Stereo (Crystal Eyes only)" },
	{ kVModeRedBlue,	"Red-Blue Stereogram",	"320x200 Red-Blue Stereogram (VGA Compatible)" },
	{ 0, NULL, NULL },
};


MENU_LINE UnchainedModes[] =
{
	{ 320 << 16 | 200,	"320x200",	"320x200" },
	{ 320 << 16 | 240,	"320x240",	"320x240" },
	{ 360 << 16 | 270,	"360x270",	"360x270" },
	{ 320 << 16 | 400,	"320x400",	"320x400" },
	{ 0, NULL, NULL },
};

MENU_LINE Vesa2Modes[] =
{
	{ 320 << 16 | 200,	"320x200",	"320x200" },
	{ 320 << 16 | 240,	"320x240",	"320x240" },
	{ 320 << 16 | 400,	"320x400",	"320x400" },
	{ 640 << 16 | 400,	"640x400",	"640x400" },
	{ 640 << 16 | 480,	"640x480",	"640x480" },
	{ 800 << 16 | 600,	"800x600",	"800x600" },
	{ 0, NULL, NULL },
};


enum
{
	kMenuSound,
	kMenuMusic,
	kMenuVideo,
	kMenuControl,
	kMenuNetwork,
	kMenuModem,
	kMenuLaunch,
	kMenuSave,
};


MENU_LINE MainMenu[] =
{
	{ kMenuSound,	"Sound setup",		"Setup sound card" },
	{ kMenuMusic,	"Music setup",		"Setup music card" },
	{ kMenuVideo,	"Video setup",		"Setup video mode" },
	{ kMenuControl,	"Control setup",	"Configure input devices" },
	{ kMenuNetwork,	"Network game",		"Play a network game of Blood" },
	{ kMenuModem,	"Modem game",		"Launch a modem game of Blood" },
	{ kMenuLaunch,	"Save and play",	"Save settings and launch Blood" },
	{ kMenuSave,	"Save and exit",	"Save settings and exit" },
	{ 0, NULL, NULL },
};


int gMusicDevice;
int gMusicVolume;
int gMidiPort;
int gFXDevice;
int gMixBits;
int gChannels;
int gMixVoices;
int gMixRate;
int gFXVolume;
fx_blaster_config gSBConfig;

int gVideoMode;
int gVideoXRes;
int gVideoYRes;


/*---------------------------------------------------------------------
   Function: USRHOOKS_GetMem

   Allocates the requested amount of memory and returns a pointer to
   its location, or NULL if an error occurs.  NOTE: pointer is assumed
   to be dword aligned.
---------------------------------------------------------------------*/

int USRHOOKS_GetMem( void **ptr, unsigned long size )
{
	void *memory;

	memory = malloc( size );
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

	free( ptr );
	return( USRHOOKS_Ok );
}


/*---------------------------------------------------------------------
   Function: SelectMenu

   Draws the sound card selection screen and waits for the user to
   select a card.
---------------------------------------------------------------------*/

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

	char *saveUnder = (char *)malloc(rows * cols * 2);

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
			free(saveUnder);
			return -1;
		}
	}
	tioRestoreWindow(saveUnder, top, left, rows, cols);
	free(saveUnder);
	return list[pos].id;
}



void DoMusicMenu( void )
{
	// Get Sound fx card
	gMusicDevice = SelectMenu("Select a device for music",
		MusicCardMenu, gMusicDevice);

	if ( gMusicDevice < 0 )
		return;

	if ( gMusicDevice == SoundCanvas || gMusicDevice == GenMidi )
	{
		gMidiPort = SelectMenu("MIDI Port Address",
			MidiPortMenu, gMidiPort);
	}

	config.PutKeyInt("Sound Setup", "MusicDevice", gMusicDevice);
	config.PutKeyInt("Sound Setup", "MusicVolume", gMusicVolume);
	config.PutKeyHex("Sound Setup", "MidiPort", gMidiPort);

	return;
}


void GetSoundSettings( void )
{
	FX_GetBlasterSettings(&gSBConfig);

	// Read current setup
	gMusicDevice	= config.GetKeyInt("Sound Setup", "MusicDevice", -1);
	gMusicVolume	= config.GetKeyInt("Sound Setup", "MusicVolume", 200);
	gMidiPort		= config.GetKeyHex("Sound Setup", "MidiPort", gSBConfig.Midi);
	gFXDevice		= config.GetKeyInt("Sound Setup", "FXDevice", -1);
	gFXVolume		= config.GetKeyInt("Sound Setup", "FXVolume", 200);
	gMixBits		= config.GetKeyInt("Sound Setup", "Bits", 16);
	gChannels		= config.GetKeyInt("Sound Setup", "Channels", 2);
	gMixVoices		= config.GetKeyInt("Sound Setup", "Voices", 32);
	gMixRate		= config.GetKeyInt("Sound Setup", "MixRate", 22220);

	gSBConfig.Address	= config.GetKeyInt("Sound Setup", "BlasterAddress", gSBConfig.Address);
	gSBConfig.Type		= config.GetKeyInt("Sound Setup", "BlasterType", gSBConfig.Type);
	gSBConfig.Interrupt	= config.GetKeyInt("Sound Setup", "BlasterInterrupt", gSBConfig.Interrupt);
	gSBConfig.Dma8		= config.GetKeyInt("Sound Setup", "BlasterDma8", gSBConfig.Dma8);
	gSBConfig.Dma16		= config.GetKeyInt("Sound Setup", "BlasterDma16", gSBConfig.Dma16);
	gSBConfig.Emu		= config.GetKeyInt("Sound Setup", "BlasterEmu", gSBConfig.Emu);
}


void DoSoundMenu( void )
{
	// Get Sound fx card
	gFXDevice = SelectMenu("Select a device for digital sound",
		SoundCardMenu, gFXDevice);

	if ( gFXDevice < 0 )
		return;

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
				return;

			gChannels = SelectMenu("Select the channels mode",
				ChannelMenu, gChannels);
			if ( gChannels < 0 )
				return;
			gMixVoices = SelectMenu("Select the number of voices to use for mixing",
				VoiceMenu, gMixVoices);
			break;

		default:
			gMixBits = 8;
			gChannels = 1;
			gMixVoices = 2;
			break;
	}

	gMixRate = SelectMenu("Select the rate to use for mixing",
		MixRateMenu, gMixRate);
	if ( gMixRate < 0 )
		return;

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

	config.PutKeyInt("Sound Setup", "FXDevice", gFXDevice);
	config.PutKeyInt("Sound Setup", "FXVolume", gFXVolume);
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

	return;
}


void InitSoundCard( void )
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
	{
		printf( "%s\n", FX_ErrorString( status ) );
		exit( 1 );
	}

	status = FX_Init( gFXDevice, gMixVoices, gChannels, gMixBits, gMixRate );
	if ( status != FX_Ok )
	{
		printf( "%s\n", FX_ErrorString( status ) );
		exit( 1 );
	}
}


void DoVideoMenu( void )
{
	int packedRes = gVideoXRes << 16 | gVideoYRes;
	gVideoMode = SelectMenu("Select a video mode", VModeMenu, gVideoMode);

	switch ( gVideoMode )
	{
		case -1:
			return;

		case kVModeUnchained:
			packedRes = SelectMenu("Select resolution", UnchainedModes, packedRes);
			if ( packedRes < 0 )
				return;

			gVideoXRes = packedRes >> 16;
			gVideoYRes = packedRes & 0xFFFF;
			break;

		case kVModeVesa2:
			packedRes = SelectMenu("Select resolution", Vesa2Modes, packedRes);
			if ( packedRes < 0 )
				return;

			gVideoXRes = packedRes >> 16;
			gVideoYRes = packedRes & 0xFFFF;
			break;

		case kVModeBuffered:
		case kVModeTseng:
		case kVModeParadise:
		case kVModeS3:
		case kVModeCrystal:
		case kVModeRedBlue:
			gVideoXRes = 320;
			gVideoYRes = 200;
			break;
	}

	config.PutKeyInt("Video", "Mode", gVideoMode);
	config.PutKeyInt("Video", "XRes", gVideoXRes);
	config.PutKeyInt("Video", "YRes", gVideoYRes);

	return;
}


void DoMainMenu( void )
{
	int nMenuItem = 0;

	while (1)
	{
		nMenuItem = SelectMenu("", MainMenu, nMenuItem);
		switch ( nMenuItem )
		{
			case -1:
				return;

			case kMenuSound:
				DoSoundMenu();
				break;

			case kMenuMusic:
				DoMusicMenu();
				break;

			case kMenuVideo:
				DoVideoMenu();
				break;

			case kMenuControl:
				break;

			case kMenuNetwork:
				break;

			case kMenuModem:
				break;

			case kMenuLaunch:
				config.Save();
				tioSetAttribute(kColorLightGray);
				tioClearWindow();
				tioTerm();
				exit(spawnl(P_WAIT, "blood.exe", "blood.exe", NULL));

			case kMenuSave:
				config.Save();
				return;
		}
	}
}



/*---------------------------------------------------------------------
   Function: main

   Sets up sound cards, calls the demo, and then cleans up.
---------------------------------------------------------------------*/

void main( void )
{
	GetSoundSettings();

	gVideoMode		= config.GetKeyInt("Video", "Mode", kVModeBuffered);
	gVideoXRes		= config.GetKeyInt("Video", "XRes", 320);
	gVideoYRes		= config.GetKeyInt("Video", "YRes", 200);

	tioInit(1);

	tioFill( 0, 0, tioScreenRows - 2, tioScreenCols, ' ', kColorBlue * 16 | kColorYellow);
	tioLeftString(0, 1, "Blood Setup Version 1.0", kColorBlue * 16 | kColorYellow);
	tioRightString(0, tioScreenCols - 2, "Copyright (c) 1995 Q Studios", kColorBlue * 16 | kColorYellow);
	tioFill( 1, 0, tioScreenRows - 2, tioScreenCols, '°', kColorBlack * 16 | kColorDarkGray);

	DoMainMenu();

	tioSetAttribute(kColorLightGray);
	tioClearWindow();

	tioTerm();
}
