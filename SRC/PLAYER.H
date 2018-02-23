#ifndef __PLAYER_H
#define __PLAYER_H

#include "engine.h"
#include "db.h"
#include "qav.h"
#include "globals.h"
#include "gameutil.h"
#include "actor.h"


// global weapon type indices (see also weapon item types in db.h)
enum
{
	kWeaponNone			= 0,
	kWeaponPitchfork	= 1,
	kWeaponSprayCan		= 2,
	kWeaponTNT			= 3,
	kWeaponShotgun		= 4,
	kWeaponTommy		= 5,
	kWeaponFlare		= 6,
	kWeaponVoodoo		= 7,
	kWeaponSpear		= 8,
	kWeaponShadow 		= 9,	// change to Ecto gun
	kWeaponBeast		= 10,	// remove when beast mode working
	kWeaponMax
};

// global ammo types (see all ammo item types in db.h)
enum
{
	kAmmoNone			= -1,
	kAmmoSprayCan		= 0,
	kAmmoTNTStick,
	kAmmoTNTProximity,
	kAmmoTNTRemote,
	kAmmoShell,
	kAmmoBullet,
	kAmmoBulletAP,
	kAmmoFlare,
	kAmmoFlareSB,
	kAmmoVoodoo,
	kAmmoSpear,
	kAmmoSpearXP,
	kAmmoMax,
};

// view effects
enum
{
	kVEWeaponFlash,
	kVEBlindness,
	kVEShock,
	kVEDamage,
	kVEQuake,
	kVEPickup,
	kVEDrowning,
	kVETilt,	// leaning
	kVEDelirium,
	kVEMax,
};

// look up/down constants
#define kHorizDefault   90

// distance in pixels for picking up items and pushing things
#define kTouchXYDist	48
#define kTouchZDist		32
#define kPushXYDist		64

// Life modes
#define kModeHuman		0
#define kModeBeast		1

#define	kMaxLackeys		4


/***********************************************************************
 * Global structures
 **********************************************************************/

union BUTTONFLAGS
{
	short			byte;
	struct
	{
		unsigned	jump : 1;
		unsigned	crouch : 1;
		unsigned	shoot : 1;
		unsigned	shoot2 : 1;
		unsigned	lookup : 1;
		unsigned	lookdown : 1;
	};
};

union KEYFLAGS
{
	short			byte;
	struct
	{
		unsigned	action : 1;
		unsigned	lookcenter : 1;
		unsigned	beast : 1;
		unsigned	pause : 1;
		unsigned	master : 1;
		unsigned	restart : 1;
	};
};

union SYNCFLAGS
{
	char			byte;
	struct
	{
		unsigned	buttonChange : 1;
		unsigned	keyChange : 1;
		unsigned	weaponChange : 1;
		unsigned	forwardChange : 1;
		unsigned	turnChange : 1;
		unsigned	strafeChange : 1;
	};
};

struct INPUT
{
	SYNCFLAGS 	syncFlags;
	BUTTONFLAGS	buttonFlags;
	KEYFLAGS 	keyFlags;
	uchar		newWeapon;

	schar 		forward;
	schar 		strafe;
	uchar		ticks;
	sshort 		turn;
};


struct PLAYER
{
	SPRITE		*sprite;
	int			nSprite;
	XSPRITE		*xsprite;
	int			lifemode;			// beast or human
	int			bloodlust;
	int			impactPE;
	int			impactPhase;
	int			standSpeed;
	int			compression;
	int			viewOffZ;
	int			weapOffZ;
	int			viewOffdZ;
	int			weapOffdZ;
	int			eyeAboveZ;
	int			weaponAboveZ;
	int			look;
	int			horiz;
	int			slope;
	BOOL		run;
	int			moveDist;
	int			bobAmp, bobPhase, bobHeight, bobWidth;
	int			swayAmp, swayPhase, swayHeight, swayWidth;
	ushort		hasKey[8];
	uchar		weapon;
	int			weaponTimer;
	int			weaponState;
	int			weaponAmmo;		// indicate current ammo type in use
	BOOL		hasWeapon[kWeaponMax];
	int			weaponMode[kWeaponMax];
	int			ammoCount[kAmmoMax];
	QAV			*pWeaponQAV;
	QAVCALLBACK	weaponCallback;
	BOOL		fLoopQAV;
	int			fuseTime;	// used for TNT callbacks
	int			throwTime;	// used for TNT callbacks
	VECTOR3D	aim;
	VECTOR3D	relAim;
	int			aimSprite;
	int			deathTime;
	int			powerUpTimer[kMaxPowerUps];
	int			fragCount;
	int			fragInfo[kMaxPlayers];
	int			teamID;		// team the player is allied with in team modes, or player index in coop and bloodbath modes
	int			fraggerID;	// sprite number of dude who killed player, -1 indicated no tracking
	int			airTime;	// set when first going underwater, decremented while underwater then takes damage
	int			bloodTime;	// set when player walks through blood, decremented when on normal surface
	int			gooTime;	// set when player walks through sewage, decremented when on normal surface
	int			wetTime;	// set when player walks through water, decremented when on normal surface
	int			bubbleTime;	// set when first going underwater, decremented while underwater then takes damage
	BOOL		godMode;
	int			nLackeySprites[kMaxLackeys];
	BOOL		fScreamed;
};

struct POSTURE
{
	int frontAccel;
	int sideAccel;
	int	backAccel;
	int frontSpeed[2];
	int sideSpeed[2];
	int backSpeed[2];
	int pace[2];
	int bobV;
	int bobH;
	int	swayV;
	int	swayH;
};

struct AMMOINFO
{
	int max;
	VECTOR_TYPE vectorType;
};


extern POSTURE gPosture[];
extern PLAYER gPlayer[kMaxPlayers], *gMe, *gView;
extern INPUT gPlayerInput[kMaxPlayers];
extern AMMOINFO gAmmoInfo[kAmmoMax];

int powerupCheck( PLAYER *pPlayer, int nPowerUp );
void powerupDraw( PLAYER *pView );
BOOL powerupActivate( PLAYER *pPlayer, int nPowerUp );
void powerupDeactivate( PLAYER *pPlayer, int nPowerUp );
void powerupProcess( PLAYER *pPlayer );
void powerupClear( PLAYER *pPlayer );
void powerupInit( void );

void playerSetRace( PLAYER *pPlayer, int nLifeMode, int nType );
void playerSetGodMode( PLAYER *pPlayer, BOOL nMode );
void playerReset( PLAYER *pPlayer, INPUT *pInput, int type );
void playerInit( int nPlayer );
void playerMove( PLAYER *pPlayer, INPUT *pInput );
void playerFireMissile( PLAYER *pPlayer, long dx, long dy, long dz, int missileType );
int playerFireThing( PLAYER *pPlayer, int relSlope, int thingType, int velocity );

void playerDamageSprite( PLAYER *pPlayer, int nSource, int nDamage );
BOOL playerAddLackey( PLAYER *pPlayer, int nLackey );
void playerDeleteLackey( PLAYER *pPlayer, int nLackey );

#endif //__PLAYER_H

