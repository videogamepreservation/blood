#include "weapon.h"
#include "globals.h"
#include "qav.h"
#include "debug4g.h"
#include "actor.h"
#include "db.h"
#include "actor.h"
#include "gameutil.h"
#include "trig.h"
#include "view.h"
#include "options.h"
#include "names.h"
#include "triggers.h"
#include "error.h"
#include "misc.h"
#include "screen.h"
#include "tile.h"
#include "ai.h"		// lackey support
#include "sound.h"
#include "sfx.h"

#define kShotgunBarrels		2
#define kVectorsPerBarrel	16
#define kMaxShotgunVectors	(kShotgunBarrels * kVectorsPerBarrel)

enum {
	kVoodooStabChest	= 1,
	kVoodooStabShoulder,
	kVoodooStabEye,
	kVoodooStabGroin,
	kVoodooStabSelf,
};

// Weapon States
enum {
	kFindWeapon			= -1,	// flag to find a loaded weapon

	kForkIdle			= 0,

	kBicOpen			= 0,
	kBicIdle,
	kBicSwitch,
	kBicNext,

	kCanIdle			= kBicNext,
	kCanFire,

	kTNTStickIdle		= kBicNext,
	kTNTStickFuse1,
	kTNTStickFuse2,
	kTNTStickThrow,
	kTNTBundleIdle,
	kTNTBundleFuse1,
	kTNTBundleFuse2,
	kTNTBundleThrow,
	kTNTProxIdle,
	kTNTProxEmpty,
	kTNTProxThrow,
	kTNTRemoteIdle1,			// bundle and detonator
	kTNTRemoteIdle2,			// detonator only
	kTNTRemoteEmpty,
	kTNTRemoteThrow,

	kShotInitialize		= 0,	// indicates first-time reload required
	kShotIdle0,
	kShotIdle1,
 	kShotIdle2,
	kShotFire1,
	kShotFire2,
	kShotLower,

	kFlareInitialize	= 0,
	kFlareRaise,
	kFlareIdle,
	kFlareFire,
	kFlareReload,
	kFlareLower,

	kTommyIdle			= 0,

	kSpearInitialize	= 0,
	kSpearRaise,
	kSpearIdle1,
	kSpearIdle2,
	kSpearFire,
	kSpearReload,
	kSpearLower,

	kShadowInitialize	= 0,
	kShadowRaise,
	kShadowIdle,
	kShadowFire,
	kShadowLower,

	kBeastInitialize	= 0,
	kBeastRaise,
	kBeastIdle,
	kBeastAttack,
	kBeastLower,

	kDollInitialize	= 0,
	kDollRaise,
	kDollIdle,
	kDollStab1,
	kDollStab2,
	kDollStab3,
	kDollStab4,
	kDollStab5,
	kDollLower,
	kDollSpell,
};

// Weapon Modes
enum {
	kForkModeNormal		= 0,
	kForkModeMax,

	kSprayModeNormal	= 0,
	kSprayModeMax,

	kTNTModeStick		= 0,
	kTNTModeBundle,
	kTNTModeProximity,
	kTNTModeRemote,
	kTNTModeMax,

	kShotgunModeNormal	= 0,
	kShotgunModeMax,

	kTommyModeNormal	= 0,
	kTommyModeAP,
	kTommyModeMax,

	kFlareModeNormal	= 0,
	kFlareModeSB,
	kFlareModeMax,

	kVoodooModeNormal	= 0,
	kVoodooModeMax,

	kSpearModeNormal	= 0,
	kSpearModeXP,
	kSpearModeMax,

	kShadowModeNormal	= 0,
	kShadowModeMax,

	kBeastModeNormal	= 0,
	kBeastModeMax,
};

enum {
	kQAVForkUp = 0,
	kQAVForkIdle,
	kQAVForkStab,
	kQAVForkDown,

	kQAVBicUp,				// raise lighter and open
	kQAVBicFlame,			// light lighter
	kQAVBicIdle,			// burning lighter
	kQAVBicDown,			// lower and close burning lighter

	kQAVSprayUp,
	kQAVSprayIdle,
	kQAVSprayFire,
	kQAVSprayDown,

	kQAVStickUp,			// raise just stick
	kQAVStickDown,			// lower just stick
	kQAVStickUp2,			// raise lighter and stick
	kQAVStickDown2,			// lower lighter and stick
	kQAVStickIdle,
	kQAVStickFuse,
	kQAVStickDrop,
	kQAVStickThrow,

	kQAVBundleUp,			// raise just bundle
	kQAVBundleDown,			// lower just bundle
	kQAVBundleUp2,			// raise lighter and bundle
	kQAVBundleDown2,		// lower lighter and bundle
	kQAVBundleIdle,
	kQAVBundleFuse,
	kQAVBundleDrop,
	kQAVBundleThrow,

	kQAVTNTExplode,

	kQAVProxUp,
	kQAVProxDown,
	kQAVProxIdle,
	kQAVProxDrop,
	kQAVProxThrow,

	kQAVRemoteUp1,			// detonator present, raise bundle
	kQAVRemoteUp2,			// raise both bundle and detonator
	kQAVRemoteUp3,			// raise detonator only
	kQAVRemoteDown1,		// both present, lower bundle
	kQAVRemoteDown2,		// lower both bundle and detonator
	kQAVRemoteDown3,		// lower detonator only
	kQAVRemoteIdle1,		// both bundle and detonator
	kQAVRemoteIdle2,		// detonator only
	kQAVRemoteDrop,
	kQAVRemoteThrow,
	kQAVRemoteFire,

	kQAVFlareUp,
	kQAVFlareIdle,
	kQAVFlareFire,
//	kQAVFlareReload,
	kQAVFlareDown,

	kQAVShotUp,
	kQAVShotIdle0,
	kQAVShotIdle1,
	kQAVShotIdle2,
	kQAVShotFireL,
	kQAVShotFireR,
	kQAVShotReload,
	kQAVShotDown,

	kQAVTommyUp,
	kQAVTommyIdle,
	kQAVTommyFire1,
 	kQAVTommyDown,

	kQAVSpearUp,
	kQAVSpearIdle1,
	kQAVSpearIdle2,
	kQAVSpearFire,
	kQAVSpearDown,

	kQAVShadowUp,
	kQAVShadowIdle,
	kQAVShadowFire,
	kQAVShadowDown,

	kQAVBeastUp,
	kQAVBeastIdle,
	kQAVBeastAttack1,
	kQAVBeastAttack2,
 	kQAVBeastAttack3,
 	kQAVBeastAttack4,
	kQAVBeastDown,

	kQAVDollUp,
	kQAVDollIdleStill,
	kQAVDollIdleMoving,
	kQAVDollStab1,
	kQAVDollStab2,
	kQAVDollStab3,
	kQAVDollStab4,
	kQAVDollStab5,
	kQAVDollDown,
	kQAVDollSpell,

	kQAVEnd
};

static char * weaponQAVNames[kQAVEnd] =
{
	"FORKUP",  		//kQAVForkUp = 0,
	"FORKIDLE",     //kQAVForkIdle,
	"PFORK",        //kQAVForkStab,
	"FORKDOWN",     //kQAVForkDown,

	"LITEOPEN",     //kQAVBicUp,
	"LITEFLAM",     //kQAVBicFlame,
	"LITEIDLE",     //kQAVBicIdle,
	"LITECLO2",     //kQAVBicDown,

	"CANPREF",      //kQAVSprayUp,
	"CANIDLE",      //kQAVSprayIdle,
	"CANFIRE",		//kQAVSprayFire,
	"CANDOWN",      //kQAVSprayDown,

	"DYNUP",		//kQAVStickUp,
	"DYNDOWN",		//kQAVStickDown,
	"DYNUP2",		//kQAVStickUp2,
	"DYNDOWN2",		//kQAVStickDown2,
	"DYNIDLE",		//kQAVStickIdle,
	"DYNFUSE",		//kQAVStickFuse,
	"DYNDROP",		//kQAVStickDrop,
	"DYNTHRO",		//kQAVStickThrow,

	"BUNUP",		//kQAVBundleUp,
	"BUNDOWN",		//kQAVBundleDown,
	"BUNUP2",		//kQAVBundleUp2,
	"BUNDOWN2",		//kQAVBundleDown2,
	"BUNIDLE",		//kQAVBundleIdle,
	"BUNFUSE",		//kQAVBundleFuse,
	"BUNDROP",		//kQAVBundleDrop,
	"BUNTHRO",		//kQAVBundleThrow,

	"DYNEXPLO",		//kQAVExplode,

	"PROXUP",		//kQAVProxUp,
	"PROXDOWN",		//kQAVProxDown,
	"PROXIDLE",		//kQAVProxIdle,
	"PROXDROP",		//kQAVProxDrop,
	"PROXTHRO",		//kQAVProxThrow,

	"REMUP1",   	//kQAVRemoteUp1,
	"REMUP2",   	//kQAVRemoteUp2,
	"REMUP3",   	//kQAVRemoteUp3,
	"REMDOWN1", 	//kQAVRemoteDown1,
	"REMDOWN2", 	//kQAVRemoteDown2,
	"REMDOWN3", 	//kQAVRemoteDown3,
	"REMIDLE1", 	//kQAVRemoteIdle1,
	"REMIDLE2", 	//kQAVRemoteIdle2,
	"REMDROP",  	//kQAVRemoteDrop,
	"REMTHRO",  	//kQAVRemoteThrow,
	"REMFIRE",  	//kQAVRemoteFire,

	"FLARUP",
	"FLARIDLE",
	"FLARFIR2",
//	"FLARLOAD",
	"FLARDOWN",

	"SHOTUP",		//kQAVShotUp,
	"SHOTI3",       //kQAVShotIdle0,
	"SHOTI2",       //kQAVShotIdle1,
	"SHOTI1",       //kQAVShotIdle2,
	"SHOTF1",       //kQAVShotFireL,
	"SHOTF2",       //kQAVShotFireR,
	"SHOTL1",       //kQAVShotReload,
	"SHOTDOWN",     //kQAVShotDown,

	"TOMUP",        //kQAVTommyUp,
	"TOMIDLE",      //kQAVTommyIdle,
	"TOMFIRE",		//kQAVTommyFire1,
	"TOMDOWN",      //kQAVTommyDown,

	"SGUNUP",		//kQAVSpearUp,
	"SGUNIDL1",		//kQAVSpearIdle1,
	"SGUNIDL2",		//kQAVSpearIdle2,
	"SGUNFIRE",		//kQAVSpearFire,
	"SGUNDOWN",		//kQAVSpearDown,

	"DARKUP",       //kQAVShadowUp,
	"DARKIDLE",     //kQAVShadowIdle,
	"DARKFIRE",     //kQAVShadowFire,
	"DARKDOWN",     //kQAVShadowDown,

	"BSTUP",		//kQAVBeastUp
	"BSTIDLE",		//kQAVBeastIdle,
	"BSTATAK1",     //kQAVBeastAttack1,
	"BSTATAK2",     //kQAVBeastAttack2,
	"BSTATAK3",     //kQAVBeastAttack3,
	"BSTATAK4",     //kQAVBeastAttack4,
	"BSTDOWN",

	"VDUP",			//kQAVDollUp
	"VDIDLE1",		//kQAVDollIdleStill	(PLAYER IS STILL)
	"VDIDLE2",		//kQAVDollIdleMoving	(PLAYER IS WALKING/RUNNING)
	"VDFIRE1",		//kQAVDollStab1
	"VDFIRE2",		//kQAVDollStab2
	"VDFIRE3",		//kQAVDollStab3
	"VDFIRE4",		//kQAVDollStab4
	"VDFIRE5",		//kQAVDollStab5
	"VDDOWN", 		//kQAVDollDown
	"VDSPEL1", 		//kQAVDollSpell
};


static modeMax[kWeaponMax + 1] =
{
	0,
	kForkModeMax,
	kSprayModeMax,
	kTNTModeMax,
	kShotgunModeMax,
	kTommyModeMax,
	kFlareModeMax,
	kVoodooModeMax,
	kSpearModeMax,
	kShadowModeMax,
	kBeastModeMax,
};


static QAV *weaponQAV[kQAVEnd];


/****************************************************************************
** LOCAL callback prototypes
****************************************************************************/

static void FireReanimator( int /* nTrigger */, PLAYER *pPlayer );


/****************************************************************************
** function calls
****************************************************************************/

static BOOL CheckAmmo( PLAYER *pPlayer, int nAmmo )
{
	if ( gInfiniteAmmo )
		return TRUE;

	if ( nAmmo == kAmmoNone )
		return TRUE;

	return pPlayer->ammoCount[nAmmo] > 0;
}


static void UseAmmo( PLAYER *pPlayer, int nAmmo, int nCount )
{
	if ( gInfiniteAmmo )
		return;

	if ( nAmmo == kAmmoNone )
		return;

	pPlayer->ammoCount[nAmmo] = ClipLow(pPlayer->ammoCount[nAmmo] - nCount, 0);
}


void WeaponInit( void )
{
	// get pointers to all the QAV resources
	for (int i = 0; i < kQAVEnd; i++)
	{
		RESHANDLE hQAV = gSysRes.Lookup(weaponQAVNames[i], ".QAV");
		if (hQAV == NULL)
		{
			dprintf("Could not load %s.QAV\n", weaponQAVNames[i]);
			ThrowError("Missing Weapon QAV file", ES_ERROR);
		}
		weaponQAV[i] = (QAV *)gSysRes.Lock(hQAV);
	}

	dprintf("Preload weapon QAV tiles\n");
	for (i = 0; i < kQAVEnd; i++)
	{
		if (weaponQAV[i] != NULL )
			weaponQAV[i]->Preload();
	}
}


void WeaponDraw( PLAYER *pPlayer, int shade, int x, int y )
{
	dassert(pPlayer != NULL);
	QAV *pQAV = pPlayer->pWeaponQAV;
	ulong t;

	if ( pQAV == NULL )
		return;

	if (pPlayer->weaponTimer == 0)	// playing idle QAV?
		t = gGameClock % pQAV->duration;
	else
		t = pQAV->duration - pPlayer->weaponTimer;

	pQAV->origin.x = x;
	pQAV->origin.y = y;

	int nPowerRemaining = powerupCheck( pPlayer, kItemLtdInvisibility - kItemBase );
	int nFlags = kQFrameScale;

	if ( nPowerRemaining >= (kTimerRate * 8) || nPowerRemaining && ( gGameClock & 32 ) )
	{
		nFlags |= kQFrameTranslucent;
		shade = -128;
	}

	pQAV->Draw(t, shade, nFlags);
}


void WeaponPlay( PLAYER *pPlayer )
{
	dassert(pPlayer != NULL);
	QAV *pQAV = pPlayer->pWeaponQAV;

	if ( pQAV == NULL )
		return;

	ulong t = pQAV->duration - pPlayer->weaponTimer;

	pQAV->Play(t - kFrameTicks, t, pPlayer->weaponCallback, pPlayer);
}


static void StartQAV( PLAYER *pPlayer, int weaponIndex, QAVCALLBACK callback = NULL, BOOL fLoop = FALSE)
{
	pPlayer->pWeaponQAV = weaponQAV[weaponIndex];
	pPlayer->weaponTimer = pPlayer->pWeaponQAV->duration;
	pPlayer->weaponCallback = callback;
	pPlayer->fLoopQAV = fLoop;
}


#define kAimMaxDist 	M2X(100)
#define kAimMaxAngle	kAngle15
#define kAimMaxSlope	9459			// tan(30) << 14

#define kTrackRate		0x2000			// 0x10000 tracks instantly, 0 takes forever

static void UpdateAimVector( PLAYER *pPlayer )
{
	dassert(pPlayer != NULL);
	SPRITE *pSprite = pPlayer->sprite;

	int x = pSprite->x;
	int y = pSprite->y;
	int z = pSprite->z - pPlayer->weaponAboveZ;

	VECTOR3D aim;

	aim.dx = Cos(pSprite->ang) >> 16;
	aim.dy = Sin(pSprite->ang) >> 16;
	aim.dz = pPlayer->slope;

	int nAimSprite = -1;

	if ( gAutoAim )
	{
		int closest = 0x7FFFFFFF;
		for (short nTarget = headspritestat[kStatDude]; nTarget >= 0; nTarget = nextspritestat[nTarget] )
		{
			SPRITE *pTarget = &sprite[nTarget];

			// don't target yourself!
			if ( pTarget == pSprite )
				continue;

			int dx, dy, dz;

			dx = pTarget->x - x;
			dy = pTarget->y - y;
			dz = pTarget->z - z;

			int dist = qdist(dx, dy);
			if ( dist > closest || dist > kAimMaxDist )
				continue;

			int ang = getangle(dx, dy);
			if ( qabs(((ang - pSprite->ang + kAngle180) & kAngleMask) - kAngle180) > kAimMaxAngle )
				continue;

			int z1 = z + mulscale(pPlayer->slope, dist, 10);
			int z2 = mulscale(kAimMaxSlope, dist, 10);
			if ( (pTarget->z < z1 - z2) || (pTarget->z > z1 + z2) )
				continue;

			if ( cansee(x, y, z, pSprite->sectnum, pTarget->x, pTarget->y, pTarget->z, pTarget->sectnum) )
			{
				closest = dist;

				aim.dx = Cos(ang) >> 16;
				aim.dy = Sin(ang) >> 16;
				aim.dz = divscale(dz, dist, 10);
				nAimSprite = nTarget;
			}
		}

		// barrels
		for (nTarget = headspritestat[kStatThing]; nTarget >= 0; nTarget = nextspritestat[nTarget] )
		{
			SPRITE *pTarget = &sprite[nTarget];

			if (pTarget->type != kThingTNTBarrel )
				continue;

			int dx, dy, dz;

			dx = pTarget->x - x;
			dy = pTarget->y - y;
			dz = pTarget->z - z;

			int dist = qdist(dx, dy);
			if ( dist > closest || dist > kAimMaxDist )
				continue;

			int ang = getangle(dx, dy);
			if ( qabs(((ang - pSprite->ang + kAngle180) & kAngleMask) - kAngle180) > kAimMaxAngle )
				continue;

			int z1 = z + mulscale(pPlayer->slope, dist, 10);
			int z2 = mulscale(kAimMaxSlope, dist, 10);
			if ( (pTarget->z < z1 - z2) || (pTarget->z > z1 + z2) )
				continue;

			if ( cansee(x, y, z, pSprite->sectnum, pTarget->x, pTarget->y, pTarget->z, pTarget->sectnum) )
			{
				closest = dist;

				aim.dx = Cos(ang) >> 16;
				aim.dy = Sin(ang) >> 16;
				aim.dz = divscale(dz, dist, 10);
				nAimSprite = nTarget;
			}
		}

	}

	VECTOR3D relAim = aim;
	RotateVector(&relAim.dx, &relAim.dy, -pSprite->ang);

	pPlayer->relAim.dx = dmulscale16r(pPlayer->relAim.dx, 0x10000 - kTrackRate, relAim.dx, kTrackRate);
	pPlayer->relAim.dy = dmulscale16r(pPlayer->relAim.dy, 0x10000 - kTrackRate, relAim.dy, kTrackRate);
	pPlayer->relAim.dz = dmulscale16r(pPlayer->relAim.dz, 0x10000  -kTrackRate, relAim.dz, kTrackRate);

	pPlayer->aim = pPlayer->relAim;
	RotateVector(&pPlayer->aim.dx, &pPlayer->aim.dy, pSprite->ang);

	pPlayer->aimSprite = nAimSprite;
}


void WeaponRaise( PLAYER *pPlayer, INPUT *pInput )
{
	dassert( pPlayer != 0 );

	pPlayer->weapon = pInput->newWeapon;
	pInput->newWeapon = 0;
	dprintf("WeaponRaise:  weapon = %d, mode = %d\n",
		pPlayer->weapon, pPlayer->weaponMode[pPlayer->weapon]);

	switch( pPlayer->weapon )
	{
		case kWeaponPitchfork:
			StartQAV(pPlayer, kQAVForkUp);
			pPlayer->weaponState = kForkIdle;
			pPlayer->weaponAmmo = kAmmoNone;
			break;

		case kWeaponSprayCan:
			StartQAV(pPlayer, kQAVBicUp);
			pPlayer->weaponState = kBicOpen;
			pPlayer->weaponAmmo = kAmmoSprayCan;
			break;

		case kWeaponTNT:
			switch ( pPlayer->weaponMode[kWeaponTNT] )
			{
				case kTNTModeStick:
					StartQAV(pPlayer, kQAVStickUp2);
					pPlayer->weaponState = kTNTStickIdle;
					pPlayer->weaponAmmo = kAmmoTNTStick;
					break;

				case kTNTModeBundle:
					StartQAV(pPlayer, kQAVBundleUp2);
					pPlayer->weaponState = kTNTBundleIdle;
					pPlayer->weaponAmmo = kAmmoTNTStick;
					break;

				case kTNTModeProximity:
					StartQAV(pPlayer, kQAVProxUp);
					pPlayer->weaponState = kTNTProxIdle;
					pPlayer->weaponAmmo = kAmmoTNTProximity;
					break;

				case kTNTModeRemote:
					StartQAV(pPlayer, kQAVRemoteUp2);
					pPlayer->weaponState = kTNTRemoteIdle1;
					pPlayer->weaponAmmo = kAmmoTNTRemote;
					break;
			}
			break;

		case kWeaponShotgun:
			StartQAV(pPlayer, kQAVShotUp);
			if ( gInfiniteAmmo || pPlayer->ammoCount[kAmmoShell] > 1 )
				pPlayer->weaponState = kShotIdle2;
			else if ( pPlayer->ammoCount[kAmmoShell] > 0 )
				pPlayer->weaponState = kShotIdle1;
			else
				pPlayer->weaponState = kShotIdle0;
			pPlayer->weaponAmmo = kAmmoShell;
			break;

		case kWeaponTommy:
			StartQAV(pPlayer, kQAVTommyUp);
			pPlayer->weaponState = kTommyIdle;
			pPlayer->weaponAmmo = kAmmoBullet;
			break;

		case kWeaponVoodoo:
			StartQAV(pPlayer, kQAVDollUp);
			pPlayer->weaponState = kDollIdle;
			pPlayer->weaponAmmo = kAmmoVoodoo;
			break;

		case kWeaponFlare:
			StartQAV(pPlayer, kQAVFlareUp);
			pPlayer->weaponState = kFlareIdle;
			pPlayer->weaponAmmo = kAmmoFlare;
			break;

		case kWeaponSpear:
			StartQAV(pPlayer, kQAVSpearUp);
			if ( pPlayer->ammoCount[kAmmoSpear] > 0 || pPlayer->ammoCount[kAmmoSpearXP] > 0 )
				pPlayer->weaponState = kSpearIdle1;
			else
				pPlayer->weaponState = kSpearIdle2;
			pPlayer->weaponAmmo = kAmmoSpear;
			break;

		case kWeaponShadow:
			StartQAV(pPlayer, kQAVShadowUp);
			pPlayer->weaponState = kShadowIdle;
			pPlayer->weaponAmmo = kAmmoNone;
			break;

		case kWeaponBeast:
			StartQAV(pPlayer, kQAVBeastUp);
			pPlayer->weaponState = kBeastIdle;
			pPlayer->weaponAmmo = kAmmoNone;
			break;
	}
}


void WeaponLower( PLAYER *pPlayer, INPUT *pInput )
{
	dprintf("WeaponLower()\n");
	dassert( pPlayer != 0 );

	int nWeapon = pPlayer->weapon;
	int nState = pPlayer->weaponState;

	switch( nWeapon )
	{
		case kWeaponPitchfork:
			StartQAV(pPlayer, kQAVForkDown);
			break;

		case kWeaponSprayCan:
			switch( nState )
			{
				case kBicIdle:
			        StartQAV(pPlayer, kQAVBicDown);
					break;

				case kBicSwitch:
					pPlayer->weapon = pInput->newWeapon;
					pPlayer->weaponState = kBicIdle;
					pInput->newWeapon = 0;
					return;

				default:
				    StartQAV(pPlayer, kQAVSprayDown);
					if ( pInput->newWeapon == kWeaponTNT )
						pPlayer->weaponState = kBicSwitch;
					else
						pPlayer->weaponState = kBicIdle;
					return;

			}
			break;

		case kWeaponTNT:
		{
			switch( nState )
			{
				case kBicSwitch:
					pPlayer->weapon = pInput->newWeapon;
					pInput->newWeapon = 0;
					return;

				case kBicIdle:
			        StartQAV(pPlayer, kQAVBicDown);
					break;

				case kTNTStickIdle:
					if ( pInput->newWeapon == kWeaponSprayCan )
					{
						StartQAV(pPlayer, kQAVStickDown);
						pPlayer->weaponState = kBicSwitch;
						return;
					}
					StartQAV(pPlayer, kQAVStickDown2);
					break;

				case kTNTBundleIdle:
					if ( pInput->newWeapon == kWeaponSprayCan )
					{
						StartQAV(pPlayer, kQAVBundleDown);
						pPlayer->weaponState = kBicSwitch;
						return;
					}
					StartQAV(pPlayer, kQAVBundleDown2);
					break;

				case kTNTProxIdle:
					StartQAV(pPlayer, kQAVProxDown);
					break;

				case kTNTRemoteIdle1:
					StartQAV(pPlayer, kQAVRemoteDown2);
					break;

				case kTNTRemoteIdle2:
					StartQAV(pPlayer, kQAVRemoteDown3);
					break;
			}
			break;
		}

		case kWeaponShotgun:
	        StartQAV(pPlayer, kQAVShotDown);
			break;

		case kWeaponTommy:
	        StartQAV(pPlayer, kQAVTommyDown);
			break;

		case kWeaponFlare:
			StartQAV(pPlayer, kQAVFlareDown);
			break;

		case kWeaponVoodoo:
			StartQAV(pPlayer, kQAVDollDown);
			break;

		case kWeaponSpear:
	        StartQAV(pPlayer, kQAVSpearDown);
			break;

		case kWeaponShadow:
	        StartQAV(pPlayer, kQAVShadowDown);
			break;

		case kWeaponBeast:
	        StartQAV(pPlayer, kQAVBeastDown);
			break;
	}

	if (pInput->newWeapon == pPlayer->weapon)
	{
		if ( ++pPlayer->weaponMode[pPlayer->weapon] == modeMax[pPlayer->weapon] )
		{
			pPlayer->weaponMode[pPlayer->weapon] = 0;
			pInput->newWeapon = 0;
		}
	}

	pPlayer->weapon = 0;
}


void WeaponUpdateState( PLAYER *pPlayer)
{
	SPRITE *pSprite		= pPlayer->sprite;
	XSPRITE *pXSprite	= pPlayer->xsprite;

	int nWeapon = pPlayer->weapon;
	int nState = pPlayer->weaponState;

	switch( nWeapon )
	{
		case kWeaponPitchfork:
			pPlayer->pWeaponQAV = weaponQAV[kQAVForkIdle];
			break;

		case kWeaponSprayCan:
			switch( nState )
			{
				case kBicOpen:
					StartQAV(pPlayer, kQAVBicFlame);
					pPlayer->weaponState = kBicIdle;
					break;

				case kBicIdle:
					if ( CheckAmmo(pPlayer, kAmmoSprayCan) )
					{
						StartQAV(pPlayer, kQAVSprayUp);
						pPlayer->weaponState = kCanIdle;
					}
					else
						pPlayer->pWeaponQAV = weaponQAV[kQAVBicIdle];
					break;

				case kCanIdle:
					pPlayer->pWeaponQAV = weaponQAV[kQAVSprayIdle];
					break;

				case kCanFire:
					if ( CheckAmmo(pPlayer, kAmmoSprayCan) )
					{
						pPlayer->pWeaponQAV = weaponQAV[kQAVSprayIdle];
						pPlayer->weaponState = kCanIdle;
					}
					else
					{
						StartQAV(pPlayer, kQAVSprayDown);
						pPlayer->weaponState = kBicIdle;
					}
					break;
			}
			break;

		case kWeaponTNT:
			switch( nState )
			{
				case kBicIdle:
					switch ( pPlayer->weaponMode[kWeaponTNT] )
					{
						case kTNTModeStick:
							StartQAV(pPlayer, kQAVStickUp);
							pPlayer->weaponState = kTNTStickIdle;
							break;

						case kTNTModeBundle:
							StartQAV(pPlayer, kQAVBundleUp);
							pPlayer->weaponState = kTNTBundleIdle;
							break;
					}
					break;

				case kBicOpen:
					StartQAV(pPlayer, kQAVBicFlame);
					pPlayer->weaponState = kBicIdle;
					break;

				case kBicSwitch:
					if ( pPlayer->ammoCount[kAmmoTNTStick] > 0)
					{
						StartQAV(pPlayer, kQAVStickUp);
						pPlayer->weaponState = kTNTStickIdle;
					}
					else
						pPlayer->pWeaponQAV = weaponQAV[kQAVBicIdle];
					break;

				case kTNTStickIdle:
					pPlayer->pWeaponQAV = weaponQAV[kQAVStickIdle];
					break;

				case kTNTBundleIdle:
					pPlayer->pWeaponQAV = weaponQAV[kQAVBundleIdle];
					break;

				case kTNTProxIdle:
					pPlayer->pWeaponQAV = weaponQAV[kQAVProxIdle];
					break;

				case kTNTProxEmpty:
					StartQAV(pPlayer, kQAVProxUp);
					pPlayer->weaponState = kTNTProxIdle;
					break;

				case kTNTRemoteIdle1:
					pPlayer->pWeaponQAV = weaponQAV[kQAVRemoteIdle1];
					break;

				case kTNTRemoteIdle2:
					pPlayer->pWeaponQAV = weaponQAV[kQAVRemoteIdle2];
					break;

				case kTNTRemoteEmpty:
					StartQAV(pPlayer, kQAVRemoteUp2);
					pPlayer->weaponState = kTNTRemoteIdle1;
					break;
			}
			break;

		case kWeaponShotgun:
			switch ( nState )
			{
				case kShotIdle0:
					if ( CheckAmmo(pPlayer, kAmmoShell) )
					{
				    	StartQAV(pPlayer, kQAVShotReload);
						if ( gInfiniteAmmo || pPlayer->ammoCount[kAmmoShell] > 1 )
							pPlayer->weaponState = kShotIdle2;
						else
							pPlayer->weaponState = kShotIdle1;
					}
					else
						pPlayer->pWeaponQAV = weaponQAV[kQAVShotIdle0];
					break;

				case kShotIdle1:
					pPlayer->pWeaponQAV = weaponQAV[kQAVShotIdle1];
					break;

				case kShotIdle2:
					pPlayer->pWeaponQAV = weaponQAV[kQAVShotIdle2];
					break;

			}
			break;

		case kWeaponTommy:
			pPlayer->pWeaponQAV = weaponQAV[kQAVTommyIdle];
			break;

		case kWeaponFlare:
			if (nState == kFlareIdle)
				pPlayer->pWeaponQAV = weaponQAV[kQAVFlareIdle];
			break;

		case kWeaponVoodoo:
	 		switch ( pXSprite->moveState )
			{
				case kMoveWalk:
					if ( qabs(pPlayer->swayHeight) > (3 << 8) )
						pPlayer->pWeaponQAV = weaponQAV[kQAVDollIdleMoving];
					else
						pPlayer->pWeaponQAV = weaponQAV[kQAVDollIdleStill];
					break;

				case kMoveLand:
					pPlayer->pWeaponQAV = weaponQAV[kQAVDollIdleMoving];
					break;

				default:
					pPlayer->pWeaponQAV = weaponQAV[kQAVDollIdleStill];
					break;
			}
			break;

		case kWeaponSpear:
			switch ( nState )
			{
				case kSpearIdle1:
					pPlayer->pWeaponQAV = weaponQAV[kQAVSpearIdle1];
					break;

				case kSpearIdle2:
					pPlayer->pWeaponQAV = weaponQAV[kQAVSpearIdle2];
					break;
			}
			break;

		case kWeaponShadow:
			pPlayer->pWeaponQAV = weaponQAV[kQAVShadowIdle];
			break;

		case kWeaponBeast:
			pPlayer->pWeaponQAV = weaponQAV[kQAVBeastIdle];
			break;
	}
}


static void FirePitchfork( int /* nTrigger */, PLAYER *pPlayer )
{
	SPRITE *pSprite = pPlayer->sprite;

	int z = pSprite->z - pPlayer->weaponAboveZ;

	for ( int i = 0; i < 4; i++ )
	{
		// dispersal modifiers
		int ddx = BiRandom(2000);
		int ddy = BiRandom(2000);
		int ddz = BiRandom(2000);

		actFireVector(pPlayer->nSprite, z,
			pPlayer->aim.dx + ddx, pPlayer->aim.dy + ddy, pPlayer->aim.dz + ddz,
			kVectorTine);
	}
}


static void FireSpray( int /* nTrigger */, PLAYER *pPlayer )
{
	playerFireMissile(pPlayer, pPlayer->aim.dx, pPlayer->aim.dy, pPlayer->aim.dz,
		kMissileSprayFlame);
	UseAmmo(pPlayer, kAmmoSprayCan, kFrameTicks);
}


#define kMaxThrowTime 	(3 * kTimerRate)	// how long to hold fire to get farthest throw
#define kMinThrowVel	((M2X(8.0) << 4) / kTimerRate)
#define kMaxThrowVel	((M2X(30.0) << 4) / kTimerRate)
#define kSlopeThrow		-8192	// tan(26)

static void ThrowStick( int /* nTrigger */, PLAYER *pPlayer )
{
	pPlayer->throwTime = ClipHigh(pPlayer->throwTime, kMaxThrowTime);
	int velocity = muldiv(pPlayer->throwTime, kMaxThrowVel - kMinThrowVel, kMaxThrowTime) + kMinThrowVel;
	int nThing = playerFireThing(pPlayer, kSlopeThrow, kThingTNTStick, velocity);
	XSPRITE *pXSprite = &xsprite[sprite[nThing].extra];
	if ( pPlayer->fuseTime < 0 )
		pXSprite->triggerProximity = 1;
	else
		evPost(nThing, SS_SPRITE, pPlayer->fuseTime);
	UseAmmo(pPlayer, kAmmoTNTStick, 1);
}


static void DropStick( int /* nTrigger */, PLAYER *pPlayer )
{
	int nThing = playerFireThing(pPlayer, 0, kThingTNTStick, 0);
	evPost(nThing, SS_SPRITE, pPlayer->fuseTime);
	UseAmmo(pPlayer, kAmmoTNTStick, 1);
}


static void ExplodeStick( int /* nTrigger */, PLAYER *pPlayer )
{
	int nThing = playerFireThing(pPlayer, 0, kThingTNTStick, 0);
	evPost(nThing, SS_SPRITE, 0);	// trigger it immediately
	UseAmmo(pPlayer, kAmmoTNTStick, 1);
	StartQAV(pPlayer, kQAVTNTExplode);
	pPlayer->weapon = 0;	// drop the hands
}


static void ThrowBundle( int /* nTrigger */, PLAYER *pPlayer )
{
	pPlayer->throwTime = ClipHigh(pPlayer->throwTime, kMaxThrowTime);
	int velocity = muldiv(pPlayer->throwTime, kMaxThrowVel - kMinThrowVel, kMaxThrowTime) + kMinThrowVel;
	int nThing = playerFireThing(pPlayer, kSlopeThrow, kThingTNTBundle, velocity);
	XSPRITE *pXSprite = &xsprite[sprite[nThing].extra];
	if ( pPlayer->fuseTime < 0 )
		pXSprite->triggerProximity = 1;
	else
		evPost(nThing, SS_SPRITE, pPlayer->fuseTime);
	UseAmmo(pPlayer, kAmmoTNTStick, 7);
}


static void DropBundle( int /* nTrigger */, PLAYER *pPlayer )
{
	int nThing = playerFireThing(pPlayer, 0, kThingTNTBundle, 0);
	evPost(nThing, SS_SPRITE, pPlayer->fuseTime);
	UseAmmo(pPlayer, kAmmoTNTStick, 7);
}


static void ExplodeBundle( int /* nTrigger */, PLAYER *pPlayer )
{
	int nThing = playerFireThing(pPlayer, 0, kThingTNTBundle, 0);
	evPost(nThing, SS_SPRITE, 0);	// trigger it immediately
	UseAmmo(pPlayer, kAmmoTNTStick, 7);
	StartQAV(pPlayer, kQAVTNTExplode);
	pPlayer->weapon = 0;	// drop the hands
}


static void ThrowProx( int /* nTrigger */, PLAYER *pPlayer )
{
	pPlayer->throwTime = ClipHigh(pPlayer->throwTime, kMaxThrowTime);
	int velocity = muldiv(pPlayer->throwTime, kMaxThrowVel - kMinThrowVel, kMaxThrowTime) + kMinThrowVel;
	int nThing = playerFireThing(pPlayer, kSlopeThrow, kThingTNTBundle, velocity);
	XSPRITE *pXSprite = &xsprite[sprite[nThing].extra];
	pXSprite->triggerProximity = 1;
	UseAmmo(pPlayer, kAmmoTNTProximity, 1);
}


static void DropProx( int /* nTrigger */, PLAYER *pPlayer )
{
	int nThing = playerFireThing(pPlayer, 0, kThingTNTProxArmed, 0);
	XSPRITE *pXSprite = &xsprite[sprite[nThing].extra];
	evPost(nThing, SS_SPRITE, 2 * kTimerRate);	// arm it after 2 seconds
	UseAmmo(pPlayer, kAmmoTNTProximity, 1);
}


static void ThrowRemote( int /* nTrigger */, PLAYER *pPlayer )
{
	pPlayer->throwTime = ClipHigh(pPlayer->throwTime, kMaxThrowTime);
	int velocity = muldiv(pPlayer->throwTime, kMaxThrowVel - kMinThrowVel, kMaxThrowTime) + kMinThrowVel;
	int nThing = playerFireThing(pPlayer, kSlopeThrow, kThingTNTBundle, velocity);
	XSPRITE *pXSprite = &xsprite[sprite[nThing].extra];
	pXSprite->triggerProximity = 1;
	UseAmmo(pPlayer, kAmmoTNTRemote, 1);
}


static void DropRemote( int /* nTrigger */, PLAYER *pPlayer )
{
	int nThing = playerFireThing(pPlayer, 0, kThingTNTRemArmed, 0);
	XSPRITE *pXSprite = &xsprite[sprite[nThing].extra];
	pXSprite->rxID = kChannelRemoteFire1 + pPlayer->sprite->type - kDudePlayer1;
	UseAmmo(pPlayer, kAmmoTNTRemote, 1);
	//sndStartSample( "tnt_prox", 64 );
}


static void FireRemote( int /* nTrigger */, PLAYER *pPlayer )
{
	evSend(0, 0, kChannelRemoteFire1 + pPlayer->sprite->type - kDudePlayer1, kCommandOn);
}


static void FireShotgun( PLAYER *pPlayer, int nBarrels )
{
	dassert(nBarrels > 0 && nBarrels <= kShotgunBarrels);

	int nVectors = nBarrels * kVectorsPerBarrel;

	SPRITE *pSprite = pPlayer->sprite;

	int z = pSprite->z - pPlayer->weaponAboveZ;

	for ( int i = 0; i < nVectors; i++ )
	{
		// dispersal modifiers
		int ddx = BiRandom2(2000);
		int ddy = BiRandom2(2000);
		int ddz = BiRandom2(2000);

		actFireVector(pPlayer->nSprite, z,
			pPlayer->aim.dx + ddx, pPlayer->aim.dy + ddy, pPlayer->aim.dz + ddz,
			kVectorShell);
	}
	if (nBarrels == 1)
		sfxCreate3DSound(pSprite->x, pSprite->y, pSprite->z, kSfxShotFire);
	else
		sfxCreate3DSound(pSprite->x, pSprite->y, pSprite->z, kSfxShotFire2);
}


static void FireTommy( int /* nTrigger */, PLAYER *pPlayer )
{
	SPRITE *pSprite = pPlayer->sprite;

	int z = pSprite->z - pPlayer->weaponAboveZ;

	UseAmmo(pPlayer, pPlayer->weaponAmmo, 1);

	// dispersal modifiers
	int ddx = BiRandom(1000);
	int ddy = BiRandom(1000);
	int ddz = BiRandom(1000);

	actFireVector(pPlayer->nSprite, z,
		pPlayer->aim.dx + ddx, pPlayer->aim.dy + ddy, pPlayer->aim.dz + ddz,
		kVectorBullet);
	sfxCreate3DSound(pSprite->x, pSprite->y, pSprite->z, kSfxTomFire);
}


static void FireFlare( int /* nTrigger */, PLAYER *pPlayer )
{
	SPRITE *pSprite = pPlayer->sprite;

	playerFireMissile(pPlayer, pPlayer->aim.dx, pPlayer->aim.dy, pPlayer->aim.dz,
		kMissileFlare);
	UseAmmo(pPlayer, pPlayer->weaponAmmo, 1);
	sfxCreate3DSound(pSprite->x, pSprite->y, pSprite->z, kSfxFlareFire);
}


// this is a hack to fire a starburst flare
static void FireFlare2( int /* nTrigger */, PLAYER *pPlayer )
{
	SPRITE *pSprite = pPlayer->sprite;

	playerFireMissile(pPlayer, pPlayer->aim.dx, pPlayer->aim.dy, pPlayer->aim.dz,
		kMissileStarburstFlare);
	UseAmmo(pPlayer, pPlayer->weaponAmmo, 1);
	sfxCreate3DSound(pSprite->x, pSprite->y, pSprite->z, kSfxFlareFire);
}


static void FireVoodoo( int nStabType, PLAYER *pPlayer )
{
	SPRITE *pSprite = pPlayer->sprite;
	SPRITE *pTarget = NULL;
	int nSelfSprite = pPlayer->nSprite;
	int z = pSprite->z - pPlayer->weaponAboveZ;

	if ( nStabType == kVoodooStabSelf )
	{
		actDamageSprite(nSelfSprite, pPlayer->nSprite, kDamageStab, 2 << 4);
		if (pPlayer == gMe)
			viewSetMessage("Ouch!");
		return;
	}

	dassert(pPlayer->aimSprite >= 0);
	pTarget = &sprite[pPlayer->aimSprite];

	switch( nStabType )
	{
		case kVoodooStabChest:
			sfxCreate3DSound(pSprite->x, pSprite->y, pSprite->z, kSfxVoodooHit);
			actSpawnEffect(pTarget->sectnum, pTarget->x, pTarget->y, pTarget->z,
				ET_Squib1);
			actDamageSprite(nSelfSprite, pPlayer->aimSprite, kDamageVoodoo, 10 << 4);
			break;

		case kVoodooStabShoulder:
			sfxCreate3DSound(pSprite->x, pSprite->y, pSprite->z, kSfxVoodooHit2);
			actSpawnEffect(pTarget->sectnum, pTarget->x, pTarget->y, pTarget->z,
				ET_Squib1);
			actDamageSprite(nSelfSprite, pPlayer->aimSprite, kDamageVoodoo, 20 << 4);
			if ( pTarget->type >= kDudePlayer1 && pTarget->type <= kDudePlayer8 )
			{
				int nPlayerHit = pTarget->type - kDudePlayer1;
				WeaponLower(&gPlayer[nPlayerHit], &gPlayerInput[nPlayerHit]);
			}
			break;

		case kVoodooStabGroin:
			sfxCreate3DSound(pSprite->x, pSprite->y, pSprite->z, kSfxPlayLaugh);
			actSpawnEffect(pTarget->sectnum, pTarget->x, pTarget->y, pTarget->z,
				ET_Squib1);
			actDamageSprite(nSelfSprite, pPlayer->aimSprite, kDamageVoodoo, 50 << 4);
			break;

		case kVoodooStabEye:
			sfxCreate3DSound(pSprite->x, pSprite->y, pSprite->z, kSfxVoodooHit4);
			actSpawnEffect(pTarget->sectnum, pTarget->x, pTarget->y, pTarget->z,
				ET_Squib1);
			actDamageSprite(nSelfSprite, pPlayer->aimSprite, kDamageVoodoo, 25 << 4);
			if ( pTarget->type >= kDudePlayer1 && pTarget->type <= kDudePlayer8 )
			{
				PLAYER *pPlayerHit = &gPlayer[pTarget->type - kDudePlayer1];
				if (pPlayerHit == gView)	// change to gMe later!
				   	scrDacRelEffect(-96, -96, -96);
			}
			break;
	}
}


static void FireSpear( int /* nTrigger */, PLAYER *pPlayer )
{
	playerFireMissile(pPlayer, pPlayer->aim.dx, pPlayer->aim.dy, pPlayer->aim.dz,
		kMissileSpear);
	UseAmmo(pPlayer, pPlayer->weaponAmmo, 1);
}


static void FireShadow( int /* nTrigger */, PLAYER *pPlayer )
{
	playerFireMissile(pPlayer, pPlayer->aim.dx, pPlayer->aim.dy, pPlayer->aim.dz,
		kMissileEctoSkull);

	if ( !gInfiniteAmmo )
		actDamageSprite(pPlayer->nSprite, pPlayer->nSprite, kDamageSpirit, 10 << 4);
}


static void FireReanimator( int /* nTrigger */, PLAYER *pPlayer )
{
	SPRITE *pSprite = pPlayer->sprite;

	int z = pSprite->z - pPlayer->weaponAboveZ;

	sfxCreate3DSound(pSprite->x, pSprite->y, pSprite->z, kSfxVoodooChant);

	for ( int i = 0; i < 4; i++ )
	{
		// dispersal modifiers
		int ddx = BiRandom(2000);
		int ddy = BiRandom(2000);
		int ddz = BiRandom(2000);

		HITINFO hitInfo;

		int hitCode = HitScan( pPlayer->sprite, z,
			pPlayer->aim.dx + ddx, pPlayer->aim.dy + ddy, pPlayer->aim.dz + ddz, &hitInfo );

		// ADD: limit distance of reanimator voodoo

		switch ( hitCode )
		{
			case SS_SPRITE:
			{
				SPRITE *pSprite = &sprite[hitInfo.hitsprite];
				if (pSprite->type == kDudeAxeZombie && pSprite->owner == -1)
				{
					//dprintf("calling playerAddLackey\n");
					if (playerAddLackey(pPlayer, hitInfo.hitsprite))
						aiSetTarget( &xsprite[pSprite->extra], pPlayer->sprite->x, pPlayer->sprite->y, pPlayer->sprite->z );
				}
				break;
			}
		}
	}
}


uchar WeaponFindLoaded( PLAYER *pPlayer )
{
	dprintf("WeaponFindLoaded\n");

	// see if it just needs to switch to a different type of ammo
	switch ( pPlayer->weapon )
	{
		case kWeaponTNT:
			if ( pPlayer->ammoCount[kAmmoTNTStick] || pPlayer->ammoCount[kAmmoTNTProximity] ||
				pPlayer->ammoCount[kAmmoTNTRemote] )
				return kWeaponTNT;
			break;

		case kWeaponTommy:
			if ( pPlayer->ammoCount[kAmmoBullet] || pPlayer->ammoCount[kAmmoBulletAP] )
				return kWeaponTommy;
			break;

		case kWeaponFlare:
			if ( pPlayer->ammoCount[kAmmoFlare] || pPlayer->ammoCount[kAmmoFlareSB] )
				return kWeaponFlare;
			break;

		case kWeaponSpear:
			if ( pPlayer->ammoCount[kAmmoSpear] || pPlayer->ammoCount[kAmmoSpearXP] )
				return kWeaponSpear;
			break;
	}

	// switch to a new weapon based on available ammo
	if ( pPlayer->hasWeapon[ kWeaponShadow ] && powerupCheck(pPlayer, kItemInvulnerability - kItemBase) )
		return kWeaponShadow;
	else if ( pPlayer->hasWeapon[kWeaponTommy] && CheckAmmo(pPlayer, kAmmoBullet) )
		return kWeaponTommy;
	else if ( pPlayer->hasWeapon[kWeaponTommy] && CheckAmmo(pPlayer, kAmmoBulletAP) )
		return kWeaponTommy;
	else if ( pPlayer->hasWeapon[kWeaponShotgun] && CheckAmmo( pPlayer, kAmmoShell) )
		return kWeaponShotgun;
	else if ( pPlayer->hasWeapon[kWeaponFlare] && CheckAmmo(pPlayer, kAmmoFlareSB) )
		return kWeaponFlare;
	else if ( pPlayer->hasWeapon[kWeaponFlare] && CheckAmmo(pPlayer, kAmmoFlare) )
		return kWeaponFlare;
	else if ( pPlayer->hasWeapon[kWeaponSpear] && CheckAmmo(pPlayer, kAmmoSpearXP) )
		return kWeaponSpear;
	else if ( pPlayer->hasWeapon[kWeaponSpear] && CheckAmmo(pPlayer, kAmmoSpear) )
		return kWeaponSpear;
	else if ( pPlayer->hasWeapon[kWeaponSprayCan] && CheckAmmo(pPlayer, kAmmoSprayCan) )
		return kWeaponSprayCan;
	else if ( pPlayer->hasWeapon[kWeaponTNT] && CheckAmmo(pPlayer, kAmmoTNTStick) )
		return kWeaponTNT;
	else if ( pPlayer->hasWeapon[kWeaponTNT] && CheckAmmo(pPlayer, kAmmoTNTProximity) )
		return kWeaponTNT;
	else if ( pPlayer->hasWeapon[kWeaponTNT] && CheckAmmo(pPlayer, kAmmoTNTRemote) )
		return kWeaponTNT;
	else if ( pPlayer->hasWeapon[kWeaponVoodoo] && CheckAmmo(pPlayer, kAmmoVoodoo) )
		return kWeaponVoodoo;
	else if ( pPlayer->hasWeapon[kWeaponShadow] && pPlayer->xsprite->health > 75 )
		return kWeaponShadow;

	return kWeaponPitchfork;
}


/*
;[Weapon#]
;Name            = "WeaponNameMax24"
;SelectKey       = [ None | scancode ]
;UseByDefault    = [ TRUE | FALSE ]
;HasByDefault    = [ TRUE | FALSE ]
;RequiresWeapon  = [ None | 0..9 ]
;
;Type            = [ NonWeapon | Melee | Thrown | Projectile | Vector ]
;Type            = NonWeapon
;
;Type            = Melee
;MaxDistance     = [ Infinite | 0..max ]
;
;Type            = Thrown
;MaxDistance     = [ Infinite | 0..max ]
;
;Type            = Projectile
;MaxDistance     = [ Infinite | 0..max ]
;
;Type            = Vector (uses hitscan)
;MaxDistance     = [ Infinite | 0..max ]
;Richochet       = [ TRUE | FALSE ]
;UsesNightScope  = [ TRUE | FALSE ]
;
;Shots           = [ 0..max shots or projectiles ]
;Dispersion      = [ 0..100% ]
;Accuracy        = [ 0..100% ]
;
;AmmoStyle       = [ Infinite, Rechargable, Expendable, Health ]
;AmmoStyle       = Infinite
;
;AmmoStyle       = Rechargable
;RechargeRate    = [ 0 | ammo/second (?) ]
;MaxAmmo         = [ 0..max ]
;
;AmmoStyle       = Expendable
;MaxAmmo         = [ 0..max ]
;AmmoType        = [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 ]
;
;AmmoStyle       = Health
;HealthPerShot   = [ 0..usedpershot ]
;
;JamFrequency    = [ 0..100% ]
;JamSingleFire   = [ TRUE | FALSE ]
;JamRapidFire    = [ TRUE | FALSE ]
;
;[Ammo#]
;Name            = "AmmoNameMax24"
;Velocity        = [ ? ]
;DamageTypes     = [ Explosion, Burn, Bullet, Stab, Pummel, Cleave, Ecto]
;DamageRates     = [ 0,         0,    0,      0,    0,      0,      0 ]
*/

void WeaponProcess( PLAYER *pPlayer, INPUT *pInput )
{
	WeaponPlay(pPlayer);

	// decrement weapon ready timer
	pPlayer->weaponTimer -= kFrameTicks;

	if ( pPlayer->fLoopQAV )
	{
		if ( pInput->buttonFlags.shoot && pPlayer->ammoCount[pPlayer->weaponAmmo] )
		{
			while ( pPlayer->weaponTimer <= 0 )
				pPlayer->weaponTimer += pPlayer->pWeaponQAV->duration;
			UpdateAimVector(pPlayer);
			return;
		}
		pPlayer->weaponTimer = 0;
		pPlayer->fLoopQAV = FALSE;
	}
	else
		pPlayer->weaponTimer = ClipLow(pPlayer->weaponTimer, 0);

	// special processing for TNT
	if ( pPlayer->weapon == kWeaponTNT )
	{
		switch ( pPlayer->weaponState )
		{
			case kTNTStickFuse1:
				// player released alt fire key?
				if ( !pInput->buttonFlags.shoot2 )
					pPlayer->weaponState = kTNTStickFuse2;
				return;

			case kTNTStickFuse2:
				// dropping?
				if ( pInput->buttonFlags.shoot2 )
				{
					pPlayer->fuseTime = pPlayer->weaponTimer;
					StartQAV(pPlayer, kQAVStickDrop, (QAVCALLBACK)DropStick);
					pPlayer->weaponState = kBicIdle;
					return;
				}
				// throwing?
				if ( pInput->buttonFlags.shoot )
				{
					pPlayer->weaponState = kTNTStickThrow;
					pPlayer->fuseTime = 0;
					pPlayer->throwTime = gFrameClock;
				}
				return;

			case kTNTStickThrow:
				// releasing?
				if ( !pInput->buttonFlags.shoot )
				{
					if ( pPlayer->fuseTime == 0 )
						pPlayer->fuseTime = pPlayer->weaponTimer;
					pPlayer->throwTime = gFrameClock - pPlayer->throwTime;
					sfxCreate3DSound(pPlayer->sprite->x, pPlayer->sprite->y, pPlayer->sprite->z,
						kSfxTNTToss);
					StartQAV(pPlayer, kQAVStickThrow, (QAVCALLBACK)ThrowStick);
					pPlayer->weaponState = kBicIdle;
				}
				return;

			case kTNTBundleFuse1:
				// player released alt fire key?
				if ( !pInput->buttonFlags.shoot2 )
					pPlayer->weaponState = kTNTBundleFuse2;
				return;

			case kTNTBundleFuse2:
				// dropping?
				if ( pInput->buttonFlags.shoot2 )
				{
					pPlayer->fuseTime = pPlayer->weaponTimer;
					StartQAV(pPlayer, kQAVBundleDrop, (QAVCALLBACK)DropBundle);
					pPlayer->weaponState = kBicIdle;
					return;
				}

				// throwing?
				if ( pInput->buttonFlags.shoot )
				{
					pPlayer->weaponState = kTNTBundleThrow;
					pPlayer->fuseTime = 0;
					pPlayer->throwTime = gFrameClock;
				}
				return;

			case kTNTBundleThrow:
				// releasing?
				if ( !pInput->buttonFlags.shoot )
				{
					if ( pPlayer->fuseTime == 0 )
						pPlayer->fuseTime = pPlayer->weaponTimer;
					pPlayer->throwTime = gFrameClock - pPlayer->throwTime;
					StartQAV(pPlayer, kQAVBundleThrow, (QAVCALLBACK)ThrowBundle);
					pPlayer->weaponState = kBicIdle;
				}
				return;

			case kTNTProxThrow:
				// releasing?
				if ( !pInput->buttonFlags.shoot )
				{
					pPlayer->throwTime = gFrameClock - pPlayer->throwTime;
					StartQAV(pPlayer, kQAVBundleThrow, (QAVCALLBACK)ThrowProx);
					pPlayer->weaponState = kTNTProxEmpty;
				}
				return;

			case kTNTRemoteThrow:
				// releasing?
				if ( !pInput->buttonFlags.shoot )
				{
					pPlayer->throwTime = gFrameClock - pPlayer->throwTime;
					StartQAV(pPlayer, kQAVBundleThrow, (QAVCALLBACK)ThrowRemote);
					pPlayer->weaponState = kTNTRemoteEmpty;
				}
				return;
		}
	}

	// return if weapon is busy
	if (pPlayer->weaponTimer > 0)
		return;

	pPlayer->pWeaponQAV = NULL;

	// pPlayer->weapon is set to kMaxWeapons in the fire processing below
	if ( pPlayer->weaponState == kFindWeapon )
	{
		dprintf("Out of ammo: looking for a new weapon\n");
		pPlayer->weaponState = 0;
		pInput->newWeapon = WeaponFindLoaded( pPlayer );
	}

	// weapon select
	if ( pInput->newWeapon )
	{
		if ( pPlayer->xsprite->health == 0 || !pPlayer->hasWeapon[ pInput->newWeapon ] )
		{
			pInput->newWeapon = 0;
			return;
		}

		if (pPlayer->weapon != 0)
			WeaponLower(pPlayer, pInput);	// lower the old weapon
		else
			WeaponRaise(pPlayer, pInput);	// raise new weapon
		return;
	}

	if ( pPlayer->weapon == 0 )
		return;		// no weapon drawn, do nothing

	UpdateAimVector(pPlayer);

	// fire key
	if ( pInput->buttonFlags.shoot )
	{
		if ( CheckAmmo(pPlayer, pPlayer->weaponAmmo) )
		{
			// firing weapon (primary)
			switch ( pPlayer->weapon )
			{
				case kWeaponPitchfork:
					StartQAV(pPlayer, kQAVForkStab, (QAVCALLBACK)FirePitchfork, FALSE);
					return;

				case kWeaponSprayCan:
					if ( pPlayer->weaponState == kCanIdle )
					{
						StartQAV(pPlayer, kQAVSprayFire, (QAVCALLBACK)FireSpray, TRUE);
						pPlayer->weaponState = kCanFire;
						return;
					}
					break;

				case kWeaponTNT:
					switch ( pPlayer->weaponState )
					{
						case kTNTStickIdle:
							StartQAV(pPlayer, kQAVStickFuse, (QAVCALLBACK)ExplodeStick);
		 					pPlayer->weaponState = kTNTStickThrow;
							pPlayer->throwTime = gFrameClock;
							pPlayer->fuseTime = -1;
							return;

						case kTNTBundleIdle:
							StartQAV(pPlayer, kQAVBundleFuse, (QAVCALLBACK)ExplodeBundle);
		 					pPlayer->weaponState = kTNTBundleThrow;
							pPlayer->throwTime = gFrameClock;
							pPlayer->fuseTime = -1;
							return;

						case kTNTProxIdle:
		 					pPlayer->weaponState = kTNTProxThrow;
							pPlayer->throwTime = gFrameClock;
							return;

						case kTNTRemoteIdle1:
		 					pPlayer->weaponState = kTNTRemoteThrow;
							pPlayer->throwTime = gFrameClock;
							return;

						case kTNTRemoteIdle2:
							StartQAV(pPlayer, kQAVRemoteFire, (QAVCALLBACK)FireRemote);
							pPlayer->weaponState = kTNTRemoteEmpty;
							return;
					}
					break;

			    case kWeaponShotgun:
					switch ( pPlayer->weaponState )
					{
						case kShotIdle2:
							StartQAV(pPlayer, kQAVShotFireL);
							pPlayer->weaponState = kShotIdle1;
							UseAmmo(pPlayer, kAmmoShell, 1);
							FireShotgun(pPlayer, 1);
							return;

						case kShotIdle1:
							StartQAV(pPlayer, kQAVShotFireR);
						    pPlayer->weaponState = kShotIdle0;
							UseAmmo(pPlayer, kAmmoShell, 1);
							FireShotgun(pPlayer, 1);
							return;
					}
					break;

				case kWeaponTommy:
					StartQAV(pPlayer, kQAVTommyFire1, (QAVCALLBACK)FireTommy, TRUE);
					return;

				case kWeaponFlare:
					StartQAV(pPlayer, kQAVFlareFire, (QAVCALLBACK)FireFlare);
					return;

				case kWeaponVoodoo:
				{
					int nStabType = Random(4);
					if ( pPlayer->aimSprite == -1 )
						nStabType = 4;

					StartQAV(pPlayer, kQAVDollStab1 + nStabType, (QAVCALLBACK)FireVoodoo);
					return;
				}

			    case kWeaponSpear:
					StartQAV(pPlayer, kQAVSpearFire, (QAVCALLBACK)FireSpear);
					return;

				case kWeaponShadow:
					StartQAV(pPlayer, kQAVShadowFire, (QAVCALLBACK)FireShadow);
					return;

				case kWeaponBeast:
					StartQAV(pPlayer, kQAVBeastAttack1 + Random(4));
					pPlayer->weaponState = kBeastIdle;
					return;
			}
		}
		else
		{
			dprintf("Primary out of ammo: setting weapon state to kFindWeapon\n");
			pPlayer->weaponState = kFindWeapon;
			return;
		}
	}

	// alternate fire key
	if ( pInput->buttonFlags.shoot2 )
	{
		if ( CheckAmmo(pPlayer, pPlayer->weaponAmmo) )
		{
			// firing weapon (alternate)
			switch ( pPlayer->weapon )
			{
				case kWeaponPitchfork:
					playerFireMissile(pPlayer,
						pPlayer->aim.dx, pPlayer->aim.dy, pPlayer->aim.dz,
						kMissileFireball);
					pPlayer->weaponTimer = 20;
					return;

				case kWeaponSprayCan:
					if ( pPlayer->weaponState == kCanIdle )
					{
						StartQAV(pPlayer, kQAVSprayFire, (QAVCALLBACK)FireSpray, TRUE);
						pPlayer->weaponState = kCanFire;
						return;
					}
					break;

				case kWeaponTNT:
					switch ( pPlayer->weaponState )
					{
						case kTNTStickIdle:
							StartQAV(pPlayer, kQAVStickFuse, (QAVCALLBACK)ExplodeStick);
							pPlayer->weaponState = kTNTStickFuse1;
							return;

						case kTNTBundleIdle:
							StartQAV(pPlayer, kQAVBundleFuse, (QAVCALLBACK)ExplodeBundle);
							pPlayer->weaponState = kTNTBundleFuse1;
							return;

						case kTNTProxIdle:
							StartQAV(pPlayer, kQAVProxDrop, (QAVCALLBACK)DropProx);
							pPlayer->weaponState = kTNTProxEmpty;
							return;

						case kTNTRemoteIdle1:
							StartQAV(pPlayer, kQAVRemoteDrop, (QAVCALLBACK)DropRemote);
							pPlayer->weaponState = kTNTRemoteIdle2;
							return;

						case kTNTRemoteIdle2:
							// raise another bundle
							StartQAV(pPlayer, kQAVRemoteUp1);
							pPlayer->weaponState = kTNTRemoteIdle1;
							return;
					}
					break;

				case kWeaponShotgun:
					switch ( pPlayer->weaponState )
					{
						case kShotIdle2:
							StartQAV(pPlayer, kQAVShotFireR);
							pPlayer->weaponState = kShotIdle0;
							UseAmmo(pPlayer, kAmmoShell, 2);
							FireShotgun(pPlayer, 2);
							return;
						case kShotIdle1:
							StartQAV(pPlayer, kQAVShotFireR);
							pPlayer->weaponState = kShotIdle0;
							UseAmmo(pPlayer, kAmmoShell, 1);
							FireShotgun(pPlayer, 1);
							return;
					}
					break;

				case kWeaponTommy:
					StartQAV(pPlayer, kQAVTommyFire1, (QAVCALLBACK)FireTommy, TRUE);
					return;

				case kWeaponFlare:
					StartQAV(pPlayer, kQAVFlareFire, (QAVCALLBACK)FireFlare2);
					return;

				case kWeaponVoodoo:
					StartQAV(pPlayer, kQAVDollSpell, (QAVCALLBACK)FireReanimator, FALSE);
					return;

			    case kWeaponSpear:
					StartQAV(pPlayer, kQAVSpearFire, (QAVCALLBACK)FireSpear);
					return;

				case kWeaponShadow:
					StartQAV(pPlayer, kQAVShadowFire, (QAVCALLBACK)FireShadow);
					return;
			}
		}
		else
		{
			dprintf("Alternate out of ammo: setting weapon state to kFindWeapon\n");
			pPlayer->weaponState = kFindWeapon;
			return;
		}
	}

	WeaponUpdateState(pPlayer);
}
