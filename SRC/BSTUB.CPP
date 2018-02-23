/*******************************************************************************
	FILE:			BSTUB.CPP

	DESCRIPTION:	This file contains enhancements to the BUILD editor, both
					in 3d mode and 2d mode.

	AUTHOR:			Peter M. Freese
	CREATED:		94/11/01
	COPYRIGHT:		Copyright (c) 1995 Q Studios Corporation
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <io.h>
#include <fcntl.h>
#include <dos.h>
#include <conio.h>

#include "engine.h"
#include "build.h"

#include "typedefs.h"
#include "bstub.h"
#include "db.h"
#include "screen.h"
#include "misc.h"
#include "gameutil.h"
#include "sectorfx.h"
#include "globals.h"
#include "key.h"
#include "textio.h"
#include "inifile.h"
#include "helix.h"
#include "debug4g.h"
#include "qheap.h"
#include "resource.h"
#include "gui.h"
#include "error.h"
#include "protect.h"
#include "tile.h"
#include "options.h"
#include "gfx.h"
#include "timer.h"
#include "trig.h"
#include "edit2d.h"
#include "mouse.h"
#include "eventq.h"

#include "names.h"

#include <memcheck.h>


/*******************************************************************************
							Structures and typedefs
*******************************************************************************/

struct NAMED_TYPE
{
	short id;
	char *name;
};


/*******************************************************************************
							   Global Variables
*******************************************************************************/
int gHighlightThreshold;
int gStairHeight;
int gLBIntensity;
int gLBAttenuation;
int gLBReflections;
int gLBMaxBright;
int gLBRampDist;
int gPalette = kPalNormal;
BOOL gBeep;
BOOL gOldKeyMapping;
BOOL gPanning = TRUE;
char *gBoolNames[2];
char *gSpriteNames[1024];
char *gWallNames[1024];
char *gSectorNames[1024];
char *gWaveNames[16];
char *gDepthNames[4];
char *gCommandNames[256];
char *gRespawnNames[4];
int gSaveTime = 0;
int gSaveInterval;

/*******************************************************************************
These were globals that were referenced in other parts of build.c.  Ken should
make these non-static.
*******************************************************************************/
short grid = 4, gridlock = 1;
long zoom = 768;


/*******************************************************************************
								Local Variables
*******************************************************************************/
static char buffer[256] = "";
static EHF prevErrorHandler;

// options
IniFile MapEditINI("MAPEDIT.INI");


char buildkeys[] =
{
	KEY_UP,
	KEY_DOWN,
	KEY_LEFT,
	KEY_RIGHT,
	KEY_LSHIFT,
	KEY_RCTRL,
	KEY_LCTRL,
	KEY_SPACE,
	KEY_A,
	KEY_Z,
	KEY_PAGEDN,
	KEY_PAGEUP,
	KEY_PAD7,
	KEY_PAD9,
	KEY_PADENTER,
	KEY_ENTER,
	KEY_PLUS,
	KEY_MINUS,
	KEY_TAB,
};


static NAMED_TYPE boolNTNames[] =
{
	0,		"Off",
	1,		"On",
};


static NAMED_TYPE spriteNTNames[] =
{
	kNothing,				"Decoration",
	kMarkerPlayerStart, 	"Player Start",
	kMarkerDeathStart, 		"Bloodbath Start",
	kMarkerOff,				"Off marker",
	kMarkerOn,				"On marker",
	kMarkerAxis,			"Axis marker",

	kMarkerUpperLink,		"Upper link",
	kMarkerLowerLink,		"Lower link",
	kMarkerWarpDest,		"Teleport target",
	kMarkerRaingen,			"Rain generator",

	kSwitchToggle,			"Toggle switch",
	kSwitchMomentary,		"Momentary switch",
	kSwitchCombination,		"Combination switch",
	kSwitchPadlock,			"Padlock (1-shot)",

	kMiscTorch,				"Torch",
	kMiscHangingTorch,		"Hanging Torch",

	kWeaponItemRandom,		gWeaponText[kWeaponItemRandom		- kWeaponItemBase],
	kWeaponItemShotgun,		gWeaponText[kWeaponItemShotgun		- kWeaponItemBase],
	kWeaponItemTommyGun,	gWeaponText[kWeaponItemTommyGun		- kWeaponItemBase],
	kWeaponItemFlareGun,	gWeaponText[kWeaponItemFlareGun		- kWeaponItemBase],
	kWeaponItemVoodooDoll,	gWeaponText[kWeaponItemVoodooDoll	- kWeaponItemBase],
	kWeaponItemSpearGun,	gWeaponText[kWeaponItemSpearGun		- kWeaponItemBase],
	kWeaponItemShadowGun,	gWeaponText[kWeaponItemShadowGun	- kWeaponItemBase],
	kWeaponItemPitchfork,	gWeaponText[kWeaponItemPitchfork	- kWeaponItemBase],
	kWeaponItemSprayCan,	gWeaponText[kWeaponItemSprayCan		- kWeaponItemBase],
	kWeaponItemTNT,			gWeaponText[kWeaponItemTNT			- kWeaponItemBase],

	kAmmoItemSprayCan,		gAmmoText[kAmmoItemSprayCan     - kAmmoItemBase],
	kAmmoItemTNTStick,		gAmmoText[kAmmoItemTNTStick     - kAmmoItemBase],
	kAmmoItemTNTBundle,		gAmmoText[kAmmoItemTNTBundle    - kAmmoItemBase],
	kAmmoItemTNTCase,		gAmmoText[kAmmoItemTNTCase		- kAmmoItemBase],
	kAmmoItemTNTProximity,	gAmmoText[kAmmoItemTNTProximity	- kAmmoItemBase],
 	kAmmoItemTNTRemote,		gAmmoText[kAmmoItemTNTRemote	- kAmmoItemBase],
 	kAmmoItemTNTTimer,		gAmmoText[kAmmoItemTNTTimer		- kAmmoItemBase],
	kAmmoItemShells,		gAmmoText[kAmmoItemShells		- kAmmoItemBase],
	kAmmoItemShellBox,		gAmmoText[kAmmoItemShellBox		- kAmmoItemBase],
	kAmmoItemBullets,		gAmmoText[kAmmoItemBullets		- kAmmoItemBase],
	kAmmoItemBulletBox,		gAmmoText[kAmmoItemBulletBox	- kAmmoItemBase],
	kAmmoItemAPBullets,		gAmmoText[kAmmoItemAPBullets	- kAmmoItemBase],
	kAmmoItemTommyDrum,		gAmmoText[kAmmoItemTommyDrum	- kAmmoItemBase],
	kAmmoItemSpear,			gAmmoText[kAmmoItemSpear		- kAmmoItemBase],
	kAmmoItemSpearPack,		gAmmoText[kAmmoItemSpearPack	- kAmmoItemBase],
	kAmmoItemHESpears,		gAmmoText[kAmmoItemHESpears		- kAmmoItemBase],
	kAmmoItemFlares,		gAmmoText[kAmmoItemFlares		- kAmmoItemBase],
	kAmmoItemHEFlares,		gAmmoText[kAmmoItemHEFlares		- kAmmoItemBase],
	kAmmoItemStarFlares,	gAmmoText[kAmmoItemStarFlares	- kAmmoItemBase],
	kAmmoItemRandom,	    "Random Ammo",

	kItemKey1,              gItemText[kItemKey1				- kItemBase],
	kItemKey2,              gItemText[kItemKey2				- kItemBase],
	kItemKey3,              gItemText[kItemKey3				- kItemBase],
	kItemKey4,              gItemText[kItemKey4				- kItemBase],
	kItemKey5,              gItemText[kItemKey5				- kItemBase],
	kItemKey6,              gItemText[kItemKey6				- kItemBase],
	kItemKey7,              gItemText[kItemKey7				- kItemBase],
	kItemDoctorBag,         gItemText[kItemDoctorBag		- kItemBase],
	kItemMedPouch,          gItemText[kItemMedPouch			- kItemBase],
	kItemLifeEssence,       gItemText[kItemLifeEssence		- kItemBase],
	kItemLifeSeed,          gItemText[kItemLifeSeed			- kItemBase],
	kItemPotion1,           gItemText[kItemPotion1			- kItemBase],
	kItemFeatherFall,       gItemText[kItemFeatherFall		- kItemBase],
	kItemLtdInvisibility,   gItemText[kItemLtdInvisibility	- kItemBase],
	kItemInvulnerability,   gItemText[kItemInvulnerability	- kItemBase],
	kItemJumpBoots,			gItemText[kItemJumpBoots		- kItemBase],
	kItemRavenFlight,		gItemText[kItemRavenFlight		- kItemBase],
	kItemGunsAkimbo,        gItemText[kItemGunsAkimbo		- kItemBase],
	kItemDivingSuit,        gItemText[kItemDivingSuit		- kItemBase],
	kItemGasMask,           gItemText[kItemGasMask			- kItemBase],
	kItemClone,             gItemText[kItemClone			- kItemBase],
	kItemCrystalBall,       gItemText[kItemCrystalBall		- kItemBase],
	kItemDecoy,             gItemText[kItemDecoy			- kItemBase],
	kItemDoppleganger,      gItemText[kItemDoppleganger		- kItemBase],
	kItemReflectiveShots,   gItemText[kItemReflectiveShots	- kItemBase],
	kItemRoseGlasses,       gItemText[kItemRoseGlasses		- kItemBase],
	kItemShadowCloak,       gItemText[kItemShadowCloak		- kItemBase],
	kItemShroomRage,        gItemText[kItemShroomRage		- kItemBase],
	kItemShroomDelirium,    gItemText[kItemShroomDelirium	- kItemBase],
	kItemShroomGrow,        gItemText[kItemShroomGrow		- kItemBase],
	kItemShroomShrink,      gItemText[kItemShroomShrink		- kItemBase],
	kItemDeathMask,         gItemText[kItemDeathMask		- kItemBase],
	kItemWineGoblet,        gItemText[kItemWineGoblet		- kItemBase],
	kItemWineBottle,        gItemText[kItemWineBottle		- kItemBase],
	kItemSkullGrail,        gItemText[kItemSkullGrail		- kItemBase],
	kItemSilverGrail,       gItemText[kItemSilverGrail		- kItemBase],
	kItemTome,              gItemText[kItemTome				- kItemBase],
	kItemBlackChest,        gItemText[kItemBlackChest		- kItemBase],
	kItemWoodenChest,       gItemText[kItemWoodenChest		- kItemBase],
	kItemAsbestosArmor,		gItemText[kItemAsbestosArmor	- kItemBase],
	kItemRandom,            "Random Item",

	kDudeRandom,			"Random Creature",
	kDudeTommyCultist, 		"Cultist w/Tommy",
	kDudeShotgunCultist,	"Cultist w/Shotgun",
	kDudeAxeZombie,    		"Axe Zombie",
	kDudeFatZombie,    		"Fat Zombie",
	kDudeEarthZombie,  		"Earth Zombie",
	kDudeFleshGargoyle,		"Flesh Gargoyle",
	kDudeStoneGargoyle,		"Stone Gargoyle",
	kDudeFleshStatue,  		"Flesh Statue",
	kDudeStoneStatue,  		"Stone Statue",
	kDudePhantasm,     		"Phantasm",
	kDudeHound,        		"Hound",
	kDudeHand,         		"Hand",
	kDudeBrownSpider,  		"Brown Spider",
	kDudeRedSpider,    		"Red Spider",
	kDudeBlackSpider,  		"Black Spider",
	kDudeMotherSpider, 		"Mother Spider",
	kDudeGillBeast,    		"GillBeast",
	kDudeEel,          		"Eel",
	kDudeBat,          		"Bat",
	kDudeRat,          		"Rat",
	kDudeGreenPod,     		"Green Pod",
	kDudeGreenTentacle,		"Green Tentacle",
	kDudeFirePod,      		"Fire Pod",
	kDudeFireTentacle, 		"Fire Tentacle",
	kDudeMotherPod,    		"Mother Pod",
	kDudeMotherTentacle,	"Mother Tentacle",
	kDudeCerberus,     		"Cerberus",
	kDudeTchernobog,   		"Tchernobog",
	kDudeRachel,       		"Rachel",

	kThingTNTBarrel,		"TNT Barrel",
	kThingTNTProxArmed,		"Armed Proximity Bomb",
	kThingTNTRemArmed,		"Armed Remote Bomb",
	kThingBlueVase,			"Blue Vase",
	kThingBrownVase,		"Brown Vase",
	kThingCrateFace,		"Crate Face",
	kThingClearGlass,		"Clear Glass",
	kThingFluorescent,		"Fluorescent Light",
	kThingWallCrack,		"Wall Crack",
	kThingWoodBeam,			"Wood Beam",
	kThingWeb,				"Spider's Web",
	kThingMetalGrate1,		"MetalGrate1",
	kThingFlammableTree,	"FlammableTree",
	kThingMachineGun,		"Machine Gun",

	kTrapSpiketrap,			"Spike Trap",
	kTrapRocktrap,			"Rock Trap",
	kTrapSawBlade,			"Saw Blade",
	kTrapUnpoweredZap,		"Electric Zap",
	kTrapPoweredZap,		"Switched Zap",
	kTrapPendulum,			"Pendulum",
	kTrapGuillotine,		"Guillotine",

	kGenTrigger,			"Trigger Gen",
	kGenWaterDrip,			"WaterDrip Gen",
	kGenBloodDrip,			"BloodDrip Gen",
	kGenFireball,			"Fireball Gen",
	kGenEctoSkull,			"EctoSkull Gen",
	kGenDart,				"Dart Gen",
	kGenBubble,				"Bubble Gen",
	kGenBubbles,			"Multi-Bubble Gen",
	kGenSound,				"Sound Gen",
};


static NAMED_TYPE wallNTNames[] =
{
	0,                      "Normal",
	kSwitchToggle,			"Toggle switch",
	kSwitchMomentary,		"Momentary switch",
	kWallForcefield,		"Forcefield",
	kWallCrushing,			"Crushing",
//	kWallPendulum,			"Pendulum",
	kWallSpeartrap,			"Speartrap",
	kWallFlametrap,			"Flametrap",
	kWallRazordoor,			"Razor Door",
//	kWallGuillotine,		"Guillotine",
	kWallClearGlass,		"Clear Glass",
	kWallStainedGlass,		"Stained Glass",
	kWallWoodBeams,			"Wood Beams",
	kWallWeb,				"Spider's Web",
//	kWallMetalGrate1,		"MetalGrate1"
};


static NAMED_TYPE sectorNTNames[] =
{
	0,						"Normal",
	kSectorZMotion,			"Z Motion",
	kSectorZCrusher,		"Z Crusher",
	kSectorZSprite,			"Z Motion SPRITE",
	kSectorWarp,			"Warp",
	kSectorTeleport,		"Teleporter",
	kSectorUpperwater,		"Upper water",
	kSectorLowerwater,		"Lower water",
	kSectorSlideMarked,		"Slide Marked",
	kSectorRotateMarked,	"Rotate Marked",
	kSectorSlide,			"Slide",
	kSectorRotate,			"Rotate",
	kSectorSlideCrush,		"Slide Crush",
	kSectorRotateCrush,		"Rotate Crush",
};


static NAMED_TYPE waveNTNames[] =
{
	kWaveNone,				"None",
	kWaveSquare,			"Square",
	kWaveSaw,				"Saw",
	kWaveRampup,			"Ramp up",
	kWaveRampdown,			"Ramp down",
	kWaveSine,				"Sine",
	kWaveFlicker1,			"Flicker1",
	kWaveFlicker2,			"Flicker2",
	kWaveFlicker3,			"Flicker3",
	kWaveFlicker4,			"Flicker4",
	kWaveStrobe,			"Strobe",
	kWaveSearch,			"Search",
};


static NAMED_TYPE depthNTNames[] =
{
	kDepthWalk,   "Walk",
	kDepthTread,  "Tread",
	kDepthWade,   "Wade",
	kDepthSwim,   "Swim*",
};


static NAMED_TYPE commandNTNames[] =
{
	kCommandOff,		"OFF",
	kCommandOn,			"ON",
	kCommandState,		"State",
	kCommandToggle,		"Toggle",
	kCommandNotState,	"!State",
	kCommandLink,		"Link",
	kCommandLock,		"Lock",
	kCommandUnlock,		"Unlock",
	kCommandToggleLock,	"Tog-Lock",
};


static NAMED_TYPE respawnNTNames[] =
{
	0, "Never",
	1, "Optional",
	2, "Always",
	3, "Permanent",
};


/***********************************************************************
 * Functions
 **********************************************************************/
void ProcessKeys3D( void );
void ProcessKeys2D( void );


/***********************************************************************
 * Wait()
 *
 * Delays for a period of time specified in 1/120th secs
 **********************************************************************/
void Wait(long time)
{
	long done = gGameClock + time;
	while (gGameClock < done);
}


void BeepOkay( void )
{
	if (gBeep)
	{
		sound(6000);
		Wait(2);
		nosound();
		Wait(2);
	}
	asksave = 1;
}


void BeepError( void )
{
	sound(1000);
	Wait(4);
	sound(800);
	Wait(4);
	nosound();
}


/***********************************************************************
 * CloseBanner()
 *
 * Display credits when program exits
 **********************************************************************/
void CloseBanner( void )
{
	setvmode(gOldDisplayMode);
	printf("\nBUILD engine by Ken Silverman\n");
	printf("MAPEDIT version by Peter Freese\n");
	printf("Copyright (c) 1994, 1995 Q Studios\n");
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


void InitializeNames( void )
{
	static char numbers[192][4];
	FillNameArray(gBoolNames, boolNTNames, LENGTH(boolNTNames));
	FillNameArray(gSpriteNames, spriteNTNames, LENGTH(spriteNTNames));
	FillNameArray(gWallNames, wallNTNames, LENGTH(wallNTNames));
	FillNameArray(gSectorNames, sectorNTNames, LENGTH(sectorNTNames));
	FillNameArray(gWaveNames, waveNTNames, LENGTH(waveNTNames));
	FillNameArray(gDepthNames, depthNTNames, LENGTH(depthNTNames));
	FillNameArray(gCommandNames, commandNTNames, LENGTH(commandNTNames));
	for (int i = 0; i < 192; i++)
	{
		sprintf(numbers[i], "%d", i);
		gCommandNames[64 + i] = numbers[i];
	}
	FillNameArray(gRespawnNames, respawnNTNames, LENGTH(respawnNTNames));
}


/*******************************************************************************
	FUNCTION:		CleanUp()

	DESCRIPTION:	Call this function periodically to fix all the problems
					Ken causes by rearranging the database with abandon.
*******************************************************************************/
void CleanUp( void )
{
	int nSprite, nSector, j;

	dbXSectorClean();
	dbXWallClean();
	dbXSpriteClean();

	// make sure proper sectors are on the effect lists
	InitSectorFX();

	// back propagate marker references
	for (nSector = 0; nSector < numsectors; nSector++)
	{
		int nXSector = sector[nSector].extra;
		if (nXSector > 0)
		{
			switch (sector[nSector].type)
			{
				case kSectorTeleport:
					if (xsector[nXSector].marker0 >= 0)
					{
						int nSprite = xsector[nXSector].marker0;
						if (nSprite < kMaxSprites && sprite[nSprite].statnum == kStatMarker && sprite[nSprite].type == kMarkerWarpDest)
							sprite[nSprite].owner = (short)nSector;
						else
						{
							xsector[nXSector].marker0 = -1;
							dprintf("Invalid teleport sector -> target marker reference found\n");
						}
					}
					break;

				case kSectorSlide:
				case kSectorSlideMarked:
				case kSectorSlideCrush:
					if (xsector[nXSector].marker0 >= 0)
					{
						int nSprite = xsector[nXSector].marker0;
						if (nSprite < kMaxSprites && sprite[nSprite].statnum == kStatMarker && sprite[nSprite].type == kMarkerOff)
							sprite[nSprite].owner = (short)nSector;
						else
						{
							xsector[nXSector].marker0 = -1;
							dprintf("Invalid slide sector -> off marker reference found\n");
						}
					}

 					if (xsector[nXSector].marker1 >= 0)
					{
						int nSprite = xsector[nXSector].marker1;
						if (nSprite < kMaxSprites && sprite[nSprite].statnum == kStatMarker && sprite[nSprite].type == kMarkerOn)
							sprite[nSprite].owner = (short)nSector;
						else
						{
							xsector[nXSector].marker1 = -1;
							dprintf("Invalid slide sector -> on marker reference found\n");
						}
					}
					break;

				case kSectorRotate:
				case kSectorRotateMarked:
				case kSectorRotateCrush:
					if (xsector[nXSector].marker0 >= 0)
					{
						int nSprite = xsector[nXSector].marker0;
						if (nSprite < kMaxSprites && sprite[nSprite].statnum == kStatMarker && sprite[nSprite].type == kMarkerAxis)
							sprite[nSprite].owner = (short)nSector;
						else
						{
							xsector[nXSector].marker0 = -1;
							dprintf("Invalid rotate sector -> axis marker reference found\n");
						}
					}
					break;
			}
		}
	}

	// clear invalid marker references and add new markers as necessary
	for (nSector = 0; nSector < numsectors; nSector++)
	{
		int nXSector = sector[nSector].extra;
		if (nXSector > 0)
		{
			switch (sector[nSector].type)
			{
				case kSectorTeleport:
					if (xsector[nXSector].marker0 >= 0)
					{
						int nSprite = xsector[nXSector].marker0;
						if (sprite[nSprite].owner != (short)nSector)
						{
							// clone the marker
							dprintf("Cloning teleport sector -> target marker\n");
							int nSprite2 = insertsprite(sprite[nSprite].sectnum, kStatMarker);
							sprite[nSprite2] = sprite[nSprite];
							sprite[nSprite2].owner = (short)nSector;
							xsector[nXSector].marker0 = nSprite2;
						}
					}

					if (xsector[nXSector].marker0 < 0)
					{
						dprintf("Creating new teleport sector -> target marker\n");
						int nSprite = insertsprite((short)nSector, kStatMarker);
						sprite[nSprite].x = wall[sector[nSector].wallptr].x;
						sprite[nSprite].y = wall[sector[nSector].wallptr].y;
						sprite[nSprite].cstat |= kSpriteInvisible;
						sprite[nSprite].owner = (short)nSector;
						sprite[nSprite].type = kMarkerWarpDest;
						xsector[nXSector].marker0 = nSprite;
					}
					break;

				case kSectorSlide:
				case kSectorSlideMarked:
				case kSectorSlideCrush:
					if (xsector[nXSector].marker0 >= 0)
					{
						int nSprite = xsector[nXSector].marker0;
						if (sprite[nSprite].owner != (short)nSector)
						{
							// clone the marker
							dprintf("Cloning slide sector -> off marker\n");
							int nSprite2 = insertsprite(sprite[nSprite].sectnum, kStatMarker);
							sprite[nSprite2] = sprite[nSprite];
							sprite[nSprite2].owner = (short)nSector;
							xsector[nXSector].marker0 = nSprite2;
						}
					}

					if (xsector[nXSector].marker0 < 0)
					{
						// create a new marker
						dprintf("Creating new slide sector -> off marker\n");
						int nSprite = insertsprite((short)nSector, kStatMarker);
						sprite[nSprite].x = wall[sector[nSector].wallptr].x;
						sprite[nSprite].y = wall[sector[nSector].wallptr].y;
						sprite[nSprite].cstat |= kSpriteInvisible;
						sprite[nSprite].owner = (short)nSector;
						sprite[nSprite].type = kMarkerOff;
						xsector[nXSector].marker0 = nSprite;
					}

 					if (xsector[nXSector].marker1 >= 0)
					{
						int nSprite = xsector[nXSector].marker1;
						if (sprite[nSprite].owner != (short)nSector)
						{
							// clone the marker
							dprintf("Cloning slide sector -> on marker\n");
							int nSprite2 = insertsprite(sprite[nSprite].sectnum, kStatMarker);
							sprite[nSprite2] = sprite[nSprite];
							sprite[nSprite2].owner = (short)nSector;
							xsector[nXSector].marker1 = (short)nSprite2;
						}
					}

					if (xsector[nXSector].marker1 < 0)
					{
						dprintf("Creating new slide sector -> on marker\n");
						int nSprite = insertsprite((short)nSector, kStatMarker);
						sprite[nSprite].x = wall[sector[nSector].wallptr].x;
						sprite[nSprite].y = wall[sector[nSector].wallptr].y;
						sprite[nSprite].cstat |= kSpriteInvisible;
						sprite[nSprite].owner = (short)nSector;
						sprite[nSprite].type = kMarkerOn;
						xsector[nXSector].marker1 = nSprite;
					}
					break;

				case kSectorRotate:
				case kSectorRotateMarked:
				case kSectorRotateCrush:
					if (xsector[nXSector].marker0 >= 0)
					{
						int nSprite = xsector[nXSector].marker0;
						if (sprite[nSprite].owner != (short)nSector)
						{
							// clone the marker
							dprintf("Cloning rotate sector -> axis marker\n");
							int nSprite2 = insertsprite(sprite[nSprite].sectnum, kStatMarker);
							sprite[nSprite2] = sprite[nSprite];
							sprite[nSprite2].owner = (short)nSector;
							xsector[nXSector].marker0 = nSprite2;
						}
					}

					if (xsector[nXSector].marker0 < 0)
					{
						dprintf("Creating new rotate sector -> axis marker\n");
						int nSprite = insertsprite((short)nSector, kStatMarker);
						sprite[nSprite].x = wall[sector[nSector].wallptr].x;
						sprite[nSprite].y = wall[sector[nSector].wallptr].y;
						sprite[nSprite].cstat |= kSpriteInvisible;
						sprite[nSprite].owner = (short)nSector;
						sprite[nSprite].type = kMarkerAxis;
						xsector[nXSector].marker0 = nSprite;
					}
					break;

				default:
					xsector[nXSector].marker0 = -1;
					xsector[nXSector].marker1 = -1;
					break;
			}
		}
	}

	// delete all invalid marker sprites
	for (nSprite = headspritestat[kStatMarker]; nSprite != -1; nSprite = j)
	{
		j = nextspritestat[nSprite];
		sprite[nSprite].extra = -1;
		sprite[nSprite].cstat |= kSpriteInvisible;
		sprite[nSprite].cstat &= ~kSpriteBlocking;

		int nSector = sprite[nSprite].owner;
		int nXSector = sector[nSector].extra;

		if (nSector >= 0 && nSector < numsectors && nXSector > 0 && nXSector < kMaxXSectors)
		{
			switch (sprite[nSprite].type)
			{
				case kMarkerOff:
					if ( xsector[nXSector].marker0 == nSprite )
						continue;
					break;

				case kMarkerOn:
					if ( xsector[nXSector].marker1 == nSprite)
						continue;
					break;

				case kMarkerAxis:
					if ( xsector[nXSector].marker0 == nSprite)
						continue;
					break;

				case kMarkerWarpDest:
					if ( xsector[nXSector].marker0 == nSprite)
						continue;
					break;
			}
		}

		dprintf("Deleting unreferenced marker sprite\n");
		deletesprite((short)nSprite);
	}
}


/***********************************************************************
 * GetXSector()
 *
 * Returns the index of the xsector structure for a sector index. If
 * the xsector structure does not exist, it is created.
 **********************************************************************/
int GetXSector( int nSector )
{
	dassert(nSector >= 0 && nSector < kMaxSectors);

	if (sector[nSector].extra != -1)
		return sector[nSector].extra;

	return dbInsertXSector(nSector);
}


/***********************************************************************
 * GetXWall()
 *
 * Returns the index of the xwall structure for a wall index. If
 * the xwall structure does not exist, it is created.
 **********************************************************************/
int GetXWall( int nWall )
{
	int nXWall;

	dassert(nWall >= 0 && nWall < kMaxWalls);

	if (wall[nWall].extra > 0)
		return wall[nWall].extra;

	nXWall = dbInsertXWall(nWall);
	return nXWall;
}


/***********************************************************************
 * GetXSprite()
 *
 * Returns the index of the xsprite structure for a sprite index. If
 * the xsprite structure does not exist, it is created.
 **********************************************************************/
int GetXSprite( int nSprite )
{
	int nXSprite;

	dassert(nSprite >= 0 && nSprite < kMaxSprites);

	if (sprite[nSprite].extra > 0)
		return sprite[nSprite].extra;

	nXSprite = dbInsertXSprite(nSprite);
	return nXSprite;
}


struct AUTODATA
{
	short	type;
	short	picnum;
	short	xrepeat, yrepeat;
	uchar	hitBit;
	short	plu;
};

AUTODATA autoData[] =
{
	{   kMarkerPlayerStart,	-1,					-1,	-1,	0,	kPLUNormal },
	{   kMarkerDeathStart,	-1,					-1,	-1,	0,	kPLUGray },
	{   kMarkerUpperLink,	2332,				16,	16,	0,	-1 },
	{   kMarkerLowerLink,	2332,				16,	16,	0,	-1 },

	{	kSwitchToggle,		-1,					-1,	-1,	1,	-1 },
	{	kSwitchMomentary,	-1,					-1,	-1,	1,	-1 },
	{	kSwitchCombination,	-1,					-1,	-1,	1,	-1 },
	{	kSwitchPadlock,		kPicPadlock,		-1,	-1,	1,	-1 },

	{	kMiscTorch,			kPicTorchPot1,		-1,	-1,	1,	-1 },
	{	kMiscTorch,			kPicTorchPot2,		-1,	-1,	1,	-1 },
	{	kMiscTorch,			kPicTorchStand1,	-1,	-1,	1,	-1 },
	{	kMiscTorch,			kPicTorchStand2,	-1,	-1,	1,	-1 },
	{	kMiscTorch,			kPicTorchSconce1,	-1,	-1,	1,	-1 },
	{	kMiscTorch,			kPicTorchSconce2,	-1,	-1,	1,	-1 },

	{	kMiscHangingTorch,	kPicTorchBowl1,		-1,	-1,	1,	-1 },

	{	kWeaponItemRandom,		kPicRandomUp,		48,	48,	0,	kPLUNormal },
	{	kWeaponItemShotgun,		kPicShotgun,		48,	48,	0,	kPLUNormal },
	{	kWeaponItemTommyGun,	kPicTommyGun,		48,	48,	0,	kPLUNormal },
	{	kWeaponItemFlareGun,	kPicFlareGun,		48,	48,	0,	kPLUNormal },
	{	kWeaponItemVoodooDoll,	kPicVoodooDoll,		48,	48,	0,	kPLUNormal },
	{	kWeaponItemSpearGun,	kPicSpearGun,		48,	48,	0,	kPLUNormal },
	{	kWeaponItemShadowGun,	kPicShadowGun,		48,	48,	0,	kPLUNormal },

	{	kAmmoItemSprayCan,		kPicSprayCan,		40,	40,	1,	kPLUNormal },
	{	kAmmoItemTNTStick,		kPicTNTStick,		48,	48,	1,	kPLUNormal },
	{	kAmmoItemTNTBundle,		kPicTNTPak,			48,	48,	1,	kPLUNormal },
	{	kAmmoItemTNTCase,		kPicTNTBox,			48,	48,	1,	kPLUNormal },
	{	kAmmoItemTNTProximity,	kPicTNTProx,		48,	48,	0,	kPLUNormal },
	{	kAmmoItemTNTRemote,		kPicTNTRemote,		48,	48,	0,	kPLUNormal },
	{	kAmmoItemTNTTimer,		kPicTNTTimer,		48,	48,	0,	kPLUNormal },
	{	kAmmoItemShells,		kPicShotShells,		48,	48,	0,	kPLUNormal },
	{	kAmmoItemShellBox,		kPicShellBox,		48,	48,	0,	kPLUNormal },
	{	kAmmoItemBullets,		kPicBullets,		48,	48,	0,	kPLUNormal },
	{	kAmmoItemBulletBox,		kPicBulletBox,		48,	48,	0,	kPLUNormal },
	{	kAmmoItemAPBullets,		kPicBulletBoxAP,	48,	48,	0,	kPLUNormal },
	{	kAmmoItemTommyDrum,		kPicTommyDrum,		48,	48,	0,	kPLUNormal },
	{	kAmmoItemSpear,			kPicSpear,			48,	48,	0,	kPLUNormal },
	{	kAmmoItemSpearPack,		kPicSpears,			48,	48,	0,	kPLUNormal },
	{	kAmmoItemHESpears,		kPicSpearExplode,	48,	48,	1,	kPLUNormal },
	{	kAmmoItemFlares,		kPicFlares,			48,	48,	0,	kPLUNormal },
	{	kAmmoItemHEFlares,		kPicFlareHE,		48,	48,	1,	kPLUNormal },
	{	kAmmoItemStarFlares,	kPicFlareBurst,		48,	48,	0,	kPLUNormal },
	{	kAmmoItemRandom,		kPicRandomUp,		40,	40,	0,	kPLUNormal },

	{	kItemKey1,			kPicKey1,			32,	32,	0,	kPLUNormal },
	{	kItemKey2,			kPicKey2,			32,	32,	0,	kPLUNormal },
	{	kItemKey3,			kPicKey3,			32,	32,	0,	kPLUNormal },
	{	kItemKey4,			kPicKey4,			32,	32,	0,	kPLUNormal },
	{	kItemKey5,			kPicKey5,			32,	32,	0,	kPLUNormal },
	{	kItemKey6,			kPicKey6,			32,	32,	0,	kPLUNormal },
	{	kItemKey7,			-1,					-1,	-1,	0,	kPLUNormal },
	{	kItemDoctorBag,     kPicDocBag,			48,	48,	0,	kPLUNormal },
	{	kItemMedPouch,		kPicMedPouch,		40,	40,	0,	kPLUNormal },
	{	kItemLifeEssence,	kPicEssence,		40,	40,	0,	kPLUNormal },
	{	kItemLifeSeed,		kAnmLifeSeed,		40,	40,	0,	kPLUNormal },
	{	kItemPotion1,		kPicPotion,			40,	40,	0,	kPLUNormal },
	{	kItemFeatherFall,	kAnmFeather,		40,	40,	0,	kPLUNormal },
	{	kItemLtdInvisibility,kAnmInviso,		40,	40,	0,	kPLUNormal },
	{	kItemInvulnerability,kPicInvulnerable,	40,	40,	0,	kPLUNormal },
	{	kItemJumpBoots,		kPicJumpBoots,		40,	40,	0,	kPLUNormal },
	{	kItemRavenFlight,	kPicRavenFlight,	40,	40,	0,	kPLUNormal },
	{	kItemGunsAkimbo,	kPicGunsAkimbo,		40,	40,	0,	kPLUNormal },
	{	kItemDivingSuit,	kPicDivingSuit,		80,	64,	0,	kPLUNormal },
	{	kItemGasMask,		kPicGasMask,		40,	40,	0,	kPLUNormal },
	{	kItemClone,			kAnmClone,			40,	40,	0,	kPLUNormal },
	{	kItemCrystalBall,   kPicCrystalBall,	40,	40,	0,	kPLUNormal },
	{	kItemDecoy,			kPicDecoy,			40,	40,	0,	kPLUNormal },
	{	kItemDoppleganger,	kAnmDoppleganger,	40,	40,	0,	kPLUNormal },
	{	kItemReflectiveShots,kAnmReflectShots,	40,	40,	0,	kPLUNormal },
	{	kItemRoseGlasses,	kPicRoseGlasses,	40,	40,	0,	kPLUNormal },
	{	kItemShadowCloak,	kAnmCloakNDagger,	64,	64,	0,	kPLUNormal },
	{	kItemShroomRage,    kPicShroom1,		48,	48,	0,	kPLUNormal },
	{	kItemShroomDelirium,kPicShroom2,		48,	48,	0,	kPLUNormal },
	{	kItemShroomGrow,	kPicShroom3,		48,	48,	0,	kPLUNormal },
	{	kItemShroomShrink,	kPicShroom4,		48,	48,	0,	kPLUNormal },
	{	kItemDeathMask,		kPicDeathMask,		40,	40,	0,	kPLUNormal },
	{	kItemWineGoblet,	kPicGoblet,			40,	40,	0,	kPLUNormal },
	{	kItemWineBottle,	kPicBottle1,		40,	40,	0,	kPLUNormal },
	{	kItemSkullGrail,	kPicSkullGrail,		40,	40,	0,	kPLUNormal },
	{	kItemSilverGrail,   kPicSilverGrail,	40,	40,	0,	kPLUNormal },
	{	kItemTome,			kPicTome1,			40,	40,	0,	kPLUNormal },
	{	kItemBlackChest,	kPicBlackChest,		40,	40,	0,	kPLUNormal },
	{	kItemWoodenChest,	kPicWoodChest,		40,	40,	0,	kPLUNormal },
	{	kItemAsbestosArmor,	kPicAsbestosSuit,	80,	64,	0,	kPLUNormal },
	{	kItemRandom,		kPicRandomUp,		40,	40,	0,	kPLUNormal },

	{	kDudeRandom,		kPicRandomUp,		64,	64,	1,	kPLUNormal },
	{	kDudeTommyCultist,	kAnmCultistA1,		40,	40,	1,	kPLUCultist2 },
	{	kDudeShotgunCultist,kPicShotgunIdle,	40,	40,	1,	kPLUNormal },
	{	kDudeAxeZombie,		kAnmZomb1M1,		38,	38,	1,	kPLUNormal },
	{	kDudeFatZombie,		kAnmZomb2M1,		40,	40,	1,	kPLUNormal },
	{	kDudeEarthZombie,	kPicEarthIdle,		38,	38,	1,	kPLUNormal },
	{	kDudeFleshGargoyle,	kAnmGargoyleM1,		40,	40,	1,	kPLUNormal },
	{	kDudeStoneGargoyle,	kAnmGargoyleM1,		40,	40,	1,	kPLUGray },
	{	kDudeFleshStatue,	kAnmGargStatue,		40,	40,	1,	kPLUNormal },
	{	kDudeStoneStatue,	kAnmGargStatue,		40,	40,	1,	kPLUGray },
	{	kDudePhantasm,		kAnmPhantasmM1,		40,	40,	1,	kPLUNormal },
	{	kDudeHound,         kAnmHellM2,			40,	40,	1,	kPLUNormal },
	{	kDudeHand,			kAnmHandM1,			32,	32,	1,	kPLUNormal },
	{	kDudeBrownSpider,   kAnmSpiderM1,		16,	16,	1,	kPLUSpider1 },
	{	kDudeRedSpider,		kAnmSpiderM1B,		24,	24,	1,	kPLUSpider2 },
	{	kDudeBlackSpider,	kAnmSpiderM1C,		32,	32,	1,	kPLUSpider3 },
	{	kDudeMotherSpider,	kAnmSpiderM1D,		40,	40,	1,	kPLUNormal },
	{	kDudeGillBeast,		kAnmGillM1,			48,	48,	1,	kPLUNormal },
	{	kDudeEel,			kAnmEelM1,			32,	32,	1,	kPLUNormal },
	{	kDudeBat,			kAnmBatM2,			32,	32,	1,	kPLUNormal },
	{	kDudeRat,			kAnmRatM1,			24,	24,	1,	kPLUNormal },
	{	kDudeGreenPod,		kAnmPodM1,			32,	32,	1,	kPLUNormal },
	{	kDudeGreenTentacle,	kAnmTentacleM1,		32,	32,	1,	kPLUNormal },
	{	kDudeFirePod,		kAnmPodM1,			48,	48,	1,	kPLURed },
	{	kDudeFireTentacle,	kAnmTentacleM1,		48,	48,	1,	kPLURed },
	{	kDudeMotherPod,		kAnmPodM1,			64,	64,	1,	kPLUGrayish },
	{	kDudeMotherTentacle,kAnmTentacleM1,		64,	64,	1,	kPLUGrayish },
	{	kDudeCerberus,		kAnmCerberusM1,		32,	32,	1,	kPLUNormal },
	{	kDudeTchernobog,	kAnmTchernobogM1,	40,	40,	1,	kPLUNormal },
	{	kDudeRachel,		kAnmRachelM1,		40,	40,	1,	kPLUNormal },
	{	kDudePlayer1,		kAnmCultistM1,		40,	40,	1,	kPLUNormal },
	{	kDudePlayer2,		kAnmCultistM1,		40,	40,	1,	kPLUNormal },
	{	kDudePlayer3,		kAnmCultistM1,		40,	40,	1,	kPLUNormal },
	{	kDudePlayer4,		kAnmCultistM1,		40,	40,	1,	kPLUNormal },
	{	kDudePlayer5,		kAnmCultistM1,		40,	40,	1,	kPLUNormal },
	{	kDudePlayer6,		kAnmCultistM1,		40,	40,	1,	kPLUNormal },
	{	kDudePlayer7,		kAnmCultistM1,		40,	40,	1,	kPLUNormal },
	{	kDudePlayer8,		kAnmCultistM1,		40,	40,	1,	kPLUNormal },
	{	kDudePlayer_Owned,	kAnmCultistM1,		40,	40,	1,	kPLUNormal },
	{	kDudeHound_Owned,	kAnmHellM2,			40,	40,	1,	kPLUNormal },
	{	kDudeEel_Owned,		kAnmEelM1,			32,	32,	1,	kPLUNormal },
	{	kDudeSpider_Owned,	kAnmSpiderM1,		16,	16,	1,	kPLUNormal },

	{	kThingTNTBarrel,	kPicTNTBarrel,		64,	64,	1,	kPLUNormal },
	{	kThingTNTProxArmed,	kAnmTNTProxArmed,	40,	40,	1,	kPLUNormal },
	{	kThingTNTRemArmed,	kAnmTNTRemArmed,	40,	40,	1,	kPLUNormal },
	{	kThingBlueVase,		kPicVase1,			-1,	-1,	1,	kPLUNormal },
	{	kThingBrownVase,	kPicVase2,			-1,	-1,	1,	kPLUNormal },
	{	kThingCrateFace,	kPicCrateFace,		-1,	-1,	1,	kPLUNormal },
	{	kThingClearGlass,	kPicGlass,			-1,	-1,	1,	kPLUNormal },
	{	kThingFluorescent,	kPicFluorescent,	-1,	-1,	1,	kPLUNormal },
	{	kThingWallCrack,	kPicWallCrack,		-1,	-1,	1,	kPLUNormal },
	{	kThingWoodBeam,		kPicWoodBeam,		-1,	-1,	1,	kPLUNormal },
	{	kThingWeb,			kPicBlockWeb,		-1,	-1,	1,	kPLUNormal },
	{	kThingMetalGrate1,	kPicMetalGrate1,	-1, -1, 1,	-1 },
	{	kThingFlammableTree,-1,					-1, -1, 1,	-1 },
	{	kThingMachineGun,	-1,					64,	64,	0,	kPLUNormal },

	{	kTrapSpiketrap,		kAnmFloorSpike,		64,	64,	0,	kPLUNormal },
	{	kTrapRocktrap,		-1,					64,	64,	0,	kPLUNormal },
	{	kTrapSatellite,		-1,					-1,	-1,	0,	kPLUNormal },
	{	kTrapSawBlade,		kAnmCircSaw1,		27,	32,	0,	kPLUNormal },
	{	kTrapUnpoweredZap,	kAnmElectrify,		-1,	-1,	0,	kPLUNormal },
	{	kTrapPoweredZap,	kAnmElectrify,		-1,	-1,	0,	kPLUNormal },
	{	kTrapPendulum,		kAnmPendulum,		-1,	-1,	0,	kPLUNormal },
	{	kTrapGuillotine,	kPicGuillotine,		-1,	-1,	0,	kPLUNormal },
};


/***********************************************************************
 * AutoAdjustSprites()
 *
 * Adjust sprite x/yrepeat values for oddly sized sprites
 **********************************************************************/
void AutoAdjustSprites(void)
{
	dprintf("Auto adjusting sprite attributes... ");
	for ( short nSprite = 0; nSprite < kMaxSprites; nSprite++ )
	{
		SPRITE *pSprite = &sprite[nSprite];

		if ( pSprite->statnum < kMaxStatus )
		{
			int i, iPicnum = -1, iType = -1;

			// clear out types having no name (probably invalid)
			if ( pSprite->type != 0 && gSpriteNames[sprite[nSprite].type] == NULL )
				pSprite->type = 0;

			// look up by picnum
			for ( i = 0; i < LENGTH(autoData); i++)
			{
				if ( autoData[i].picnum >= 0 && autoData[i].picnum == pSprite->picnum )
				{
					iPicnum = i;
					break;
				}
			}

			// look up by type
			for ( i = 0; i < LENGTH(autoData); i++)
			{
				if ( autoData[i].type == pSprite->type )
				{
					iType = i;
					break;
				}
			}

			i = -1;

			// this set of rules determine which row to use from the autoData table
			if ( iPicnum >= 0 )
				i = iPicnum;
			if ( iType >= 0 )
				i = iType;
			if ( iPicnum >= 0 && autoData[iPicnum].type == pSprite->type )
				i = iPicnum;

			// if nothing found, ignore this sprite
			if ( i < 0 )
				continue;

			// convert spin sprite flags to face sprites
			if ( (pSprite->cstat & kSpriteSpin) == kSpriteSpin )
				pSprite->cstat &= ~kSpriteSpin;

			short type = autoData[i].type;
			if ( type == kMarkerPlayerStart || type == kMarkerDeathStart)
			{
				pSprite->cstat &= ~kSpriteBlocking;
				changespritestat(nSprite, kStatDefault);

				int nXSprite = GetXSprite(nSprite);
				xsprite[nXSprite].data1 &= 7;	// force 0..7 player range
				pSprite->picnum = (short)(kPicStart1 + xsprite[nXSprite].data1);
			}
			else if ( type == kMarkerUpperLink || type == kMarkerLowerLink )
			{
				pSprite->cstat &= ~kSpriteBlocking;
				changespritestat(nSprite, kStatDefault);

				int nXSprite = GetXSprite(nSprite);

				if (type == kMarkerUpperLink)
					pSprite->cstat &= ~kSpriteFlipY;
				else
					pSprite->cstat |= kSpriteFlipY;
			}
			else if ( type >= kSwitchBase && type < kSwitchMax )
			{
				pSprite->cstat &= ~kSpriteBlocking;
				changespritestat(nSprite, kStatDefault);
				GetXSprite(nSprite);
			}
			else if ( type >= kWeaponItemBase && type < kWeaponItemMax )
			{
				if ( (pSprite->cstat & kSpriteRMask) != kSpriteFace )
					continue;

				pSprite->cstat &= ~kSpriteBlocking;
				changespritestat(nSprite, kStatItem);
			}
			else if ( type >= kAmmoItemBase && type < kAmmoItemMax )
			{
				if ( (pSprite->cstat & kSpriteRMask) != kSpriteFace )
					continue;

				pSprite->cstat &= ~kSpriteBlocking;
				changespritestat(nSprite, kStatItem);
			}
			else if ( type >= kItemBase && type < kItemMax )
			{
				if ( (pSprite->cstat & kSpriteRMask) != kSpriteFace )
					continue;

				pSprite->cstat &= ~kSpriteBlocking;
				changespritestat(nSprite, kStatItem);
			}
			else if ( type >= kDudeBase && type < kDudeMax )
			{
				if ( (pSprite->cstat & kSpriteRMask) != kSpriteFace )
					continue;

				pSprite->cstat &= ~kSpriteBlocking;
				changespritestat(nSprite, kStatDude);
				GetXSprite(nSprite);
			}
			else if ( type >= kThingBase && type < kThingMax )
			{
//				if ( (pSprite->cstat & kSpriteRMask) != kSpriteFace )
//					continue;

				pSprite->cstat |= kSpriteBlocking;
				changespritestat(nSprite, kStatThing);
				GetXSprite(nSprite);
			}
			else if ( type >= kTrapBase && type < kTrapMax )
			{
				// these get moved onto their appropriate stat lists in trInit()
				changespritestat(nSprite, kStatTraps);
				GetXSprite(nSprite);
			}
			else
				changespritestat(nSprite, kStatDefault);

			pSprite->type = type;

			if ( autoData[i].picnum >= 0 )
				pSprite->picnum = autoData[i].picnum;

			if ( autoData[i].xrepeat >= 0 )
				pSprite->xrepeat = (uchar)autoData[i].xrepeat;

			if ( autoData[i].yrepeat >= 0 )
				pSprite->yrepeat = (uchar)autoData[i].yrepeat;

			if ( autoData[i].hitBit )
				pSprite->cstat |= kSpriteHitscan;
			else
				pSprite->cstat &= ~kSpriteHitscan;

			if ( autoData[i].plu >= 0 )
				pSprite->pal = (uchar)autoData[i].plu;

			// make sure Things & Dudes aren't in the floor or ceiling
			if ( pSprite->statnum == kStatThing || pSprite->statnum == kStatDude || pSprite->statnum == kStatInactive )
			{
				int zTop, zBot;
				GetSpriteExtents(pSprite, &zTop, &zBot);
				if ( !(sector[pSprite->sectnum].ceilingstat & kSectorParallax) )
					pSprite->z += ClipLow(sector[pSprite->sectnum].ceilingz - zTop, 0);
				if ( !(sector[pSprite->sectnum].floorstat & kSectorParallax) )
					pSprite->z += ClipHigh(sector[pSprite->sectnum].floorz - zBot, 0);
			}
		}
	}
	dprintf("done\n");
}


/***********************************************************************
 * ShowSpriteStatistics()
 *
 * Generate and display game sprite statistics
 *
 **********************************************************************/
void ShowSpriteStatistics(short nSprite)
{
	short spriteCount[ 1024 ];

	memset( spriteCount, 0, sizeof(spriteCount) );

	// ignore nSprite parameter and use as iterator instead
	for (nSprite = 0; nSprite < kMaxSprites; nSprite++)
	{
		if ( sprite[nSprite].statnum < kMaxStatus )
			spriteCount[ sprite[nSprite].type ]++;
	}
}


/***********************************************************************
 * ExtGetSectorCaption()
 *
 * Build calls this to generate the sector caption displayed on the
 * 2D map.
 **********************************************************************/
const char *ExtGetSectorCaption(short nSector)
{
	dassert(nSector >= 0 && nSector < kMaxSectors);
	int nXSector = sector[nSector].extra;

	buffer[0] = 0;
	if (nXSector > 0)
	{
		char temp[256];
		if ( xsector[nXSector].rxID > 0 )
		{
			sprintf(buffer, "%i:", xsector[nXSector].rxID);
		}

		strcat(buffer, gSectorNames[sector[nSector].type]);

		if ( xsector[nXSector].txID > 0 )
		{
			sprintf(temp, ":%i", xsector[nXSector].txID);
			strcat(buffer, temp);
		}

		if ( xsector[nXSector].panVel != 0 )
		{
			sprintf(temp, " PAN(%i,%i)", xsector[nXSector].panAngle, xsector[nXSector].panVel);
			strcat(buffer, temp);
		}

		strcat(buffer, " ");
		strcat(buffer, gBoolNames[xsector[nXSector].state]);
	}
	else
	{
		if ( sector[nSector].type != 0 || sector[nSector].hitag != 0 )
			sprintf(buffer,"{%i:%i}", sector[nSector].hitag, sector[nSector].type);
	}

	return buffer;
}


/***********************************************************************
 * ExtGetWallCaption()
 *
 * Build calls this to generate the wall caption displayed on the
 * 2D map.
 **********************************************************************/
const char *ExtGetWallCaption(short nWall)
{
	dassert(nWall >= 0 && nWall < kMaxWalls);
	int nXWall = wall[nWall].extra;

	buffer[0] = 0;
	if (nXWall > 0)
	{
		char temp[256];
		if ( xwall[nXWall].rxID > 0 )
		{
			sprintf(temp, "%i:", xwall[nXWall].rxID);
			strcat(buffer, temp);
		}

		strcat(buffer, gWallNames[wall[nWall].type]);

		if ( xwall[nXWall].txID > 0 )
		{
			sprintf(temp, ":%i", xwall[nXWall].txID);
			strcat(buffer, temp);
		}

		if ( xwall[nXWall].panXVel != 0 || xwall[nXWall].panYVel != 0 )
		{
			sprintf(temp, " PAN(%i,%i)", xwall[nXWall].panXVel, xwall[nXWall].panYVel);
			strcat(buffer, temp);
		}

		strcat(buffer, " ");
		strcat(buffer, gBoolNames[xwall[nXWall].state]);
	}
	else
	{
		if ( wall[nWall].type != 0 || wall[nWall].hitag != 0 )
			sprintf(buffer,"{%i:%i}", wall[nWall].hitag, wall[nWall].type);
	}

	return buffer;
}


/***********************************************************************
 * ExtGetSpriteCaption()
 *
 * Build calls this to generate the sprite caption displayed on the
 * 2D map.
 **********************************************************************/
const char *ExtGetSpriteCaption( short nSprite )
{
	dassert(nSprite >= 0 && nSprite < kMaxSprites);
	SPRITE *pSprite = &sprite[nSprite];

	if ( pSprite->type == 0 )
		return "";

	if ( pSprite->statnum == kStatMarker )
		return "";

	char *typeName = gSpriteNames[pSprite->type];
	if ( typeName == NULL )
		return "";

	int nXSprite = pSprite->extra;
	if ( nXSprite > 0 )
	{
		XSPRITE *pXSprite = &xsprite[nXSprite];

		if ( pSprite->type == kMarkerUpperLink || pSprite->type == kMarkerLowerLink )
		{
			sprintf(buffer, "%s [%d]", typeName, pXSprite->data1);
			return buffer;
		}

		buffer[0] = 0;
		char temp[256];
		if ( pXSprite->rxID > 0 )
		{
			sprintf(temp, "%i:", pXSprite->rxID);
			strcat(buffer, temp);
		}

		strcat(buffer, typeName);

		if ( pXSprite->txID > 0 )
		{
			sprintf(temp, ":%i", pXSprite->txID);
			strcat(buffer, temp);
		}

		if ( pSprite->type >= kSwitchBase && pSprite->type < kMiscMax )
		{
		 	strcat(buffer, " ");
		 	strcat(buffer, gBoolNames[pXSprite->state]);
		}

		return buffer;
	}
	else
		return typeName;
}


/***********************************************************************
 * ExtShowSectorData()
 *
 * Build calls this when F5 is pressed.
 **********************************************************************/
void ExtShowSectorData( short nSector )
{
	if ( keystatus[KEY_LALT] | keystatus[KEY_RALT] )
		EditSectorData(nSector);
	else
		ShowSectorData(nSector);
}


/***********************************************************************
 * ExtShowWallData()
 *
 * Build calls this when F6 is pressed.
 **********************************************************************/
void ExtShowWallData( short nWall )
{
	if ( keystatus[KEY_LALT] | keystatus[KEY_RALT] )
		EditWallData(nWall);
	else
		ShowWallData(nWall);
}


/***********************************************************************
 * ExtShowSpriteData()
 *
 * Build calls this when F6 is pressed.
 **********************************************************************/
void ExtShowSpriteData( short nSprite )
{
	if ( keystatus[KEY_LCTRL] | keystatus[KEY_RCTRL] )
		ShowSpriteStatistics(nSprite);
	else if ( keystatus[KEY_LALT] | keystatus[KEY_RALT] )
		EditSpriteData(nSprite);
	else
		ShowSpriteData(nSprite);
}


/***********************************************************************
 * ExtEditSectorData()
 *
 * Build calls this when F7 is pressed.
 **********************************************************************/
void ExtEditSectorData( short /* nSector */ )
{
}


/***********************************************************************
 * ExtEditWallData()
 *
 * Build calls this when F8 is pressed.
 **********************************************************************/
void ExtEditWallData(short /* nWall */ )
{
}


/***********************************************************************
 * ExtEditSpriteData()
 *
 * This function allows editing of the game specific sprite data.
 * Build calls this when F8 is pressed.
 **********************************************************************/
void ExtEditSpriteData( short /* nSprite */ )
{
}


/***********************************************************************
 * ExtLoadMap()
 *
 * Build calls this right after the loadmap() call.  Use this opportunity
 * to preload all the necessary tiles.
 **********************************************************************/
void ExtLoadMap( char * )
{
	long i;

	for (i = 0; i < numsectors; i++)
	{
		tilePreloadTile(sector[i].ceilingpicnum);
		tilePreloadTile(sector[i].floorpicnum);
	}

	for (i = 0; i < numwalls; i++)
	{
		tilePreloadTile(wall[i].picnum);
		if (wall[i].overpicnum >= 0)
			tilePreloadTile(wall[i].overpicnum);
	}

	for (i = 0; i < kMaxSprites; i++)
	{
		if (sprite[i].statnum < kMaxStatus)
			tilePreloadTile(sprite[i].picnum);
	}

	dprintf("ExtLoadMap: highlightsectorcnt = %d\n", highlightsectorcnt);
}


/***********************************************************************
 * Overridden functions
 **********************************************************************/

#if 0
short sectorofwall(short nWall)
{
	int l = 0, r = numsectors - 1, mid = 0;

	if ( nWall < 0 )
		return -1;

	if ( wall[nWall].nextwall >= 0 )
		return wall[wall[nWall].nextwall].nextsector;

	while ( l <= r )
	{
		mid = (l + r) >> 1;
		int nStart = sector[mid].wallptr;
		int nEnd = nStart + sector[mid].wallnum - 1;
		if ( nWall < nStart )
			r = mid - 1;
		else if (nWall > nEnd )
			l = mid + 1;
		else
			break;
	}

	dassert(nWall >= sector[mid].wallptr && nWall < sector[mid].wallptr + sector[mid].wallnum);
	return (short)mid;
}

void checksectorpointer(short nWall, short nSector)
{
	long x1, y1, x2, y2;

	if ( nWall < 0 )
		return;

	if ( nSector < 0 )
		return;

	x1 = wall[nWall].x;
	y1 = wall[nWall].y;
	x2 = wall[wall[nWall].point2].x;
	y2 = wall[wall[nWall].point2].y;

	if (wall[nWall].nextwall >= 0)          //Check for early exit
	{
		short k = wall[nWall].nextwall;
		if ((wall[k].x == x2) && (wall[k].y == y2))
			if ((wall[wall[k].point2].x == x1) && (wall[wall[k].point2].y == y1))
				return;
	}

	short startwall, endwall;
	wall[nWall].nextsector = -1;
	wall[nWall].nextwall = -1;
	for (short j = 0; j < numsectors; j++)
	{
		if (j == nSector)
			continue;

		startwall = sector[j].wallptr;
		endwall = (short)(startwall + sector[j].wallnum - 1);
		for (short k = startwall; k <= endwall; k++)
		{
			if ((wall[k].x == x2) && (wall[k].y == y2))
				if ((wall[wall[k].point2].x == x1) && (wall[wall[k].point2].y == y1))
				{
					wall[nWall].nextsector = j;
					wall[nWall].nextwall = k;
					wall[k].nextsector = nSector;
					wall[k].nextwall = nWall;
				}
		}
	}
}
#endif


void setbrightness( char /* brightness */, char * /* pal */ )
{
	scrSetGamma(gGamma);
	scrSetDac(0);
}


int loadboard( char *filename, long *x, long *y, long *z, short *ang, short *sectnum )
{
	// don't try to load it if it doesn't exist
	if (access(filename, F_OK) == -1)
		return -1;

	dbLoadMap(filename, x, y, z, ang, sectnum);

	CleanUp();

	// fix sprite type attributes
	AutoAdjustSprites();

	if (qsetmode != 200)	// in 2D mode
	{
		sprintf( buffer, "Map Revisions: %i", gMapRev );
		printext16(0 * 8 + 4, 0 * 8 + 28, 11, 8, buffer, 0);
	}

	dprintf("loadboard: highlightsectorcnt = %d\n", highlightsectorcnt);

	return 0;
}

// this replaces ken's timer handler
void bstubClockStrobe( void )
{
	totalclock = ++gGameClock;
	keytimerstuff();
}


void inittimer( void )
{
	timerRegisterClient(bstubClockStrobe, kTimerRate);
	timerInstall();
}

void uninittimer( void )
{
	timerRemove();
}

void initkeys( void )
{
	keyInstall();
}

void uninitkeys( void )
{
	keyRemove();
}


void faketimerhandler( void )
{ }


/***********************************************************************
 * saveboard()
 *
 * Saves the given board from memory into the specified filename.  Returns -1
 * if unable to save.  If no extension is given, .MAP will be appended to the
 * filename.
 **********************************************************************/
int saveboard(char *filename, long *x, long *y, long *z, short *ang,
	short *sectnum)
{
	// fix up floor and ceiling shade values for sectors that have dynamic lighting
	UndoSectorLighting();

	CleanUp();

	// fix sprite type attributes
	AutoAdjustSprites();

	dbSaveMap(filename, *x, *y, *z, *ang, *sectnum);

	asksave = 0;
	return 0;
}


/***********************************************************************
 * ExtSaveMap()
 *
 * Build calls this right after the savemap() call.  This function saves
 * the game and object specific tag information.
 **********************************************************************/
void ExtSaveMap( char * )
{
}


/***********************************************************************
 * ExtPreCheckKeys()
 *
 * Called before drawrooms, drawmasks and nextpage in 3D mode.
 *
 **********************************************************************/
void ExtPreCheckKeys(void)
{
	if (qsetmode == 200)	// in 3D mode
	{
		// animate sector lighting
		DoSectorLighting();

		// we need this because Ken called nextpage() in build.c
		switch ( vidoption )
		{
			case 0:	//	non-chained
				if (chainnumpages > 1)
					gPageTable[0].begin = (unsigned)chainplace;
				break;

			case 1:	// VESA 2.0 linear buffer
				gPageTable[0].begin = (unsigned)frameplace;

			case 2:	// memory buffered VESA/13h
				break;

			case 3:	// Tseng ET4000
				break;

			case 4:	// Paradise
				break;

			case 5:	// S3
				break;

			case 6:	// Crystal Eyes
				break;

			case 7:	// Red/Blue Stereogram
				break;
		}
	}
}


/***********************************************************************
 * ExtAnalyzeSprites()
 *
 * Called between drawrooms and drawmasks in 3D mode.
 *
 **********************************************************************/
void ExtAnalyzeSprites(void)
{
	int i, nOctant;
	short nSprite, nXSprite;
	long dx, dy;

	for (i = 0; i < spritesortcnt; i++)
	{
		int nTile = tsprite[i].picnum;
		dassert(nTile >= 0 && nTile < kMaxTiles);
		int nFrames = picanm[nTile].frames + 1;

		nSprite = tsprite[i].owner;
		dassert(nSprite >= 0 && nSprite < kMaxSprites);
		int j = nSprite;

		// KEN'S CODE TO SHADE SPRITES
		// Add relative shading code for textures, and possibly sector cstat bit to
		// indicate where shading comes from (i.e. shadows==floorshade, sky==ceilingshade)
		tsprite[i].shade = (schar)ClipRange( tsprite[i].shade + 6, -128, 127 );
		if ( (sector[tsprite[i].sectnum].ceilingstat & kSectorParallax)
		&& !(sector[tsprite[i].sectnum].floorstat & kSectorFloorShade) )
			tsprite[i].shade = (schar)ClipRange( tsprite[i].shade + sector[tsprite[i].sectnum].ceilingshade, -128, 127 );
		else
			tsprite[i].shade = (schar)ClipRange( tsprite[i].shade + sector[tsprite[i].sectnum].floorshade, -128, 127 );

		nXSprite = tsprite[i].extra;

		switch ( picanm[nTile].view )
		{
			case kSpriteViewSingle:

				if (nXSprite > 0)
				{
					dassert(nXSprite < kMaxXSprites);
					switch ( sprite[nSprite].type )
					{
						case kSwitchToggle:
						case kSwitchMomentary:
							if ( xsprite[nXSprite].state )
								tsprite[i].picnum = (short)(nTile + nFrames);
								break;

						case kSwitchCombination:
							tsprite[i].picnum = (short)(nTile + xsprite[nXSprite].data1 * nFrames);
							break;

					}
				}
				break;

			case kSpriteView5Full:

				// Calculate which of the 8 angles of the sprite to draw (0-7)
				dx = posx - tsprite[i].x;
				dy = posy - tsprite[i].y;
				RotateVector(&dx, &dy, -tsprite[i].ang + kAngle45 / 2);
				nOctant = GetOctant(dx, dy);

				if (nOctant <= 4)
				{
					tsprite[i].picnum = (short)(nTile + nOctant);
					tsprite[i].cstat &= ~4;   // clear x-flipping bit
				}
				else
				{
					tsprite[i].picnum = (short)(nTile + 8 - nOctant);
					tsprite[i].cstat |= 4;   // set x-flipping bit
				}
				break;

			case kSpriteView8Full:
				break;

			case kSpriteView5Half:
				break;
		}
	}
}


/***********************************************************************
 * ExtCheckKeys()
 *
 * Called just before nextpage in both 2D and 3D modes.
 *
 **********************************************************************/
void ExtCheckKeys( void )
{
	gFrameTicks = gGameClock - gFrameClock;
	gFrameClock += gFrameTicks;

	// autosave code
	if ( gFrameClock > gSaveTime + gSaveInterval )
	{
		gSaveTime = gFrameClock;

		if ( asksave )
		{
			// fix up floor and ceiling shade values for sectors that have dynamic lighting
			UndoSectorLighting();

			CleanUp();

			// fix sprite type attributes
			AutoAdjustSprites();

			dbSaveMap("AUTOSAVE.MAP", posx, posy, posz, ang, cursectnum);
			if (qsetmode == 200)	// in 3D mode
				scrSetMessage("Map autosaved to AUTOSAVE.MAP");
			else
				printmessage16("Map autosaved to AUTOSAVE.MAP");
		}
	}

	CalcFrameRate();

	if (qsetmode == 200)	// in 3D mode
	{
		// animate sector lighting
		UndoSectorLighting();

		ProcessKeys3D();

		sprintf(buffer, "%3i", gFrameRate);
		printext256(xdim - 12, 0, gStdColor[kColorWhite], -1, buffer, 1);
		scrDisplayMessage(gStdColor[kColorWhite]);

		// animate panning sectors
		if ( gPanning )
			DoSectorPanning();
	}
	else	// 2D mode
	{
		ProcessKeys2D();
		sprintf(buffer, "%3i", gFrameRate);
		printext16(640 - 24, pageoffset / 640, 15, -1, buffer, 0);
	}
}


/***********************************************************************
 * EditorErrorHandler()
 *
 * Terminate from error condition, displaying a message in text mode.
 *
 **********************************************************************/
ErrorResult EditorErrorHandler( const Error& error )
{
	timerRemove();
	uninitengine();
	keyRemove();
	setvmode(gOldDisplayMode);

	// chain to the default error handler
	return prevErrorHandler(error);
};


#define kAttrTitle      (kColorGreen * 16 + kColorWhite)

/***********************************************************************
 * ExtInit()
 *
 * Called once before BUILD.EXE makes calls to loadpics()/loadboard()
 **********************************************************************/
void ExtInit(void)
{
	char title[256];
	int i;

	void *p = NULL;

	atexit(CloseBanner);

	gOldDisplayMode = getvmode();

	sprintf(title, "MapEdit Alpha Build [%s] -- DO NOT DISTRIBUTE", gBuildDate);
	tioInit();
	tioCenterString(0, 0, tioScreenCols - 1, title, kAttrTitle);
	tioCenterString(tioScreenRows - 1, 0, tioScreenCols - 1,
		"Copyright (c) 1994, 1995 Q Studios Corporation", kAttrTitle);

	tioWindow(1, 0, tioScreenRows - 2, tioScreenCols);

	if ( _grow_handles(kRequiredFiles) < kRequiredFiles )
		ThrowError("Not enough file handles available", ES_ERROR);

	InitializeNames();

	tioPrint("Loading preferences");
	gGamma = BloodINI.GetKeyInt("View", "Gamma", 0);
	gBeep = MapEditINI.GetKeyBool("Options", "Beep", TRUE);
	gHighlightThreshold = MapEditINI.GetKeyInt("Options", "HighlightThreshold", 40);
	gStairHeight = MapEditINI.GetKeyInt("Options", "StairHeight", 8);
	gOldKeyMapping = MapEditINI.GetKeyBool("Options", "OldKeyMapping", FALSE);
	gSaveInterval = kTimerRate * MapEditINI.GetKeyBool("Options", "AutoSaveInterval", 5 * 60);

	gLBIntensity = MapEditINI.GetKeyInt("LightBomb", "Intensity", 16);
	gLBAttenuation = MapEditINI.GetKeyInt("LightBomb", "Attenuation", 0x1000);
	gLBReflections = MapEditINI.GetKeyInt("LightBomb", "Reflections", 2);
	gLBMaxBright = MapEditINI.GetKeyInt("LightBomb", "MaxBright", -4);
	gLBRampDist = MapEditINI.GetKeyInt("LightBomb", "RampDist", 0x10000);

	tioPrint("Initializing heap and resource system");
	Resource::heap = new QHeap(dpmiDetermineMaxRealAlloc());

	tioPrint("Initializing resource archive");
	gSysRes.Init("BLOOD.RFF", "*.*");
	gGuiRes.Init("GUI.RFF", NULL);

	tioPrint("Initializing mouse");
	if ( !initmouse() )
		tioPrint("Mouse not detected");

	CheckDemoDate(gSysRes);

	// install our error handler
	prevErrorHandler = errSetHandler(EditorErrorHandler);

	InitEngine();

	Mouse::SetRange(xdim, ydim);

	tioPrint("Loading tiles");
	if (tileInit() == 0)
		ThrowError("ART files not found", ES_ERROR);

	tioPrint("Loading cosine table");
	trigInit(gSysRes);

	scrInit();

	tioPrint("Creating standard color lookups");
	scrCreateStdColors();

	dbInit();
	visibility = 800;

	kensplayerheight = 56 << 8;   // eyeHeight

	zmode = 0;
	defaultspritecstat = kSpriteOriginAlign;

	for (i = 0; i < kMaxSectors; i++)
		sector[i].extra = -1;

	for (i = 0; i < kMaxWalls; i++)
		wall[i].extra = -1;

	for (i = 0; i < kMaxSprites; i++)
		sprite[i].extra = -1;

//	qsetmode = 200;		// inform the rest of the code that we're in 3D mode
	scrSetGameMode();
}


/***********************************************************************
 * ExtUnInit()
 *
 * Called once right before BUILD.EXE uninitengine()
 **********************************************************************/
void ExtUnInit(void)
{
	unlink("AUTOSAVE.MAP");
}
