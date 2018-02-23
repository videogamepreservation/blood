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


// 3d view types
enum {
	kSpriteViewSingle = 0,
	kSpriteView5Full,
	kSpriteView8Full,
	kSpriteView5Half,
	kSpriteView3Flat,
	kSpriteView4Flat
};

// status lists
enum {
	kStatDefault	= 0,	// inactive items, like torches and stuff, but NOT dudes
	kStatTouch		= 1,	// items that can be picked up
	kStatProximity	= 2,	// sprites triggered by proximity
	kStatMissile	= 3,	// player/enemy missiles that do damage
	kStatExplosion	= 4,	// an explosion that gives damage and inertia
	kStatDebris		= 5,	// debris from explosions
	kStatDude		= 6,	// an active dude
	kStatInactive	= 7,	// an inactive dude
	kStatExplodable	= 8,	// non-dude objects affected by explosions
	kStatPurge		= 9,	// use these for purgeable sprites
	kStatMarker		= 10,
	kStatEffect     = 12,	// non-damaging ricochets, splashes, etc.
	kStatSpares     = kMaxStatus - 1, // allocated invisible sprites
	kStatInvalid	= kMaxStatus,	  // invalid sprite status list
};

enum {
	kStatDefault=0,	// completely inactive sprites
	kStatTouch,		// sprite items that can be picked up by players
	kStatProxBomb,	// bombs triggers by proximity or impact damage
	kStatDudes1,	// dudes processed in phase 1
	kStatDudes2,	// dudes processed in phase 2
	kStatDudes3,	// dudes processed in phase 3
	kStatDudes4,	// dudes processed in phase 4
};


Player
	Water Splash
		One-shot animation - stays on floor
	Sludge Splash
		One-shot animation - stays on floor
	Blood Squib
		One-shot animation - falls to floor
Shotgun/Tommy Gun
	Spark Ricochet
		One-shot animation - stays on wall
	Wood Ricochet
		One-shot animation - stays on wall
	Bullet Holes
		Semi-permanent flat sprite - stays on wall
Pitchfork
	Spark Ricochet (?)
		4 one-shot animations - stays on wall
	Blood Squib
		One-shot animation - falls to floor
	Guts (FPP)
		One-shot animation - falls off screen
Speargun
	Spear
		Missile
			Flat or multi-view sprite - arcs slowly to floor
		Blood Squib
			One-shot animation - falls to floor
	HE Spear
		Missile
			Flat or multi-view sprite - arcs slowly to floor
		Explosion
			One-shot animation - stays on floor
	Broken Spear
		Face sprite - falls to floor
Flare Gun
	Flare
		Fire
	HE Flare
		Explosion
	Starburt Flare
		Multiple Explosions


enum {
	kMarkerOff,
	kMarkerOn,
	kMarkerAxis,
	kMarkerUpperLink,
	kMarkerLowerLink,
};

enum {
	kNothing			= 0,
	kMarkerPlayerstart	= 1,
	kMarkerBathstart,
	kMarkerTeleportdest,
	kMarkerAsyncsound,
	kMarkerSyncsound,
	kMarkerRaingen,

	kSwitchToggle		= 20,
	kSwitchMomentary,
	kSwitchCombination,
	kSwitchMax,

	kWeaponItemBase			= 40,
	kWeaponItem1_Pitchfork	= kWeaponItemBase,
	kWeaponItemSprayCan,
	kWeaponItem3_TNT,
	kWeaponItemShotgun,
	kWeaponItemFlareGun,
	kWeaponItemSpearGun,
	kWeaponItemTommyGun,
	kWeaponItemEctoBlaster,
	kWeaponItemShadowBlaster,
	kWeaponItemLifeStealer,
	kWeaponItemRandom,
	kWeaponItemMax,

	kAmmoBase			= 60,
	kAmmoSprayCan		= kAmmoBase,
	kAmmoTNTStick,
	kAmmoTNTBundle,
	kAmmoTNTCase,
	kAmmoTNTProximity,
	kAmmoTNTRemote,
	kAmmoTNTTimer,
	kAmmoShells,
	kAmmoShellBox,
	kAmmoBullets,
	kAmmoBulletBox,
	kAmmoAPBullets,
	kAmmoHEBullets,
	kAmmoTommyDrum,
	kAmmoSpear,
	kAmmoSpearPack,
	kAmmoHESpears,
	kAmmoFlares,
	kAmmoHEFlares,
	kAmmoStarFlares,
	kAmmoRandom,
	kAmmoMax,

	kItemBase			= kAmmoMax,
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
	kItemWitchBroom,
	kItemGunsAkimbo,
	kItemDivingSuit,
	kItemGasMask,
	kItemTrickOrTreat,
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

	kItemSnorkel,
	kItemDeathmask,
	kItemGauntlets,
	kItemBoots,
	kItemTalisman,
	kItemWineGoblet,
	kItemWineBottle,
	kItemSkullGrail,
	kItemSilverGrail,
	kItemShrub,
	kItemTome,
	kItemBlackChest,
	kItemWoodenChest,
	kItemLocket,
	kItemAsbestosArmor,
	kItemRandom,
	kItemMax,

	kCreatureBase       = 200,
	kCreatureCultist	= kCreatureBase,
	kCreatureZombie1,
	kCreatureZombie2,
	kCreatureGargoyle,
	kCreaturePhantasm,
	kCreatureHellhound,
	kCreatureHand,
	kCreatureSpider,
	kCreatureChrysalid,
	kCreatureGillbeast,
	kCreaturePirahna,
	kCreatureEel,
	kCreatureBat,
	kCreatureSnake,
	kCreatureRat,
	kCreaturePod,
	kCreatureTentacle,
	kCreatureCerberus,
	kCreatureIshkabaal,
	kCreatureDarkone,
	kCreatureRachel,
	kCreatureRandom,
	kCreaturePlayer,
	kCreatureClone,
	kCreatureMax,

// missile things
	kMissileBase         = 300,
	kMissileButcherKnife = kMissileBase,
	kMissileEctoSkull,
	kMissileSpear,
	kMissileShadowBall,
	kMissileFlare,
	kMissileGreenPuke,
	kMissileRedPuke,
	kMissileSprayFlame,
	kMissileTNTStick,
	kMissileMax,

// special thing types
	kThingBase			= 400,
	kThingTNTBarrel		= kThingBase,
	kThingSpiketrap,
	kThingRocktrap,
	kThingGastrap,
	kThingSatellite,
	kThingSawBlade,
	kThingTNTProxArmed,
	kThingTNTRemArmed,
	kThingUnpoweredZap,
	kThingPoweredZap,
	kThingNormalStatue,
	kThingLiveStatue,
	kThingCrateFace,
	kThingMax,

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
	kWallBreakable,

// special sector types
	kSectorBase       = 600,
	kSectorZMotion    = kSectorBase,
	kSectorZCrusher,
	kSectorFloorPad,
	kSectorWarp,          // warp when triggered
	kSectorTeleport,
	kSectorUpperlink,
	kSectorLowerlink,
	kSectorUpperwater,
	kSectorLowerwater,
	kSectorSwingDoor,
	kSectorTopDoor,
	kSectorBottomDoor,
	kSectorPlatformUp,
	kSectorPlatformDown,
	kSectorVSplitDoor,   // door splits left/right
	kSectorHSplitDoor,   // door splits top/bottom
	kSectorSlide,        // slide marked walls within sector
	kSectorRotate,		 // rotating sector that rotates sprites within
	kSectorSlideCrush,   // slide crushing walls within sector
	kSectorRotateCrush,  // rotating sector with crushing walls
	kSectorMax,
};

enum {
	kSurfStone,
	kSurfWood,
	kSurfDirt,
	kSurfMetal,
	kSurfWater,
	kSurfLava,
};

enum {
	kDepthWalk  = 0,
	kDepthTread = 1,
	kDepthWade  = 2,
	kDepthSwim  = 3
};

/***********************************************************************
 * Structures and typedefs
 **********************************************************************/
struct XSPRITE
{
	signed reference :			16;
	unsigned state :			1;

	// trigger data
	unsigned busy :				17;
	unsigned type :				10;
	signed data :				16;
	unsigned txID :				10;
	unsigned rxID :				10;
	unsigned command : 			3;
	unsigned triggerOn : 		1;
	unsigned triggerOff : 		1;
	unsigned triggerOnce :		1;
	unsigned restState :		1; // state to return to on callback
	unsigned key :				3;

	unsigned difficulty :		2;
	unsigned detail :			2;
	unsigned map : 				2;
	unsigned view :				4;
	unsigned soundKit :			8;

	unsigned isTriggered :      1; // used for triggerOnce objects
	unsigned triggerPush :      1; // gates?
	unsigned triggerImpact :    1; // exploding things, etc.
	unsigned triggerPickup :    1; // secrets
	unsigned triggerTouch :     1; // sawblades, spikes, zaps?
	unsigned triggerSight :     1; // dunno, yet.
	unsigned triggerProximity : 1; // proximity bombs

	unsigned decoupled :		1;
	unsigned waitTime : 		8;

	unsigned permanent :		1; // 0=not permanent, 1=permanent (respawn ignored)
	unsigned respawn :			2; // 0=never, 1=always, 2=optional never, 3=optional always
	unsigned respawnTime :		8; // 0=instant, >0=time in tenths of a second,
	unsigned launchMode :		2; // 0=all, 1=bloodbath, 2=ally, 3=ally&bloodbath,

	unsigned pad1 : 32;
	unsigned pad2 : 32;
};

struct XWALL
{
	signed reference :			16;
	unsigned state : 			1;
	unsigned map :				2;

	// trigger data
	unsigned busy : 			17;
	unsigned type :				10;
	signed data :				16;
	unsigned txID :				10;
	unsigned rxID :				10;
	unsigned command : 			3;
	unsigned triggerOn : 		1;
	unsigned triggerOff : 		1;
	unsigned triggerOnce :		1;
	unsigned key :				3;

	// panning data
	signed panXVel :			8;
	signed panYVel :			8;

	unsigned isTriggered :   	1; // used for triggerOnce objects
	unsigned triggerPush :   	1;
	unsigned triggerImpact : 	1;
	unsigned triggerRsvd1 :   	1;
	unsigned triggerRsvd2 : 	1;

	unsigned decoupled :		1;
	unsigned panAlways :		1;
	unsigned restState :		1; // state to return to on callback
	unsigned waitTime : 		8; // delay before callback

	unsigned pad1 : 32;
	unsigned pad2 : 32;
};

struct XSECTOR
{
	signed reference :			16;
	unsigned state : 			1;

	// trigger data
	unsigned busy : 			17;
	unsigned type :				10;
	signed data :				16;
	unsigned txID :				10;
	unsigned rxID :				10;
	unsigned command : 			3;
	unsigned triggerOn : 		1;
	unsigned triggerOff : 		1;
	unsigned triggerOnce :		1;
	unsigned restState :		1; // state to return to on callback
	unsigned key :				3;

	// lighting data
	signed amplitude :			8;
	unsigned freq :				8;
	unsigned wave :				4;
	unsigned shadeAlways : 		1;
	unsigned phase :			8;
	unsigned shadeFloor :		1;
	unsigned shadeCeiling : 	1;
	unsigned shadeWalls :		1;
	signed shade :				8;

	// panning data
	unsigned panFloor :			1;
	unsigned panCeiling :		1;
	unsigned panDrag :			1;
	unsigned waitTime : 		8; // delay before callback
	unsigned panAlways :		1;

	unsigned busyTime :			8; // time to reach next state

	// wind/water stuff
	unsigned underwater :		1;
	unsigned depth :			2;
	unsigned panVel :      		8;
	unsigned panAngle :			11;

	unsigned wind :		    	1;

	unsigned isTriggered :		1;
	unsigned triggerPush :		1;
	unsigned triggerImpact :	1;
	unsigned triggerEnter :		1;
	unsigned triggerExit :		1;
	unsigned decoupled :		1;

	unsigned triggerWPush :		1;

	signed offCeilZ :			32;
	signed onCeilZ :			32;
	signed offFloorZ :			32;
	signed onFloorZ :			32;

	unsigned pad1 : 32;
	unsigned pad2 : 32;
};

#if 0
/***********************************************************************
 * Structures and typedefs
 **********************************************************************/
struct XSPRITE
{
	signed reference :			16;
	unsigned state :			1;

	// trigger data
	unsigned busy :				16;
	unsigned type :				10;
	signed data :				16;
	unsigned txID :				10;
	unsigned rxID :				10;
	unsigned command : 			3;
	unsigned triggerOn : 		1;
	unsigned triggerOff : 		1;
	unsigned triggerOnce :		1;
	unsigned restState :		1; // state to return to on callback
	unsigned key :				3;

	unsigned difficulty :		2;
	unsigned detail :			2;
	unsigned map : 				2;
	unsigned view :				4;
	unsigned soundKit :			8;

	unsigned isTriggered :      1; // used for triggerOnce objects
	unsigned triggerPush :      1; // gates?
	unsigned triggerImpact :    1; // exploding things, etc.
	unsigned triggerPickup :    1; // secrets
	unsigned triggerTouch :     1; // sawblades, spikes, zaps?
	unsigned triggerSight :     1; // dunno, yet.
	unsigned triggerProximity : 1; // proximity bombs

	unsigned decoupled :		1;
	unsigned waitTime : 		8;

	// unsigned permanent :		1; // 0=not permanent, 1=permanent (respawn ignored)
	// unsigned respawn :		2; // 0=never, 1=always, 2=optional never, 3=optional always
	// unsigned respawnTime :	8; // 0=instant, >0=time in tenths of a second,
	// unsigned launchMode :	2; // 0=all, 1=bloodbath, 2=ally, 3=ally&bloodbath,
};

struct XWALL
{
	signed reference :			16;
	unsigned state : 			1;
	unsigned map :				2;

	// trigger data
	unsigned busy : 			16;
	unsigned type :				10;
	signed data :				16;
	unsigned txID :				10;
	unsigned rxID :				10;
	unsigned command : 			3;
	unsigned triggerOn : 		1;
	unsigned triggerOff : 		1;
	unsigned triggerOnce :		1;
	unsigned key :				3;

	// panning data
	signed panXVel :			8;
	signed panYVel :			8;

	unsigned isTriggered :   	1; // used for triggerOnce objects
	unsigned triggerPush :   	1;
	unsigned triggerImpact : 	1;
	unsigned triggerRsvd1 :   	1;
	unsigned triggerRsvd2 : 	1;

	unsigned decoupled :		1;
	unsigned panAlways :		1;
	unsigned restState :		1; // state to return to on callback
	unsigned waitTime :			8;
};

struct XSECTOR
{
	signed reference :			16;
	unsigned state : 			1;

	// trigger data
	unsigned busy : 			16;
	unsigned type :				10;
	signed data :				16;
	unsigned txID :				10;
	unsigned rxID :				10;
	unsigned command : 			3;
	unsigned triggerOn : 		1;
	unsigned triggerOff : 		1;
	unsigned triggerOnce :		1;
	unsigned restState :		1; // state to return to on callback
	unsigned key :				3;

	// lighting data
	signed amplitude :			8;
	unsigned freq :				8;
	unsigned wave :				4;
	unsigned shadeAlways : 		1;
	unsigned unused2 :			3;	// !!! USE ME !!!
	unsigned phase :			8;
	unsigned shadeFloor :		1;
	unsigned shadeCeiling : 	1;
	unsigned shadeWalls :		1;
	signed shade :				8;

	// panning data
	unsigned panFloor :			1;
	unsigned panCeiling :		1;
	unsigned panDrag :			1;
	unsigned waitTime : 		8;
	unsigned panAlways :		1;
	unsigned unused3 :			3;	// !!! USE ME !!!

	// speed for moving sectors
	unsigned busyTime :			8;

	// water stuff
	unsigned underwater :		1;
	unsigned depth :			2;
	unsigned panVel :      		8;
	unsigned panAngle :			12; // change to 11 bits in CONVDB4!!!

	unsigned wind :		    	3;

	unsigned isTriggered :		1; // used for triggerOnce objects
	unsigned triggerPush :		1;
	unsigned triggerImpact :	1;
	unsigned triggerEnter :		1;
	unsigned triggerExit :		1;
	unsigned decoupled :		1;

	unsigned triggerWPush :		1;	// push from outside wall (convenience bit)

 	// uncomment these for CONVDB4
//	signed ceilingOffZ :    	32;
//	signed ceilingOnZ :    	32;
//	signed floorOffZ :      	32;
//	signed floorOnZ :      	32;
};
#endif

#if 0
// PROPOSED XSECTOR CHANGES TO DECREASE MEMORY REQUIREMENTS
//
struct XSECTOR
{
	signed reference :    		15;  // *** Made 1 bit smaller to accommodate state
	unsigned state :       		1;

	unsigned trigger :    		16;  // these fields could be smaller
	unsigned lighting :   		16;  // unless you want to make them pointers
	unsigned surfMotion : 		16;  // and allocate them at load time.
	unsigned motion :     		16;  // this might work since sectors are not
	unsigned zMotion :    		16;  // allocated/deallocated during play
};

struct TRIGGER
{
	unsigned type :			 	10;
	unsigned state :         	1;
	unsigned command : 		 	3;
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
	unsigned panDrag :			1;
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
	ZMOTION     zMotion;
};

// these could be allocated, see XSECTOR notes above
TRIGGER    gSectorTriggers[ kMaxSectorTriggers ];
LIGHTING   gLighting[ kMaxLighting ];
SURFMOTION gSurfMotion[ kMaxSurfMotion ];
MOTION     gMotion[ kMaxMotion ];
ZMOTION    gZMotion[ kMaxZMotion ];

NOTES: A SECTOR structure would be saved out prior to the DISK_XSECTOR.
Also, surftypes are not accounted for here, but could be saved in both the
lotag and hitag fields of the sector. For walls, the surftype could be
placed in either the lotag or hitag.

#endif


/***********************************************************************
 * Variables
 **********************************************************************/
extern XSPRITE xsprite[];
extern XWALL xwall[];
extern XSECTOR xsector[];

extern uchar spriteSurf[];
extern uchar wallSurf[];
extern uchar floorSurf[];
extern uchar ceilingSurf[];

#define kMapAuthorLen 64
#define kMapNameLen   64

extern char gMapAuthor[];
extern char gMapName[];
extern int gMapRev;
extern int gSongId;
extern ulong gMapCRC;
extern int gSkyCount;


/***********************************************************************
 * Global Variables / MPX related strings
 **********************************************************************/
extern char *gItemText[ kItemMax - kItemBase ];
extern char *gAmmoText[ kAmmoMax - kAmmoBase ];
extern char *gWeaponText[ kWeaponItemMax - kWeaponItemBase ];
extern char *gCreatureText[ kCreatureMax - kCreatureBase ];
extern char *gThingText[ kThingMax - kThingBase ];


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
