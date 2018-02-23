#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <io.h>
#include <fcntl.h>
#include <malloc.h>

#include "engine.h"
#include "multi.h"

#include "typedefs.h"
#include "misc.h"
#include "gameutil.h"
#include "globals.h"
#include "names.h"
#include "key.h"
#include "timer.h"
#include "textio.h"
#include "inifile.h"
#include "screen.h"
#include "view.h"
#include "db.h"
#include "debug4g.h"
#include "error.h"
#include "fire.h"
#include "sectorfx.h"
#include "helix.h"
#include "getopt.h"
#include "levels.h"
#include "sound.h"
#include "controls.h"
#include "actor.h"
#include "player.h"
#include "resource.h"
#include "protect.h"
#include "triggers.h"
#include "weapon.h"
#include "options.h"
#include "tile.h"
#include "gui.h"
#include "seq.h"
#include "trig.h"
#include "mirrors.h"
#include "warp.h"
#include "credits.h"
#include "sfx.h"

#include <memcheck.h>

#define kAttrTitle      (kColorRed * 16 + kColorYellow)

EHF prevErrorHandler;
static BOOL gForceMap = FALSE;

short gPacketIndex = 0;
BOOL ready2send = FALSE;

static char playerreadyflag[kMaxPlayers];
#define kNetFifoSize	256
INPUT gNetInput[kMaxPlayers];		// input received from net players
INPUT gFifoInput[kNetFifoSize][kMaxPlayers];
int gNetFifoClock;
int gNetFifoHead, gNetFifoTail;

ulong gFifoSendCRC[kNetFifoSize], gFifoReceiveCRC[kNetFifoSize];
int gSendCRCHead, gReceiveCRCHead, gCRCTail;
BOOL bOutOfSync = FALSE;


/***********************************************************************
 * Function prototypes
 **********************************************************************/
enum {
//	kTweakDecel	= 0,
//	kTweakSideDecel,
	kTweakFrontAccel,
	kTweakSideAccel,
	kTweakBackAccel,
	kTweakFrontSpeed1,
	kTweakFrontSpeed2,
	kTweakSideSpeed1,
	kTweakSideSpeed2,
	kTweakBackSpeed1,
	kTweakBackSpeed2,
	kTweakPace1,
	kTweakPace2,
	kMaxTweaks
};

struct TWEAKS
{
	int x;
	int y;
	int *pData;
	int nDefault;
} tweakSet[ kMaxTweaks ] =
{
//	{2,		2,		&gPosture[0].decel,			0},
//	{2,		20,		&gPosture[0].sideDecel,		0},
	{2,		38,		&gPosture[0].frontAccel,	0},
	{2,		56,		&gPosture[0].sideAccel,		0},
	{2,		74,		&gPosture[0].backAccel,		0},
	{2,		92,		&gPosture[0].frontSpeed[0],	0},
	{2,		110,	&gPosture[0].frontSpeed[1],	0},
	{2,		128,	&gPosture[0].sideSpeed[0],	0},
	{2,		146,	&gPosture[0].sideSpeed[1],	0},
	{2,		164,	&gPosture[0].backSpeed[0],	0},
	{90,	2,		&gPosture[0].backSpeed[1],	0},
	{90,	20,		&gPosture[0].pace[0],		0},
	{90,	38,		&gPosture[0].pace[1],		0},
};

void TweakProfile( char *title )
{
	int i;
	EditNumber *enTweaks[kMaxTweaks];

	// create the dialog
	Window dialog(0, 0, 319, 199, title);

	// these will automatically be destroyed when the dialog is destroyed
	for (i = 0; i < kMaxTweaks; i++)
	{
		tweakSet[i].nDefault = *tweakSet[i].pData;	// initialize the defaults
		enTweaks[i] = new EditNumber(tweakSet[i].x, tweakSet[i].y, 80, 16, tweakSet[i].nDefault);
		dialog.Insert(enTweaks[i]);
	}

	ShowModal(&dialog);

	for (i = 0; i < kMaxTweaks; i++)
	{
		if ( dialog.endState != mrOk)
			*tweakSet[i].pData = tweakSet[i].nDefault;	// restore the defaults
		else
			*tweakSet[i].pData = enTweaks[i]->value;
	}
}


void PreloadCache( void )
{
	long i;

	dprintf("Preload floor and ceiling tiles\n");
	for (i = 0; i < numsectors; i++)
	{
		tilePreloadTile(sector[i].floorpicnum);
		tilePreloadTile(sector[i].ceilingpicnum);
		if ( sector[i].ceilingstat & kSectorParallax )
		{
			for (int j = 1; j < gSkyCount; j++)
				tilePreloadTile(sector[i].ceilingpicnum + j);
		}
	}

	dprintf("Preload wall tiles\n");
	for (i = 0; i < numwalls; i++)
	{
		tilePreloadTile(wall[i].picnum);
		if (wall[i].overpicnum >= 0)
			tilePreloadTile(wall[i].overpicnum);
	}

	dprintf("Preload sprite tiles\n");
	for (i = 0; i < kMaxSprites; i++)
	{
		if (sprite[i].statnum < kMaxStatus)
			tilePreloadTile(sprite[i].picnum);
	}
}


void prepareboard( void )
{
	if ( !gForceMap )
		levelGetName(gEpisodeId, gMapId);

	dbLoadMap(gLevelName, &startposx, &startposy, &startposz, &startang, &startsectnum);
	srand(gMapCRC);

	levelLoadDef();

	scrLoadPLUs();

	for ( int i = 0; i < kMaxPlayers; i++ )
	{
		gStartZone[i].x = startposx;
		gStartZone[i].y = startposy;
		gStartZone[i].z = sector[startsectnum].floorz;
		gStartZone[i].sector = startsectnum;
		gStartZone[i].angle = startang;
	}

	// make sure the player starts out on the floor
	startposz = sector[startsectnum].floorz;

	seqKillAll();	// clear the current sequence list

	InitSectorFX();
	InitPlayerStartZones();

	actInit();

	// initialize all the tag buckets
	dprintf("evInit()\n");
	evInit();

	gFrameClock = gGameClock = 0;

	// initialize external trigger masks for sprites, sectors, and walls
	dprintf("trInit()\n");
	trInit();

	PreloadCache();

	//Map starts out blank; you map out what you see in 3D mode
	automapping = 1;

	for (i = 0; i < numplayers; i++)
		playerInit( i );

	InitMirrors();

	// start the music for the level
	sndPlaySong(gLevelSong, TRUE);

	gNetFifoClock = gFrameClock = gGameClock = 0;
	gNetFifoHead = gNetFifoTail = 0;
	gSendCRCHead = gReceiveCRCHead = gCRCTail = 0;
}


BYTE cheatAmmo[]	= { KEY_Q, KEY_S, KEY_A, KEY_M, KEY_M, KEY_O };
BYTE cheatRate[]	= { KEY_Q, KEY_S, KEY_R, KEY_A, KEY_T, KEY_E };
BYTE cheatGoto[]	= { KEY_Q, KEY_S, KEY_G, KEY_O, KEY_T, KEY_O };
BYTE cheatJump[]	= { KEY_Q, KEY_S, KEY_J, KEY_U, KEY_M, KEY_P };
BYTE cheatClip[]	= { KEY_Q, KEY_S, KEY_C, KEY_L, KEY_I, KEY_P };
BYTE cheatKeys[]	= { KEY_Q, KEY_S, KEY_K, KEY_E, KEY_Y, KEY_S };
BYTE cheatMaps[]	= { KEY_Q, KEY_S, KEY_M, KEY_A, KEY_P, KEY_S };
BYTE cheatHurt[]	= { KEY_Q, KEY_S, KEY_O, KEY_U, KEY_C, KEY_H };
BYTE cheatExplode[]	= { KEY_Q, KEY_S, KEY_B, KEY_O, KEY_O, KEY_M };
BYTE cheatBurn[]	= { KEY_Q, KEY_S, KEY_B, KEY_U, KEY_R, KEY_N };
BYTE cheatHigh[]	= { KEY_Q, KEY_S, KEY_H, KEY_I, KEY_G, KEY_H };
BYTE cheatInviso[]	= { KEY_Q, KEY_S, KEY_O, KEY_N, KEY_E, KEY_R, KEY_I, KEY_N, KEY_G };
BYTE cheatBeast[]	= { KEY_Q, KEY_S, KEY_L, KEY_U, KEY_S, KEY_T };
BYTE cheatHealth[]	= { KEY_Q, KEY_S, KEY_L, KEY_A, KEY_M, KEY_E };
BYTE cheatSeer[]	= { KEY_Q, KEY_S, KEY_I, KEY_C, KEY_U };
BYTE cheatFrag[]	= { KEY_Q, KEY_S, KEY_F, KEY_R, KEY_A, KEY_G };
BYTE cheatGod[]		= { KEY_Q, KEY_S, KEY_G, KEY_O, KEY_D };
BYTE cheatSight[]	= { KEY_Q, KEY_S, KEY_O, KEY_S, KEY_C , KEY_A , KEY_R };
BYTE cheatAim[]		= { KEY_Q, KEY_S, KEY_A, KEY_I, KEY_M };

void LocalKeys( void )
{
	char buffer[80];
	BYTE key;
	BYTE alt = keystatus[KEY_LALT] | keystatus[KEY_RALT];

	while ( (key = keyGet()) != 0 )
	{
		switch( key )
		{
			case KEY_F1:	// help/info screens
				TweakProfile( "Player Profile" );
				break;
			case KEY_F2:	// save game
				break;
			case KEY_F3:	// load game
				break;
			case KEY_F4:	// volume
				break;
			case KEY_F5:	// detail
				break;
			case KEY_F6:	// quicksave
				break;
			case KEY_F7:	// end game
				break;
			case KEY_F8:	// messages
				break;
			case KEY_F10:	// quit game
				break;

			case KEY_F11:	// gamma
				if (gGamma == gGammaLevels - 1)
					gGamma = 0;
				else
					gGamma = ClipHigh(gGamma + 1, gGammaLevels - 1);
				BloodINI.PutKeyInt("View", "Gamma", gGamma);
				scrSetGamma(gGamma);
				sprintf(buffer, "Gamma correction level %i", gGamma);
				viewSetMessage(buffer);
				break;

			case KEY_F12:
				// setup gViewIndex for different play and power-up modes
				if ( gNetGame && gNetPlayers > 1 )
				{
					if ( powerupCheck(gMe, kItemCrystalBall - kItemBase) )
					{
						gViewIndex = connectpoint2[gViewIndex];
						if (gViewIndex == -1)
							gViewIndex = connecthead;
					}
					else
					{
						if (gNetMode == kNetModeTeams)
						{
							gViewIndex = connectpoint2[gViewIndex];
							if (gViewIndex == -1)
								gViewIndex = connecthead;
							while ( gPlayer[gViewIndex].teamID != gMe->teamID )
							{
								gViewIndex = connectpoint2[gViewIndex];
								if (gViewIndex == -1)
									gViewIndex = connecthead;
							}
							gView = &gPlayer[gViewIndex];
						}
					}
				}
				else
				{
					gViewIndex = connectpoint2[gViewIndex];
					if (gViewIndex == -1)
						gViewIndex = connecthead;
					gView = &gPlayer[gViewIndex];
				}
				break;

			case KEY_MINUS:
				if ( keystatus[KEY_G] )
				{
					gGamma = ClipLow(gGamma - 1, 0);
					BloodINI.PutKeyInt("View", "Gamma", gGamma);
					scrSetGamma(gGamma);
					sprintf(buffer, "Gamma correction level %i", gGamma);
					viewSetMessage(buffer);
				}
				else if ( keystatus[KEY_D] )
				{
					visibility = ClipHigh(visibility + 16, 4096);
					sprintf(buffer, "Depth cueing level %i", visibility);
					viewSetMessage(buffer);
				}
				else if ( keystatus[KEY_S] )
				{
					gSpring = ClipLow(gSpring - 1, 1);
					BloodINI.PutKeyInt("Player", "SpringCoefficient", gSpring);
					gSpringPhaseInc = (int)(kAngle360 / (2 * 3.141592654 * (double)gSpring));

					sprintf(buffer, "Spring coefficient = %i", gSpring);
					viewSetMessage(buffer);
				}
				else if (gViewMode == kView3D)
				{
					viewResizeView(gViewSize + 1);
					BloodINI.PutKeyInt("View", "Size", gViewSize);
				}
				break;

			case KEY_PLUS:
				if ( keystatus[KEY_G] )
				{
					gGamma = ClipHigh(gGamma + 1, gGammaLevels - 1);
					BloodINI.PutKeyInt("View", "Gamma", gGamma);
					scrSetGamma(gGamma);
					sprintf(buffer, "Gamma correction level %i", gGamma);
					viewSetMessage(buffer);
				}
				else if ( keystatus[KEY_D] )
				{
					visibility = ClipLow(visibility - 16, 128);
					sprintf(buffer, "Depth cueing level %i", visibility);
					viewSetMessage(buffer);
				}
				else if ( keystatus[KEY_S])
				{
					gSpring++;
					BloodINI.PutKeyInt("Player", "SpringCoefficient", gSpring);
					gSpringPhaseInc = (int)(kAngle360 / (2 * 3.141592654 * (double)gSpring));

					sprintf(buffer, "Spring coefficient = %i", gSpring);
					viewSetMessage(buffer);
				}
				else if (gViewMode == kView3D)
				{
					viewResizeView(gViewSize - 1);
					BloodINI.PutKeyInt("View", "Size", gViewSize);
				}
				break;

			case KEY_P:
				parallaxtype = (char)IncRotate(parallaxtype, 3);
				break;

			case KEY_V:
				if ( alt )
				{
					if ( gViewPos > 0 )
						gViewPos = (VIEWPOS)((gViewPos & 7) + 1);
					sprintf(buffer, "gViewPos = %i", gViewPos);
					viewSetMessage(buffer);
				}
				else
				{
					if ( gViewPos > 0 )
						gViewPos = kViewPosCenter;
					else
						gViewPos = kViewPosBack;
				}
				break;

			case KEY_PRINTSCREEN:
				screencapture("captxxxx.pcx");
				break;

			default:
				break;
		}
	}

	if (keystatus[KEY_MINUS] && (gViewMode == kView2D || gViewMode == kView2DIcon))
	{
		gZoom = ClipLow(gZoom - (gZoom >> 6), 64);
	}

	if (keystatus[KEY_PLUS] && (gViewMode == kView2D || gViewMode == kView2DIcon))
	{
		gZoom = ClipHigh(gZoom + (gZoom >> 6), 4096);
	}

	int nDelirium = powerupCheck(gView, kItemShroomDelirium - kItemBase);
	if ( nDelirium )
	{
		static int timer = 0;
		timer += kFrameTicks;

		int maxTilt = kAngle30;
		int maxTurn = kAngle30;
		int maxPitch = 20;

// this constant determine when the effect starts to wear off
#define kWaneTime	512

		if (nDelirium < kWaneTime)
		{
			int scale = (nDelirium << 16) / kWaneTime;
			maxTilt = mulscale16(maxTilt, scale);
			maxTurn = mulscale16(maxTurn, scale);
			maxPitch = mulscale16(maxPitch, scale);
		}

		gScreenTilt = mulscale30(Sin(timer * 2) / 2 + Sin(timer * 3) / 2, maxTilt);
		deliriumTurn = mulscale30(Sin(timer * 3) / 2 + Sin(timer * 4) / 2, maxTurn);
		deliriumPitch = mulscale30(Sin(timer * 4) / 2 + Sin(timer * 5) / 2, maxPitch);
	}
	else
	{
		//	this code needs a different calculation,
		//	and should probably be partially placed in ctrlGetInput

		if ( keystatus[KEY_LBRACE] )
			gScreenTilt += kFrameTicks * 4;

		if ( keystatus[KEY_RBRACE] )
			gScreenTilt -= kFrameTicks * 4;

		// put gScreenTilt in the range -kAngle180 to kAngle180
		gScreenTilt = ((gScreenTilt + kAngle180) & kAngleMask) - kAngle180;

		// pull screen to vertical
		if ( gScreenTilt > 0 )
			gScreenTilt = ClipLow(gScreenTilt - kFrameTicks * 2, 0);
		else if ( gScreenTilt < 0 )
			gScreenTilt = ClipHigh(gScreenTilt + kFrameTicks * 2, 0);
	}

	if ( keyCompareStream( cheatRate, LENGTH(cheatRate)) )
	{
		keyPokeStream(0);
		gShowFrameRate = !gShowFrameRate;
	}

	if ( keyCompareStream( cheatFrag, LENGTH(cheatFrag)) )
	{
		keyPokeStream(0);
		gShowFrags = !gShowFrags;
	}

	if ( keyCompareStream( cheatAim, LENGTH(cheatAim)) )
	{
		keyPokeStream(0);
		gAimReticle = !gAimReticle;
	}

	if ( gNetGame == FALSE )
	{
		if ( keyCompareStream( cheatAmmo, LENGTH(cheatAmmo)) )
		{
			keyPokeStream(0);
			for (int i = 0; i < kWeaponMax; i++)
			{
				gMe->hasWeapon[i] = TRUE;
				//gMe->weaponState[i] = 0;
			}
			for (i = 0; i < kAmmoMax; i++)
			{
				gMe->ammoCount[i] = gAmmoInfo[i].max;
			}
			viewSetMessage("Full armory granted!");
		}

		if ( keyCompareStream( cheatKeys, LENGTH(cheatKeys)) )
		{
			keyPokeStream(0);
			for (int i = 1; i <= 6; i++)
				gMe->hasKey[i] = (short)(kPicKey1 + i - 1);
			viewSetMessage("Full keychain granted!");
		}

		if ( keyCompareStream( cheatMaps, LENGTH(cheatMaps)) )
		{
			keyPokeStream(0);
			gFullMap = !gFullMap;
			if ( gFullMap )
				viewSetMessage("Full atlas granted");
			else
				viewSetMessage("Full atlas revoked");
		}

		if ( keyCompareStream( cheatClip, LENGTH(cheatClip)) )
		{
			keyPokeStream(0);
			gNoClip = !gNoClip;
			if ( gNoClip )
				viewSetMessage("Non-clipped mode enabled");
			else
				viewSetMessage("Non-clipped mode disabled");
		}

		if ( keyCompareStream( cheatGoto, LENGTH(cheatGoto)) )
		{
			keyPokeStream(0);
			long x = GetNumberBox("X=", gMe->sprite->x, gMe->sprite->x);
			long y = GetNumberBox("Y=", gMe->sprite->y, gMe->sprite->y);

			short nSector;
			updatesector(x, y, &nSector);
			if (nSector != -1) {
				changespritesect((short)gMe->nSprite, nSector);
				gMe->sprite->x = x;
				gMe->sprite->y = y;
				gMe->sprite->z = sector[nSector].floorz;
				gMe->sprite->zvel = 0;
			}
		}

		if ( keyCompareStream( cheatJump, LENGTH(cheatJump)) )
		{
			keyPokeStream(0);
			long newEpisodeId = GetNumberBox("Episode (1..3)=", gEpisodeId, 1);
			long newMapId = GetNumberBox("Map (1..15)=", gMapId, 1);

			if ((newEpisodeId != 0) && (newMapId != 0)) {
				gEpisodeId = newEpisodeId;
				gMapId = newMapId;
				gForceMap = FALSE;
				ready2send = FALSE;
				prepareboard();
				ready2send = TRUE;
			}
			viewResizeView(gViewSize);
		}

		if ( keyCompareStream( cheatHurt, LENGTH(cheatHurt)) )
		{
			keyPokeStream(0);
			actDamageSprite(gMe->nSprite, gMe->nSprite, kDamagePummel, 100 << 4);
			viewSetMessage("Ouch!");
		}

		if ( keyCompareStream( cheatExplode, LENGTH(cheatExplode)) )
		{
			keyPokeStream(0);
			actDamageSprite(gMe->nSprite, gMe->nSprite, kDamageExplode, 100 << 4);
			viewSetMessage("Boom.");
		}

		if ( keyCompareStream( cheatBurn, LENGTH(cheatBurn)) )
		{
			keyPokeStream(0);
			actAddBurnTime( gMe->nSprite, gMe->xsprite, kMaxBurnTime );
			viewSetMessage("Crispy!");
		}

		if ( keyCompareStream( cheatHigh, LENGTH(cheatHigh)) )
		{
			keyPokeStream(0);
			if ( powerupCheck( gMe, kItemShroomDelirium - kItemBase ) )
				powerupDeactivate( gMe, kItemShroomDelirium - kItemBase );
			else
				powerupActivate( gMe, kItemShroomDelirium - kItemBase );
			viewSetMessage("Cosmic, man!");
		}

		if ( keyCompareStream( cheatGod, LENGTH(cheatGod)) )
		{
			keyPokeStream(0);
			playerSetGodMode( gMe, !gMe->godMode );
			if (gMe->godMode)
				viewSetMessage("Entering Broussard mode!");
			else
				viewSetMessage("Back to normal mode!");
		}

		if ( keyCompareStream( cheatInviso, LENGTH(cheatInviso)) )
		{
			keyPokeStream(0);
			if ( powerupCheck( gMe, kItemLtdInvisibility - kItemBase ) )
			{
				viewSetMessage("Deactivating the One Ring");
				powerupDeactivate( gMe, kItemLtdInvisibility - kItemBase );
			}
			else
			{
				viewSetMessage("Activating the One Ring");
				powerupActivate( gMe, kItemLtdInvisibility - kItemBase );
			}
		}

		if ( keyCompareStream( cheatSeer, LENGTH(cheatSeer)) )
		{
			keyPokeStream(0);
			if ( powerupCheck( gMe, kItemCrystalBall - kItemBase ) )
			{
				viewSetMessage("Deactivating the Crystal Ball");
				powerupDeactivate( gMe, kItemCrystalBall - kItemBase );
			}
			else
			{
				viewSetMessage("Activating the Crystal Ball");
				powerupActivate( gMe, kItemCrystalBall - kItemBase );
			}
		}

		if ( keyCompareStream( cheatBeast, LENGTH(cheatBeast)) )
		{
			keyPokeStream(0);
			if (gMe->lifemode == kModeBeast)
				playerSetRace( gMe, kModeHuman, gMe->sprite->type );
			else
				playerSetRace( gMe, kModeBeast, gMe->sprite->type );
		}

		if ( keyCompareStream( cheatHealth, LENGTH(cheatHealth)) )
		{
			keyPokeStream(0);
			gMe->xsprite->health = 200 << 4;
			viewSetMessage("Full health granted, you maggot!");
		}

		if ( keyCompareStream( cheatSight, LENGTH(cheatSight)) )
		{
			keyPokeStream(0);
			if ( powerupCheck( gMe, kItemRoseGlasses - kItemBase ) )
			{
				viewSetMessage("Deactivating Oscar Sight");
				powerupDeactivate( gMe, kItemRoseGlasses - kItemBase );
			}
			else
			{
				viewSetMessage("Activating Oscar Sight");
				powerupActivate( gMe, kItemRoseGlasses - kItemBase );
			}
		}
	}

	if (*control[kAutomapToggle])
	{
		*control[kAutomapToggle] = 0;

		if (gViewMode == kView3D)
		{
			gViewMode = kView2D;
		}
		else if (gViewMode == kView2D)
		{
			gViewMode = kView2DIcon;
		}
		else
		{
			gViewMode = kView3D;
			viewResizeView(gViewSize);
		}
	}

	if (gNetPlayers == 1)           //Single player only keys
	{
		if (keystatus[KEY_INSERT])   //Insert - Insert player
		{
			keystatus[KEY_INSERT] = 0;
			if (numplayers < kMaxPlayers)
			{
				connectpoint2[numplayers-1] = numplayers;
				connectpoint2[numplayers] = -1;

				gViewIndex = myconnectindex = numplayers++;
				gView = gMe = &gPlayer[myconnectindex];
				playerInit( myconnectindex );
			}
		}
		if (keystatus[KEY_DELETE])   //Delete - Delete player
		{
			keystatus[KEY_DELETE] = 0;
			if (numplayers > 1)
			{
				// actDeletePlayer( myconnectindex ); // should probably add something like this!

				numplayers--;
				connectpoint2[numplayers-1] = -1;

				seqKill( SS_SPRITE, gPlayer[numplayers].sprite->extra );

				actPostSprite( gPlayer[numplayers].nSprite, kStatFree );
				gPlayer[numplayers].sprite = NULL;

				if (myconnectindex >= numplayers) myconnectindex = 0;
				gMe = &gPlayer[myconnectindex];
				if (gViewIndex >= numplayers) gViewIndex = 0;
				gView = &gPlayer[gViewIndex];
			}
		}
		if (keystatus[KEY_SCROLLLOCK])   //Scroll Lock
		{
			keystatus[KEY_SCROLLLOCK] = 0;

			myconnectindex = connectpoint2[myconnectindex];
			if (myconnectindex < 0) myconnectindex = connecthead;
			gViewIndex = myconnectindex;
			gView = gMe = &gPlayer[myconnectindex];
		}
	}
}

ulong CalcGameChecksum( void )
{
	ulong value = 0;

	value = Random(65536);
//	dprintf("Frame #%d: randCount1 = %d, randCount2 = %d, Random() = %4x\n",
//		gFrame, randCount1, randCount2, value);
	//value ^= CRC32(gPlayer, sizeof(gPlayer));
	for ( int i = connecthead; i >= 0; i = connectpoint2[i] )
		value ^= CRC32(gPlayer[i].sprite, sizeof(SPRITE));

	return value;
}

/*
void CheckMasterSwitch( void )
{
	int i, j;

	i = connecthead; j = connectpoint2[i];
	while (j >= 0)
	{
		if (gPlayer[j].keyFlags.master)
		{
			connectpoint2[i] = connectpoint2[j];
			connectpoint2[j] = connecthead;
			connecthead = (short)j;

			gGameClock = gFrameClock;
			if (connecthead == myconnectindex)
				viewSetMessage("Master");
			else
				viewSetMessage("Slave");
			gPlayer[j].keyFlags.master = 0;
			return;
		}
		i = j; j = connectpoint2[i];
	}
}
*/


/*******************************************************************************
	FUNCTION:		ProcessFrame()

	DESCRIPTION:	Main game processing function
*******************************************************************************/
void ProcessFrame()
{
	for ( int i = connecthead; i >= 0; i = connectpoint2[i] )
	{
		gPlayerInput[i].syncFlags = gFifoInput[gNetFifoHead][i].syncFlags;
		gPlayerInput[i].buttonFlags = gFifoInput[gNetFifoHead][i].buttonFlags;

		// keys and weapon selection are non-destructive
		gPlayerInput[i].keyFlags.byte |= gFifoInput[gNetFifoHead][i].keyFlags.byte;
		if (gFifoInput[gNetFifoHead][i].newWeapon != 0)
			gPlayerInput[i].newWeapon = gFifoInput[gNetFifoHead][i].newWeapon;

		gPlayerInput[i].forward = gFifoInput[gNetFifoHead][i].forward;
		gPlayerInput[i].strafe = gFifoInput[gNetFifoHead][i].strafe;
		gPlayerInput[i].turn = gFifoInput[gNetFifoHead][i].turn;
		gPlayerInput[i].ticks = gFifoInput[gNetFifoHead][i].ticks;
	}
	gNetFifoHead = IncRotate(gNetFifoHead, kNetFifoSize);

	gFifoSendCRC[gSendCRCHead] = CalcGameChecksum();
	gSendCRCHead = IncRotate(gSendCRCHead, kNetFifoSize);

	for (i = connecthead; i >= 0; i = connectpoint2[i])
	{
		if ( gPlayerInput[i].keyFlags.restart )
		{
			gPlayerInput[i].keyFlags.restart = 0;
			gForceMap = TRUE;
			gPaused = FALSE;

			ready2send = FALSE;
			prepareboard();
			ready2send = TRUE;
		}
		if ( gPlayerInput[i].keyFlags.pause )
		{
			gPlayerInput[i].keyFlags.pause = 0;
			gPaused = !gPaused;
		}
	}

	if (gPaused)
		return;

	for (i = connecthead; i >= 0; i = connectpoint2[i])
		viewBackupPlayerLoc(i);

	trProcessBusy();			// process busy triggers
	evProcess(gFrameClock);		// process event queue
	seqProcess(kFrameTicks);

	for (i = connecthead; i >= 0; i = connectpoint2[i])
	{
		powerupProcess(&gPlayer[i]);	// process the power-ups
		playerMove(&gPlayer[i], &gPlayerInput[i]);		// Move player
	}

	actProcessSprites();		// Actor code: projectiles, etc.
	actPostProcess();	// clean up deleted sprites or sprites changing status
	sndProcess();
	sfxUpdate3DSounds();

	gFrame++;
	gFrameClock += kFrameTicks;
}


void UpdateNetFifo( void )
{
	for ( int i = connecthead; i >= 0; i = connectpoint2[i] )
	{
		gFifoInput[gNetFifoTail][i] = gNetInput[i];
	}
	gNetFifoTail = IncRotate(gNetFifoTail, kNetFifoSize);
	// CheckMasterSwitch();
}


char packet[576];

inline char GetPacketByte(char * &p)
{ return *p++; }

inline PutPacketByte(char * &p, int b)
{ *p++ = (char)b; }

inline short GetPacketShort(char * &p)
{
	short w = *(short *)p;
	p += 2;
	return w;
}

inline PutPacketShort(char * &p, int w)
{
	*(short *)p = (short)w;
	p += 2;
}

inline long GetPacketLong(char * &p)
{
	long l = *(long *)p;
	p += 4;
	return l;
}

inline PutPacketLong(char * &p, int l)
{
	*(long *)p = (long)l;
	p += 4;
}


int GetPackets( void )
{
	short nPlayer, length;
	long i, j;
	char *p;
	int n = 0;
	int nMoveCount = 0;

	while ( (length = getpacket(&nPlayer, packet)) > 0)
	{
		n++;
		p = packet;

		switch( GetPacketByte(p) )
		{
			case 0:  // receive master sync buffer

				j = GetPacketShort(p);
				if (j != gPacketIndex++)
					ThrowError("Missing master packet", ES_ERROR);

				for (i = connecthead; i >= 0; i = connectpoint2[i])
				{
					SYNCFLAGS syncFlags;
					syncFlags.byte = GetPacketByte(p);
					gNetInput[i].syncFlags.byte = syncFlags.byte;

					if (syncFlags.buttonChange)
						gNetInput[i].buttonFlags.byte = GetPacketByte(p);
					if (syncFlags.keyChange)
						gNetInput[i].keyFlags.byte = GetPacketByte(p);
					if (syncFlags.weaponChange)
						gNetInput[i].newWeapon = GetPacketByte(p);
					if (syncFlags.forwardChange)
						gNetInput[i].forward = GetPacketByte(p);
					if (syncFlags.turnChange)
						gNetInput[i].turn = GetPacketShort(p);
					if (syncFlags.strafeChange)
						gNetInput[i].strafe = GetPacketByte(p);
					gNetInput[i].ticks = GetPacketByte(p);
				}

				while ( p < &packet[length] )
				{
					gFifoReceiveCRC[gReceiveCRCHead] = GetPacketLong(p);
					gReceiveCRCHead = IncRotate(gReceiveCRCHead, kNetFifoSize);
				}

				if ( gReceiveCRCHead != gCRCTail && gSendCRCHead != gCRCTail )
				{
					bOutOfSync = FALSE;
					do
					{
						if ( gFifoReceiveCRC[gCRCTail] != gFifoSendCRC[gCRCTail] )
						{
							dprintf("SYNC: gFifoReceiveCRC[%d] = %8x, gFifoSendCRC[%d] = %8x\n",
								gCRCTail, gFifoReceiveCRC[gCRCTail], gCRCTail, gFifoSendCRC[gCRCTail]);
							bOutOfSync = TRUE;
						}

						gCRCTail = IncRotate(gCRCTail, kNetFifoSize);
					}
					while ( gReceiveCRCHead != gCRCTail && gSendCRCHead != gCRCTail );
				}

				UpdateNetFifo();
				nMoveCount++;
				continue;

			case 1:  // receive slave sync buffer

				SYNCFLAGS syncFlags;
				syncFlags.byte = GetPacketByte(p);
				gNetInput[nPlayer].syncFlags.byte = syncFlags.byte;

				if (syncFlags.buttonChange)
					gNetInput[nPlayer].buttonFlags.byte = GetPacketByte(p);
				if (syncFlags.keyChange)
					gNetInput[nPlayer].keyFlags.byte = GetPacketByte(p);
				if (syncFlags.weaponChange)
					gNetInput[nPlayer].newWeapon = GetPacketByte(p);
				if (syncFlags.forwardChange)
					gNetInput[nPlayer].forward = GetPacketByte(p);
				if (syncFlags.turnChange)
					gNetInput[nPlayer].turn = GetPacketShort(p);
				if (syncFlags.strafeChange)
					gNetInput[nPlayer].strafe = GetPacketByte(p);
				gNetInput[nPlayer].ticks = GetPacketByte(p);

				continue;

			case 250:
				playerreadyflag[nPlayer] = GetPacketByte(p);

				// if the message came from the master, acknowledge it
				if ( nPlayer == connecthead && playerreadyflag[connecthead] == 2 )
					sendpacket(connecthead, packet, 2);
				break;

			case 255:  //[255] (logout)
				keystatus[1] = 1;
				break;
		}
			break;
	}

	// try to prevent slave and master from getting into phase lock-step
	if ( myconnectindex != connecthead && nMoveCount != 1 )
	{
		if (nMoveCount == 2)
			dprintf("%4d: Received 2 packets from master\n", gPacketIndex);
//		if (nMoveCount & 2)
//			gNetFifoClock += kFrameTicks / 2;
//		else
//			gNetFifoClock -= kFrameTicks / 2;
	}
	return n;
}


void GetInput( void )
{
	if ( gNetPlayers > 1 )
		GetPackets();

	// this doesn't currently do anything but Ken might add to this later
	if (getoutputcirclesize() >= 16)
		return;

	ctrlGetInput();

	char *p = packet;

	// single player or master
	if (gNetPlayers == 1 || myconnectindex == connecthead)
	{
		// copy in local input
		gNetInput[myconnectindex].syncFlags.byte = gInput.syncFlags.byte;

		if (gInput.syncFlags.buttonChange)
			gNetInput[myconnectindex].buttonFlags.byte = gInput.buttonFlags.byte;
		if (gInput.syncFlags.keyChange)
			gNetInput[myconnectindex].keyFlags.byte = gInput.keyFlags.byte;
		if (gInput.syncFlags.weaponChange)
			gNetInput[myconnectindex].newWeapon = gInput.newWeapon;
		if (gInput.syncFlags.forwardChange)
			gNetInput[myconnectindex].forward = gInput.forward;
		if (gInput.syncFlags.turnChange)
			gNetInput[myconnectindex].turn = gInput.turn;
		if (gInput.syncFlags.strafeChange)
			gNetInput[myconnectindex].strafe = gInput.strafe;
		gNetInput[myconnectindex].ticks = gInput.ticks;

		if ( gNetPlayers > 1 )
		{
			// build master packet
			PutPacketByte(p, 0);
			PutPacketShort(p, gPacketIndex++);

			for (int i = connecthead; i >= 0; i = connectpoint2[i])
			{
				PutPacketByte(p, gNetInput[i].syncFlags.byte);

				if (gNetInput[i].syncFlags.buttonChange)
					PutPacketByte(p, gNetInput[i].buttonFlags.byte);
				if (gNetInput[i].syncFlags.keyChange)
					PutPacketByte(p, gNetInput[i].keyFlags.byte);
				if (gNetInput[i].syncFlags.weaponChange)
					PutPacketByte(p, gNetInput[i].newWeapon);
				if (gNetInput[i].syncFlags.forwardChange)
					PutPacketByte(p, gNetInput[i].forward);
				if (gNetInput[i].syncFlags.turnChange)
					PutPacketShort(p, gNetInput[i].turn);
				if (gNetInput[i].syncFlags.strafeChange)
					PutPacketByte(p, gNetInput[i].strafe);
				PutPacketByte(p, gNetInput[i].ticks);

				gNetInput[i].syncFlags.byte = 0;
			}

			while ( gCRCTail != gSendCRCHead )
			{
				PutPacketLong(p, gFifoSendCRC[gCRCTail]);
				gCRCTail = IncRotate(gCRCTail, kNetFifoSize);	// master increment SEND tail
			}

			// send packets to slaves
			for (i = connectpoint2[connecthead]; i >= 0; i = connectpoint2[i])
				sendpacket((short)i, packet, (short)(p - packet));

		}
		UpdateNetFifo();
	}
	else                        //I am a SLAVE
	{
		// build slave packet
		PutPacketByte(p, 1);
		PutPacketByte(p, gInput.syncFlags.byte);

		if (gInput.syncFlags.buttonChange)
			PutPacketByte(p, gInput.buttonFlags.byte);
		if (gInput.syncFlags.keyChange)
			PutPacketByte(p, gInput.keyFlags.byte);
		if (gInput.syncFlags.weaponChange)
			PutPacketByte(p, gInput.newWeapon);
		if (gInput.syncFlags.forwardChange)
			PutPacketByte(p, gInput.forward);
		if (gInput.syncFlags.turnChange)
			PutPacketShort(p, gInput.turn);
		if (gInput.syncFlags.strafeChange)
			PutPacketByte(p, gInput.strafe);
		PutPacketByte(p, gInput.ticks);

		// send packet to master
		sendpacket((short)connecthead, packet, (short)(p - packet));
	}
}


void faketimerhandler( void )
{
	if ( gGameClock >= gNetFifoClock && ready2send )
	{
		gNetFifoClock += kFrameTicks;
		GetInput();
	}
}


BOOL WaitForAllPlayers( void )
{
	long i, j, oldtotalclock;

	if (numplayers < 2) return TRUE;

	// if I'm the Master
	if (myconnectindex == connecthead)
	{
		for (j = 1; j <= 2; j++)
		{
			for (i = connectpoint2[connecthead]; i >= 0; i = connectpoint2[i])
				playerreadyflag[i] = 0;
			oldtotalclock = gGameClock - 8;
			do
			{
				if (keystatus[1]) return FALSE;

				if ( gNetPlayers > 1 )
					GetPackets();

				if (gGameClock >= oldtotalclock + 8)
				{
					oldtotalclock = gGameClock;
					char *p = packet;
					PutPacketByte(p, 250);
					PutPacketByte(p, j);
					for(i = connectpoint2[connecthead]; i >= 0; i = connectpoint2[i])
						if (playerreadyflag[i] != j)
							sendpacket((short)i, packet, 2);
				}
				for(i = connectpoint2[connecthead]; i >= 0; i = connectpoint2[i])
					if (playerreadyflag[i] != j) break;
			} while (i >= 0);
		}
	}
	else
	{
		playerreadyflag[connecthead] = 0;
		while (playerreadyflag[connecthead] != 2)
		{
			if (keystatus[1]) return FALSE;
			if ( gNetPlayers > 1 )
				GetPackets();

			if (playerreadyflag[connecthead] == 1)
			{
				playerreadyflag[connecthead] = 0;
				sendpacket(connecthead, packet, 2);
			}
		}
	}
	return TRUE;
}


ErrorResult GameErrorHandler( const Error& error )
{
	// shut down the game engine
	if ( gNetGame )
	{
		sendlogoff();
		uninitmultiplayers();
	}

	sndTerm();
	timerRemove();
	ctrlTerm();
	UnlockClockStrobe();
	uninitengine();
	setvmode(gOldDisplayMode);

	// chain to the default error handler
	return prevErrorHandler(error);
};

enum
{
	kSwitchNet,
	kSwitchBloodBath,
	kSwitchTeams,
};

SWITCH switches[] =
{
	{ "net", kSwitchNet, TRUE },
	{ "bloodbath", kSwitchBloodBath, FALSE },
	{ "teams", kSwitchBloodBath, FALSE },
	{ NULL, 0, FALSE },
};

/***********************************************************************
 * Process command line arguments
 **********************************************************************/
void ParseOptions( void )
{
	char buffer[256];
	int r;
	while ( (r = GetOptions(switches)) != GO_EOF )
	{
		switch (r)
		{
			case GO_INVALID:
				sprintf(buffer, "Invalid argument: %s", OptArgument);
				ThrowError(buffer, ES_ERROR);

			case kSwitchNet:
				gNetGame = TRUE;
				dprintf("Net parameter is %s\n", OptArgument);
				break;

			case kSwitchBloodBath:
				// NET PARAMETER MUST COME FIRST: Can you add a CheckParm?
				gNetMode = kNetModeBloodBath;
				dprintf("Bloodbath network mode\n");
				break;

			case kSwitchTeams:
				// NET PARAMETER MUST COME FIRST: Can you add a CheckParm?
				gNetMode = kNetModeTeams;
				dprintf("Team network mode\n");
				break;

			case GO_FULL:
				strcpy(gLevelName, OptArgument);
				gForceMap = TRUE;
				break;
		}
	}

}


/***********************************************************************
 * main()
 *
 * It all starts here. Have a Bloody day.
 *
 **********************************************************************/
void main( /* int argc, char *argv[]  */ )
{
	char title[256];

	gOldDisplayMode = getvmode();

	sprintf(title, "BLOOD ALPHA BUILD [%s] -- DO NOT DISTRIBUTE", gBuildDate);
	tioInit();
	tioCenterString(0, 0, tioScreenCols - 1, title, kAttrTitle);
	tioCenterString(tioScreenRows - 1, 0, tioScreenCols - 1,
		"Copyright (c) 1994, 1995 Q Studios Corporation", kAttrTitle);

	tioWindow(1, 0, tioScreenRows - 2, tioScreenCols);

	if ( _grow_handles(kRequiredFiles) < kRequiredFiles )
	{
		tioPrint("Increase FILES=## value in CONFIG.SYS.");
		ThrowError("Not enough file handles available", ES_ERROR);
	}

	ParseOptions();

	tioPrint("Loading INI file");
	optLoadINI();

	tioPrint("Initializing heap and resource system");
	Resource::heap = new QHeap(dpmiDetermineMaxRealAlloc());

	tioPrint("Initializing resource archive");
	gSysRes.Init("BLOOD.RFF", "*.*");
	gGuiRes.Init("GUI.RFF", NULL);

	CheckDemoDate(gSysRes);

	// install our error handler
	prevErrorHandler = errSetHandler(GameErrorHandler);

	tioPrint("Initializing 3D engine");
	InitEngine();

	tioPrint("Creating standard color lookups");
	scrCreateStdColors();

	tioPrint("Loading tiles");
	if (tileInit() == 0)
		ThrowError("ART files not found", ES_ERROR);

	powerupInit(); // calc zoom table for power-up animations

	tioPrint("Loading cosine table");
	trigInit(gSysRes);

	tioPrint("Initializing screen");
	scrInit();

	tioPrint("Initializing view subsystem");
	viewInit();

	tioPrint("Initializing dynamic fire");
	FireInit();

	tioPrint("Initializing weapon animations");
	WeaponInit();

	tioPrint("Installing keyboard handler");
	keyInstall();

	tioPrint("Initializing network users");
	initmultiplayers(0, 0, 0);
	gNetPlayers = numplayers;

	gMe = gView = &gPlayer[myconnectindex];
	gViewIndex = myconnectindex;

	// this must be done before initializing timer, because mouse interrupt hooks timer vector
	tioPrint("Loading control setup");
	ctrlInit();

	tioPrint("Installing timer");
	LockClockStrobe();
	timerRegisterClient(ClockStrobe, kTimerRate);
	timerInstall();

	tioPrint("Initializing sound system");
	sndInit();
	sfxInit();

	tioPrint("Waiting for network players");
	WaitForAllPlayers();

	scrSetGameMode();
	scrSetGamma(gGamma);

//	if ( !gForceMap )
//		credCredits();

	viewResizeView(gViewSize);

	prepareboard();

	gNetFifoClock = gFrameClock = gGameClock = 0;
	gNetFifoHead = gNetFifoTail = 0;
	gSendCRCHead = gReceiveCRCHead = gCRCTail = 0;
	ready2send = TRUE;

	//This is the whole game loop!
	while ( keystatus[KEY_ESC] == 0 && !syncstate )
	{
		LocalKeys();

		if ( gNetPlayers > 1 )
			GetPackets();

		faketimerhandler();

		while ( gNetFifoHead != gNetFifoTail )
			ProcessFrame();

		if ( !gPaused )
			gInterpolate = (gGameClock + kFrameTicks - gFrameClock) * (0x10000 / kFrameTicks);

		viewDrawScreen();

		if (bOutOfSync)
			viewSetMessage("Out of Sync!");

		if ( gEndLevelFlag )
		{
			// fade out the current music
			sndFadeSong(4000);

			if ( gEndLevelFlag < 0 )
			{
				// end episode or something special like that
			}
			else
				gMapId = gEndLevelFlag;

			gEndLevelFlag = 0;
			gForceMap = FALSE;
			ready2send = FALSE;
			prepareboard();
			ready2send = TRUE;
			viewResizeView(gViewSize);
		}
	}
	ready2send = FALSE;

	setvmode(gOldDisplayMode);

	sndTerm();

	dprintf("Removing timer\n");
	timerRemove();

	ctrlTerm();
	UnlockClockStrobe();

	if ( gNetGame )
	{
		sendlogoff();
		uninitmultiplayers();
	}

 	dprintf("uninitengine()\n");
	uninitengine();

	BloodINI.Save();

	if (syncstate)
		printf("A packet was lost! (syncstate)\n");

	dprintf("All subsystems shut down.  Processing exit functions\n");

	errSetHandler(prevErrorHandler);
}
