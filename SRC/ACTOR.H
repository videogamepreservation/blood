#ifndef __ACTOR_H
#define __ACTOR_H

#include "globals.h"
#include "qav.h"
#include "db.h"
#include "misc.h"

#define kGroundFriction	20

// Player movement types
enum {
	kMoveDead = 0,
	kMoveStill,
	kMoveWalk,
	kMoveFall,
	kMoveLand,
	kMoveStand,
	kMoveSwim,
	kMoveFly,
	kMoveHang,
};

// Damage types
enum DAMAGE_TYPE
{
	kDamagePummel = 0,	// punching/pummel/impact/crush
	kDamageFall,		// damage from falling (landing, actually)
	kDamageBurn,		// burning/heat/fire
	kDamageBullet,		// bullets or other piercing projectiles
	kDamageStab,		// stabbing, impaling, slicing, dicing
	kDamageExplode,		// explosion damage from concussive effect
	kDamageGas,			// damage from inhaling poison gas
	kDamageDrown,		// been underwater too long, eh?
	kDamageSpirit,		// ectoplasm or spirit damage
	kDamageVoodoo,		// hoodoo voodoo damage
	kDamageMax
};

#define kNoDamage	31	// no damage constant for damage shift table

enum VECTOR_TYPE
{
	kVectorNone			= -1,
	kVectorTine			= 0,
	kVectorShell,
	kVectorBullet,
	kVectorBulletAP,
	kVectorAxe,
	kVectorCleaver,
	kVectorClaw,
	kVectorHoundBite,
	kVectorRatBite,
	kVectorSpiderBite,
	kVectorMax,
};


#define kMaxBurnTime	(30 * kTimerRate)	// 30 seconds

/* effect types */
enum {
	ET_Splash1,   		// splash from dude landing in water
	ET_Splash2,   		// splash from bullet hitting water
	ET_Ricochet1, 		// bullet ricochet off stone/metal
	ET_Ricochet2, 		// bullet ricochet off wood
	ET_Squib1,			// bloody squib
	ET_SmokeTrail,		// smoke trail for fireball
};

struct SPRITEHIT
{
	int moveHit;
	int ceilHit;
	int floorHit;
};

extern SPRITEHIT gSpriteHit[ kMaxXSprites ];

struct ITEMDATA
{
	ushort cstat;
	sshort picnum;
	schar shade;
	uchar pal;
	uchar xrepeat, yrepeat;
	short statnum;
	short type;
	ushort flags;
};

struct AMMOITEMDATA
{
	ushort cstat;
	sshort picnum;
	schar shade;
	uchar pal;
	uchar xrepeat, yrepeat;
	ushort flags;
	short count;
	uchar ammoType;
	uchar weaponType;
};

struct WEAPONITEMDATA
{
	ushort cstat;
	sshort picnum;
	schar shade;
	uchar pal;
	uchar xrepeat, yrepeat;
	ushort flags;
	short weaponType;
	short ammoType;
	short count;
};

// ammo and power up constants
#define kMaxItemTypes	(kItemMax - kItemBase)
#define kMaxPowerUps	(kItemMax - kItemBase)

extern ITEMDATA gItemData[ kMaxItemTypes ];
extern AMMOITEMDATA gAmmoItemData[ kAmmoItemMax - kAmmoItemBase ];
extern WEAPONITEMDATA gWeaponItemData[ kWeaponItemMax - kWeaponItemBase ];


/***********************************************************************
 * Function Prototypes
 **********************************************************************/
void actInit( void );
void actAllocateSpares( void );
BOOL actHealDude(XSPRITE *pXSprite, int healValue, int maxHealthClip = 100);
void actDamageSprite( int nSource, int nSprite, DAMAGE_TYPE damageType, int damageValue );
void actProcessSprites( void );
int actSpawnSprite( short nSector, int x, int y, int z, short nStatus, BOOL bAddXSprite );
int actCloneSprite( SPRITE *pSprite );
SPRITE *actSpawnEffect( short nSector, int x, int y, int z, int nEffect );
void actExplodeSprite( int nSprite );

BOOL actCheckRespawn( int nSprite );
void actRespawnSprite( int nSprite );	// respawn dude or item

int actSpawnThing( short nSector, int x, int y, int z, int nThing );
int actFireThing( int nActor, int z, int nSlope, int thingType, int velocity );
void actFireMissile( int nActor, int z, int dx, int dy, int dz, int missileType );
void actFireVector( int nActor, int z, int dx, int dy, int dz, VECTOR_TYPE vectorType );

void actPostSprite( int nSprite, int nStatus );
void actPostProcess( void );


/***********************************************************************
 * Inline Functions
 **********************************************************************/
inline BOOL IsPlayerSprite( SPRITE *pSprite )
{
	return (pSprite->type >= kDudePlayer1 && pSprite->type <= kDudePlayer8) ? TRUE : FALSE;
}

inline BOOL IsPlayerSprite( int nSprite )
{
	return IsPlayerSprite(&sprite[nSprite]);
}

inline BOOL IsDudeSprite( SPRITE *pSprite )
{
	return (pSprite->type >= kDudeBase && pSprite->type < kDudeMax) ? TRUE : FALSE;
}

inline BOOL IsDudeSprite( int nSprite )
{
	return IsDudeSprite(&sprite[nSprite]);
}

inline void actAddBurnTime( int nBurnSource, XSPRITE *pXActor, int nBurnTime )
{
	pXActor->burnTime = ClipHigh(pXActor->burnTime + nBurnTime, kMaxBurnTime );
	pXActor->burnSource = (short)nBurnSource;
}


inline int actGetBurnTime( XSPRITE *pXActor )
{
	return pXActor->burnTime;
}

#endif //__ACTOR_H
