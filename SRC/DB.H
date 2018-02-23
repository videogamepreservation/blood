#ifndef __DB_H
#define __DB_H

#include "typedefs.h"
#include "engine.h" // kMaxStatus

/***********************************************************************
 * Constants
 **********************************************************************/

/*******************************************************************************
The following max constants should be eventually removed, and storage
dynamically allocated.
*******************************************************************************/
#define kMaxXSprites		2048
#define kMaxXWalls			512
#define kMaxXSectors		512


#define kMaxBusyValue   0x10000
#define kFluxMask		0x0FFFF


// status lists
enum {
	kStatDefault	= 0,	// inactive items, like torches and stuff, but NOT dudes
	kStatEffect     = 1,	// non-damaging ricochets, splashes, etc.
	kStatExplosion	= 2,
	kStatItem		= 3,	// items that can be picked up
	kStatThing		= 4,	// things that can be destroyed/moved
	kStatMissile	= 5,	// player/enemy missiles that do damage
	kStatDude		= 6,	// an active dude
	kStatInactive	= 7,	// an inactive dude
	kStatRespawn	= 8,
	kStatPurge		= 9,	// use these for purgeable sprites
	kStatMarker		= 10,
	kStatTraps		= 11,
	kStatProximity	= 12,	// sprites triggered by proximity
	kStatSpares     = 13,	// allocated invisible sprites
	kStatFlare		= 14,	// flares that are stuck in dudes
	kStatDebris		= 15,	// moving non-Thing debris
	kStatFree		= kMaxStatus,
};

enum {
	kRespawnNever		= 0,	// sprite cannot respawn
	kRespawnOptional	= 1,	// sprite can optionally respawn in respawnTime seconds
	kRespawnAlways		= 2,	// sprite always respawns in respawnTime seconds
	kRespawnPermanent	= 3,	// sprite is permanent (respawnTime ignored)
};

enum {
	kNothing			= 0,
	kMarkerPlayerStart	= 1,
	kMarkerDeathStart,
	kMarkerOff,
	kMarkerOn,
	kMarkerAxis,
	kMarkerUpperLink,
	kMarkerLowerLink,
	kMarkerWarpDest,
	kMarkerRaingen,

	kSwitchBase			= 20,
	kSwitchToggle		= kSwitchBase,
	kSwitchMomentary,
	kSwitchCombination,
	kSwitchPadlock,
	kSwitchMax,

	kMiscBase			= 30,
	kMiscTorch			= kMiscBase,
	kMiscHangingTorch,
	kMiscMax,

	kWeaponItemBase			= 40,
	kWeaponItemRandom		= kWeaponItemBase,
	kWeaponItemShotgun,
	kWeaponItemTommyGun,
	kWeaponItemFlareGun,
	kWeaponItemVoodooDoll,
	kWeaponItemSpearGun,
	kWeaponItemShadowGun,
	kWeaponItemPitchfork,	// default weapon, infinite ammo
	kWeaponItemSprayCan,	// default weapon, just need ammo
	kWeaponItemTNT,			// default weapon, just need ammo
	kWeaponItemMax,

	kAmmoItemBase			= 60,
	kAmmoItemSprayCan		= kAmmoItemBase,
	kAmmoItemTNTStick,
	kAmmoItemTNTBundle,
	kAmmoItemTNTCase,
	kAmmoItemTNTProximity,
	kAmmoItemTNTRemote,
	kAmmoItemTNTTimer,
	kAmmoItemShells,
	kAmmoItemShellBox,
	kAmmoItemBullets,
	kAmmoItemBulletBox,
	kAmmoItemAPBullets,
	kAmmoItemTommyDrum,
	kAmmoItemSpear,
	kAmmoItemSpearPack,
	kAmmoItemHESpears,
	kAmmoItemFlares,
	kAmmoItemHEFlares,
	kAmmoItemStarFlares,
	kAmmoItemRandom,
	kAmmoItemMax,

	kItemBase			= 100,
	kItemKey1			= kItemBase,
	kItemKey2,
	kItemKey3,
	kItemKey4,
	kItemKey5,
	kItemKey6,
	kItemKey7,
	kItemDoctorBag,
	kItemMedPouch,
	kItemLifeEssence,
	kItemLifeSeed,
	kItemPotion1,
	kItemFeatherFall,
	kItemLtdInvisibility,
	kItemInvulnerability,
	kItemJumpBoots,
	kItemRavenFlight,
	kItemGunsAkimbo,
	kItemDivingSuit,
	kItemGasMask,
	kItemClone,
	kItemCrystalBall,
	kItemDecoy,
	kItemDoppleganger,
	kItemReflectiveShots,
	kItemRoseGlasses,
	kItemShadowCloak,
	kItemShroomRage,
	kItemShroomDelirium,
	kItemShroomGrow,
	kItemShroomShrink,
	kItemDeathMask,
	kItemWineGoblet,
	kItemWineBottle,
	kItemSkullGrail,
	kItemSilverGrail,
	kItemTome,
	kItemBlackChest,
	kItemWoodenChest,
	kItemAsbestosArmor,
	kItemRandom,
	kItemMax,

	kDudeBase			= 200,
	kDudeRandom			= kDudeBase,
	kDudeTommyCultist,
	kDudeShotgunCultist,
	kDudeAxeZombie,
	kDudeFatZombie,
	kDudeEarthZombie,
	kDudeFleshGargoyle,
	kDudeStoneGargoyle,
	kDudeFleshStatue,
	kDudeStoneStatue,
	kDudePhantasm,
	kDudeHound,
	kDudeHand,
	kDudeBrownSpider,
	kDudeRedSpider,
	kDudeBlackSpider,
	kDudeMotherSpider,
	kDudeGillBeast,
	kDudeEel,
	kDudeBat,
	kDudeRat,
	kDudeGreenPod,
	kDudeGreenTentacle,
	kDudeFirePod,
	kDudeFireTentacle,
	kDudeMotherPod,
	kDudeMotherTentacle,
	kDudeCerberus,
	kDudeCerberus2,
	kDudeTchernobog,
	kDudeRachel,
	kDudePlayer1,
	kDudePlayer2,
	kDudePlayer3,
	kDudePlayer4,
	kDudePlayer5,
	kDudePlayer6,
	kDudePlayer7,
	kDudePlayer8,
	kDudePlayerBurning,		// this one can't explode while burning
	kDudeCultistBurning,	// this one may explode while burning
	kDudeAxeZombieBurning,	// this one is taller
	kDudeFatZombieBurning,	// this one is fatter
	kDudePlayer_Owned,
	kDudeHound_Owned,
	kDudeEel_Owned,
	kDudeSpider_Owned,
	kDudeMax,

// missile things
	kMissileBase         = 300,
	kMissileButcherKnife = kMissileBase,
	kMissileFlare,
	kMissileExplodingFlare,
	kMissileStarburstFlare,
	kMissileSprayFlame,
	kMissileFireball,
	kMissileSpear,
	kMissileEctoSkull,
	kMissileHoundFire,
	kMissileGreenPuke,
	kMissileRedPuke,
	kMissileMax,

	kThingBase			= 400,
	kThingTNTBarrel		= kThingBase,
	kThingTNTProxArmed,
	kThingTNTRemArmed,
	kThingBlueVase,
	kThingBrownVase,
	kThingCrateFace,
	kThingClearGlass,
	kThingFluorescent,
	kThingWallCrack,
	kThingWoodBeam,
	kThingWeb,
	kThingMetalGrate1,
	kThingFlammableTree,
	kThingMachineGun,

	// Maintain static Thing numbers above
	kThingTNTStick,
	kThingTNTBundle,
	kThingBoneClub,
	kThingZombieBones,
	kThingWaterDrip,
	kThingBloodDrip,
	kThingBubble,		// one floating fart bubble
	kThingBubbles,		// group of floating fart bubbles
	kThingGibSmall,
	kThingGibCultist,
	kThingGibAxeZombie,
	kThingGibFatZombie,
	kThingGibFleshGargoyle,
	kThingGibHound,
	kThingGibGillBeast,
	kThingGibGreenPod,
	kThingGibFirePod,
	kThingMax,

// special trap types : traps are damaging to touch
	kTrapBase			= 450,
	kTrapSpiketrap		= kTrapBase,
	kTrapRocktrap,
	kTrapUNUSED1,
	kTrapSatellite,
	kTrapSawBlade,
	kTrapUnpoweredZap,
	kTrapPoweredZap,
	kTrapPendulum,
	kTrapGuillotine,
	kTrapMax,

// special wall types
	kWallXpanning		= 500,
	kWallYpanning,
	kWallCWpivot,
	kWallCCWpivot,
	kWallForcefield,
	kWallCrushing,
	kWallPendulum,
	kWallSpeartrap,
	kWallFlametrap,
	kWallRazordoor,
	kWallGuillotine,
	kWallClearGlass,
	kWallStainedGlass,
	kWallWoodBeams,
	kWallWeb,
	kWallMetalGrate1,

// special sector types
	kSectorBase       = 600,
	kSectorZMotion    = kSectorBase,
	kSectorZCrusher,		// OBSOLETE
	kSectorZSprite,
	kSectorWarp,			// warp when triggered
	kSectorTeleport,
	kSectorUpperlink,		// OBSOLETE
	kSectorLowerlink,		// OBSOLETE
	kSectorUpperwater,
	kSectorLowerwater,
	kSectorSwingDoor,		// OBSOLETE
	kSectorTopDoor,			// OBSOLETE
	kSectorBottomDoor,		// OBSOLETE
	kSectorPlatformUp,		// OBSOLETE
	kSectorPlatformDown,	// OBSOLETE
	kSectorSlideMarked,		// slide marked walls and sprites within sector
	kSectorRotateMarked,	// rotate marked walls and sprites within sector
	kSectorSlide,			// slide all walls and sprites within sector
	kSectorRotate,			// rotate all walls and sprites within sector
	kSectorSlideCrush,		// slide crushing walls within sector
	kSectorRotateCrush,		// rotating sector with crushing walls
	kSectorMax,

	kGeneratorBase		= 700,
	kGenTrigger		= kGeneratorBase,
	kGenWaterDrip,
	kGenBloodDrip,
	kGenFireball,
	kGenEctoSkull,
	kGenDart,
	kGenBubble,			// a floating bubble
	kGenBubbles,        // group of floating bubbles
	kGenSound,
	kGeneratorMax,
};

enum {
	kSurfNone = 0,
	kSurfStone,
	kSurfMetal,
	kSurfWood,
	kSurfFlesh,
	kSurfWater,
	kSurfDirt,
	kSurfClay,
	kSurfSnow,
	kSurfIce,
	kSurfLeaves,
	kSurfCloth,
	kSurfPlant,
	kSurfGoo,
	kSurfLava,
	kSurfMax,
};

enum {
	kDepthWalk  = 0,
	kDepthTread = 1,
	kDepthWade  = 2,
	kDepthSwim  = 3
};

struct AISTATE;	// forward declaration for aiState

/***********************************************************************
 * Structures and typedefs
 **********************************************************************/
struct XSPRITE
{
	signed reference :			14;
	unsigned state :			1;

	// trigger data
	unsigned busy :				17;
	// 0 bits unused

	unsigned txID :				10;
	unsigned rxID :				10;
	unsigned command : 			8;
	unsigned triggerOn : 		1;
	unsigned triggerOff : 		1;
	// 2 bits unused

	unsigned busyTime :			12;	// time to reach next state
	unsigned waitTime :			12;	// delay before callback
	unsigned restState :		1;	// state to return to on callback
	unsigned interruptable :	1;

	unsigned difficulty :		2;
	// 4 bits unused

	unsigned soundId :			8;

	// physical triggers
	unsigned decoupled :		1;
	unsigned triggerOnce :		1;
	unsigned isTriggered :      1;	// used for triggerOnce objects
	unsigned key :				3;
	unsigned triggerPush :      1;	// action key
	unsigned triggerImpact :    1;	// vector hits
	unsigned triggerReserved0 :	1;
	unsigned triggerPickup :    1;	// secrets
	unsigned triggerTouch :     1;	// sawblades, spikes, zaps?
	unsigned triggerSight :     1;	// dunno, yet.
	unsigned triggerProximity : 1;	// proximity bombs
	unsigned triggerReserved1 :	1;
	unsigned triggerReserved2 :	1;
	// 9 bits unused

	signed data1 :				16;	// combo value?
	signed data2 :				16;	// combo key?
	// 0 bits unused

	signed data3 :				16;	// combo max?

	unsigned goalAng :			11;	// dudes
	signed dodgeDir :			2;	// dudes
	unsigned locked:			1;
	unsigned unused0 :			2;	// USE ME!
	// 0 bits unused

	unsigned respawn :			2;	// 0=optional never, 1=optional always, 2=always, 3=never
	unsigned respawnTime :		12; // 0=permanent, >0=time in tenths of a second,
	unsigned launchMode :		2;	// 0=all, 1=bloodbath, 2=ally, 3=ally&bloodbath,

	// this stuff needed for dudes
	unsigned moveState :		8;	// same as player move states
	unsigned unused1 :			8;	// USE ME!
	// 0 bits unused

	unsigned health :			12;
	unsigned dudeDeaf :			1;	// can't hear players
	unsigned dudeAmbush :		1;	// must be triggered manually
	unsigned dudeGuard :		1;	// won't leave sector
	unsigned dudeFlag4 :		1;

	signed target :				16;
	// 0 bits unused

	signed targetX :			32;
	signed targetY :			32;
	signed targetZ :			32;

	unsigned burnTime :			16;
	signed burnSource :			16;

	unsigned unused2 :			16;
	unsigned stateTimer :		16;

	AISTATE *aiState;
};

struct XWALL
{
	signed reference :			14;
	unsigned state : 			1;

	// trigger data
	unsigned busy : 			17;
	signed data :				16;
	unsigned txID :				10;
	unsigned rxID :				10;
	unsigned command : 			8;
	unsigned triggerOn : 		1;
	unsigned triggerOff : 		1;
	unsigned busyTime :			12;	// time to reach next state
	unsigned waitTime :			12;	// delay before callback
	unsigned restState :		1;	// state to return to on callback
	unsigned interruptable :	1;

	// panning data
	unsigned panAlways :		1;
	signed panXVel :			8;
	signed panYVel :			8;

	// physical triggers
	unsigned decoupled :		1;
	unsigned triggerOnce :		1;
	unsigned isTriggered :   	1;	// used for triggerOnce objects
	unsigned key :				3;
	unsigned triggerPush :   	1;
	unsigned triggerImpact : 	1;
	unsigned triggerReserved0 :	1;
	unsigned triggerReserved1 :	1;
	unsigned triggerReserved2 :	1;

	unsigned xpanFrac :			8;
	unsigned ypanFrac :			8;

	unsigned locked : 			1;
	unsigned pad : 				31;
};

struct XSECTOR
{
	signed reference :			14;
	unsigned state : 			1;

	// trigger data
	unsigned busy :				17;
	signed data :				16;
	unsigned txID :				10;
	unsigned rxID :				10;
	unsigned command : 			8;
	unsigned triggerOn : 		1;
	unsigned triggerOff : 		1;
	unsigned busyTime :			12;	// time to reach next state
	unsigned waitTime :			12;	// delay before callback
	unsigned restState :		1;	// state to return to on callback
	unsigned interruptable :	1;

	// lighting data
	signed amplitude :			8;
	unsigned freq :				8;
	unsigned phase :			8;
	unsigned wave :				4;
	unsigned shadeAlways : 		1;
	unsigned shadeFloor :		1;
	unsigned shadeCeiling : 	1;
	unsigned shadeWalls :		1;
	signed shade :				8;

	// panning data
	unsigned panAlways :		1;
	unsigned panFloor :			1;
	unsigned panCeiling :		1;
	unsigned drag :				1;

	// wind/water stuff
	unsigned underwater :		1;
	unsigned depth :			2;
	unsigned panVel :      		8;
	unsigned panAngle :			11;
	unsigned wind :		    	1;

	// physical triggers
	unsigned decoupled :		1;
	unsigned triggerOnce :		1;
	unsigned isTriggered :		1;
	unsigned key :				3;
	unsigned triggerPush :		1;
	unsigned triggerImpact :	1;
	unsigned triggerReserved0 :	1;
	unsigned triggerEnter :		1;
	unsigned triggerExit :		1;
	unsigned triggerWPush :		1;
	unsigned triggerReserved1 :	1;
	unsigned triggerReserved2 :	1;

	// movement data
	signed offCeilZ :			32;
	signed onCeilZ :			32;
	signed offFloorZ :			32;
	signed onFloorZ :			32;
	signed marker0 :			16;	// could be 13 bits
	signed marker1 :			16;	// could be 13 bits
	unsigned crush :			1;

	unsigned ceilxpanFrac :		8;
	unsigned ceilypanFrac :		8;
	unsigned floorxpanFrac :	8;
	unsigned floorypanFrac :	8;

	unsigned locked : 			1;
	unsigned pad : 				31;

//	unsigned upperLink :		11;
//	signed upperLinkZ :			32;
//	unsigned lowerLink :		11;
//	signed lowerLinkZ :			32;
};

#if 0

// each physical trigger: command, to
// going on: command, to
// going off: command, to


// PROPOSED XSECTOR CHANGES TO DECREASE MEMORY REQUIREMENTS
//
struct XSECTOR
{
	signed reference :    		14;
	unsigned state :       		1;

	unsigned trigger :    		16;  // these fields could be smaller
	unsigned lighting :   		16;  // unless you want to make them pointers
	unsigned surfMotion : 		16;  // and allocate them at load time.
	unsigned motion :     		16;  // this might work since sectors are not
	unsigned zMotion :    		16;  // allocated/deallocated during play
};

struct TRIGGER
{
	unsigned state :         	1;
	unsigned command : 		 	8;
	unsigned txID :			 	10;
	unsigned rxID :			 	10;
	  signed data :			 	16;
	unsigned busy : 		 	16;
	unsigned triggerOn : 	 	1; // trigger on "on" state change
	unsigned triggerOff : 	 	1; // trigger on "off" state change

	unsigned isTriggered :   	1; // trigger is "tripped" - for triggerOnce types
	unsigned triggerOnce :	 	1; // trigger only once

	unsigned key :			 	3; // key required to activate, if any
	unsigned triggerEnter :  	1; // trigger on player entrance
	unsigned triggerExit :   	1; // trigger on player exit
	unsigned triggerPush :   	1; // trigger on player action
	unsigned triggerImpact : 	1; // trigger on player weapon impact
	unsigned triggerWPush :		1; // trigger on push from outside wall (convenience bit)

	unsigned waitTime :			8;
	unsigned restState :		1; // state to return to on callback

};

struct SURFMOTION
{
	// panning/movement data
	unsigned depth :			2;
	unsigned underwater :		1;
	unsigned wind       :   	1; // wind or underwater current
	unsigned panAlways :		1;
	unsigned panFloor :			1;
	unsigned panCeiling :		1;
	unsigned drag :				1;
	unsigned panVel :      		8;
	unsigned panAngle :			11;
};


struct LIGHTING
{
	// lighting data
	signed amplitude :			8;
	unsigned freq :				8;
	unsigned wave :				4;
	unsigned phase :			8;
	unsigned shadeFloor :		1;
	unsigned shadeCeiling : 	1;
	unsigned shadeWalls :		1;
	signed shade :				8;
	unsigned shadeAlways : 		1
}

struct MOTION
{
	signed marker0 :			16;
	signed marker1 :			16;

	int		upperOffZ;
	int		upperOnZ;
	int		lowerOffZ;
 	int 	lowerOnZ;
	uchar	busyTime;

	unsigned upperModelType :	2; // 0:from trigger's, 1:lowest's adjacent, 2-3:unused
	unsigned upperOffModel :	3; // 0:same, 1:pic, 2:effects, 3-7:unused
	unsigned upperOnModel :		3; // 0:same, 1:pic, 2:effects, 3-7:unused

	unsigned lowerModelType :	2; // 0:from trigger's, 1:lowest's adjacent, 2-3:unused
	unsigned lowerOnModel :		3; // 0:same, 1:pic, 2:effects, 3-7:unused
	unsigned lowerOffModel :	3; // 0:same, 1:pic, 2:effects, 3-7:unused
};

struct DISK_XSECTOR
{
	BOOL        state;
	TRIGGER     trigger;
	LIGHTING    lighting;
	SURFMOTION  surfMotion;
	MOTION      motion;
};

// these could be allocated, see XSECTOR notes above
TRIGGER    gSectorTriggers[ kMaxSectorTriggers ];
LIGHTING   gLighting[ kMaxLighting ];
SURFMOTION gSurfMotion[ kMaxSurfMotion ];
MOTION     gMotion[ kMaxMotion ];

NOTES: A SECTOR structure would be saved out prior to the DISK_XSECTOR.

#endif

/***********************************************************************
 * Variables
 **********************************************************************/
extern XSPRITE xsprite[];
extern XWALL xwall[];
extern XSECTOR xsector[];

#define kMapAuthorLen 64
#define kMapNameLen   64

extern char gMapAuthor[];
extern char gMapName[];
extern int gMapRev;
extern int gSongId;
extern ulong gMapCRC;
extern int gSkyCount;


/***********************************************************************
 * Global Variables
 **********************************************************************/
extern char *gItemText[ kItemMax - kItemBase ];
extern char *gAmmoText[ kAmmoItemMax - kAmmoItemBase ];
extern char *gWeaponText[ kWeaponItemMax - kWeaponItemBase ];


/***********************************************************************
 * Functions
 **********************************************************************/

ushort dbInsertXSprite( int nSprite );
void dbDeleteXSprite( int nXSprite );
void dbXSpriteClean( void );

ushort dbInsertXWall( int nWall );
void dbDeleteXWall( int nXWall );
void dbXWallClean( void );

ushort dbInsertXSector( int nSector );
void dbDeleteXSector( int nXSector );
void dbXSectorClean( void );

void dbInit( void );
void dbLoadMap(const char *mapname, long *x, long *y, long *z, short *angle, short *nSector);
void dbSaveMap(const char *mapname, long x, long y, long z, short angle, short nSector);

#endif
