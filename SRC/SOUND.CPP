#include <string.h>
#include <dos.h>

#include "sound.h"
#include "debug4g.h"
#include "error.h"
#include "inifile.h"
#include "misc.h"
#include "globals.h"
#include "music.h"
#include "fx_man.h"

#include <memcheck.h>

#define kSampleRate		11110

#define kMaxChannels	32

static BOOL sndActive = FALSE;

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

RESHANDLE hSong;
BYTE *pSong = NULL;
int songSize;

struct SAMPLE2D
{
	int hVoice;
	RESHANDLE hResource;
};


SAMPLE2D Channel[kMaxChannels];


SAMPLE2D *FindChannel( void )
{
	for (int i = kMaxChannels - 1; Channel[i].hResource != NULL > 0; i--)
	{
		if ( i < 0 )
			ThrowError("No free channel available for sample", ES_ERROR);
	}

	return &Channel[i];
}


/*---------------------------------------------------------------------
   Function: LoadMidi

   Loads the midi file from disk.
---------------------------------------------------------------------*/
void sndPlaySong( char *songName, BOOL loopflag )
{
	if ( gMusicDevice == -1 )
		return;

	if ( pSong )
		sndStopSong();

	if ( strlen(songName) == 0 )
		return;

	hSong = gSysRes.Lookup(songName, ".MID");
	if (hSong == NULL)
	{
		dprintf("Could not load song %s\n", songName);
		return;
	}

	songSize = gSysRes.Size(hSong);

	pSong = (BYTE *)gSysRes.Lock(hSong);
	dpmiLockMemory(FP_OFF(pSong), songSize);

	// reset the volume, in case we were fading
	MUSIC_SetVolume(gMusicVolume);

	// start the song
	MUSIC_PlaySong(pSong, loopflag);
}


BOOL sndIsSongPlaying( void )
{
	if ( gMusicDevice == -1 )
		return FALSE;

	return (BOOL)MUSIC_SongPlaying();
}


void sndFadeSong( int nMilliseconds )
{
	MUSIC_FadeVolume(0, nMilliseconds);
}


void sndStopSong( void )
{
	if ( gMusicDevice == -1 )
		return;

	if ( pSong )
	{
		MUSIC_StopSong();
		dpmiUnlockMemory(FP_OFF(pSong), songSize);
		gSysRes.Unlock(hSong);
		pSong = NULL;
	}
}


static void SoundCallback( ulong id )
{
	int *phVoice = (int *)id;
	*phVoice = 0;
}


void sndStartSample( char *sampleName, int nVol, int nChannel )
{
	if ( gFXDevice == -1 )
		return;

	if ( strlen(sampleName) == 0 )
		return;

	SAMPLE2D *pChannel;
	if (nChannel == -1)
		pChannel = FindChannel();
	else
		pChannel = &Channel[nChannel];

	// if this is a fixed channel with an active voice, kill the sound
	if ( pChannel->hVoice > 0 )
		FX_StopSound(pChannel->hVoice);

	pChannel->hResource = gSysRes.Lookup(sampleName, ".RAW");
	if (pChannel->hResource == NULL)
	{
		dprintf("Could not load sample %s\n", sampleName);
		return;
	}

	int sampleSize = gSysRes.Size(pChannel->hResource);

	BYTE *pRaw = (BYTE *)gSysRes.Lock(pChannel->hResource);

	pChannel->hVoice = FX_PlayRaw(
		pRaw, sampleSize, kSampleRate, 0, nVol, nVol, nVol, nVol,
		(ulong)&pChannel->hVoice);
}


void sndProcess( void )
{
	for (int i = 0; i < kMaxChannels; i++)
	{
		if (Channel[i].hVoice <= 0 && Channel[i].hResource != NULL )
		{
			gSysRes.Unlock(Channel[i].hResource);
			Channel[i].hResource = NULL;
		}
	}
}


static void GetSettings( void )
{
	FX_GetBlasterSettings(&gSBConfig);

	// Read current setup
	gMusicDevice	= BloodINI.GetKeyInt("Sound Setup", "MusicDevice", -1);
	gMusicVolume	= BloodINI.GetKeyInt("Sound Setup", "MusicVolume", 200);
	gMidiPort		= BloodINI.GetKeyHex("Sound Setup", "MidiPort", gSBConfig.Midi);
	gFXDevice		= BloodINI.GetKeyInt("Sound Setup", "FXDevice", -1);
	gMixBits		= BloodINI.GetKeyInt("Sound Setup", "Bits", 16);
	gChannels		= BloodINI.GetKeyInt("Sound Setup", "Channels", 2);
	gMixVoices		= BloodINI.GetKeyInt("Sound Setup", "Voices", 16);
	gMixRate		= BloodINI.GetKeyInt("Sound Setup", "MixRate", 22050);
	gFXVolume		= BloodINI.GetKeyInt("Sound Setup", "FXVolume", 200);

	gSBConfig.Address	= BloodINI.GetKeyInt("Sound Setup", "BlasterAddress", gSBConfig.Address);
	gSBConfig.Type		= BloodINI.GetKeyInt("Sound Setup", "BlasterType", gSBConfig.Type);
	gSBConfig.Interrupt	= BloodINI.GetKeyInt("Sound Setup", "BlasterInterrupt", gSBConfig.Interrupt);
	gSBConfig.Dma8		= BloodINI.GetKeyInt("Sound Setup", "BlasterDma8", gSBConfig.Dma8);
	gSBConfig.Dma16		= BloodINI.GetKeyInt("Sound Setup", "BlasterDma16", gSBConfig.Dma16);
	gSBConfig.Emu		= BloodINI.GetKeyInt("Sound Setup", "BlasterEmu", gSBConfig.Emu);
}


void InitSoundDevice(void)
{
	int status;

	if ( gFXDevice == -1 )
		return;

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

	// Set up our fx callback so we can unlock sound resources
	FX_SetCallBack(SoundCallback);
}


void InitMusicDevice(void)
{
	int status;

	if ( gMusicDevice == -1 )
		return;

	status = MUSIC_Init( gMusicDevice, gMidiPort);
	if ( status != MUSIC_Ok )
		ThrowError(MUSIC_ErrorString( status ), ES_ERROR);

	RESHANDLE hTimbres = gSysRes.Lookup("GMTIMBRE", ".TMB");
	if ( hTimbres != NULL )
		MUSIC_RegisterTimbreBank((BYTE *)gSysRes.Load(hTimbres));

	MUSIC_SetVolume(gMusicVolume);
}


/***********************************************************************
 * Remove sound/music drivers
 **********************************************************************/
void sndTerm(void)
{
	// prevent this from shutting down more than once
	if ( sndActive )
	{
		sndStopSong();

		dprintf("MUSIC_Shutdown()\n");
		MUSIC_Shutdown();

		FX_Shutdown();

		sndActive = FALSE;
	}
}

/***********************************************************************
 * Initialize sound/music drivers
 **********************************************************************/
void sndInit(void)
{
	GetSettings();
	InitSoundDevice();
	InitMusicDevice();
	atexit(sndTerm);
	sndActive = TRUE;
}
