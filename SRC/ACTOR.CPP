#include <stdlib.h>
#include <string.h>

#include "actor.h"
#include "debug4g.h"
#include "engine.h"
#include "trig.h"
#include "gameutil.h"
#include "misc.h"
#include "db.h"
#include "multi.h"
#include "names.h"
#include "screen.h"
#include "sectorfx.h"
#include "triggers.h"
#include "error.h"
#include "globals.h"
#include "seq.h"
#include "eventq.h"
#include "dude.h"
#include "ai.h"
#include "view.h"
#include "warp.h"
#include "tile.h"
#include "player.h"
#include "options.h"
#include "sfx.h"
#include "sound.h"	// can be removed once sfx complete

#define kMaxSpareSprites	50

SPRITEHIT gSpriteHit[kMaxXSprites];

static void MakeSplash( SPRITE *pSprite, XSPRITE *pXSprite );
static void FireballCallback( int /* type */, int nXIndex );
//static void FlareCallback( int /* type */, int nXIndex );

struct VECTORDATA
{
	DAMAGE_TYPE damageType;
	int damageValue;
	int maxDist;
	struct
	{
		int nEffect;
		int nSoundId;
	} impact[kSurfMax];
};


static VECTORDATA gVectorData[kVectorMax] =
{
	// kVectorTine
	{ kDamageStab, 4, M2X(2.25),
		{
			{ -1, -1 },							// kSurfNone
			{ ET_Ricochet1, kSfxForkStone },	// kSurfStone
			{ ET_Ricochet1, kSfxForkMetal },	// kSurfMetal
			{ ET_Ricochet2, kSfxForkWood },		// kSurfWood
			{ ET_Squib1, kSfxForkFlesh },		// kSurfFlesh
			{ ET_Splash2, -1 },					// kSurfWater
			{ ET_Ricochet2, kSfxForkWood },		// kSurfDirt
			{ ET_Ricochet2, kSfxForkWood },		// kSurfClay
			{ -1, -1 },							// kSurfSnow
			{ -1, -1 },							// kSurfIce
			{ -1, -1 },							// kSurfLeaves
			{ -1, -1 },							// kSurfCloth
			{ -1, kSfxForkWood },				// kSurfPlant
			{ -1, -1 },							// kSurfGoo
			{ -1, -1 },							// kSurfLava
		}
	},

	// kVectorShell
	{ kDamageBullet, 4, 0,
		{
			{ -1, -1 },							// kSurfNone
			{ ET_Ricochet1, -1 },				// kSurfStone
			{ ET_Ricochet1, kSfxForkMetal },	// kSurfMetal
			{ ET_Ricochet2, -1 },				// kSurfWood
			{ ET_Squib1, -1 },					// kSurfFlesh
			{ ET_Splash2, -1 },					// kSurfWater
			{ ET_Ricochet2, -1 },				// kSurfDirt
			{ ET_Ricochet2, -1 },				// kSurfClay
			{ -1, -1 },							// kSurfSnow
			{ -1, -1 },							// kSurfIce
			{ -1, -1 },							// kSurfLeaves
			{ -1, -1 },							// kSurfCloth
			{ -1, -1 },							// kSurfPlant
			{ -1, -1 },							// kSurfGoo
			{ -1, -1 },							// kSurfLava
		}
	},

	// kVectorBullet
	{ kDamageBullet, 4, 0,
		{
			{ -1, -1 },							// kSurfNone
			{ ET_Ricochet1, kSfxForkStone },	// kSurfStone
			{ ET_Ricochet1, kSfxForkMetal },	// kSurfMetal
			{ ET_Ricochet2, kSfxForkWood },		// kSurfWood
			{ ET_Squib1, kSfxForkFlesh },		// kSurfFlesh
			{ ET_Splash2, -1 },					// kSurfWater
			{ ET_Ricochet2, kSfxForkWood },		// kSurfDirt
			{ ET_Ricochet2, kSfxForkWood },		// kSurfClay
			{ -1, -1 },							// kSurfSnow
			{ -1, -1 },							// kSurfIce
			{ -1, -1 },							// kSurfLeaves
			{ -1, -1 },							// kSurfCloth
			{ -1, kSfxForkWood },				// kSurfPlant
			{ -1, -1 },							// kSurfGoo
			{ -1, -1 },							// kSurfLava
		}
	},

	// kVectorBulletAP
	{ kDamageBullet, 4, 0,
		{
			{ -1, -1 },							// kSurfNone
			{ ET_Ricochet1, kSfxForkStone },	// kSurfStone
			{ ET_Ricochet1, kSfxForkMetal },	// kSurfMetal
			{ ET_Ricochet2, kSfxForkWood },		// kSurfWood
			{ ET_Squib1, kSfxForkFlesh },		// kSurfFlesh
			{ ET_Splash2, -1 },					// kSurfWater
			{ ET_Ricochet2, kSfxForkWood },		// kSurfDirt
			{ ET_Ricochet2, kSfxForkWood },		// kSurfClay
			{ -1, -1 },							// kSurfSnow
			{ -1, -1 },							// kSurfIce
			{ -1, -1 },							// kSurfLeaves
			{ -1, -1 },							// kSurfCloth
			{ -1, kSfxForkWood },				// kSurfPlant
			{ -1, -1 },							// kSurfGoo
			{ -1, -1 },							// kSurfLava
		}
	},

	// kVectorAxe
	{ kDamageStab, 20, M2X(2.0),
		{
			{ -1, -1 },							// kSurfNone
			{ ET_Ricochet1, kSfxForkStone },	// kSurfStone
			{ ET_Ricochet1, kSfxForkMetal },	// kSurfMetal
			{ ET_Ricochet2, kSfxForkWood },		// kSurfWood
			{ ET_Squib1, kSfxForkFlesh },		// kSurfFlesh
			{ ET_Splash2, -1 },					// kSurfWater
			{ ET_Ricochet2, kSfxForkWood },		// kSurfDirt
			{ ET_Ricochet2, kSfxForkWood },		// kSurfClay
			{ -1, -1 },							// kSurfSnow
			{ -1, -1 },							// kSurfIce
			{ -1, -1 },							// kSurfLeaves
			{ -1, -1 },							// kSurfCloth
			{ -1, kSfxForkWood },				// kSurfPlant
			{ -1, -1 },							// kSurfGoo
			{ -1, -1 },							// kSurfLava
		}
	},

	// kVectorCleaver
	{ kDamageStab, 10, M2X(2.0),
		{
			{ -1, -1 },							// kSurfNone
			{ ET_Ricochet1, kSfxForkStone },	// kSurfStone
			{ ET_Ricochet1, kSfxForkMetal },	// kSurfMetal
			{ ET_Ricochet2, kSfxForkWood },		// kSurfWood
			{ ET_Squib1, kSfxForkFlesh },		// kSurfFlesh
			{ ET_Splash2, -1 },					// kSurfWater
			{ ET_Ricochet2, kSfxForkWood },		// kSurfDirt
			{ ET_Ricochet2, kSfxForkWood },		// kSurfClay
			{ -1, -1 },							// kSurfSnow
			{ -1, -1 },							// kSurfIce
			{ -1, -1 },							// kSurfLeaves
			{ -1, -1 },							// kSurfCloth
			{ -1, kSfxForkWood },				// kSurfPlant
			{ -1, -1 },							// kSurfGoo
			{ -1, -1 },							// kSurfLava
		}
	},

	// kVectorClaw
	{ kDamageStab, 20, M2X(2.0),
		{
			{ -1, -1 },							// kSurfNone
			{ ET_Ricochet1, kSfxForkStone },	// kSurfStone
			{ ET_Ricochet1, kSfxForkMetal },	// kSurfMetal
			{ ET_Ricochet2, kSfxForkWood },		// kSurfWood
			{ ET_Squib1, kSfxForkFlesh },		// kSurfFlesh
			{ ET_Splash2, -1 },					// kSurfWater
			{ ET_Ricochet2, kSfxForkWood },		// kSurfDirt
			{ ET_Ricochet2, kSfxForkWood },		// kSurfClay
			{ -1, -1 },							// kSurfSnow
			{ -1, -1 },							// kSurfIce
			{ -1, -1 },							// kSurfLeaves
			{ -1, -1 },							// kSurfCloth
			{ -1, kSfxForkWood },				// kSurfPlant
			{ -1, -1 },							// kSurfGoo
			{ -1, -1 },							// kSurfLava
		}
	},

	// kVectorHoundBite
	{ kDamageStab, 10, M2X(1.2),
		{
			{ -1, -1 },							// kSurfNone
			{ ET_Ricochet1, kSfxForkStone },	// kSurfStone
			{ ET_Ricochet1, kSfxForkMetal },	// kSurfMetal
			{ ET_Ricochet2, kSfxForkWood },		// kSurfWood
			{ ET_Squib1, kSfxForkFlesh },		// kSurfFlesh
			{ ET_Splash2, -1 },					// kSurfWater
			{ ET_Ricochet2, kSfxForkWood },		// kSurfDirt
			{ ET_Ricochet2, kSfxForkWood },		// kSurfClay
			{ -1, -1 },							// kSurfSnow
			{ -1, -1 },							// kSurfIce
			{ -1, -1 },							// kSurfLeaves
			{ -1, -1 },							// kSurfCloth
			{ -1, kSfxForkWood },				// kSurfPlant
			{ -1, -1 },							// kSurfGoo
			{ -1, -1 },							// kSurfLava
		}
	},

	// kVectorRatBite
	{ kDamageStab, 4, M2X(1.8),
		{
			{ -1, -1 },							// kSurfNone
			{ ET_Ricochet1, kSfxForkStone },	// kSurfStone
			{ ET_Ricochet1, kSfxForkMetal },	// kSurfMetal
			{ ET_Ricochet2, kSfxForkWood },		// kSurfWood
			{ ET_Squib1, kSfxForkFlesh },		// kSurfFlesh
			{ ET_Splash2, -1 },					// kSurfWater
			{ ET_Ricochet2, kSfxForkWood },		// kSurfDirt
			{ ET_Ricochet2, kSfxForkWood },		// kSurfClay
			{ -1, -1 },							// kSurfSnow
			{ -1, -1 },							// kSurfIce
			{ -1, -1 },							// kSurfLeaves
			{ -1, -1 },							// kSurfCloth
			{ -1, kSfxForkWood },				// kSurfPlant
			{ -1, -1 },							// kSurfGoo
			{ -1, -1 },							// kSurfLava
		}
	},

	// kVectorSpiderBite
	{ kDamageStab, 8, M2X(1.2),
		{
			{ -1, -1 },							// kSurfNone
			{ ET_Ricochet1, kSfxForkStone },	// kSurfStone
			{ ET_Ricochet1, kSfxForkMetal },	// kSurfMetal
			{ ET_Ricochet2, kSfxForkWood },		// kSurfWood
			{ ET_Squib1, kSfxForkFlesh },		// kSurfFlesh
			{ ET_Splash2, -1 },					// kSurfWater
			{ ET_Ricochet2, kSfxForkWood },		// kSurfDirt
			{ ET_Ricochet2, kSfxForkWood },		// kSurfClay
			{ -1, -1 },							// kSurfSnow
			{ -1, -1 },							// kSurfIce
			{ -1, -1 },							// kSurfLeaves
			{ -1, -1 },							// kSurfCloth
			{ -1, kSfxForkWood },				// kSurfPlant
			{ -1, -1 },							// kSurfGoo
			{ -1, -1 },							// kSurfLava
		}
	},
};


ITEMDATA gItemData[ kMaxItemTypes ] =
{
/*
{cstat,	picnum,				sh,	pal,xr,	yr,	stat,		type,				flags},
*/
	{0,	kPicKey1,			-8,	0,	32,	32,	kStatItem,	kItemKey1,				0},
	{0,	kPicKey2,			-8,	0,	32,	32,	kStatItem,	kItemKey2,				0},
	{0,	kPicKey3,			-8,	0,	32,	32,	kStatItem,	kItemKey3,				0},
	{0,	kPicKey4,			-8,	0,	32,	32,	kStatItem,	kItemKey4,				0},
	{0,	kPicKey5,			-8,	0,	32,	32,	kStatItem,	kItemKey5,				0},
	{0,	kPicKey6,			-8,	0,	32,	32,	kStatItem,  kItemKey6,				0},
	{0,	-1,					-8,	0,	-1,	-1,	kStatItem,	kItemKey7,				0},
	{0,	kPicDocBag,			-8,	0,	48,	48,	kStatItem,	kItemDoctorBag,			0},
	{0,	kPicMedPouch,		-8,	0,	40,	40,	kStatItem,	kItemMedPouch,			0},
	{0,	kPicEssence,		-8,	0,	40,	40,	kStatItem,	kItemLifeEssence,		0},
	{0,	kAnmLifeSeed,		-8,	0,	40,	40,	kStatItem,	kItemLifeSeed,			0},
	{0,	kPicPotion,			-8,	0,	40,	40,	kStatItem,	kItemPotion1,			0},
	{0,	kAnmFeather,		-8,	0,	40,	40,	kStatItem,	kItemFeatherFall,		0},
	{0,	kAnmInviso,			-8,	0,	40,	40,	kStatItem,	kItemLtdInvisibility,	0},
	{0,	kPicInvulnerable,	-8,	0,	40,	40,	kStatItem,	kItemInvulnerability,	0},
	{0,	kPicJumpBoots,		-8,	0,	40,	40,	kStatItem,	kItemJumpBoots,			0},
	{0,	kPicRavenFlight,	-8,	0,	40,	40,	kStatItem,	kItemRavenFlight,		0},
	{0,	kPicGunsAkimbo,		-8,	0,	40,	40,	kStatItem,	kItemGunsAkimbo,		0},
	{0,	kPicDivingSuit,		-8,	0,	80,	64,	kStatItem,	kItemDivingSuit,		0},
	{0,	kPicGasMask,		-8,	0,	40,	40,	kStatItem,	kItemGasMask,			0},
	{0,	kAnmClone,			-8,	0,	40,	40,	kStatItem,	kItemClone,				0},
	{0,	kPicCrystalBall,	-8,	0,	40,	40,	kStatItem,	kItemCrystalBall,		0},
	{0,	kPicDecoy,			-8,	0,	40,	40,	kStatItem,	kItemDecoy,				0},
	{0,	kAnmDoppleganger,	-8,	0,	40,	40,	kStatItem,	kItemDoppleganger,		0},
	{0,	kAnmReflectShots,	-8,	0,	40,	40,	kStatItem,	kItemReflectiveShots,	0},
	{0,	kPicRoseGlasses,	-8,	0,	40,	40,	kStatItem,	kItemRoseGlasses,		0},
	{0,	kAnmCloakNDagger,	-8,	0,	64,	64,	kStatItem,	kItemShadowCloak,		0},
	{0,	kPicShroom1,		-8,	0,	48,	48,	kStatItem,	kItemShroomRage,		0},
	{0,	kPicShroom2,		-8,	0,	48,	48,	kStatItem,	kItemShroomDelirium,	0},
	{0,	kPicShroom3,		-8,	0,	48,	48,	kStatItem,	kItemShroomGrow,		0},
	{0,	kPicShroom4,		-8,	0,	48,	48,	kStatItem,	kItemShroomShrink,		0},
	{0,	kPicDeathMask,		-8,	0,	40,	40,	kStatItem,	kItemDeathMask,			0},
	{0,	kPicGoblet,			-8,	0,	40,	40,	kStatItem,	kItemWineGoblet,		0},
	{0,	kPicBottle1,		-8,	0,	40,	40,	kStatItem,	kItemWineBottle,		0},
	{0,	kPicSkullGrail,		-8,	0,	40,	40,	kStatItem,	kItemSkullGrail,		0},
	{0,	kPicSilverGrail,	-8,	0,	40,	40,	kStatItem,	kItemSilverGrail,		0},
	{0,	kPicTome1,			-8,	0,	40,	40,	kStatItem,	kItemTome,				0},
	{0,	kPicBlackChest,		-8,	0,	40,	40,	kStatItem,	kItemBlackChest,		0},
	{0,	kPicWoodChest,		-8,	0,	40,	40,	kStatItem,	kItemWoodenChest,		0},
	{0,	kPicAsbestosSuit,	-8,	0,	80,	64,	kStatItem,	kItemAsbestosArmor,		0},
};

AMMOITEMDATA gAmmoItemData[kAmmoItemMax - kAmmoItemBase] =
{
/*
{cstat,	picnum,				sh,	pal,xr,	yr,		flags,	count,			ammoType,			weaponType},
*/
	{0,	kPicSprayCan,		-8,	0,	40,	40,		0,		kTimerRate*8,	kAmmoSprayCan,		kWeaponSprayCan},
	{0,	kPicTNTStick,		-8,	0,	48,	48,		0,		1,				kAmmoTNTStick,		kWeaponTNT},
	{0,	kPicTNTPak,			-8,	0,	48,	48,		0,		7,	 			kAmmoTNTStick,		kWeaponTNT},
	{0,	kPicTNTBox,			-8,	0,	48,	48,		0,		14,				kAmmoTNTStick,		kWeaponTNT},
	{0,	kPicTNTProx,		-8,	0,	48,	48,		0,		1,	 			kAmmoTNTProximity,	kWeaponTNT},
	{0,	kPicTNTRemote,		-8,	0,	48,	48,		0,		1,				kAmmoTNTRemote,		kWeaponTNT},
	{0,	kPicTNTTimer,		-8,	0,	48,	48,		0,		1,	 			kAmmoTNTStick,		kWeaponTNT},
	{0,	kPicShotShells,		-8,	0,	48,	48,		0,		4,	 			kAmmoShell},
	{0,	kPicShellBox,		-8,	0,	48,	48,		0,		16,				kAmmoShell},
	{0,	kPicBullets,		-8,	0,	48,	48,		0,		8,	 			kAmmoBullet},
	{0,	kPicBulletBox,		-8,	0,	48,	48,		0,		50,				kAmmoBullet},
	{0,	kPicBulletBoxAP,	-8,	0,	48,	48,		0,		50,				kAmmoBulletAP},
	{0,	kPicTommyDrum,		-8,	0,	48,	48,		0,		100,			kAmmoBullet},
	{0,	kPicSpear,			-8,	0,	48,	48,		0,		1,	 			kAmmoSpear},
	{0,	kPicSpears,			-8,	0,	48,	48,		0,		6,	 			kAmmoSpear},
	{0,	kPicSpearExplode,	-8,	0,	48,	48,		0,		6,	 			kAmmoSpearXP},
	{0,	kPicFlares,			-8,	0,	48,	48,		0,		8,	 			kAmmoFlare},
	{0,	kPicFlareHE,		-8,	0,	48,	48,		0,		8,	 			kAmmoFlare},
	{0,	kPicFlareBurst,		-8,	0,	48,	48,		0,		8,	 			kAmmoFlareSB},
};

WEAPONITEMDATA gWeaponItemData[ kWeaponItemMax - kWeaponItemBase ] =
{
/*
{cstat,	picnum,			sh,	pal,xr,	yr,	flags,	weaponType,			ammoType,		count },
*/
	{ 0,	-1,				0,	0,	0,	0,	0,		kWeaponNone,		kAmmoNone,		0 },
	{ 0,	kPicShotgun,	-8,	0,	48,	48,	0,		kWeaponShotgun,		kAmmoShell,		8 },
	{ 0,	kPicTommyGun,	-8,	0,	48,	48,	0,		kWeaponTommy,		kAmmoBullet,	50 },
	{ 0,	kPicFlareGun,	-8,	0,	48,	48,	0,		kWeaponFlare,		kAmmoFlare,		8 },
	{ 0,	kPicVoodooDoll,	-8,	0,	48,	48,	0,		kWeaponVoodoo,		kAmmoVoodoo,	1 },
	{ 0,	kPicSpearGun,	-8,	0,	48,	48,	0,		kWeaponSpear,		kAmmoSpear,		6 },
	{ 0,	kPicShadowGun,	-8,	0,	48,	48,	0,		kWeaponShadow,		kAmmoNone,		0 },
	{ 0,	-1,				0,	0,	0,	0,	0,		kWeaponPitchfork,	kAmmoNone,		0 },	// don't actually pick this up
	{ 0,	kPicSprayCan,	-8,	0,	48,	48,	0,		kWeaponSprayCan,	kAmmoSprayCan,	kTimerRate*8},
	{ 0,	kPicTNTStick,	-8,	0,	48,	48,	0,		kWeaponTNT,			kAmmoTNTStick,	1},
};


struct MissileType {
	short picnum;
	int velocity;
	int angleOfs;
	uchar xrepeat;
	uchar yrepeat;
	char  shade;
} missileInfo[] = {
	{ kAnmButcherKnife,	(M2X(14.0) << 4) / kTimerRate,	kAngle90, 	40, 40,  -16 },	// kMissileButcherKnife
	{ kAnmFlare,		(M2X(20.0) << 4) / kTimerRate,	0,        	32, 32, -128 },	// kMissileFlare
	{ kAnmFlare,		(M2X(20.0) << 4) / kTimerRate,	0,        	32, 32, -128 },	// kMissileExplodingFlare
	{ kAnmFlare,		(M2X(20.0) << 4) / kTimerRate,	0,        	32, 32, -128 },	// kMissileStarburstFlare
	{ 0,				(M2X(4.0) << 4) / kTimerRate,	0,        	24, 24, -128 },	// kMissileSprayFlame
	{ 0,				(M2X(16.0) << 4) / kTimerRate,	0,        	32, 32, -128 },	// kMissileFireball
	{ kAnmSpear,		(M2X(16.0) << 4) / kTimerRate,	kAngle270, 	64, 64,   -8 },	// kMissileSpear	// 18.0
	{ kAnmEctoSkull,	(M2X(16.0) << 4) / kTimerRate,	0,			32, 32,  -24 },	// kMissileEctoSkull
	{ 0,				(M2X(6.0) << 4) / kTimerRate,	0,        	24, 24, -128 },	// kMissileHoundFire
	{ 0,				(M2X(12.0) << 4) / kTimerRate,	0,        	8,   8,    0 },	// kMissileGreenPuke
	{ 0,				(M2X(12.0) << 4) / kTimerRate,	0,        	8,   8,    0 },	// kMissileRedPuke
};


static int dragTable[] =
{
	0,
	0x0C00,		// kDepthTread
	0x1A00,		// kDepthWade
	0x1A00,		// kDepthSwim
};

// miscellaneous effects
enum
{
	kSeqSprayFlame1	= 0,
	kSeqSprayFlame2,
	kSeqSkull,
	kSeqExplodeC1L,	// large concussion on ground
	kSeqExplodeC1M,	// medium concussion on ground
	kSeqExplodeC1S, // small concussion on ground
	kSeqExplodeC2L, // large concussion in air
	kSeqExplodeC2M, // medium concussion in air
	kSeqExplodeC2S, // small concussion in air
	kSeqExplodeC2T, // tiny concussion in air
	kSeqSplash1,
	kSeqSplash2,
	kSeqSplash3,	// blood spash
	kSeqRicochet1,
	kSeqGoreWing,
	kSeqGoreHead,
	kSeqBarrel,
	kSeqBloodPool,
	kSeqRespawn,
	kSeqFlareSmoke,
	kSeqSquib1,
	kSeqFluorescentLight,
	kSeqClearGlass,
	kSeqStainedGlass,
	kSeqWeb,
	kSeqBeam,
	kSeqVase1,
	kSeqVase2,
	kSeqZombieBones,
	kSeqSkullExplode,
	kSeqMetalGrate1,
	kSeqFireball,
	kSeqBoneBreak,
	kSeqBurningTree1,
	kSeqBurningTree2,
	kSeqHoundFire,
	kSeqMGunDead,
	kSeqRicochet2,
	kSeqEffectMax,
};


struct THINGINFO
{
	short	startHealth;
	short	mass;			// in KG
	char	clipdist;
	ushort	flags;
	int 	damageShift[kDamageMax];	// use to indicate resistance to damage types
};

static THINGINFO thingInfo[kThingMax - kThingBase] =
{
	{	// kThingTNTBarrel
		40,
		150,
		32,
		kAttrMove | kAttrGravity,
		{
			kNoDamage,	// kDamagePummel
			0,          // kDamageFall
			0,          // kDamageBurn
			0,          // kDamageBullet
			1,			// kDamageStab
			0,          // kDamageExplode
			kNoDamage,	// kDamageGas
			kNoDamage,  // kDamageDrown
			kNoDamage,  // kDamageSpirit
			kNoDamage,	// kDamageVoodoo
		},
	},

	{	// kThingTNTProxArmed
		5,
		5,
		16,
		kAttrMove | kAttrGravity,
		{
			kNoDamage,	// kDamagePummel
			kNoDamage,	// kDamageFall
			0,			// kDamageBurn
			0,			// kDamageBullet
			kNoDamage,	// kDamageStab
			0,			// kDamageExplode
			kNoDamage,	// kDamageGas
			kNoDamage,	// kDamageDrown
			kNoDamage,	// kDamageSpirit
			kNoDamage,	// kDamageVoodoo
		},
	},

	{	// kThingTNTRemArmed
		5,
		5,
		16,
		kAttrMove | kAttrGravity,
		{
			kNoDamage,	// kDamagePummel
			kNoDamage,	// kDamageFall
			0,			// kDamageBurn
			0,			// kDamageBullet
			kNoDamage,	// kDamageStab
			0,			// kDamageExplode
			kNoDamage,	// kDamageGas
			kNoDamage,	// kDamageDrown
			kNoDamage,	// kDamageSpirit
			kNoDamage,	// kDamageVoodoo
		},
	},

	{ 	// kThingBlueVase
		1,
		20,
		32,
		kAttrMove | kAttrGravity,
		{
			kNoDamage,	// kDamagePummel
			0,			// kDamageFall
			kNoDamage,	// kDamageBurn
			0,			// kDamageBullet
			0,			// kDamageStab
			0,			// kDamageExplode
			kNoDamage,	// kDamageGas
			kNoDamage,	// kDamageDrown
			kNoDamage,	// kDamageSpirit
			kNoDamage,	// kDamageVoodoo
		},
	},

	{	// kThingBrownVase
		1,
		150,
		32,
		kAttrMove | kAttrGravity,
		{
			kNoDamage,	// kDamagePummel
			0,			// kDamageFall
			kNoDamage,	// kDamageBurn
			0,			// kDamageBullet
			0,			// kDamageStab
			0,			// kDamageExplode
			kNoDamage,	// kDamageGas
			kNoDamage,	// kDamageDrown
			kNoDamage,	// kDamageSpirit
			kNoDamage,	// kDamageVoodoo
		},
	},

	{	// kThingCrateFace
		10,
		0,
		0,
		0,
		{
			kNoDamage,	// kDamagePummel
			kNoDamage,	// kDamageFall
			kNoDamage,	// kDamageBurn
			kNoDamage,	// kDamageBullet
			kNoDamage,	// kDamageStab
			0,			// kDamageExplode
			kNoDamage,	// kDamageGas
			kNoDamage,	// kDamageDrown
			kNoDamage,	// kDamageSpirit
			kNoDamage,	// kDamageVoodoo
		},
	},

	{	// kThingClearGlass
		1,
		0,
		0,
		0,
		{
			0,			// kDamagePummel
			0,			// kDamageFall
			kNoDamage,	// kDamageBurn
			0,			// kDamageBullet
			0,			// kDamageStab
			0,			// kDamageExplode
			kNoDamage,	// kDamageGas
			kNoDamage,	// kDamageDrown
			kNoDamage,	// kDamageSpirit
			kNoDamage,	// kDamageVoodoo
		},
	},

	{	// kThingFluorescent
		1,
		0,
		0,
		0,
		{
			0,			// kDamagePummel
			kNoDamage,	// kDamageFall
			kNoDamage,	// kDamageBurn
			0,			// kDamageBullet
			0,			// kDamageStab
			0,			// kDamageExplode
			kNoDamage,	// kDamageGas
			kNoDamage,	// kDamageDrown
			kNoDamage,	// kDamageSpirit
			kNoDamage,	// kDamageVoodoo
		},
	},

	{	// kThingWallCrack
		8,
		0,
		0,
		0,
		{
			kNoDamage,	// kDamagePummel
			kNoDamage,	// kDamageFall
			kNoDamage,	// kDamageBurn
			kNoDamage,	// kDamageBullet
			kNoDamage,	// kDamageStab
			0,			// kDamageExplode
			kNoDamage,	// kDamageGas
			kNoDamage,	// kDamageDrown
			kNoDamage,	// kDamageSpirit
			kNoDamage,	// kDamageVoodoo
		},
	},

	{	// kThingWoodBeam
		8,
		0,
		0,
		0,
		{
			0,			// kDamagePummel
			kNoDamage,	// kDamageFall
			kNoDamage,	// kDamageBurn
			0,			// kDamageBullet
			0,			// kDamageStab
			0,			// kDamageExplode
			kNoDamage,	// kDamageGas
			kNoDamage,	// kDamageDrown
			kNoDamage,	// kDamageSpirit
			kNoDamage,	// kDamageVoodoo
		},
	},

	{	// kThingWeb
		4,
		0,
		0,
		0,
		{
			0,			// kDamagePummel
			kNoDamage,	// kDamageFall
			0,			// kDamageBurn
			1,			// kDamageBullet
			0,			// kDamageStab
			0,			// kDamageExplode
			kNoDamage,	// kDamageGas
			kNoDamage,	// kDamageDrown
			kNoDamage,	// kDamageSpirit
			kNoDamage,	// kDamageVoodoo
		},
	},

	{	// kThingMetalGrate1
		20,
		0,
		0,
		0,
		{
			3,			// kDamagePummel
			kNoDamage,	// kDamageFall
			kNoDamage,	// kDamageBurn
			2,			// kDamageBullet
			4,			// kDamageStab
			1,			// kDamageExplode
			kNoDamage,	// kDamageGas
			kNoDamage,	// kDamageDrown
			kNoDamage,	// kDamageSpirit
			kNoDamage,	// kDamageVoodoo
		},
	},

	{	// kThingBurnableTree
		1,
		0,
		0,
		0,
		{
			kNoDamage,	// kDamagePummel
			kNoDamage,	// kDamageFall
			0,			// kDamageBurn
			kNoDamage,	// kDamageBullet
			kNoDamage,	// kDamageStab
			kNoDamage,	// kDamageExplode
			kNoDamage,	// kDamageGas
			kNoDamage,	// kDamageDrown
			0,			// kDamageSpirit
			kNoDamage,	// kDamageVoodoo
		},
	},

	{	// kThingMachineGun
		50,
		0,
		0,
		0,
		{
			kNoDamage,	// kDamagePummel
			kNoDamage,	// kDamageFall
			kNoDamage,	// kDamageBurn
			2,			// kDamageBullet
			2,			// kDamageStab
			1,			// kDamageExplode
			kNoDamage,	// kDamageGas
			kNoDamage,	// kDamageDrown
			kNoDamage,	// kDamageSpirit
			kNoDamage,	// kDamageVoodoo
		},
	},

	{	// kThingTNTStick
		5,
		2,
		16,
		kAttrMove | kAttrGravity,
		{
			0,			// kDamagePummel
			kNoDamage,	// kDamageFall
			0,			// kDamageBurn
			0,			// kDamageBullet
			0,			// kDamageStab
			0,			// kDamageExplode
			kNoDamage,	// kDamageGas
			kNoDamage,	// kDamageDrown
			kNoDamage,	// kDamageSpirit
			kNoDamage,	// kDamageVoodoo
		},
	},

	{	// kThingTNTBundle
		5,
		14,
		16,
		kAttrMove | kAttrGravity,
		{
			0,			// kDamagePummel
			kNoDamage,	// kDamageFall
			0,			// kDamageBurn
			0,			// kDamageBullet
			0,			// kDamageStab
			0,			// kDamageExplode
			kNoDamage,	// kDamageGas
			kNoDamage,	// kDamageDrown
			kNoDamage,	// kDamageSpirit
			kNoDamage,	// kDamageVoodoo
		},
	},

	{	// kThingBoneClub
		5,
		6,
		16,
		kAttrMove | kAttrGravity,
		{
			kNoDamage,	// kDamagePummel
			kNoDamage,	// kDamageFall
			kNoDamage,	// kDamageBurn
			kNoDamage,	// kDamageBullet
			kNoDamage,	// kDamageStab
			kNoDamage,	// kDamageExplode
			kNoDamage,	// kDamageGas
			kNoDamage,	// kDamageDrown
			kNoDamage,	// kDamageSpirit
			kNoDamage,	// kDamageVoodoo
		},
	},

	{	// kThingZombieBones
		8,
		3,
		16,
		kAttrMove | kAttrGravity,
		{
			0,			// kDamagePummel
			0,			// kDamageFall
			kNoDamage,	// kDamageBurn
			0,			// kDamageBullet
			0,			// kDamageStab
			0,			// kDamageExplode
			kNoDamage,	// kDamageGas
			kNoDamage,	// kDamageDrown
			kNoDamage,	// kDamageSpirit
			kNoDamage,	// kDamageVoodoo
		},
	},

	{	// kThingWaterDrip
		0,				// startHealth
		1,				// mass
		1,         		// clipDist
		kAttrGravity,
		{
			kNoDamage,	// kDamagePummel
			kNoDamage,	// kDamageFall
			kNoDamage,	// kDamageBurn
			kNoDamage,	// kDamageBullet
			kNoDamage,	// kDamageStab
			kNoDamage,	// kDamageExplode
			kNoDamage,	// kDamageGas
			kNoDamage,	// kDamageDrown
			kNoDamage,	// kDamageSpirit
			kNoDamage,	// kDamageVoodoo
		},
	},

	{	// kThingBloodDrip
		0,				// startHealth
		1,				// mass
		1,         		// clipDist
		kAttrGravity,
		{
			kNoDamage,	// kDamagePummel
			kNoDamage,	// kDamageFall
			kNoDamage,	// kDamageBurn
			kNoDamage,	// kDamageBullet
			kNoDamage,	// kDamageStab
			kNoDamage,	// kDamageExplode
			kNoDamage,	// kDamageGas
			kNoDamage,	// kDamageDrown
			kNoDamage,	// kDamageSpirit
			kNoDamage,	// kDamageVoodoo
		},
	},

	{	// kThingBubble
		0,				// startHealth
		-1,				// mass
		1,         		// clipDist
		kAttrMove,
		{
			kNoDamage,	// kDamagePummel
			kNoDamage,	// kDamageFall
			kNoDamage,	// kDamageBurn
			kNoDamage,	// kDamageBullet
			kNoDamage,	// kDamageStab
			kNoDamage,	// kDamageExplode
			kNoDamage,	// kDamageGas
			kNoDamage,	// kDamageDrown
			kNoDamage,	// kDamageSpirit
			kNoDamage,	// kDamageVoodoo
		},
	},

	{	// kThingBubbles
		0,				// startHealth
		-1,				// mass
		1,         		// clipDist
		kAttrMove,
		{
			kNoDamage,	// kDamagePummel
			kNoDamage,	// kDamageFall
			kNoDamage,	// kDamageBurn
			kNoDamage,	// kDamageBullet
			kNoDamage,	// kDamageStab
			kNoDamage,	// kDamageExplode
			kNoDamage,	// kDamageGas
			kNoDamage,	// kDamageDrown
			kNoDamage,	// kDamageSpirit
			kNoDamage,	// kDamageVoodoo
		},
	},

	{	// kThingGibSmall
		0,				// startHealth
		2,				// mass
		4,         		// clipDist
		kAttrMove | kAttrGravity,
		{
			kNoDamage,	// kDamagePummel
			kNoDamage,	// kDamageFall
			kNoDamage,	// kDamageBurn
			kNoDamage,	// kDamageBullet
			kNoDamage,	// kDamageStab
			kNoDamage,	// kDamageExplode
			kNoDamage,	// kDamageGas
			kNoDamage,	// kDamageDrown
			kNoDamage,	// kDamageSpirit
			kNoDamage,	// kDamageVoodoo
		},
	},
};


void actAllocateSpares( void )
{
/*
	dprintf("Creating spare sprites\n");
	for (int i = 0; i < kMaxSpareSprites; i++)
	{
		int nSprite = insertsprite( 0, kStatSpares );
		dassert(nSprite != -1);
		dbInsertXSprite(nSprite);
		sprite[nSprite].cstat |= kSpriteInvisible;
	}
*/
}


/*******************************************************************************
	FUNCTION:		actInit()

	DESCRIPTION:	Initialize the actor subsystem.  Locks all sequences, and
					preloads all tiles for sequences used in the map.

	NOTES:
*******************************************************************************/
void actInit( void )
{
	int nSprite;
	BOOL used[kDudeMax - kDudeBase];

	memset(used, FALSE, sizeof(used));

	// allocate sprites to use for effects
	actAllocateSpares();

	// see which dudes are present
	for (nSprite = headspritestat[kStatDude]; nSprite >= 0; nSprite = nextspritestat[nSprite])
	{
		SPRITE *pSprite = &sprite[nSprite];
		if ( pSprite->type < kDudeBase || pSprite->type >= kDudeMax )
		{
			dprintf("ERROR IN SPRITE %i\n", nSprite);
			dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
		}

		used[pSprite->type - kDudeBase] = TRUE;
	}


	// preload tiles for dude sequences
	dprintf("Preload dude sequence tiles\n");
	for (int i = 0; i < kDudeMax - kDudeBase; i++)
	{
		if ( used[i] )
		{
			// only preload art for idle sequence
//			if ( dudeInfo[i].pSeq[0] != NULL )
//				dudeInfo[i].pSeq[0]->Preload();
		}
	}

	// initialize all dudes
	for (nSprite = headspritestat[kStatDude]; nSprite >= 0; nSprite = nextspritestat[nSprite])
	{
		SPRITE *pSprite = &sprite[nSprite];
		int nXSprite = pSprite->extra;
		dassert(nXSprite > 0 && nXSprite < kMaxXSprites);
		XSPRITE *pXSprite = &xsprite[nXSprite];

		int dudeIndex = pSprite->type - kDudeBase;

		pSprite->cstat |= kSpriteBlocking | kSpriteHitscan;
		pSprite->clipdist = dudeInfo[dudeIndex].clipdist;
		pSprite->flags = kAttrMove | kAttrGravity | kAttrFalling;
		pSprite->xvel = pSprite->yvel = pSprite->zvel = 0;
		pXSprite->health = dudeInfo[dudeIndex].startHealth << 4;

		if ( gSysRes.Lookup( dudeInfo[dudeIndex].seqStartID + kSeqDudeIdle, ".SEQ") != NULL )
			seqSpawn(dudeInfo[dudeIndex].seqStartID + kSeqDudeIdle, SS_SPRITE, nXSprite);
	}

	// initialize all things
	for (nSprite = headspritestat[kStatThing]; nSprite >= 0; nSprite = nextspritestat[nSprite])
	{
		SPRITE *pSprite = &sprite[nSprite];
		int nXSprite = pSprite->extra;
		dassert(nXSprite > 0 && nXSprite < kMaxXSprites);
		XSPRITE *pXSprite = &xsprite[nXSprite];

		int thingIndex = pSprite->type - kThingBase;

		pSprite->cstat |= kSpriteBlocking; //  | kSpriteHitscan should be handled in BSTUB by hitbit
		pSprite->clipdist = thingInfo[thingIndex].clipdist;
		pSprite->flags = thingInfo[thingIndex].flags;
 		pSprite->xvel = pSprite->yvel = pSprite->zvel = 0;
 		pXSprite->health = thingInfo[thingIndex].startHealth << 4;
		switch(pSprite->type)
		{
			case kThingMachineGun:
				pXSprite->state = 0;	// these must start off
				break;
			default:
				pXSprite->state = 1;
				break;
		}
	}

	aiInit();
}


#define kGlobalForceShift	26		// arbitrary scale for all concussion
static void ConcussSprite( int nSource, int nSprite, int x, int y, int z, int ticks )
{
	SPRITE *pSprite = &sprite[nSprite];
	int dx = pSprite->x - x;
	int dy = pSprite->y - y;
	int dz = (pSprite->z - z) >> 4;

	int dist2 = ClipLow(dx * dx + dy * dy + dz * dz, 32 << 4);
	int nTile = pSprite->picnum;
 	int area = tilesizx[nTile] * pSprite->xrepeat * tilesizy[nTile] * pSprite->yrepeat >> 12;

	int force = divscale(ticks, dist2, kGlobalForceShift);

	if ( pSprite->flags & kAttrMove )
	{
		int mass = 0;

		if ( pSprite->type >= kDudeBase && pSprite->type < kDudeMax )
			mass = dudeInfo[pSprite->type - kDudeBase].mass;
		else if ( pSprite->type >= kThingBase && pSprite->type < kThingMax )
			mass = thingInfo[pSprite->type - kThingBase].mass;
		else
			ThrowError("Unexpected type encountered in ConcussSprite()", ES_ERROR);

		dassert(mass != 0);

		int impulse = muldiv(force, area, qabs(mass));
		dx = mulscale16(impulse, dx);
		dy = mulscale16(impulse, dy);
		dz = mulscale16(impulse, dz);

		pSprite->xvel += dx;
		pSprite->yvel += dy;
		pSprite->zvel += dz;
	}

	actDamageSprite(nSource, nSprite, kDamageExplode, force);
}


/*******************************************************************************
	FUNCTION:		ReflectVector()

	DESCRIPTION:	Reflects a vector off a wall

	PARAMETERS:     nFraction is elasticity (0x10000 == perfectly elastic)
*******************************************************************************/
static void ReflectVector( short *dx, short *dy, int nWall, int nFraction )
{
	// calculate normal for wall
	int nx = -(wall[wall[nWall].point2].y - wall[nWall].y) >> 4;
	int ny = (wall[wall[nWall].point2].x - wall[nWall].x) >> 4;
	int dotProduct = *dx * nx + *dy * ny;

	int length2 = nx * nx + ny * ny;
	dassert(length2 > 0);

	int dot2 = dotProduct + mulscale16r(dotProduct, nFraction);
	int sd = divscale16(dot2, length2);

	*dx -= mulscale16r(nx, sd);
	*dy -= mulscale16r(ny, sd);
}


static void DropPickupObject( int nActor, int nObject )
{
	dassert( nActor >= 0 && nActor < kMaxSprites && sprite[nActor].statnum < kMaxStatus );
	dassert( (nObject >= kItemBase && nObject < kItemMax)
		|| (nObject >= kAmmoItemBase && nObject < kAmmoItemMax)
		|| (nObject >= kWeaponItemBase && nObject < kWeaponItemMax) );

	// create a sprite for the dropped ammo
	SPRITE *pActor = &sprite[nActor];

	int nSprite = actSpawnSprite( pActor->sectnum, pActor->x, pActor->y, sector[pActor->sectnum].floorz, kStatItem, FALSE );
	SPRITE *pSprite = &sprite[ nSprite ];

	if ( nObject >= kItemBase && nObject < kItemMax )
	{
		int nItemIndex = nObject - kItemBase;

		pSprite->type = (short)nObject;
		pSprite->picnum = gItemData[nItemIndex].picnum;
		pSprite->shade = gItemData[nItemIndex].shade ;
		pSprite->xrepeat = gItemData[nItemIndex].xrepeat;
		pSprite->yrepeat = gItemData[nItemIndex].yrepeat;
		if (nObject >= kItemKey1 && nObject <= kItemKey7)
		{
			// PF/NN: should this be in bloodbath too?
			if ( gNetMode == kNetModeCoop )	// force permanent keys in Coop mode
			{
				dbInsertXSprite( nSprite );
				XSPRITE *pXSprite = &xsprite[ pSprite->extra ];
				pXSprite->respawn = kRespawnPermanent;
				pXSprite->respawnTime = 0;
			}
		}
	}
	else if ( nObject >= kAmmoItemBase && nObject < kAmmoItemMax )
	{
		int nAmmoIndex = nObject - kAmmoItemBase;

		pSprite->type = (short)nObject;
		pSprite->picnum = gAmmoItemData[nAmmoIndex].picnum;
		pSprite->shade = gAmmoItemData[nAmmoIndex].shade ;
		pSprite->xrepeat = gAmmoItemData[nAmmoIndex].xrepeat;
		pSprite->yrepeat = gAmmoItemData[nAmmoIndex].yrepeat;
	}
	else if ( nObject >= kWeaponItemBase && nObject < kWeaponItemMax )
	{
		int nWeaponIndex = nObject - kWeaponItemBase;

		pSprite->type = (short)nObject;
		pSprite->picnum = gWeaponItemData[nWeaponIndex].picnum;
		pSprite->shade = gWeaponItemData[nWeaponIndex].shade ;
		pSprite->xrepeat = gWeaponItemData[nWeaponIndex].xrepeat;
		pSprite->yrepeat = gWeaponItemData[nWeaponIndex].yrepeat;
	}
	else
		ThrowError("Unhandled nObject passed to DropPickupObject()", ES_ERROR);
}


BOOL actHealDude( XSPRITE *pXDude, int healValue, int maxHealthClip)
{
	dassert(pXDude != NULL);

	healValue <<= 4;	// fix this later in the calling code
	maxHealthClip <<= 4;
	if ( pXDude->health < maxHealthClip )
	{
		pXDude->health = ClipHigh(pXDude->health + healValue, maxHealthClip);
		dprintf("Health=%d\n", pXDude->health >> 4);
		return TRUE;
	}
	return FALSE;
}


void actKillSprite( int nSource, int nSprite, DAMAGE_TYPE damageType )
{
	dassert(nSprite >= 0 && nSprite < kMaxSprites);
	SPRITE *pSprite = &sprite[nSprite];
	SPRITE *pSource = &sprite[nSource];

	dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	int dudeIndex = pSprite->type - kDudeBase;
	DUDEINFO *pDudeInfo = &dudeInfo[dudeIndex];

	int nXSprite = pSprite->extra;
	dassert(nXSprite > 0);
	XSPRITE *pXSprite = &xsprite[pSprite->extra];
	pXSprite->moveState = kMoveStill;

	// handle first cerberus head death
	if (pSprite->type == kDudeCerberus)
	{
		seqSpawn(dudeInfo[dudeIndex].seqStartID + kSeqDudeDeath1, SS_SPRITE, nXSprite);
		return;
	}

	actPostSprite( nSprite, kStatThing );
	trTriggerSprite( nSprite, pXSprite, kCommandOff ); // trigger death message

	pSprite->flags |= kAttrMove | kAttrGravity | kAttrFalling;

	if ( IsPlayerSprite(nSprite) )
	{
		PLAYER *pPlayer = &gPlayer[pSprite->type - kDudePlayer1];
		powerupClear( pPlayer );
		dprintf("health = %i\n",pXSprite->health);
		if (pXSprite->health == 0)
			pPlayer->deathTime = 0;

		if ( IsPlayerSprite(nSource) )
		{
			int nKilledIndex = pSprite->type - kDudePlayer1;
			int nKillerIndex = pSource->type - kDudePlayer1;
			PLAYER *pFragger = &gPlayer[nKillerIndex];
			if (nSource == nSprite) // fragged yourself, eh?
			{
				pPlayer->fragCount--;
				pPlayer->fragInfo[nKillerIndex]--;	// frags against self is negative
			}
			else
			{
				pFragger->fragCount++;
				pFragger->fragInfo[nKilledIndex]++;	// frags against others are positive
			}
		}

		// drop pickup items
		for (int i = 1; i < 8; i++)
			if ( pPlayer->hasKey[i] )
				DropPickupObject(nSprite, kItemKey1 + i - 1);

		if ( pPlayer->hasWeapon[kWeaponShotgun] )
			DropPickupObject(nSprite, kWeaponItemShotgun);

		if ( pPlayer->hasWeapon[kWeaponTommy] )
			DropPickupObject(nSprite, kWeaponItemTommyGun);

		if ( pPlayer->hasWeapon[kWeaponFlare] )
			DropPickupObject(nSprite, kWeaponItemFlareGun);

		if ( pPlayer->hasWeapon[kWeaponSpear] )
			DropPickupObject(nSprite, kWeaponItemSpearGun);

		if ( pPlayer->hasWeapon[kWeaponShadow] )
			DropPickupObject(nSprite, kWeaponItemShadowGun);
	}

	if ( pXSprite->key > 0 )
		DropPickupObject( nSprite, kItemKey1 + pXSprite->key - 1 );

	int deathType;
	switch (damageType)
	{
		case kDamageFall:
		case kDamageExplode:
			deathType = kSeqDudeDeath2;
//			sfxStart3DSound(nXSprite, kSfxThingSplat);
			break;

		case kDamageBurn:
			deathType = kSeqDudeDeath3;
//			sfxStart3DSound(nXSprite, kSfxThingBurn);
			break;

		default:
			deathType = kSeqDudeDeath1;
			break;
	}

	// are we missing this sequence?  if so, just delete it
	if ( gSysRes.Lookup( dudeInfo[dudeIndex].seqStartID + deathType, ".SEQ") == NULL )
	{
		dprintf("sprite missing death sequence: deleted\n");
		seqKill(SS_SPRITE, nXSprite);	// make sure we remove any active sequence
		actPostSprite( nSprite, kStatFree );
		return;
	}

	switch (pSprite->type)
	{
		case kDudeAxeZombie:
			if (pSprite->owner >= 0)
			{
				PLAYER *pPlayer = &gPlayer[sprite[pSprite->owner].type - kDudePlayer1];
				playerDeleteLackey( pPlayer, nSprite );
			}
			seqSpawn(dudeInfo[dudeIndex].seqStartID + deathType, SS_SPRITE, nXSprite);
			break;

		case kDudeFatZombie:
//			if ( damageType == kDamageBurn )
//			{
//				int thingIndex = kThingZombieBones - kThingBase;
//
//				pSprite->type = kThingZombieBones;
//				pSprite->clipdist = thingInfo[thingIndex].clipdist;
//				pSprite->flags = thingInfo[thingIndex].flags;
//				pSprite->xvel = pSprite->yvel = pSprite->zvel = 0;
//				pXSprite->health = thingInfo[thingIndex].startHealth << 4;
//			}
			seqSpawn(dudeInfo[dudeIndex].seqStartID + deathType, SS_SPRITE, nXSprite);
			break;

		default:
			dprintf("spawning sprite %i death sequence %x\n", nSprite, dudeInfo[dudeIndex].seqStartID + deathType);
			seqSpawn(dudeInfo[dudeIndex].seqStartID + deathType, SS_SPRITE, nXSprite);
			break;
	}

	// drop any items or weapons
	if (pSprite->type == kDudeTommyCultist)
	{
		int nDropCheck = Random(100);

		// constants?  table?
		if (nDropCheck < 80)
			DropPickupObject( nSprite, kAmmoItemBullets );
		else if (nDropCheck < 95)
			DropPickupObject( nSprite, kAmmoItemBulletBox );
		else
			DropPickupObject( nSprite, kWeaponItemTommyGun );
	}
	else if (pSprite->type == kDudeShotgunCultist)
	{
		int nDropCheck = Random(100);
		if (nDropCheck < 40)
			DropPickupObject( nSprite, kAmmoItemShells );
		else if (nDropCheck < 75)
			DropPickupObject( nSprite, kAmmoItemShellBox );
		else
			DropPickupObject( nSprite, kWeaponItemShotgun );
	}

//		SpawnGibs( nSprite, pXSprite );
	// gib generator
	if ( damageType == kDamageExplode )
	{
		int angle, velocity = 120;

		// thing gibs
		for (int i = 0; i < kGibMax && pDudeInfo->gib[i].chance > 0; i++)
		{
			if ( Random(256) < pDudeInfo->gib[i].chance )
			{
				int nGib = actCloneSprite(pSprite);
				changespritestat( (short)nGib, kStatThing );
				SPRITE *pGib = &sprite[nGib];

				angle = Random(kAngle360);
				pGib->type = kThingGibSmall;
				pGib->picnum = pDudeInfo->gib[i].tile;
				pGib->xvel += mulscale30(velocity, Cos(angle));
				pGib->yvel += mulscale30(velocity, Sin(angle));
				pGib->zvel -= 128;	// toss it in the air a bit
				pGib->cstat &= ~kSpriteBlocking & ~kSpriteHitscan;
				pGib->flags = kAttrMove | kAttrGravity | kAttrFalling;
				pGib->pal = kPLUNormal;
			}
		}

		// debris gibs
		for (i = 0; i < 50; i++)
		{
			int nGib = actCloneSprite(pSprite);
			changespritestat( (short)nGib, kStatDebris);
			SPRITE *pGib = &sprite[nGib];

			pGib->picnum = 2053;	// "worm" giblet
			pGib->xvel += BiRandom(500);
			pGib->yvel += BiRandom(500);
			pGib->zvel += BiRandom(500);
			pGib->cstat &= ~kSpriteBlocking & ~kSpriteHitscan;
			pGib->flags = kAttrMove | kAttrGravity | kAttrFalling;
			pGib->pal = kPLUNormal;
		}

	}
}


void actDamageSprite( int nSource, int nSprite, DAMAGE_TYPE nDamageType, int nDamage )
{
	dassert(nSprite >= 0 && nSprite < kMaxSprites);
	SPRITE *pSprite = &sprite[nSprite];

	int nXSprite = pSprite->extra;
	if (nXSprite < 0)
		return;

	dassert(nXSprite > 0 && nXSprite < kMaxXSprites);
	XSPRITE *pXSprite = &xsprite[nXSprite];

	if (pXSprite->health == 0)	// it's already toast
		return;

	switch ( pSprite->statnum )
	{
		case kStatDude:
		{
			dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);

			// calculate and apply damage to the dude or player sprite
			int nShift = dudeInfo[pSprite->type - kDudeBase].damageShift[nDamageType];
			nDamage = (nShift >= kNoDamage) ? 0 : nDamage >> nShift;
			pXSprite->health = ClipLow(pXSprite->health - nDamage, 0);

			// process results and effects of damage
			if ( IsPlayerSprite(pSprite) )
				playerDamageSprite( &gPlayer[pSprite->type - kDudePlayer1], nSource, nDamage );
			else
				aiDamageSprite( pSprite, pXSprite, nSource, nDamageType, nDamage );

			// kill dudes lacking health to sustain life
			if (pXSprite->health == 0)
			{
				// prevent dudes from exploding from weak explosion damage
				if ( nDamageType == kDamageExplode && nDamage < (5 << 4) )
					nDamageType = kDamagePummel;
				if ( IsPlayerSprite(pSprite) )
					sfxStart3DSound(nXSprite, kSfxPlayDie);
				actKillSprite(nSource, nSprite, nDamageType);
			}
			break;
		}

		case kStatThing:
		case kStatProximity:
		{
			dassert(pSprite->type >= kThingBase && pSprite->type < kThingMax);
			int thingIndex = pSprite->type - kThingBase;
			nDamage >>= thingInfo[thingIndex].damageShift[nDamageType];
			pXSprite->health = ClipLow(pXSprite->health - nDamage, 0);
			if (pXSprite->health == 0)
			{
				pSprite->owner = (short)nSource;
				trTriggerSprite(nSprite, pXSprite, kCommandOff);

				switch ( pSprite->type )
				{
					case kThingMachineGun:
						seqSpawn(kSeqMGunDead, SS_SPRITE, pSprite->extra);
						sfxStart3DSound(nXSprite, kSfxMGDie);
						break;
					case kThingBlueVase:
						seqSpawn(kSeqVase1, SS_SPRITE, pSprite->extra);
						sfxStart3DSound(nXSprite, kSfxPotteryHit);
						if (pXSprite->data1 > 0)
							DropPickupObject( nSprite, pXSprite->data1 );
						if (pXSprite->data2 > 0)
							DropPickupObject( nSprite, pXSprite->data2 );
						break;
					case kThingBrownVase:
						seqSpawn(kSeqVase2, SS_SPRITE, pSprite->extra);
						sfxStart3DSound(nXSprite, kSfxPotteryHit);
						if (pXSprite->data1 > 0)
							DropPickupObject( nSprite, pXSprite->data1 );
						if (pXSprite->data2 > 0)
							DropPickupObject( nSprite, pXSprite->data2 );
						break;
					case kThingClearGlass:
						pSprite->yrepeat >>= 1;
						seqSpawn(kSeqClearGlass, SS_SPRITE, pSprite->extra);
						sfxStart3DSound(nXSprite, kSfxGlassHit);
						break;
					case kThingFluorescent:
						seqSpawn(kSeqFluorescentLight, SS_SPRITE, pSprite->extra);
						break;
					case kThingWoodBeam:
						seqSpawn(kSeqBeam, SS_SPRITE, pSprite->extra);
						break;
					case kThingWeb:
						seqSpawn(kSeqWeb, SS_SPRITE, pSprite->extra);
						break;
					case kThingMetalGrate1:
						seqSpawn(kSeqMetalGrate1, SS_SPRITE, pSprite->extra);
						break;
				 	case kThingFlammableTree:
						if (pXSprite->data1 == 0)
							seqSpawn(kSeqBurningTree1, SS_SPRITE, pSprite->extra);
						else if (pXSprite->data1 == 1)
							seqSpawn(kSeqBurningTree2, SS_SPRITE, pSprite->extra);
						sfxStart3DSound(nXSprite, kSfxBurn);
						break;
					case kThingZombieBones:
						dprintf("damaging zombie bones\n");
						if ( seqGetStatus(SS_SPRITE, nXSprite) < 0 )	// body finished burning
							seqSpawn(kSeqZombieBones, SS_SPRITE, pSprite->extra);
						break;
				}
			}
			break;
		}
	}
}


void actImpactMissile( int nSprite, int hitInfo )
{
	SPRITE *pMissile = &sprite[nSprite];
	int hitType = hitInfo & kHitTypeMask;
	int hitObject = hitInfo & kHitIndexMask;

	int nXMissile = pMissile->extra;

	switch (pMissile->type)
	{
		case kMissileFireball:
			actExplodeSprite( nSprite );
			break;

		case kMissileFlare:
		case kMissileExplodingFlare:
		case kMissileStarburstFlare:
		{
			XSPRITE *pXMissile = &xsprite[pMissile->extra];

			if (pMissile->type == kMissileExplodingFlare || pMissile->type == kMissileStarburstFlare)
			{
				actExplodeSprite( nSprite );
				break;
			}

			if ( hitType == kHitSprite && sprite[hitObject].extra > 0 )
			{
				SPRITE *pObject = &sprite[hitObject];
				XSPRITE *pXObject = &xsprite[pObject->extra];

				pMissile->picnum = kAnmFlareBurn;
				pXMissile->target = hitObject;
				pXMissile->targetZ = pMissile->z - pObject->z;
				pXMissile->goalAng = getangle( pMissile->x - pObject->x, pMissile->y - pObject->y ) - pObject->ang;
				pXMissile->state = 1;
				actAddBurnTime(pMissile->owner, pXObject, 8 * kTimerRate);
				actDamageSprite(pMissile->owner, hitObject, kDamageStab, 10 << 4);
				actPostSprite( nSprite, kStatFlare );
			 	evPost(nSprite, SS_SPRITE, 8 * kTimerRate);	// callback to flare
			}
			else
			{
				// ADD: spawn an explosion later
				actPostSprite( nSprite, kStatFree );
			}
			break;
		}

		case kMissileSprayFlame:
		case kMissileHoundFire:
//			seqKill(SS_SPRITE, nXMissile);
//			deletesprite((short)nSprite);
			if ( hitType == kHitSprite && sprite[hitObject].extra > 0 )
			{
				XSPRITE *pXObject = &xsprite[sprite[hitObject].extra];
				actAddBurnTime( pMissile->owner, pXObject, kFrameTicks );
			}
			break;

		case kMissileEctoSkull:
			actPostSprite( nSprite, kStatEffect );
			seqSpawn(kSeqSkullExplode, SS_SPRITE, pMissile->extra);
			if ( hitType == kHitSprite && sprite[hitObject].statnum == kStatDude )
			{
				actDamageSprite(pMissile->owner, hitObject, kDamageSpirit, 50 << 4);
				SPRITE *pDude = &sprite[pMissile->owner];
				XSPRITE *pXDude = &xsprite[pDude->extra];
				if ( pXDude->health > 0 )
					actHealDude(pXDude, 25);
			}
			break;

		case kMissileButcherKnife:
			actPostSprite( nSprite, kStatEffect );
			pMissile->cstat &= ~kSpriteWall;
			pMissile->type = kNothing;
			seqSpawn(kSeqSkullExplode, SS_SPRITE, pMissile->extra);
			if ( hitType == kHitSprite && sprite[hitObject].statnum == kStatDude )
			{
				actDamageSprite(pMissile->owner, hitObject, kDamageSpirit, 15 << 4);
				SPRITE *pDude = &sprite[pMissile->owner];
				XSPRITE *pXDude = &xsprite[pDude->extra];

				int dudeIndex = pDude->type - kDudeBase;
				if ( pXDude->health > 0 )
					actHealDude(pXDude, 10, dudeInfo[dudeIndex].startHealth);
			}
			break;

		default:
			seqKill(SS_SPRITE, nXMissile);
			actPostSprite( nSprite, kStatFree );
			if (hitType == kHitSprite)
				actDamageSprite(pMissile->owner, hitObject, kDamagePummel, 5 << 4);
			break;
	}
}


void ProcessTouchObjects( int nSprite, int nXSprite )
{
	SPRITE *pSprite = &sprite[nSprite];
	XSPRITE *pXSprite = &xsprite[nXSprite];
	SPRITEHIT *pSpriteHit = &gSpriteHit[nXSprite];

	int nHitObject = pSpriteHit->ceilHit & kHitIndexMask;
	switch( pSpriteHit->ceilHit & kHitTypeMask )
	{
		case kHitSprite:
			if ( sprite[nHitObject].extra > 0 )
			{
				SPRITE *pHit = &sprite[nHitObject];
				XSPRITE *pXHit = &xsprite[pHit->extra];
				switch ( pHit->type )
				{
					case kTrapSawBlade:
						if ( pXHit->state )
						{
							pXHit->data1 = 1;
							pXHit->data2 = ClipHigh(pXHit->data2 + 2 * kFrameTicks, kTimerRate * 5);
							actDamageSprite(nSprite, nSprite, kDamageStab, 4);
						}
						else
							actDamageSprite(nSprite, nSprite, kDamageStab, 1);
						break;
				}
			}
			break;

		case kHitWall:
		case kHitSector:
			break;
	}

	nHitObject = pSpriteHit->moveHit & kHitIndexMask;
	switch( pSpriteHit->moveHit & kHitTypeMask )
	{
		case kHitSprite:
			break;

		case kHitWall:
		case kHitSector:
			break;
	}

	nHitObject = pSpriteHit->floorHit & kHitIndexMask;
	switch( pSpriteHit->floorHit & kHitTypeMask )
	{
		case kHitSprite:
			if ( sprite[nHitObject].extra > 0 )
			{
				SPRITE *pHit = &sprite[nHitObject];
				XSPRITE *pXHit = &xsprite[pHit->extra];
				switch ( pHit->type )
				{
					case kTrapSawBlade:
						if ( pXHit->state )
						{
							pXHit->data1 = 1;
							pXHit->data2 = ClipHigh(pXHit->data2 + 2 * kFrameTicks, kTimerRate * 5);
							actDamageSprite(nSprite, nSprite, kDamageStab, 4);
						}
						else
							actDamageSprite(nSprite, nSprite, kDamageStab, 1);
						break;
				}
			}
			break;

		case kHitWall:
		case kHitSector:
			break;
	}
}


#define kMinZVel 		(6 << 4)
#define kAirDrag		0x0100


static int MoveThing( int nSprite, char cliptype )
{
	dassert(nSprite >= 0 && nSprite < kMaxSprites);
	SPRITE *pSprite = &sprite[nSprite];
	int nXSprite = pSprite->extra;
	XSPRITE *pXSprite = &xsprite[nXSprite];

	if ( !(pSprite->flags & kAttrFalling) && !pSprite->xvel && !pSprite->yvel && !pSprite->zvel )
		return 0;

	short nSector = pSprite->sectnum;
	dassert(nSector >= 0 && nSector < kMaxSectors);

	gSpriteHit[nXSprite].ceilHit = 0;
	gSpriteHit[nXSprite].floorHit = 0;
	gSpriteHit[nXSprite].moveHit = 0;

	int zTop, zBot;
	GetSpriteExtents(pSprite, &zTop, &zBot);

	if ( pSprite->xvel || pSprite->yvel )
	{
		short oldcstat = pSprite->cstat;
		pSprite->cstat &= ~kSpriteBlocking & ~kSpriteHitscan;

		gSpriteHit[nXSprite].moveHit = ClipMove(&pSprite->x, &pSprite->y, &pSprite->z, &nSector,
			pSprite->xvel * kFrameTicks >> 4, pSprite->yvel * kFrameTicks >> 4, pSprite->clipdist << 2,
				(pSprite->z - zTop) / 4, (zBot - pSprite->z) / 4, cliptype);

 		pSprite->cstat = oldcstat;

		if ((nSector != pSprite->sectnum) && (nSector >= 0))
			changespritesect((short)nSprite, nSector);

		switch (gSpriteHit[nXSprite].moveHit & kHitTypeMask)
		{
			case kHitWall:
			{
				int nWall = gSpriteHit[nXSprite].moveHit & kHitIndexMask;
				ReflectVector(&pSprite->xvel, &pSprite->yvel, nWall, 0x4000);
				pSprite->zvel = (short)mulscale16(pSprite->zvel, 0xC000);
				break;
			}

			// need to handle sprite collisions here....
		}
	}

	long ceilz, ceilhit, floorz, floorhit;
	GetZRange(pSprite, &ceilz, &ceilhit, &floorz, &floorhit, pSprite->clipdist << 2, cliptype);

	if ( pSprite->zvel )
	{
		pSprite->z += pSprite->zvel * kFrameTicks;
		pSprite->zvel -= mulscale16r(pSprite->zvel, kAirDrag);
	}

	if ( (pSprite->flags & kAttrGravity) && zBot < floorz )
	{
		pSprite->z += kGravity * kFrameTicks * kFrameTicks / 2;
		pSprite->zvel += kGravity * kFrameTicks;
		pSprite->flags |= kAttrFalling;
	}

	// check for warping in linked sectors
	int nUpper = gUpperLink[nSector], nLower = gLowerLink[nSector];
	if ( nUpper >= 0 && pSprite->z < sprite[nUpper].z )
	{
		nLower = sprite[nUpper].owner;
		changespritesect((short)nSprite, sprite[nLower].sectnum);
		pSprite->x += sprite[nLower].x - sprite[nUpper].x;
		pSprite->y += sprite[nLower].y - sprite[nUpper].y;
		pSprite->z += sprite[nLower].z - sprite[nUpper].z;
		viewBackupSpriteLoc(nSprite, pSprite);	// prevent interpolation
		GetZRange(pSprite, &ceilz, &ceilhit, &floorz, &floorhit, pSprite->clipdist << 2, cliptype);
	}
	else if ( nLower >= 0 && pSprite->z > sprite[nLower].z )
	{
		nUpper = sprite[nLower].owner;
		changespritesect((short)nSprite, sprite[nUpper].sectnum);
		pSprite->x += sprite[nUpper].x - sprite[nLower].x;
		pSprite->y += sprite[nUpper].y - sprite[nLower].y;
		pSprite->z += sprite[nUpper].z - sprite[nLower].z;
		viewBackupSpriteLoc(nSprite, pSprite);	// prevent interpolation
		GetZRange(pSprite, &ceilz, &ceilhit, &floorz, &floorhit, pSprite->clipdist << 2, cliptype);
	}

	GetSpriteExtents(pSprite, &zTop, &zBot);

	// hit floor?
	if ( zBot >= floorz )
	{
		gSpriteHit[nXSprite].floorHit = floorhit;
		pSprite->z += floorz - zBot;

		if ( pSprite->flags & kAttrFalling )
		{
			pSprite->xvel = (short)mulscale16(pSprite->xvel, 0xC000);
			pSprite->yvel = (short)mulscale16(pSprite->yvel, 0xC000);
			pSprite->zvel = (short)mulscale16(-pSprite->zvel, 0x4000);
			if ( qabs(pSprite->zvel) < kMinZVel)
			{
				pSprite->zvel = 0;
				pSprite->flags &= ~kAttrFalling;
			}
			return kHitFloor | nSector;
		}
		pSprite->zvel = 0;
	}

	// hit ceiling
	if ( zTop < ceilz && ((ceilhit & kHitTypeMask) != kHitSector || !(sector[nSector].ceilingstat & kSectorParallax)) )
	{
		gSpriteHit[nXSprite].ceilHit = ceilhit;
		pSprite->z += ClipLow(ceilz - zTop, 0);

		pSprite->xvel = (short)mulscale16(pSprite->xvel, 0xC000);
		pSprite->yvel = (short)mulscale16(pSprite->yvel, 0xC000);
		pSprite->zvel = (short)mulscale16(-pSprite->zvel, 0x4000);
		// return kHitCeiling | nSector;
	}

	// drag and friction
	if ( pSprite->xvel || pSprite->yvel )
	{
		// air drag
		pSprite->xvel -= mulscale16r(pSprite->xvel, kAirDrag);
		pSprite->yvel -= mulscale16r(pSprite->yvel, kAirDrag);

		if ( !(pSprite->flags & kAttrFalling) )
		{
			// sliding
			int vel = qdist(pSprite->xvel, pSprite->yvel);
			int nFrict = ClipHigh(kFrameTicks * kGroundFriction, vel);

			if ( (floorhit & kHitTypeMask) == kHitSprite )
			{
				int nUnderSprite = floorhit & kHitIndexMask;
				if ( (sprite[nUnderSprite].cstat & kSpriteRMask) == kSpriteFace )
				{
					// push it off the face sprite
					pSprite->xvel += mulscale(kFrameTicks, pSprite->x - sprite[nUnderSprite].x, 6);
					pSprite->yvel += mulscale(kFrameTicks, pSprite->y - sprite[nUnderSprite].y, 6);
					return gSpriteHit[nXSprite].moveHit;
				}
			}

			if (vel > 0)
			{
				nFrict = divscale16(nFrict, vel);
				pSprite->xvel -= mulscale16(nFrict, pSprite->xvel);
				pSprite->yvel -= mulscale16(nFrict, pSprite->yvel);
			}
		}

	}

	return gSpriteHit[nXSprite].moveHit;
}


static void MoveDebris( int nSprite )
{
	dassert(nSprite >= 0 && nSprite < kMaxSprites);
	SPRITE *pSprite = &sprite[nSprite];
	int nXSprite = pSprite->extra;

	short nSector = pSprite->sectnum;
	dassert(nSector >= 0 && nSector < kMaxSectors);

	if ( pSprite->xvel || pSprite->yvel )
	{
		pSprite->x += pSprite->xvel * kFrameTicks >> 4;
		pSprite->y += pSprite->yvel * kFrameTicks >> 4;

		// air drag
		pSprite->xvel -= mulscale16r(pSprite->xvel, kAirDrag);
		pSprite->yvel -= mulscale16r(pSprite->yvel, kAirDrag);
	}

	if ( pSprite->zvel )
	{
		pSprite->z += pSprite->zvel * kFrameTicks;
		pSprite->zvel -= mulscale16r(pSprite->zvel, kAirDrag);
	}

	if ( pSprite->flags & kAttrGravity )
	{
		pSprite->z += kGravity * kFrameTicks * kFrameTicks / 2;
		pSprite->zvel += kGravity * kFrameTicks;
	}

	if ( !FindSector(pSprite->x, pSprite->y, pSprite->z, &nSector) )
	{
		actPostSprite(nSprite, kStatFree);
		return;
	}

	if ( nSector != pSprite->sectnum )
		changespritesect((short)nSprite, nSector);
}



#define kScreamVel		1200	// zvel at which player screams
#define kGruntVel		700
#define kDudeDrag		0x2800
#define kMinDudeVel 	(2 << 4)

static void MoveDude( int nSprite )
{
	dassert(nSprite >= 0 && nSprite < kMaxSprites);
	SPRITE *pSprite = &sprite[nSprite];
	int nXSprite = pSprite->extra;
	XSPRITE *pXSprite = &xsprite[nXSprite];

	if ( !(pSprite->flags & kAttrFalling) && !pSprite->xvel && !pSprite->yvel && !pSprite->zvel )
		return;

	PLAYER *pPlayer = NULL;
	if ( IsPlayerSprite(pSprite) )
		pPlayer = &gPlayer[pSprite->type - kDudePlayer1];

	dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];

	int zTop, zBot;
	GetSpriteExtents(pSprite, &zTop, &zBot);
	int floorDist = (zBot - pSprite->z) / 4;
	int ceilDist = (pSprite->z - zTop) / 4;
	int clipDist = pSprite->clipdist << 2;
	short nSector = pSprite->sectnum;
	dassert(nSector >= 0 && nSector < kMaxSectors);

//	if ( !(pSprite->xvel || pSprite->yvel || pSprite->zvel) && fWasOnFloor )
//		return;

	gSpriteHit[nXSprite].ceilHit = 0;
	gSpriteHit[nXSprite].floorHit = 0;
	gSpriteHit[nXSprite].moveHit = 0;

	if ( pSprite->xvel || pSprite->yvel )
	{
		int oldX = pSprite->x;
		int oldY = pSprite->y;

		if ( pPlayer && gNoClip )
		{
			pSprite->x += pSprite->xvel * kFrameTicks >> 4;
			pSprite->y += pSprite->yvel * kFrameTicks >> 4;
			updatesector( pSprite->x, pSprite->y, &nSector);

			if (nSector == -1)
			{
				pSprite->x = oldX;
				pSprite->y = oldY;
				nSector = pSprite->sectnum;
			}
		}
		else
		{
			short oldcstat = pSprite->cstat;
			pSprite->cstat &= ~kSpriteBlocking & ~kSpriteHitscan;

			gSpriteHit[nXSprite].moveHit = ClipMove(
				&pSprite->x, &pSprite->y, &pSprite->z, &nSector,
				pSprite->xvel * kFrameTicks >> 4, pSprite->yvel * kFrameTicks >> 4, clipDist,
				ceilDist, floorDist, 0);

	 		pSprite->cstat = oldcstat;
		}

		if ( pPlayer )
		{
			if ( nSector != pSprite->sectnum && nSector >= 0 )
			{
				// process sector exit/enter triggers
				int nXSector;
				nXSector = sector[pSprite->sectnum].extra;
				if ( nXSector > 0 && xsector[nXSector].triggerExit )
					trTriggerSector(pSprite->sectnum, &xsector[nXSector], kCommandSectorExit);

				nXSector = sector[nSector].extra;
				if ( nXSector > 0 && xsector[nXSector].triggerEnter )
					trTriggerSector(nSector, &xsector[nXSector], kCommandSectorEnter);
			}
		}

		if ( nSector != pSprite->sectnum )
			changespritesect((short)nSprite, nSector);

		switch (gSpriteHit[nXSprite].moveHit & kHitTypeMask)
		{
			case kHitWall:
			{
				int nWall = gSpriteHit[nXSprite].moveHit & kHitIndexMask;
				WALL *pWall = &wall[nWall];

				// don't bounce off wall if not fully blocking
				if ( pWall->nextsector != -1 )
				{
					SECTOR *pSector = &sector[pWall->nextsector];
					if ( pSector->floorz > zTop || pSector->ceilingz < zBot )
					{
						ReflectVector(&pSprite->xvel, &pSprite->yvel, nWall, 0);
						break;
					}
				}

				ReflectVector(&pSprite->xvel, &pSprite->yvel, nWall, 0x4000);
				pSprite->zvel = (short)mulscale16(pSprite->zvel, 0xC000);
				break;
			}

			// need to handle sprite collisions here....
		}
	}

	long ceilz, ceilhit, floorz, floorhit;
	if ( pPlayer )
		clipDist += 16;	// increase clipdist to allow jumping onto ledges

	GetZRange(pSprite, &ceilz, &ceilhit, &floorz, &floorhit, clipDist, 0);

	if ( pSprite->zvel )
	{
		pSprite->z += pSprite->zvel * kFrameTicks;
		pSprite->zvel -= mulscale16r(pSprite->zvel, kAirDrag);
	}

	if ( pPlayer && pSprite->zvel > kScreamVel && !pPlayer->fScreamed )
	{
		pPlayer->fScreamed = TRUE;
		sfxStart3DSound(nXSprite, kSfxPlayFall);
	}

	if ( (pSprite->flags & kAttrGravity) && zBot < floorz )
	{
		pSprite->z += kGravity * kFrameTicks * kFrameTicks / 2;
		pSprite->zvel += kGravity * kFrameTicks;
		pSprite->flags |= kAttrFalling;
	}

	// check for warping in linked sectors
	int nUpper = gUpperLink[nSector], nLower = gLowerLink[nSector];
	if ( nUpper >= 0 && pSprite->z < sprite[nUpper].z )
	{
		nLower = sprite[nUpper].owner;
		changespritesect((short)nSprite, sprite[nLower].sectnum);
		pSprite->x += sprite[nLower].x - sprite[nUpper].x;
		pSprite->y += sprite[nLower].y - sprite[nUpper].y;
		pSprite->z += sprite[nLower].z - sprite[nUpper].z;
		viewBackupSpriteLoc(nSprite, pSprite);	// prevent interpolation
		GetZRange(pSprite, &ceilz, &ceilhit, &floorz, &floorhit, clipDist, 0);
	}
	else if ( nLower >= 0 && pSprite->z > sprite[nLower].z )
	{
		nUpper = sprite[nLower].owner;
		changespritesect((short)nSprite, sprite[nUpper].sectnum);
		pSprite->x += sprite[nUpper].x - sprite[nLower].x;
		pSprite->y += sprite[nUpper].y - sprite[nLower].y;
		pSprite->z += sprite[nUpper].z - sprite[nLower].z;
		viewBackupSpriteLoc(nSprite, pSprite);	// prevent interpolation
		GetZRange(pSprite, &ceilz, &ceilhit, &floorz, &floorhit, clipDist, 0);
	}

	GetSpriteExtents(pSprite, &zTop, &zBot);

	if ( pPlayer && zBot >= floorz)
	{
		long floorz2 = floorz, floorhit2 = floorhit;
		GetZRange(pSprite, &ceilz, &ceilhit, &floorz, &floorhit, pSprite->clipdist << 2, 0);
		if ( zBot <= floorz && pSprite->z - floorz2 < floorDist )
		{
			floorz = floorz2;
			floorhit2 = floorhit;
		}
	}

	// hit floor?
	if ( zBot >= floorz )
	{
		gSpriteHit[nXSprite].floorHit = floorhit;
		pSprite->z += floorz - zBot;

		if ( pSprite->flags & kAttrFalling )
		{
			// need some way of converting fall damge to a non-linear curve
			int fallDamage = mulscale16(pSprite->zvel, pSprite->zvel);
			fallDamage = mulscale(fallDamage, fallDamage, 1);
			if ( fallDamage > (20 << 4) )
				actDamageSprite(nSprite, nSprite, kDamageFall, fallDamage);

			if ( pPlayer && pXSprite->health > 0 )
			{
				pPlayer->fScreamed = FALSE;
				if ( pSprite->zvel > kGruntVel )
					sfxStart3DSound(nXSprite, kSfxPlayLand);
			}

			pSprite->xvel = (short)mulscale16(pSprite->xvel, 0xC000);
			pSprite->yvel = (short)mulscale16(pSprite->yvel, 0xC000);
			pSprite->zvel = (short)mulscale16(-pSprite->zvel, 0x2000);
			if ( pSprite->zvel > -kMinZVel )
			{
				pSprite->zvel = 0;
				pSprite->flags &= ~kAttrFalling;
			}

			int nSurfType = tileGetSurfType(floorhit);
			if ( nSurfType == kSurfWater )
			{
				actSpawnEffect(pSprite->sectnum, pSprite->x, pSprite->y, floorz, ET_Splash1);
			}
			return;
		}
		pSprite->zvel = 0;
	}

	// hit ceiling
	if ( zTop < ceilz && ((ceilhit & kHitTypeMask) != kHitSector || !(sector[nSector].ceilingstat & kSectorParallax)) )
	{
		pSprite->z += ClipLow(ceilz - zTop, 0);

		pSprite->xvel = (short)mulscale16(pSprite->xvel, 0xC000);
		pSprite->yvel = (short)mulscale16(pSprite->yvel, 0xC000);
		pSprite->zvel = (short)mulscale16(-pSprite->zvel, 0x2000);
	}

	if ( zTop <= ceilz )
		gSpriteHit[nXSprite].ceilHit = ceilhit;

	// drag and friction
	if ( pSprite->xvel || pSprite->yvel )
	{
		// air drag
		pSprite->xvel -= mulscale16(pSprite->xvel, kAirDrag);
		pSprite->yvel -= mulscale16(pSprite->yvel, kAirDrag);

		if ( !(pSprite->flags & kAttrFalling) )
		{
			if ( (floorhit & kHitTypeMask) == kHitSprite )
			{
				int nUnderSprite = floorhit & kHitIndexMask;
				if ( (sprite[nUnderSprite].cstat & kSpriteRMask) == kSpriteFace )
				{
					// push it off the face sprite
					pSprite->xvel += mulscale(kFrameTicks, pSprite->x - sprite[nUnderSprite].x, 6);
					pSprite->yvel += mulscale(kFrameTicks, pSprite->y - sprite[nUnderSprite].y, 6);
					return;
				}
			}

			// movement drag
			pSprite->xvel -= (sshort)mulscale16r(pSprite->xvel, kDudeDrag);
			pSprite->yvel -= (sshort)mulscale16r(pSprite->yvel, kDudeDrag);

			if ( qdist(pSprite->xvel, pSprite->yvel) < kMinDudeVel )
				pSprite->xvel = pSprite->yvel = 0;
		}
	}

	ProcessTouchObjects( nSprite, nXSprite );
}


// missiles are self-propelled and are unaffected by gravity
static int MoveMissile( int nSprite )
{
	dassert(nSprite >= 0 && nSprite < kMaxSprites);
	SPRITE *pSprite = &sprite[nSprite];

	return movesprite((short)nSprite, pSprite->xvel, pSprite->yvel, pSprite->zvel,
		4 << 8, 4 << 8, 1, kFrameTicks);
}


void actExplodeSprite( int nSprite )
{
	SPRITE *pSprite = &sprite[nSprite];
	int nXSprite = pSprite->extra;
	dassert(nXSprite > 0 && nXSprite < kMaxXSprites);

	// already exploding?
	if (pSprite->statnum == kStatExplosion)
		return;

	switch ( pSprite->type )
	{
		case kMissileExplodingFlare:
		case kMissileStarburstFlare:
				seqSpawn(kSeqExplodeC2T, SS_SPRITE, nXSprite);
			break;

		case kThingTNTStick:
			if ( gSpriteHit[nXSprite].floorHit == 0 )
				seqSpawn(kSeqExplodeC2S, SS_SPRITE, nXSprite);
			else
				seqSpawn(kSeqExplodeC1S, SS_SPRITE, nXSprite);
			break;

		case kThingTNTProxArmed:
		case kThingTNTRemArmed:
		case kThingTNTBundle:
//			if ( gSpriteHit[nXSprite].floorHit == 0 )
				seqSpawn(kSeqExplodeC2M, SS_SPRITE, nXSprite);
//			else
//				seqSpawn(kSeqExplodeC1M, SS_SPRITE, nXSprite);
			break;

		case kThingTNTBarrel:
		{
			// spawn an explosion effect
			int nEffect = actSpawnSprite( pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, kStatExplosion, TRUE );
			sprite[nEffect].owner = pSprite->owner;	// set owner for frag/targeting

			// place barrel on the respawn list or just delete it
			if ( actCheckRespawn( nSprite ) )
			{
				XSPRITE *pXSprite = &xsprite[nXSprite];
				pXSprite->state = 0;
				pXSprite->health = thingInfo[kThingTNTBarrel - kThingBase].startHealth << 4;
			}
			else
				actPostSprite( nSprite, kStatFree );

			// reset locals to point at the effect, not the barrel
			nSprite = nEffect;
			pSprite = &sprite[nEffect];
			nXSprite = pSprite->extra;
			seqSpawn(kSeqExplodeC2L, SS_SPRITE, nXSprite);
			break;
		}

		default:
			seqSpawn(kSeqExplodeC2M, SS_SPRITE, nXSprite);
			break;
	}
	pSprite->xvel = pSprite->yvel = pSprite->zvel = 0;
	actPostSprite( nSprite, kStatExplosion );
	pSprite->flags &= ~(kAttrMove | kAttrGravity);

	sfxCreate3DSound(pSprite->x, pSprite->y, pSprite->z, kSfxExplodeCS);
}


void actProcessSprites(void)
{
	int nSprite;
	int nDude, nNextDude;

	// process proximity triggered sprites
	for (nSprite = headspritestat[kStatProximity]; nSprite >= 0; nSprite = nextspritestat[nSprite])
	{
		SPRITE *pSprite = &sprite[nSprite];
		int nXSprite = pSprite->extra;
		dassert(nXSprite > 0 && nXSprite < kMaxXSprites);
		XSPRITE *pXSprite = &xsprite[nXSprite];

		for (nDude = headspritestat[kStatDude]; nDude >= 0; nDude = nNextDude)
		{
			nNextDude = nextspritestat[nDude];

			if ( CheckProximity(&sprite[nDude], pSprite->x, pSprite->y, pSprite->z, pSprite->sectnum, 64) )
				trTriggerSprite(nSprite, pXSprite, kCommandSpriteProximity);
		}
	}

	// process things for effects
	for (nSprite = headspritestat[kStatThing]; nSprite >= 0; nSprite = nextspritestat[nSprite])
	{
		SPRITE *pSprite = &sprite[nSprite];

		if ( pSprite->extra > 0 )
		{
			XSPRITE *pXSprite = &xsprite[pSprite->extra];
			if ( actGetBurnTime(pXSprite) > 0 )
			{
				pXSprite->burnTime = ClipLow(pXSprite->burnTime - kFrameTicks, 0);
				actDamageSprite( pXSprite->burnSource, nSprite, kDamageBurn, 4 * kFrameTicks );
			}
		}
 	}

	// process things for movement
	for (nSprite = headspritestat[kStatThing]; nSprite >= 0; nSprite = nextspritestat[nSprite])
	{
		SPRITE *pSprite = &sprite[nSprite];

		if ( pSprite->flags & (kAttrMove | kAttrGravity) )
		{
			viewBackupSpriteLoc(nSprite, pSprite);

			int hitInfo = MoveThing(nSprite, 1);
			if (hitInfo != 0)
			{
				int nXSprite = pSprite->extra;
				if (nXSprite > 0)
				{
					XSPRITE *pXSprite = &xsprite[nXSprite];
					if ( pXSprite->triggerProximity )
						trTriggerSprite(nSprite, pXSprite, kCommandOff);
					switch( pSprite->type )
					{
						case kThingWaterDrip:
						case kThingBloodDrip:
							MakeSplash(pSprite, pXSprite);
							break;

						case kThingBoneClub:
							seqSpawn(kSeqBoneBreak, SS_SPRITE, nXSprite);
							if ( (hitInfo & kHitTypeMask) == kHitSprite )
								actDamageSprite( pSprite->owner, hitInfo & kHitIndexMask, kDamagePummel, 12 );
							break;
					}
				}
			}
		}
 	}

	// process debris sprites
	for (nSprite = headspritestat[kStatDebris]; nSprite >= 0; nSprite = nextspritestat[nSprite])
	{
		SPRITE *pSprite = &sprite[nSprite];
		viewBackupSpriteLoc(nSprite, pSprite);
		MoveDebris(nSprite);
	}

	// process missile sprites
	for (nSprite = headspritestat[kStatMissile]; nSprite >= 0; nSprite = nextspritestat[nSprite])
	{
		SPRITE *pSprite = &sprite[nSprite];

		viewBackupSpriteLoc(nSprite, pSprite);

		int hitInfo = MoveMissile(nSprite);

		// process impacts
		if (hitInfo != 0)
			actImpactMissile( nSprite, hitInfo );
	}

	// process explosions
	for (nSprite = headspritestat[kStatExplosion]; nSprite >= 0; nSprite = nextspritestat[nSprite])
	{
		SPRITE *pSprite = &sprite[nSprite];
		int x = pSprite->x, y = pSprite->y, z = pSprite->z, nSector = pSprite->sectnum;
		int nAffected;
		int radius = tilesizx[pSprite->picnum] * pSprite->xrepeat >> 6;

		for (nAffected = headspritestat[kStatDude]; nAffected >= 0; nAffected = nextspritestat[nAffected])
		{
			if ( CheckProximity(&sprite[nAffected], x, y, z, nSector, radius) )
				ConcussSprite(pSprite->owner, nAffected, x, y, z, kFrameTicks);
		}

		for (nAffected = headspritestat[kStatThing]; nAffected >= 0; nAffected = nextspritestat[nAffected])
		{
			if ( CheckProximity(&sprite[nAffected], x, y, z, nSector, radius) )
				ConcussSprite(pSprite->owner, nAffected, x, y, z, kFrameTicks);
		}

		for (nAffected = headspritestat[kStatProximity]; nAffected >= 0; nAffected = nextspritestat[nAffected])
		{
			if ( CheckProximity(&sprite[nAffected], x, y, z, nSector, radius) )
				ConcussSprite(pSprite->owner, nAffected, x, y, z, kFrameTicks);
		}
	}

	// process traps for effects
	for (nSprite = headspritestat[kStatTraps]; nSprite >= 0; nSprite = nextspritestat[nSprite])
	{
		SPRITE *pSprite = &sprite[nSprite];

		if ( pSprite->extra > 0 )
		{
			XSPRITE *pXSprite = &xsprite[pSprite->extra];
			switch( pSprite->type )
			{
				case kTrapSawBlade:
					pXSprite->data2 = ClipLow(pXSprite->data2 - kFrameTicks, 0);
					break;
			}
		}
	}

	// process dudes for effects
	for (nSprite = headspritestat[kStatDude]; nSprite >= 0; nSprite = nextspritestat[nSprite])
	{
		SPRITE *pSprite = &sprite[nSprite];

		if ( pSprite->extra > 0 )
		{
			XSPRITE *pXSprite = &xsprite[pSprite->extra];
			if ( actGetBurnTime(pXSprite) > 0 )
			{
				pXSprite->burnTime = ClipLow(pXSprite->burnTime - kFrameTicks, 0);
				actDamageSprite( pXSprite->burnSource, nSprite, kDamageBurn, 2 * kFrameTicks );
			}
		}
	}

	// process dudes for movement
	for (nSprite = headspritestat[kStatDude]; nSprite >= 0; nSprite = nextspritestat[nSprite])
	{
		SPRITE *pSprite = &sprite[nSprite];
		int nSector = pSprite->sectnum;

		viewBackupSpriteLoc(nSprite, pSprite);

		// special sector processing
		if ( sector[nSector].extra > 0 )
		{
			int nXSector = sector[ nSector ]. extra;
			dassert(nXSector > 0 && nXSector < kMaxXSectors);
			dassert(xsector[nXSector].reference == nSector);

			XSECTOR *pXSector = &xsector[nXSector];
			if ( pXSector->panVel && (pXSector->panAlways || pXSector->busy))
			{
				int windDrag = 0x0200; // 16:16 fixed point fraction
				int panVel = pXSector->panVel;
				int panAngle = pXSector->panAngle;

	 			if ( !pXSector->panAlways && pXSector->busy )
	 				panVel = mulscale16(panVel, pXSector->busy);

				if (sector[nSector].floorstat & kSectorRelAlign)
				{
					panAngle += GetWallAngle(sector[nSector].wallptr) + kAngle90;
					panAngle &= kAngleMask;
				}

				if (pXSector->wind)
				{
			        int windX = mulscale30(panVel, Cos(panAngle)) - pSprite->xvel;
			        int windY = mulscale30(panVel, Sin(panAngle)) - pSprite->yvel;
				    pSprite->xvel += mulscale16(kFrameTicks * windX, windDrag);
				    pSprite->yvel += mulscale16(kFrameTicks * windY, windDrag);
				}
			}

			// handle water dragging
			if ( pXSector->depth > 0 )
			{
				if ( pSprite->z >= sector[nSector].floorz )
				{
					int pushDrag  = dragTable[pXSector->depth];
					int panVel   = 0;
					int panAngle = pXSector->panAngle;

					if (pXSector->panAlways || pXSector->state || pXSector->busy)
					{
						panVel = pXSector->panVel;
						if ( !pXSector->panAlways && pXSector->busy )
							panVel = mulscale16(panVel, pXSector->busy);
					}

					if (sector[nSector].floorstat & kSectorRelAlign)
					{
						panAngle += GetWallAngle(sector[nSector].wallptr) + kAngle90;
						panAngle &= kAngleMask;
					}
				    int pushX = mulscale30(panVel, Cos(panAngle)) - pSprite->xvel;
				    int pushY = mulscale30(panVel, Sin(panAngle)) - pSprite->yvel;
					int nDrag = ClipHigh(kFrameTicks * pushDrag, 0x10000);
					pSprite->xvel += mulscale16(pushX, nDrag);
					pSprite->yvel += mulscale16(pushY, nDrag);
				}
			} else if ( pXSector->underwater )	// handle underwater dragging
			{
				int pushDrag  = dragTable[pXSector->depth];
				int panVel   = 0;
				int panAngle = pXSector->panAngle;

				if (pXSector->panAlways || pXSector->state || pXSector->busy)
				{
					panVel = pXSector->panVel;
					if ( !pXSector->panAlways && pXSector->busy )
						panVel = mulscale16(panVel, pXSector->busy);
				}

				if (sector[nSector].floorstat & kSectorRelAlign)
				{
					panAngle += GetWallAngle(sector[nSector].wallptr) + kAngle90;
					panAngle &= kAngleMask;
				}
				int pushX = mulscale30(panVel, Cos(panAngle)) - pSprite->xvel;
				int pushY = mulscale30(panVel, Sin(panAngle)) - pSprite->yvel;
				pSprite->xvel += mulscale16(kFrameTicks * pushX, pushDrag);
				pSprite->yvel += mulscale16(kFrameTicks * pushY, pushDrag);
			}
		}

		MoveDude(nSprite);
 	}

	// process flares to keep them burning on dudes
	for (nSprite = headspritestat[kStatFlare]; nSprite >= 0; nSprite = nextspritestat[nSprite])
	{
		SPRITE *pSprite = &sprite[nSprite];
		XSPRITE *pXSprite = &xsprite[pSprite->extra];
		SPRITE *pTarget = &sprite[pXSprite->target];

		viewBackupSpriteLoc(nSprite, pSprite);

		int nAngle = pTarget->ang + pXSprite->goalAng;
		int x = pTarget->x + mulscale30r( Cos(nAngle), pTarget->clipdist << 1 );	// halfway into clipdist
		int y = pTarget->y + mulscale30r( Sin(nAngle), pTarget->clipdist << 1 );
		int z = pTarget->z + pXSprite->targetZ;

		setsprite((short)pXSprite->reference, x, y, z);
	}

	aiProcessDudes();
}


/***********************************************************************
 * actSpawnSprite()
 *
 * Spawns a new sprite at the specified world coordinates.
 **********************************************************************/
int actSpawnSprite( short nSector, int x, int y, int z, short nStatus, BOOL bAddXSprite )
{
	int nSprite = insertsprite( nSector, nStatus );
	if (nSprite >= 0)
		sprite[nSprite].extra = -1;
	else
	{
		dprintf("Out of sprites -- reclaiming sprite from purge list\n");
		nSprite = headspritestat[kStatPurge];
		dassert(nSprite >= 0);
		changespritesect((short)nSprite, nSector);
		changespritestat((short)nSprite, nStatus);
	}
	setsprite( (short)nSprite, x, y, z );

	SPRITE *pSprite = &sprite[nSprite];

	pSprite->type = 0;

	viewBackupSpriteLoc(nSprite, pSprite);

	// optionally create an xsprite
	if ( bAddXSprite && pSprite->extra == -1 )
	{
		int nXSprite = dbInsertXSprite(nSprite);
		gSpriteHit[nXSprite].floorHit = 0;
		gSpriteHit[nXSprite].ceilHit = 0;
	}

	return nSprite;
}


int actCloneSprite( SPRITE *pSourceSprite )
{
	int nSprite = insertsprite( pSourceSprite->sectnum, pSourceSprite->statnum);
	if ( nSprite < 0 )
	{
		dprintf("Out of sprites -- reclaiming sprite from purge list\n");
		nSprite = headspritestat[kStatPurge];
		dassert(nSprite >= 0);
		changespritesect((short)nSprite, pSourceSprite->sectnum);
		changespritestat((short)nSprite, pSourceSprite->statnum);
	}
	SPRITE *pSprite = &sprite[nSprite];
	*pSprite = *pSourceSprite;
	viewBackupSpriteLoc(nSprite, pSprite);

	// don't copy xsprite
	pSprite->extra = -1;
	return nSprite;
}


int actSpawnThing( short nSector, int x, int y, int z, int thingType )
{
	dassert( thingType >= kThingBase && thingType < kThingMax );

	int nThing = actSpawnSprite(nSector, x, y, z, kStatThing, TRUE );

	SPRITE *pThing = &sprite[nThing];
	pThing->type = (short)thingType;
	int thingIndex = thingType - kThingBase;

	int nXThing = pThing->extra;
	dassert(nXThing > 0 && nXThing < kMaxXSprites);
	XSPRITE *pXThing = &xsprite[nXThing];

	pThing->clipdist = thingInfo[thingIndex].clipdist;
	pThing->flags = thingInfo[thingIndex].flags;
	if ( pThing->flags & kAttrGravity )
		pThing->flags |= kAttrFalling;
	pThing->pal = 0;

	pXThing->health = thingInfo[thingIndex].startHealth << 4;

	SetBitString(show2dsprite, nThing);

	switch (thingType)
	{
		case kThingTNTStick:
			pThing->shade = -32;
			pThing->picnum = 2169;
			pThing->cstat |= kSpriteHitscan;
			pThing->xrepeat = pThing->yrepeat = 32;
			break;

		case kThingTNTBundle:
			pThing->shade = -32;
			pThing->picnum = 2172;
			pThing->cstat |= kSpriteHitscan;
			pThing->xrepeat = pThing->yrepeat = 32;
			break;

		case kThingTNTProxArmed:
			pThing->shade = -16;
			pThing->picnum = kAnmTNTProxArmed;
			pThing->cstat |= kSpriteHitscan;
			pThing->xrepeat = pThing->yrepeat = 32;
			break;

		case kThingTNTRemArmed:
			pThing->shade = -16;
			pThing->picnum = kAnmTNTRemArmed;
			pThing->cstat |= kSpriteHitscan;
			pThing->xrepeat = pThing->yrepeat = 32;
			break;

		case kThingBoneClub:
			pThing->shade = 0;
			pThing->picnum = kAnmBoneClub;
			pThing->cstat |= kSpriteHitscan;
			pThing->xrepeat = pThing->yrepeat = 32;
			break;

		case kThingWaterDrip:
			pThing->picnum = kAnmDrip;
			pThing->pal = kPLUCold;
			//pThing->cstat |= kSpriteTranslucent | kSpriteTranslucentR;
			pThing->shade = 0; // -12;
			break;

		case kThingBloodDrip:
			pThing->picnum = kAnmDrip;
			pThing->pal = kPLURed;
			//pThing->cstat |= kSpriteTranslucent | kSpriteTranslucentR;
			pThing->shade = 0; // -12;
			break;
	}

	return nThing;
}


int actFireThing( int nActor, int z, int nSlope, int thingType, int velocity )
{
	dassert( thingType >= kThingBase && thingType < kThingMax );

	SPRITE *pActor = &sprite[nActor];

	int nThing = actSpawnThing(pActor->sectnum,
		pActor->x + mulscale30(pActor->clipdist << 2, Cos(pActor->ang)),
		pActor->y + mulscale30(pActor->clipdist << 2, Sin(pActor->ang)),
		z, thingType);

	SPRITE *pThing = &sprite[nThing];

	pThing->owner = (short)nActor;
	pThing->ang = pActor->ang;
	pThing->xvel = (short)mulscale30(velocity, Cos(pActor->ang));
	pThing->yvel = (short)mulscale30(velocity, Sin(pActor->ang));
	pThing->zvel = (short)mulscale(velocity, nSlope, 14);
	pThing->xvel += pActor->xvel / 2;
	pThing->yvel += pActor->yvel / 2;
	pThing->zvel += pActor->zvel / 2;

	return nThing;
}


void actFireMissile( int nActor, int z, int dx, int dy, int dz, int missileType )
{
	dassert( missileType >= kMissileBase && missileType < kMissileMax );

	SPRITE *pActor = &sprite[nActor];

	int nSprite = actSpawnSprite(pActor->sectnum,
		pActor->x + mulscale30(pActor->clipdist << 1, Cos(pActor->ang)),
		pActor->y + mulscale30(pActor->clipdist << 1, Sin(pActor->ang)),
		z, kStatMissile, TRUE );

	SPRITE *pSprite = &sprite[nSprite];
	pSprite->type = (short)missileType;

	MissileType *pMissType = &missileInfo[ missileType - kMissileBase ];
	int velocity = pMissType->velocity;

	pSprite->shade = pMissType->shade;
	pSprite->pal = 0;
	pSprite->clipdist = 16;
	pSprite->flags = kAttrMove;
	pSprite->xrepeat = pMissType->xrepeat;
	pSprite->yrepeat = pMissType->yrepeat;
	pSprite->picnum = pMissType->picnum;
	pSprite->ang = (short)((pActor->ang +
		missileInfo[missileType - kMissileBase].angleOfs) & kAngleMask);
	pSprite->xvel = (short)mulscale(velocity, dx, 14);
	pSprite->yvel = (short)mulscale(velocity, dy, 14);
	pSprite->zvel = (short)mulscale(velocity, dz, 14);
	pSprite->owner = (short)nActor;

	SetBitString(show2dsprite, nSprite);

	int nXSprite = pSprite->extra;
	dassert(nXSprite > 0 && nXSprite < kMaxXSprites);
	XSPRITE *pXSprite = &xsprite[nXSprite];

	switch( missileType )
	{
		case kMissileButcherKnife:
			pSprite->cstat |= kSpriteWall;
			break;
		case kMissileSpear:
			pSprite->cstat |= kSpriteWall;
			break;
		case kMissileEctoSkull:
			seqSpawn(kSeqSkull, SS_SPRITE, nXSprite);
 			break;
		case kMissileFireball:
			seqSpawn(kSeqFireball, SS_SPRITE, nXSprite, &FireballCallback);
 			break;
		case kMissileHoundFire:
			seqSpawn(kSeqHoundFire, SS_SPRITE, nXSprite);
			pSprite->xvel += (pActor->xvel / 2) + BiRandom(50);
			pSprite->yvel += (pActor->yvel / 2) + BiRandom(50);
			pSprite->zvel += (pActor->zvel / 2) + BiRandom(50);
			break;
		case kMissileSprayFlame:
			if ( Chance(0x8000) )
				seqSpawn(kSeqSprayFlame1, SS_SPRITE, nXSprite);
			else
				seqSpawn(kSeqSprayFlame2, SS_SPRITE, nXSprite);
			pSprite->xvel += (pActor->xvel / 2) + BiRandom(50);
			pSprite->yvel += (pActor->yvel / 2) + BiRandom(50);
			pSprite->zvel += (pActor->zvel / 2) + BiRandom(50);
			break;
		case kMissileStarburstFlare:
		 	evPost(nSprite, SS_SPRITE, kTimerRate/6 );	// callback to flare
			break;
		case kMissileFlare:
		case kMissileExplodingFlare:
			//seqSpawn(kSeqFlare, SS_SPRITE, nXSprite, &FlareCallback);
			//pSprite->pal = kPLURed;
			break;
		default:
			break;
	}
}


/***********************************************************************
 * actSpawnEffect()
 *
 * Spawns an effect sprite at the specified world coordinates.
 **********************************************************************/
SPRITE *actSpawnEffect( short nSector, int x, int y, int z, int nEffect )
{
	int nSprite = actSpawnSprite(nSector, x, y, z, kStatEffect, TRUE);
	SPRITE *pSprite = &sprite[nSprite];
	int nXSprite = pSprite->extra;
	dassert(nXSprite > 0 && nXSprite < kMaxXSprites);

	switch( nEffect )
	{
		case ET_Splash1:   		// splash from dude landing in water
			seqSpawn(kSeqSplash1, SS_SPRITE, nXSprite);
			break;

		case ET_Splash2:   		// splash from bullet hitting water
			seqSpawn(kSeqSplash2, SS_SPRITE, nXSprite);
			break;

		case ET_Ricochet1:		// bullet ricochet off stone/metal
			if ( Chance(0x8000) )
				pSprite->cstat |= kSpriteFlipX;
			seqSpawn(kSeqRicochet1, SS_SPRITE, nXSprite);
			break;

		case ET_Ricochet2:      // bullet ricochet off wood/dirt/clay
			if ( Chance(0x8000) )
				pSprite->cstat |= kSpriteFlipX;
			seqSpawn(kSeqRicochet2, SS_SPRITE, nXSprite);
			break;

		case ET_Squib1:
			if ( Chance(0x8000) )
				pSprite->cstat |= kSpriteFlipX;
			seqSpawn(kSeqSquib1, SS_SPRITE, nXSprite);
			break;

		case ET_SmokeTrail:
			if ( Chance(0x8000) )
				pSprite->cstat |= kSpriteFlipX;
			if ( Chance(0x8000) )
				pSprite->cstat |= kSpriteFlipY;
			seqSpawn(kSeqFlareSmoke, SS_SPRITE, nXSprite);
			break;

		default:
			// don't have a sequence yet, so just delete it for now
			dprintf("actSpawnEffect: missing sequence\n");
			actPostSprite( nSprite, kStatFree );
			return NULL;
	}
	return pSprite;
}


BOOL actCheckRespawn( int nSprite )
{
	SPRITE *pSprite = &sprite[nSprite];

	if ( pSprite->extra > 0 )
	{
		XSPRITE *pXSprite = &xsprite[pSprite->extra];

		if (pXSprite->respawn == kRespawnPermanent)
			return TRUE;

		// if (sprite respawn is forced OR (sprite has optional respawning set
		// AND (sprite is a dude AND global dude respawning is set)
		// OR (sprite is a thing AND global thing respawning is set)))
		if (pXSprite->respawn == kRespawnAlways || (pXSprite->respawn == kRespawnOptional
		&& (pSprite->type >= kDudeBase && pSprite->type < kDudeMax && gRespawnEnemies)
		|| ((pSprite->type < kDudeBase || pSprite->type > kDudeMax) && gRespawnItems)))
		{
			if  (pXSprite->respawnTime > 0)
			{
				dprintf("Respawn statnum: %d\n",pSprite->statnum);
				pSprite->owner = pSprite->statnum; // store the sprite's status list for respawning
				actPostSprite( nSprite, kStatRespawn );
				pSprite->cstat &= ~kSpriteBlocking & ~kSpriteHitscan;
				pSprite->cstat |= kSpriteInvisible;
			 	evPost(nSprite, SS_SPRITE, pXSprite->respawnTime * kTimerRate / 10, kCommandRespawn);
				return TRUE;
			}
		}
	}
	return FALSE;	// indicate sprite will not respawn, and should be deleted, exploded, etc.
}


void actRespawnDude( int nSprite )
{
	SPRITE *pSprite = &sprite[nSprite];
	XSPRITE *pXSprite = &xsprite[pSprite->extra];

	// probably need more dude-specific initialization
	pXSprite->moveState = 0;
	pXSprite->aiState = 0;
	pXSprite->health = 0;
	pXSprite->target = 0;
	pXSprite->targetX = 0;
	pXSprite->targetY = 0;
	pXSprite->targetZ = 0;
	pXSprite->key = 0;	// respawned dudes don't drop additional permanent keys
}


void actRespawnSprite( int nSprite )
{
	SPRITE *pSprite = &sprite[nSprite];

	actPostSprite( nSprite, pSprite->owner );
	pSprite->owner = -1;
	pSprite->cstat &= ~kSpriteInvisible;

	if ( pSprite->extra > 0)
	{
		XSPRITE *pXSprite = &xsprite[pSprite->extra];
		pXSprite->isTriggered = 0;

		if (pSprite->type >= kDudeBase && pSprite->type < kDudeMax)
			actRespawnDude( nSprite );
	}

	// spawn the respawn special effect
	int nRespawn = actSpawnSprite( pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, kStatDefault, TRUE );
	seqSpawn(kSeqRespawn, SS_SPRITE, sprite[nRespawn].extra);
}


void actFireVector( int nActor, int z, int dx, int dy, int dz, VECTOR_TYPE vectorType )
{
	SPRITE *pActor = &sprite[nActor];
	HITINFO hitInfo;

	dassert(vectorType >= 0 && vectorType < kVectorMax);
	VECTORDATA *pVectorData = &gVectorData[vectorType];
	int maxDist = pVectorData->maxDist;

	int hitCode = VectorScan( pActor, z, dx, dy, dz, &hitInfo );

	// determine a point just a bit back from the intersection to place the ricochet
	int rx = hitInfo.hitx - mulscale(dx, 1 << 4, 14);
	int ry = hitInfo.hity - mulscale(dy, 1 << 4, 14);
	int rz = hitInfo.hitz - mulscale(dz, 1 << 4, 14);
	BYTE nSurf = kSurfNone;

	if ( maxDist == 0 || qdist(hitInfo.hitx - pActor->x, hitInfo.hity - pActor->y) < maxDist )
	{
		switch ( hitCode )
		{
			case SS_CEILING:
				if ( sector[hitInfo.hitsect].ceilingstat & kSectorParallax )
					nSurf = kSurfNone;
				else
					nSurf = surfType[sector[hitInfo.hitsect].ceilingpicnum];
				break;

			case SS_FLOOR:
				if ( sector[hitInfo.hitsect].floorstat & kSectorParallax )
					nSurf = kSurfNone;
				else
					nSurf = surfType[sector[hitInfo.hitsect].floorpicnum];
				break;

			case SS_WALL:
			{
				int nWall = hitInfo.hitwall;
				dassert( nWall >= 0 && nWall < kMaxWalls );

				nSurf = surfType[wall[nWall].picnum];
				WALL *pWall = &wall[nWall];

				int nXWall = wall[nWall].extra;
				if ( nXWall > 0 )
				{
					XWALL *pXWall = &xwall[nXWall];
					if ( pXWall->triggerImpact )
						trTriggerWall(nWall, pXWall, kCommandWallImpact);
				}
				break;
			}

			case SS_MASKED:
			{
				int nWall = hitInfo.hitwall;
				dassert( nWall >= 0 && nWall < kMaxWalls );

				nSurf = surfType[wall[nWall].overpicnum];
				WALL *pWall = &wall[nWall];

				int nXWall = wall[nWall].extra;
				if ( nXWall > 0 )
				{
					XWALL *pXWall = &xwall[nXWall];
					if ( pXWall->triggerImpact )
						trTriggerWall(nWall, pXWall, kCommandWallImpact);
				}
				break;
			}

			case SS_SPRITE:
			{
				int nSprite = hitInfo.hitsprite;
				nSurf = surfType[sprite[nSprite].picnum];

				dassert( nSprite >= 0 && nSprite < kMaxSprites);
				SPRITE *pSprite = &sprite[nSprite];

				// back off ricochet (squibs) even more
				rx -= mulscale(dx, 7 << 4, 14);
				ry -= mulscale(dy, 7 << 4, 14);
				rz -= mulscale(dz, 7 << 4, 14);

				actDamageSprite(nActor, nSprite, pVectorData->damageType,
					pVectorData->damageValue << 4);

				int nXSprite = pSprite->extra;
				if ( nXSprite > 0 )
				{
					XSPRITE *pXSprite = &xsprite[nXSprite];
					if ( pXSprite->triggerImpact )
						trTriggerSprite(nSprite, pXSprite, kCommandSpriteImpact);
				}
				break;
			}
		}
	}

	if ( pVectorData->impact[nSurf].nEffect >= 0 )
		actSpawnEffect(hitInfo.hitsect, rx, ry, rz, pVectorData->impact[nSurf].nEffect);

	if ( pVectorData->impact[nSurf].nSoundId >= 0 )
		sfxCreate3DSound(rx, ry, rz, pVectorData->impact[nSurf].nSoundId);
}


static void FireballCallback( int /* type */, int nXIndex )
{
	XSPRITE *pXSprite = &xsprite[nXIndex];
	int nSprite = pXSprite->reference;
	SPRITE *pSprite = &sprite[nSprite];

	SPRITE *pSmoke = actSpawnEffect( pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, ET_SmokeTrail );
	if (gDetail < kDetailLevelMax)
		pSmoke->cstat |= kSpriteInvisible;
}

//static void FlareCallback( int /* type */, int nXIndex )
//{
//	XSPRITE *pXSprite = &xsprite[nXIndex];
//	int nSprite = pXSprite->reference;
//	SPRITE *pSprite = &sprite[nSprite];
//
//	actSpawnEffect( pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z, ET_SmokeTrail );
//}

struct POSTPONE
{
	short nSprite;
	short nStatus;
};

int			gPostCount = 0;
POSTPONE	gPost[ kMaxSprites ];

/***********************************************************************
 * actPostSprite()
 *
 * Postpones deletion or status list change for a sprite.
 * An nStatus value of kStatFree passed to this function will
 * postpone deletion of the sprite until the gPostpone list is
 * next processed.
 **********************************************************************/
void actPostSprite( int nSprite, int nStatus )
{
	dassert( gPostCount < kMaxSprites );
	dassert( nSprite < kMaxSprites && sprite[nSprite].statnum < kMaxStatus );
	dassert( nStatus >= 0 && nStatus <= kStatFree );

	// see if it is already in the list (we may want to semaphore with an attr bit for speed)
	for (int n = 0; n < gPostCount; n++)
		if ( gPost[n].nSprite == nSprite )
			break;

	gPost[n].nSprite = (short)nSprite;
	gPost[n].nStatus = (short)nStatus;

	if ( n == gPostCount )
		gPostCount++;
}

/***********************************************************************
 * actPostProcess()
 *
 * Processes postponed sprite events to ensure that sprite list
 * processing functions normally when sprites are deleted or change
 * status.
 *
 **********************************************************************/
void actPostProcess( void )
{
	if (gPostCount)
	{
		for ( int i = 0; i < gPostCount; i++ )
		{
			POSTPONE *pPost = &gPost[i];

			if ( pPost->nStatus == kStatFree )
				deletesprite( pPost->nSprite );
			else
				changespritestat( pPost->nSprite, pPost->nStatus );
		}
		gPostCount = 0;
	}
}


static void MakeSplash( SPRITE *pSprite, XSPRITE *pXSprite )
{
	int nXSprite = pSprite->extra;
	int nSprite = pXSprite->reference;

	pSprite->flags &= ~kAttrGravity;			// no bouncing...
	pSprite->z -= 4 << 8;					// up one pixel
	viewBackupSpriteLoc(nSprite, pSprite);	// prevent interpolation

	int nSurfType = tileGetSurfType(gSpriteHit[nXSprite].floorHit);

	switch ( pSprite->type )
	{
		case kThingWaterDrip:
			switch (nSurfType)
			{
				case kSurfWater:
					seqSpawn(kSeqSplash1, SS_SPRITE, nXSprite);
					sfxCreate3DSound(pSprite->x, pSprite->y, pSprite->z,
						kSfxDrip3, 0x2000);
					// add a check for depth that uses either kSfxDrip2 (deep) or kSfxDrip3 (not deep)
					break;

				default:
					seqSpawn(kSeqSplash2, SS_SPRITE, nXSprite);
					sfxCreate3DSound(pSprite->x, pSprite->y, pSprite->z,
						kSfxDrip1, 0x2000);
					break;
			}
			break;

		case kThingBloodDrip:
			seqSpawn(kSeqSplash3, SS_SPRITE, nXSprite);
			sfxCreate3DSound(pSprite->x, pSprite->y, pSprite->z,
				kSfxDrip1, 0x2000);
			break;
	}
}
