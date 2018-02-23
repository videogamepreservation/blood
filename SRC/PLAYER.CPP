#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "actor.h"
#include "ai.h"
#include "db.h"
#include "debug4g.h"
#include "dude.h"
#include "engine.h"
#include "error.h"
#include "eventq.h"
#include "gameutil.h"
#include "globals.h"
#include "misc.h"
#include "multi.h"
#include "names.h"
#include "options.h"
#include "player.h"
#include "screen.h"
#include "sectorfx.h"
#include "seq.h"
#include "tile.h"
#include "trig.h"
#include "triggers.h"
#include "view.h"
#include "warp.h"
#include "weapon.h"
#include "aizomba.h"	// currently required for lackeys
#include "sfx.h"
#include "sound.h"	// can be removed once sfx complete


#define kLookMax		60
#define kHorizUpMax		120
#define kHorizDownMax	180

// this should probably go into the posture table too
#define kClimbSpeed		160

#define kPowerUpTime	(kTimerRate * 30)
#define kMaxPowerUpTime	((kTimerRate * 60) * 60)


PLAYER	gPlayer[kMaxPlayers];
PLAYER	*gMe = NULL;
PLAYER	*gView = NULL;
INPUT	gPlayerInput[kMaxPlayers];


/*******************************************************************************
Notes on the posture table:

Acceleration units are units per clock tick.  If the timer rate is changed,
these values will need to change as well.
*******************************************************************************/

POSTURE gPosture[] =
{
	// player posture
	{
		10,       	// frontAccel		25,
		5,			// sideAccel		30,
		5,	    	// backAccel		20,
		350,     	// frontSpeed[0]    350,
		800,     	// frontSpeed[1]    700,
		300,     	// sideSpeed[0]     300,
		500,     	// sideSpeed[1]     500,
		250,     	// backSpeed[0]     250,
		500,     	// backSpeed[1]     500,
		100,     	// pace[0]
		140,     	// pace[1]
		3,       	// bobV
		2,       	// bobH
		4,			// swayV
		10,			// swayH
	},
	// beast posture  ******** THESE ALL NEED TO LOOK LIKE ABOVE (TO DO!) **********
	{
		20,       	// frontAccel		20,
		40,			// sideAccel		40,
		24,	    	// backAccel		24,
		500,     	// frontSpeed[0]    500,
		1000,     	// frontSpeed[1]    1000,
		400,     	// sideSpeed[0]     400,
		800,     	// sideSpeed[1]     800,
		300,     	// backSpeed[0]     300,
		600,     	// backSpeed[1]     600,
		100,     	// pace[0]
		80,     	// pace[1]
		12,       	// bobV
		4,       	// bobH
		8,			// swayV
		16,			// swayH
	},
};

AMMOINFO gAmmoInfo[kAmmoMax] =
{
//	 max,	vectorType
	{ kTimerRate*40, kVectorNone },	// kAmmoSprayCan
	{ 24,	kVectorNone },			// kAmmoTNTStick
	{ 8,	kVectorNone },			// kAmmoTNTProximity
	{ 8,	kVectorNone },			// kAmmoTNTRemote
	{ 50,	kVectorShell },			// kAmmoShell
	{ 500,	kVectorBullet },		// kAmmoBullet
	{ 500,	kVectorBulletAP },		// kAmmoBulletAP
	{ 24,	kVectorNone },			// kAmmoFlare
	{ 24,	kVectorNone },			// kAmmoFlareSB
	{ 10,	kVectorNone },			// kAmmoVoodoo
	{ 24,	kVectorNone },			// kAmmoSpear
	{ 24,	kVectorNone },			// kAmmoSpearXP
};



#if 0
enum POWERUP {
	kPupFeatherFall = 0,
	kPupJumpBoots,
	kPupInvisible,
	kPupInvulnerable,
	kPupJumpBoots,
	kPupRavenFlight,
	kPupGunsAkimbo,
	kPupDivingSuit,
	kPupGasMask,
	kPupClone,
	kPupCrystalBall,
	kPupDecoy,
	kPupDoppleganger,
	kPupReflectiveShots,
	kPupRoseGlasses,
	kPupShadowCloak,
	kPupShroomRage,
	kPupShroomDelirium,
	kPupShroomGrow,
	kPupShroomShrink,
	kPupDeathMask,
	kPupAsbestosArmor,
	kMaxPowerUps
};

enum POWERUPFLAGS {
	kPowerPermanent =	0x0001,
	kPowerUnique =		0x0002,
};
#endif

struct POWERUPINFO {
	short	picnum;
	long	zoom;
	BOOL	isUnique;
	int		addPower;
	int		maxPower;
};

POWERUPINFO gPowerUpInfo[ kMaxPowerUps ] =
{
	{-1,				0,	TRUE,	1,		1},							// kItemKey1
	{-1,				0,	TRUE,	1,		1},							// kItemKey2
	{-1,				0,	TRUE,	1,		1},							// kItemKey3
	{-1,				0,	TRUE,	1,		1},							// kItemKey4
	{-1,				0,	TRUE,	1,		1},							// kItemKey5
	{-1,				0,	TRUE,	1,		1},							// kItemKey6
	{-1,				0,	TRUE,	1,		1},							// kItemKey7
	{-1,				0,	FALSE,	10,	100},  							// kItemDoctorBag           INVENTORY
	{-1,				0,	FALSE,	5,	100},     						// kItemMedPouch
	{-1,				0,	FALSE,	20,	100},     						// kItemLifeEssence
	{-1,				0,	FALSE,	100,	200},     					// kItemLifeSeed
	{-1,				0,	FALSE,	2,	200},     						// kItemPotion1
	{kAnmFeather,		0,	FALSE,	kPowerUpTime,	kMaxPowerUpTime},	// kItemFeatherFall         INVENTORY
	{-1,				0,	FALSE,	kPowerUpTime,	kMaxPowerUpTime},	// kItemLtdInvisibility
	{-1,				0,	FALSE,	kPowerUpTime,	kMaxPowerUpTime},	// kItemInvulnerability
	{kPicJumpBoots,		0,	FALSE,	kPowerUpTime,	kMaxPowerUpTime},	// kItemJumpBoots           INVENTORY
	{kPicRavenFlight,	0,	FALSE,	kPowerUpTime,	kMaxPowerUpTime},	// kItemRavenFlight         INVENTORY
	{-1,				0,	FALSE,	kPowerUpTime,	kMaxPowerUpTime},	// kItemGunsAkimbo
	{-1,				0,	FALSE,	kPowerUpTime,	kMaxPowerUpTime},	// kItemDivingSuit			INVENTORY
	{-1,				0,	FALSE,	kPowerUpTime,	kMaxPowerUpTime},	// kItemGasMask				INVENTORY or CUT
	{-1,				0,	FALSE,	kPowerUpTime,	kMaxPowerUpTime},	// kItemClone
	{-1,				0,	FALSE,	kPowerUpTime,	kMaxPowerUpTime},	// kItemCrystalBall			INVENTORY
	{-1,				0,	FALSE,	kPowerUpTime,	kMaxPowerUpTime},	// kItemDecoy               INVENTORY
	{kAnmDoppleganger,	0,	FALSE,	kPowerUpTime,	kMaxPowerUpTime},	// kItemDoppleganger
	{kAnmReflectShots,	0,	FALSE,	kPowerUpTime,	kMaxPowerUpTime},	// kItemReflectiveShots
	{-1,				0,	FALSE,	kPowerUpTime,	kMaxPowerUpTime},	// kItemRoseGlasses         INVENTORY
	{-1,				0,	FALSE,	kPowerUpTime,	kMaxPowerUpTime},	// kItemShadowCloak
	{-1,				0,	FALSE,	kPowerUpTime,	kMaxPowerUpTime},	// kItemShroomRage
	{-1,				0,	FALSE,	kPowerUpTime/4,	kMaxPowerUpTime},	// kItemShroomDelirium
	{-1,				0,	FALSE,	kPowerUpTime,	kMaxPowerUpTime},	// kItemShroomGrow
	{-1,				0,	FALSE,	kPowerUpTime,	kMaxPowerUpTime},	// kItemShroomShrink
	{-1,				0,	FALSE,	kPowerUpTime,	kMaxPowerUpTime},	// kItemDeathMask           INVENTORY
	{-1,				0,	FALSE,	kPowerUpTime,	kMaxPowerUpTime},	// kItemWineGoblet
	{-1,				0,	FALSE,	kPowerUpTime,	kMaxPowerUpTime},	// kItemWineBottle
	{-1,				0,	FALSE,	kPowerUpTime,	kMaxPowerUpTime},	// kItemSkullGrail
	{-1,				0,	FALSE,	kPowerUpTime,	kMaxPowerUpTime},	// kItemSilverGrail
	{-1,				0,	FALSE,	kPowerUpTime,	kMaxPowerUpTime},	// kItemTome
	{-1,				0,	FALSE,	kPowerUpTime,	kMaxPowerUpTime},	// kItemBlackChest
	{-1,				0,	FALSE,	kPowerUpTime,	kMaxPowerUpTime},	// kItemWoodenChest
	{-1,				0,	FALSE,	kPowerUpTime,	kMaxPowerUpTime},	// kItemAsbestosArmor		INVENTORY
};


int powerupCheck( PLAYER *pPlayer, int nPowerUp )
{
	dassert( pPlayer != NULL );
	dassert( nPowerUp >= 0 && nPowerUp < kMaxPowerUps );
	return pPlayer->powerUpTimer[nPowerUp];
}


void powerupDraw( PLAYER *pView )
{
	for (int nPowerUp = kItemMax - kItemBase - 1; nPowerUp >= 0; nPowerUp--)
	{
		int nPowerRemaining = powerupCheck( pView, nPowerUp );
		if ( nPowerRemaining )
		{
			POWERUPINFO *pInfo = &gPowerUpInfo[ nPowerUp ];
			switch( nPowerUp + kItemBase )	// switch off of actual type
			{
				case kItemFeatherFall:
				case kItemLtdInvisibility:
				case kItemInvulnerability:
				case kItemJumpBoots:
				case kItemRavenFlight:
				case kItemGunsAkimbo:
				case kItemDivingSuit:
				case kItemGasMask:
				case kItemClone:
				case kItemCrystalBall:
				case kItemDecoy:
				case kItemDoppleganger:
				case kItemReflectiveShots:
				case kItemRoseGlasses:
				case kItemShadowCloak:
				case kItemShroomRage:
				case kItemShroomDelirium:
				case kItemShroomGrow:
				case kItemShroomShrink:
				case kItemDeathMask:
				case kItemWineGoblet:
				case kItemWineBottle:
				case kItemSkullGrail:
				case kItemSilverGrail:
				case kItemTome:
				case kItemBlackChest:
				case kItemWoodenChest:
				case kItemAsbestosArmor:
				{
					int nFlags = kRotateNormal;
					if ( nPowerRemaining < (kTimerRate * 8) && ( gGameClock & 32 ) )
						nFlags |= kRotateTranslucent;

					short nTile = pInfo->picnum;
					if (nTile >= 0)
					{
						nTile += animateoffs(nTile, 0);
						rotatesprite(xdim<<15, ydim<<15, pInfo->zoom, 0, nTile, -128, kPLUNormal,
							(char)nFlags, gViewX0, gViewY0, gViewX1, gViewY1);
					}
				}
					break;

				default:
					break;
			}
		}
	}
}


BOOL powerupActivate( PLAYER *pPlayer, int nPowerUp )
{
//	char buffer[80];

	// skip the power-up if it is unique and already activated
	if ( powerupCheck( pPlayer, nPowerUp ) > 0 && gPowerUpInfo[nPowerUp].isUnique )
		return FALSE;

	pPlayer->powerUpTimer[nPowerUp] = ClipHigh( pPlayer->powerUpTimer[nPowerUp] +
		gPowerUpInfo[nPowerUp].addPower, gPowerUpInfo[nPowerUp].maxPower );

	DUDEINFO *pDudeInfo = &dudeInfo[pPlayer->sprite->type - kDudeBase];
	switch( nPowerUp + kItemBase )	// switch off of actual type
	{
		case kItemFeatherFall:
		case kItemJumpBoots:
			pDudeInfo->damageShift[ kDamageFall ] += kNoDamage;
			break;
		case kItemInvulnerability:
		{
			for ( int i = 0; i < kDamageMax; i++ )
				pDudeInfo->damageShift[ i ] += kNoDamage;
			break;
		}
		case kItemDivingSuit:
			pDudeInfo->damageShift[ kDamageDrown ] += kNoDamage;
			break;
		case kItemGasMask:
			pDudeInfo->damageShift[ kDamageGas ] += kNoDamage;
			break;
		case kItemAsbestosArmor:
			pDudeInfo->damageShift[ kDamageBurn ] += kNoDamage;
			break;
		default:
			break;
	}

	SPRITE *pSprite = pPlayer->sprite;
	sfxCreate3DSound(pSprite->x, pSprite->y, pSprite->z, kSfxPlayPowerUp);
//	sprintf( buffer, "Activating %s for %i",
//		gItemText[ nPowerUp ], pPlayer->powerUpTimer[nPowerUp] );
//	viewSetMessage(buffer);
	return TRUE;
}


void powerupDeactivate( PLAYER *pPlayer, int nPowerUp )
{
	DUDEINFO *pDudeInfo = &dudeInfo[pPlayer->sprite->type - kDudeBase];

	switch( nPowerUp + kItemBase )	// switch off of actual type
	{
		case kItemFeatherFall:
		case kItemJumpBoots:
			pDudeInfo->damageShift[ kDamageFall ] -= kNoDamage;
			break;
		case kItemInvulnerability:
		{
			for ( int i = 0; i < kDamageMax; i++ )
				pDudeInfo->damageShift[ i ] -= kNoDamage;
			break;
		}
		case kItemDivingSuit:
			pDudeInfo->damageShift[ kDamageDrown ] -= kNoDamage;
			break;
		case kItemGasMask:
			pDudeInfo->damageShift[ kDamageGas ] -= kNoDamage;
			break;
		case kItemAsbestosArmor:
			pDudeInfo->damageShift[ kDamageBurn ] -= kNoDamage;
			break;
		default:
			break;
	}

	pPlayer->powerUpTimer[ nPowerUp ] = 0;
}


void powerupProcess( PLAYER *pPlayer )
{
	for (int nPowerUp = kItemMax - kItemBase - 1; nPowerUp >= 0; nPowerUp--)
	{
		if ( pPlayer->powerUpTimer[nPowerUp] )
		{
			pPlayer->powerUpTimer[nPowerUp] = ClipLow(pPlayer->powerUpTimer[nPowerUp] - kFrameTicks, 0);
			if ( !powerupCheck( pPlayer, nPowerUp ) )
				powerupDeactivate( pPlayer, nPowerUp );
		}
	}
}


void powerupClear( PLAYER *pPlayer )
{
	for (int nPowerUp = kItemMax - kItemBase - 1; nPowerUp >= 0; nPowerUp--)
	{
		pPlayer->powerUpTimer[nPowerUp] = 0;
	}
}


void powerupInit( void )
{
	for (int nPowerUp = kItemMax - kItemBase - 1; nPowerUp >= 0; nPowerUp--)
	{
		POWERUPINFO *pInfo =  &gPowerUpInfo[ nPowerUp ];

		dassert( pInfo->picnum < kMaxTiles );

		if ( pInfo->picnum >= 0 )
		{
			int maxSize = 0;

			PICANM *pAnm = &picanm[ pInfo->picnum ];
			int nFrame = (pAnm->type != 0) ? pAnm->frames : 0;

			for (; nFrame >= 0; nFrame-- )
			{
				if (pAnm->type == 3)	// special case for backward anims
				{
					maxSize = tilesizx[ pInfo->picnum - nFrame ];
					if ( maxSize < tilesizy[ pInfo->picnum - nFrame ] )
						maxSize = tilesizy[ pInfo->picnum - nFrame ];
				}
				else
				{
					maxSize = tilesizx[ pInfo->picnum + nFrame ];
					if ( maxSize < tilesizy[ pInfo->picnum + nFrame ] )
						maxSize = tilesizy[ pInfo->picnum + nFrame ];
				}
			}

			dassert( maxSize > 0 );
			pInfo->zoom = ( 24 /*kPowerUpFrameSize*/ << 16 ) / maxSize;
		}
	}
}


void playerSetRace( PLAYER *pPlayer, int nLifeMode, int nType )
{
	dassert( nLifeMode >= kModeHuman && nLifeMode <= kModeBeast );
	int dudeIndex = nType - kDudeBase;
	pPlayer->lifemode = nLifeMode;
	dudeInfo[dudeIndex] = gPlayerTemplate[ nLifeMode ];
}


void playerSetGodMode( PLAYER *pPlayer, BOOL nMode )
{
	DUDEINFO *pDudeInfo = &dudeInfo[pPlayer->sprite->type - kDudeBase];

	if ( nMode )
	{
		for ( int i = 0; i < kDamageMax; i++ )
			pDudeInfo->damageShift[i] += kNoDamage;
	}
	else
	{
		for ( int i = 0; i < kDamageMax; i++ )
			pDudeInfo->damageShift[i] -= kNoDamage;
	}
	pPlayer->godMode = nMode;
	pPlayer->xsprite->health = pDudeInfo->startHealth << 4;
}


void playerReset( PLAYER *pPlayer, INPUT *pInput, int type )
{
	dprintf("Resetting player %d\n", type - kDudePlayer1);

	// get the normal player starting position, else if in bloodbath mode,
	// randomly pick one of kMaxPlayers starting positions
	ZONE *pZone;
	if (gNetMode == kNetModeOff || gNetMode == kNetModeCoop)
		pZone = &gStartZone[ type - kDudePlayer1 ];
	else
		pZone = &gStartZone[ Random(numplayers)];

	if ( pPlayer->sprite != NULL )
	{
		// ADD: make the dead body a non-player type
	}

	int nSprite = actSpawnSprite( pZone->sector, pZone->x, pZone->y, pZone->z, kStatDude, TRUE );

	SPRITE *pSprite = &sprite[nSprite];
	dassert(pSprite->extra > 0 && pSprite->extra < kMaxXSprites);
	XSPRITE *pXSprite = &xsprite[pSprite->extra];

	int dudeIndex = type - kDudeBase;
	playerSetRace( pPlayer, kModeHuman, type );
	seqSpawn(dudeInfo[dudeIndex].seqStartID + kSeqDudeIdle, SS_SPRITE, pSprite->extra);

	// make the player's own sprite visible
	if ( pPlayer == gMe )
		SetBitString(show2dsprite, nSprite);

	// raise the player up off the floor
	int zTop, zBot;
	GetSpriteExtents(pSprite, &zTop, &zBot);
	pSprite->z -= zBot - pSprite->z;

	pPlayer->sprite = pSprite;
	pPlayer->nSprite = nSprite;
	pPlayer->xsprite = pXSprite;

	pSprite->ang = pZone->angle;
	pSprite->type = (short)type;

	pSprite->clipdist = dudeInfo[dudeIndex].clipdist;
	pSprite->flags = kAttrMove | kAttrGravity | kAttrFalling;

	pXSprite->health = dudeInfo[dudeIndex].startHealth << 4;
	pXSprite->moveState = kMoveWalk;
	pXSprite->burnTime = 0;
	pXSprite->burnSource = -1;

	pPlayer->eyeAboveZ = dudeInfo[dudeIndex].eyeHeight * pSprite->yrepeat << 2;
	pPlayer->weaponAboveZ = pPlayer->eyeAboveZ / 2;
	pPlayer->bloodlust = 0;
	pPlayer->impactPE = 0;
	pPlayer->impactPhase = 0;
	pPlayer->standSpeed = 0;
	pPlayer->compression = 0;
	pPlayer->viewOffZ = 0;
	pPlayer->weapOffZ = 0;
	pPlayer->viewOffdZ = 0;
	pPlayer->weapOffdZ = 0;
	pPlayer->horiz = 0;
	pPlayer->look = 0;
	pPlayer->slope = 0;
	pPlayer->fraggerID = -1;
	pPlayer->airTime = 0;
	pPlayer->bloodTime = 0;
	pPlayer->gooTime = 0;
	pPlayer->wetTime = 0;
	pPlayer->godMode = FALSE;

	pPlayer->relAim.dx = 1 << 14;
	pPlayer->relAim.dy = 0;
	pPlayer->relAim.dz = 0;
	pPlayer->aimSprite = -1;

	for (int i = 0; i < 8; i++)
		pPlayer->hasKey[i] = 0;
	pPlayer->weapon = kWeaponNone;

	pPlayer->deathTime = 0;
	pSprite->xvel = 0;
	pSprite->yvel = 0;
 	pSprite->zvel = 0;

	pInput->forward = 0;
	pInput->strafe = 0;
	pInput->turn = 0;

	pInput->syncFlags.byte = 0;
	pInput->buttonFlags.byte = 0;
	pInput->keyFlags.byte = 0;
	pInput->newWeapon = kWeaponPitchfork;

	scrDacAbsEffect(0, 0, 0);

	for (i = 0; i < kWeaponMax; i++)
	{
		pPlayer->hasWeapon[i] = gInfiniteAmmo;
		pPlayer->weaponMode[i] = 0;
	}
	pPlayer->hasWeapon[kWeaponPitchfork] = TRUE;

	for (i = 0; i < kAmmoMax; i++)
	{
		pPlayer->ammoCount[i] = gInfiniteAmmo ? gAmmoInfo[i].max : 0;
	}
	pPlayer->weaponTimer = 0;	// idle
	pPlayer->weaponState = 0;
	pPlayer->pWeaponQAV = NULL;

	for (i = 0; i < kMaxPowerUps; i++)
		pPlayer->powerUpTimer[i] = 0;
	for (i = 0; i < kMaxLackeys; i++)
		pPlayer->nLackeySprites[i] = -1;
}


void playerInit( int nPlayer )
{
	PLAYER *pPlayer = &gPlayer[nPlayer];

	dprintf("Initializing player %d\n", nPlayer);

	pPlayer->sprite = NULL;
	pPlayer->teamID = nPlayer;
	pPlayer->fragCount = 0;
	memset(pPlayer->fragInfo, 0, sizeof(pPlayer->fragInfo));

	playerReset(pPlayer, &gPlayerInput[nPlayer], kDudePlayer1 + nPlayer);
}


static int CheckTouchSprite( SPRITE *pSprite )
{
	int i, next;
	int dx, dy, dz;

	for (i = headspritestat[kStatItem]; i >= 0; i = next)
	{
		next = nextspritestat[i];

		dx = qabs(pSprite->x - sprite[i].x) >> 4;
		if (dx < kTouchXYDist)
		{
			dy = qabs(pSprite->y - sprite[i].y) >> 4;
			if (dy < kTouchXYDist)
			{
				int zTop, zBot;
				GetSpriteExtents(pSprite, &zTop, &zBot);
				dz = 0;
				if ( sprite[i].z < zTop )
					dz = (zTop - sprite[i].z) >> 8;
				else if ( sprite[i].z > zBot )
					dz = (sprite[i].z - zBot) >> 8;
				if (dz < kTouchZDist)
				{
					if (qdist(dx, dy) < kTouchXYDist)
					{
						int zTop, zBot;
						GetSpriteExtents(&sprite[i], &zTop, &zBot);
						if (
							// center
							cansee(pSprite->x, pSprite->y,  pSprite->z, pSprite->sectnum,
								sprite[i].x, sprite[i].y, sprite[i].z, sprite[i].sectnum) ||

							// top
							cansee(pSprite->x, pSprite->y,  pSprite->z, pSprite->sectnum,
								sprite[i].x, sprite[i].y, zTop, sprite[i].sectnum) ||

							// bottom
							cansee(pSprite->x, pSprite->y,  pSprite->z, pSprite->sectnum,
								sprite[i].x, sprite[i].y, zBot, sprite[i].sectnum)
						)
						return i;
					}
				}
			}
		}
	}
	return -1;
}


static BOOL PickupItem( PLAYER *pPlayer, int nSprite, int nItemType )
{
	SPRITE *pSprite = pPlayer->sprite;
	XSPRITE *pXSprite = pPlayer->xsprite;

	int nPowerUp = nItemType - kItemBase;
	switch( nItemType )
	{
		case kItemKey1:
		case kItemKey2:
		case kItemKey3:
		case kItemKey4:
		case kItemKey5:
		case kItemKey6:
		case kItemKey7:
			if ( pPlayer->hasKey[nItemType - kItemKey1 + 1] )
				return FALSE;
			pPlayer->hasKey[nItemType - kItemKey1 + 1] = sprite[nSprite].picnum;
			break;

		case kItemDoctorBag:
		case kItemPotion1:
		case kItemMedPouch:
		case kItemLifeEssence:
		case kItemLifeSeed:
			if ( !actHealDude(pXSprite, gPowerUpInfo[nPowerUp].addPower, gPowerUpInfo[nPowerUp].maxPower) )
				return FALSE;
			break;

		default:
			if ( !powerupActivate( pPlayer, nPowerUp ) )
				return FALSE;
			break;
	}
	sfxCreate3DSound(pSprite->x, pSprite->y, pSprite->z, kSfxPlayItemUp);
	return TRUE;
}


static BOOL PickupAmmo( PLAYER *pPlayer, int /*nSprite*/, int nAmmoItemType )
{
	AMMOITEMDATA *pAmmoData = &gAmmoItemData[nAmmoItemType - kAmmoItemBase];
	int nAmmoType = pAmmoData->ammoType;

	if ( pPlayer->ammoCount[nAmmoType] >= gAmmoInfo[nAmmoType].max )
		return FALSE;

	pPlayer->ammoCount[nAmmoType] = ClipHigh(
		pPlayer->ammoCount[nAmmoType] + pAmmoData->count, gAmmoInfo[nAmmoType].max );

	// set the hasWeapon flags for weapons which are ammo
	if ( pAmmoData->weaponType != kWeaponNone )
		pPlayer->hasWeapon[pAmmoData->weaponType] = TRUE;

	SPRITE *pSprite = pPlayer->sprite;
	sfxCreate3DSound(pSprite->x, pSprite->y, pSprite->z, kSfxPlayItemUp);
	return TRUE;
}


static BOOL PickupWeapon( PLAYER *pPlayer, int nSprite, int nWeaponItem )
{
	WEAPONITEMDATA *pWeaponData = &gWeaponItemData[nWeaponItem - kWeaponItemBase];
	int nWeapon = pWeaponData->weaponType;
	int nAmmo = pWeaponData->ammoType;

	// add weapon to player inventory
	if ( !pPlayer->hasWeapon[nWeapon] )
	{
		pPlayer->hasWeapon[nWeapon] = TRUE;

		if ( nAmmo != kAmmoNone )
		{
			// add preloaded ammo
			pPlayer->ammoCount[nAmmo] = ClipHigh(
				pPlayer->ammoCount[nAmmo] + pWeaponData->count, gAmmoInfo[nAmmo].max );
		}
		SPRITE *pSprite = pPlayer->sprite;
//		sfxCreate3DSound(pSprite->x, pSprite->y, pSprite->z, kSfxWeaponUp);
		return TRUE;
	}

	if ( nAmmo == kAmmoNone )
		return FALSE;

	if ( pPlayer->ammoCount[nAmmo] >= gAmmoInfo[nAmmo].max )
		return FALSE;

	BOOL isPermanent = FALSE;

	if ( sprite[nSprite].extra > 0)
	{
		XSPRITE *pXItem = &xsprite[sprite[nSprite].extra];
		if ( pXItem->respawn == kRespawnPermanent )
			isPermanent = TRUE;
	}

	if ( !isPermanent )
	{
		// add preloaded ammo
		pPlayer->ammoCount[nAmmo] = ClipHigh(
			pPlayer->ammoCount[nAmmo] + pWeaponData->count, gAmmoInfo[nAmmo].max );
		SPRITE *pSprite = pPlayer->sprite;
//		sfxCreate3DSound(pSprite->x, pSprite->y, pSprite->z, kSfxWeaponUp);
		return TRUE;
	}

	return FALSE;
}


static void PickUp( PLAYER *pPlayer )
{
	int x, y, z;
	SPRITE *pSprite = pPlayer->sprite;
	XSPRITE *pXSprite = pPlayer->xsprite;
	char buffer[80];

	x = pSprite->x;
	y = pSprite->y;
	z = pSprite->z;

	int nSprite = CheckTouchSprite(pSprite);

	if (nSprite >= 0)
	{
		BOOL bPickedUp = FALSE;

		int nType = sprite[nSprite].type;

		if (nType >= kItemBase && nType <= kItemMax)
		{
			bPickedUp = PickupItem( pPlayer, nSprite, nType );
			sprintf(buffer, "Picked up %s", gItemText[ nType - kItemBase ] );
		}
		else if (nType >= kAmmoItemBase && nType < kAmmoItemMax) {
			bPickedUp = PickupAmmo( pPlayer, nSprite, nType );
			sprintf(buffer, "Picked up %s", gAmmoText[ nType - kAmmoItemBase ] );
		}
		else if (nType >= kWeaponItemBase && nType < kWeaponItemMax)
		{
			bPickedUp = PickupWeapon( pPlayer, nSprite, nType );
			sprintf(buffer, "Picked up %s", gWeaponText[ nType - kWeaponItemBase ] );
		}

		if (bPickedUp)
		{
			if ( sprite[nSprite].extra > 0)
			{
				XSPRITE *pXItem = &xsprite[sprite[nSprite].extra];
				if ( pXItem->triggerPickup )
					trTriggerSprite(nSprite, pXItem, kCommandSpritePickup);
			}

			if ( !actCheckRespawn( nSprite ) )
				actPostSprite( nSprite, kStatFree );

			if (pPlayer == gMe)
			{
				viewSetMessage(buffer);
				scrDacRelEffect(16, 16, -16);
			}
		}
	}
}


static int ActionScan( PLAYER *pPlayer, int *nIndex, int *nXIndex)
{
	SPRITE *pSprite = pPlayer->sprite;
	HITINFO hitInfo;

	*nIndex = 0;
	*nXIndex = 0;

	int dx = Cos(pSprite->ang) >> 16;
	int dy = Sin(pSprite->ang) >> 16;
	int dz = pPlayer->slope;

	int hitType = HitScan(pSprite, pSprite->z - pPlayer->eyeAboveZ, dx, dy, dz, &hitInfo);
	int hitDist = qdist(pSprite->x - hitInfo.hitx, pSprite->y - hitInfo.hity) >> 4;

	if ( hitDist < kPushXYDist )
	{
		switch ( hitType )
		{
			case SS_SPRITE:
				*nIndex = hitInfo.hitsprite;
				*nXIndex = sprite[*nIndex].extra;
				if (*nXIndex > 0 && xsprite[*nXIndex].triggerPush )
					return SS_SPRITE;

				if ( sprite[*nIndex].statnum == kStatDude )
				{
					SPRITE *pDude = &sprite[*nIndex];
					XSPRITE *pXDude = &xsprite[*nXIndex];
					dprintf("DUDE %d: ang=%d, goalAng=%d\n", *nIndex, pDude->ang, pXDude->goalAng);
				}
				break;

			case SS_MASKED:
			case SS_WALL:
				*nIndex = hitInfo.hitwall;
				*nXIndex = wall[*nIndex].extra;
				if (*nXIndex > 0 && xwall[*nXIndex].triggerPush )
					return SS_WALL;

				if ( wall[*nIndex].nextsector >= 0)
				{
					*nIndex = wall[*nIndex].nextsector;
					*nXIndex = sector[*nIndex].extra;
					if (*nXIndex > 0 && xsector[*nXIndex].triggerWPush )
						return SS_SECTOR;
				}
				break;

			case SS_FLOOR:
			case SS_CEILING:
				*nIndex = hitInfo.hitsect;
				*nXIndex = sector[*nIndex].extra;
				if (*nXIndex > 0 && xsector[*nXIndex].triggerPush )
					return SS_SECTOR;
				break;
		}

		if ( pPlayer == gMe )
			sfxStart3DSound(pPlayer->sprite->extra, kSfxPlayHunt, 0x1000);	// push grunt
	}

	*nIndex = pSprite->sectnum;
	*nXIndex = sector[*nIndex].extra;
	if (*nXIndex > 0 && xsector[*nXIndex].triggerPush )
		return SS_SECTOR;

	return -1;
}


static void ProcessInput( PLAYER *pPlayer, INPUT *pInput )
{
	long vel, svel, sin, cos;
	POSTURE *cp = &gPosture[pPlayer->lifemode];
	SPRITE *pSprite = pPlayer->sprite;
	XSPRITE *pXSprite = pPlayer->xsprite;
	pPlayer->run = FALSE;
	BOOL run;

	WeaponProcess(pPlayer, pInput);

	// quick hack for death
	if (pXSprite->health == 0)
	{
		if ( pPlayer->deathTime == 0 )
		{
//			pXSprite->avel = (pXSprite->avel + BiRandom(40)) & kAngleMask;
		}

		pPlayer->deathTime += kFrameTicks;
		pPlayer->horiz = mulscale16((1 << 15) - (Cos(ClipHigh(pPlayer->deathTime << 3, kAngle180)) >> 15), kHorizUpMax);
		pXSprite->moveState = kMoveDead;
		pPlayer->compression = mulscale16((1 << 15) - (Cos(ClipHigh(pPlayer->deathTime << 3, kAngle90)) >> 15), pPlayer->eyeAboveZ);

		if (pPlayer->weapon)
			pInput->newWeapon = pPlayer->weapon;	// force weapon down

		if (pInput->keyFlags.action)
		{
			playerReset(pPlayer, pInput, pPlayer->sprite->type);
			pInput->keyFlags.action = 0;
		}
		return;		// Don't allow the player to do anything else if dead
	}

	sin = Sin(pSprite->ang);
	cos = Cos(pSprite->ang);

	// find vel and svel relative to current angle
	vel = dmulscale30(pSprite->xvel, cos, pSprite->yvel, sin);
	svel = dmulscale30(pSprite->xvel, sin, -pSprite->yvel, cos);

/*
	// ground deceleration
	switch ( pXSprite->moveState )
	{
		case kMoveFall:
		case kMoveSwim:
			break;

		default:
			if (vel > 0)
				vel = ClipLow(vel - pInput->ticks * cp->decel, 0);
			else if (vel < 0)
				vel = ClipHigh(vel + pInput->ticks * cp->decel, 0);

			if (svel > 0)
				svel = ClipLow(svel - pInput->ticks * cp->sideDecel, 0);
			else if (svel < 0)
				svel = ClipHigh(svel + pInput->ticks * cp->sideDecel, 0);
			break;
	}
*/

	// acceleration
	switch ( pXSprite->moveState )
	{
		case kMoveWalk:
		case kMoveStand:
		case kMoveLand:
		case kMoveSwim:
			if ( pInput->forward > 0 )
			{
				run = pInput->forward > pInput->ticks;
				pPlayer->run |= run;
				if ( vel < cp->frontSpeed[run] );
					vel = ClipHigh(vel + cp->frontAccel * pInput->forward + kGroundFriction * kFrameTicks, cp->frontSpeed[run]);
			}
			else if ( pInput->forward < 0 )
			{
				run = pInput->forward < -pInput->ticks;
				pPlayer->run |= run;
				if ( vel > -cp->backSpeed[run] );
					vel = ClipLow(vel + cp->backAccel * pInput->forward - kGroundFriction * kFrameTicks, -cp->backSpeed[run]);
			}
			if ( pInput->strafe > 0 )
			{
				run = pInput->strafe > pInput->ticks;
				pPlayer->run |= run;
				if ( svel < cp->sideSpeed[run] );
					svel = ClipHigh(svel + cp->sideAccel * pInput->strafe + kGroundFriction * kFrameTicks, cp->sideSpeed[run]);
			}
			else if ( pInput->strafe < 0 )
			{
				run = pInput->strafe < -pInput->ticks;
				pPlayer->run |= run;
				if ( svel > -cp->sideSpeed[run] );
					svel = ClipLow(svel + cp->sideAccel * pInput->strafe - kGroundFriction * kFrameTicks, -cp->sideSpeed[run]);
			}
			break;
	}

	// turn player
	if (pInput->turn != 0)
		pSprite->ang = (short)((pSprite->ang + (pInput->turn * kFrameTicks >> 4)) & kAngleMask);

	// reconstruct x and y velocities
	pSprite->xvel = (short)dmulscale30(vel, cos, svel, sin);
	pSprite->yvel = (short)dmulscale30(vel, sin, -svel, cos);

	// action key
	if (pInput->keyFlags.action)
	{
		int nIndex, nXIndex;
		int keyId;

		switch ( ActionScan(pPlayer, &nIndex, &nXIndex) )
		{
			case SS_SECTOR:
			{
				XSECTOR *pXSector = &xsector[nXIndex];
				keyId = pXSector->key;
				dprintf("Key %d required\n", keyId);

				if ( pXSector->locked && pPlayer == gMe )
					viewSetMessage("It's locked");

				if ( keyId == 0 || pPlayer->hasKey[keyId] )
					trTriggerSector(nIndex, pXSector, kCommandSpritePush);
				else
					if (pPlayer == gMe)
					{
						viewSetMessage("That requires a key.");
					}
				break;
			}

			case SS_WALL:
			{
				XWALL *pXWall = &xwall[nXIndex];
				keyId = pXWall->key;
				dprintf("Key %d required\n", keyId);

				if ( pXWall->locked && pPlayer == gMe )
					viewSetMessage("It's locked");

				if ( keyId == 0 || pPlayer->hasKey[keyId] )
					trTriggerWall(nIndex, pXWall, kCommandWallPush);
				else
					if (pPlayer == gMe)
					{
						viewSetMessage("That requires a key.");
					}
				break;
			}

			case SS_SPRITE:
			{
				XSPRITE *pXSprite = &xsprite[nXIndex];
				keyId = pXSprite->key;
				dprintf("Key %d required\n", keyId);

				if ( pXSprite->locked && pPlayer == gMe )
					viewSetMessage("It's locked");

				if ( keyId == 0 || pPlayer->hasKey[keyId] )
					trTriggerSprite(nIndex, pXSprite, kCommandSpritePush);
				else
					if (pPlayer == gMe)
					{
						viewSetMessage("That requires a key.");
					}
				break;
			}
		}

		// non-repeating, so clear flag
		pInput->keyFlags.action = 0;
	}

	if ( pInput->keyFlags.lookcenter && !pInput->buttonFlags.lookup && !pInput->buttonFlags.lookdown )
	{
		if ( pPlayer->look < 0 )
			pPlayer->look = ClipHigh(pPlayer->look + kFrameTicks, 0);

		if ( pPlayer->look > 0 )
			pPlayer->look = ClipLow(pPlayer->look - kFrameTicks, 0);

		if ( pPlayer->look == 0 )
			pInput->keyFlags.lookcenter = 0;
	}
	else
	{
		if ( pInput->buttonFlags.lookup )
			pPlayer->look = ClipHigh(pPlayer->look + kFrameTicks, kLookMax);

		if ( pInput->buttonFlags.lookdown )
			pPlayer->look = ClipLow(pPlayer->look - kFrameTicks, -kLookMax);
	}

	if ( pPlayer->look > 0 )
		pPlayer->horiz = mulscale30(kHorizUpMax, Sin(pPlayer->look * (kAngle90 / kLookMax)));
	else if ( pPlayer->look < 0 )
		pPlayer->horiz = mulscale30(kHorizDownMax, Sin(pPlayer->look * (kAngle90 / kLookMax)));
	else
		pPlayer->horiz = 0;

	pPlayer->slope = -pPlayer->horiz << 7;

	PickUp(pPlayer);
}


void playerMove( PLAYER *pPlayer, INPUT *pInput )
{
	int nSprite = pPlayer->nSprite;
	SPRITE *pSprite = pPlayer->sprite;
	int nXSprite = pSprite->extra;
	XSPRITE *pXSprite = pPlayer->xsprite;
	short nSector;
	int nXSector;
	XSECTOR *pXSector = NULL;
	POSTURE *cp = &gPosture[pPlayer->lifemode];

	nSector = pSprite->sectnum;

	if ( (nXSector = sector[nSector].extra) > 0 )
		pXSector = &xsector[nXSector];

	int moveDist = qdist(pSprite->xvel, pSprite->yvel);
	ProcessInput(pPlayer, pInput);

	if ( pXSprite->health == 0 )
		return;

	int dudeIndex = kDudePlayer1 - kDudeBase;
	if ( moveDist == 0 )
		seqSpawn(dudeInfo[dudeIndex].seqStartID + kSeqDudeIdle, SS_SPRITE, nXSprite);
	else
		seqSpawn(dudeInfo[dudeIndex].seqStartID + kSeqCultistWalk, SS_SPRITE, nXSprite);	// hack to make player walk

	if ( gSpriteHit[nXSprite].floorHit == 0 )
		pXSprite->moveState = kMoveFall;
	else
		pXSprite->moveState = kMoveWalk;

/*
	switch (pXSprite->moveState)
	{
		case kMoveFall:
			pPlayer->impactPE = pSprite->zvel * gSpring;
			pXSprite->moveState = kMoveLand;
			pSprite->zvel = 0;
			pPlayer->impactPhase = 0;

			pPlayer->standSpeed = kAngle90 / ClipLow(fallDamage << 2, 8);
			pPlayer->standSpeed = ClipLow(pPlayer->standSpeed, 2);

			// fall through to Move Land

		case kMoveLand:
			pPlayer->impactPhase = ClipHigh(pPlayer->impactPhase + dt * gSpringPhaseInc, kAngle90);
			pPlayer->compression = mulscale30(pPlayer->impactPE, Sin(pPlayer->impactPhase));
//				pSprite->zvel = mulscale30(pPlayer->impactPE, Cos(pPlayer->impactPhase));
			if (pPlayer->impactPhase == kAngle90)
			{
				pXSprite->moveState = kMoveStand;
				pPlayer->impactPhase = 0;
				pSprite->zvel = 0;
			}
			break;

		case kMoveStand:
			pPlayer->impactPhase = ClipHigh(pPlayer->impactPhase + dt * pPlayer->standSpeed, kAngle90);
			pPlayer->compression = mulscale30(pPlayer->impactPE, (1 << 30) - Sin(pPlayer->impactPhase));
			if (pPlayer->impactPhase == kAngle90)
			{
				pXSprite->moveState = kMoveWalk;
				pSprite->zvel = 0;
				pPlayer->impactPE = 0;
			}

			// fall through

		case kMoveWalk:
			if ( zBot > loz )
			{
				pPlayer->compression += zBot - loz;
				pSprite->z += loz - zBot;
				pPlayer->impactPhase = 0;
				pXSprite->moveState = kMoveStand;
				pPlayer->standSpeed = 8;
				pPlayer->impactPE = pPlayer->compression;
			}
	}
*/

	if ( pInput->buttonFlags.jump && gSpriteHit[nXSprite].floorHit != 0 )
	{
		sfxStart3DSound(nXSprite, kSfxPlayJump, 0x1000);
 		pSprite->zvel = -400;
	}

	if (pSprite->sectnum < 0 || pSprite->sectnum >= numsectors)
	{
		dprintf("How did you get in the wrong sector (%d)?\n", pSprite->sectnum);
	}

	if (pXSector && (pXSector->underwater || (pXSector->depth == kDepthSwim && pSprite->z >= sector[nSector].floorz)))
		pXSprite->moveState = kMoveSwim;

	// bobbing and swaying code

	// this controls the decay on bobbing
	int xamp, yamp;
	pPlayer->bobAmp = ClipLow(pPlayer->bobAmp - 8 * kFrameTicks, 0);
	moveDist >>= 2;

	if ( gViewBobbing )
	{
		// this bobbing code should be moved somewhere else, VIEW maybe?
		switch ( pPlayer->xsprite->moveState )
		{
			case kMoveWalk:
				pPlayer->bobPhase = (pPlayer->bobPhase + kFrameTicks * cp->pace[pPlayer->run] * kAngle360 / 120 / kTimerRate) & kAngleMask;
				pPlayer->swayPhase = (pPlayer->swayPhase + kFrameTicks * cp->pace[pPlayer->run] / 2 * kAngle360 / 120 / kTimerRate) & kAngleMask;
				if ( pPlayer->run )
				{
					if (pPlayer->bobAmp < 512)
						pPlayer->bobAmp = ClipHigh(pPlayer->bobAmp + moveDist, 512);
				}
				else
				{
					if (pPlayer->bobAmp < 384)
						pPlayer->bobAmp = ClipHigh(pPlayer->bobAmp + moveDist, 384);
				}
				xamp = yamp = pPlayer->bobAmp;
				break;

			case kMoveSwim:
				pPlayer->bobPhase = (pPlayer->bobPhase + kFrameTicks * kAngle360 / kTimerRate / 2) & kAngleMask;
				pPlayer->swayPhase = (pPlayer->swayPhase + kFrameTicks * kAngle360 / kTimerRate / 2) & kAngleMask;
				yamp = 64;
				xamp = 0;
				break;

			default:
				xamp = yamp = pPlayer->bobAmp;
		}

		pPlayer->bobHeight = mulscale30(cp->bobV * yamp, Sin(pPlayer->bobPhase * 2));
		pPlayer->bobWidth = mulscale30(cp->bobH * xamp, Sin(pPlayer->bobPhase - kAngle45));
		pPlayer->swayHeight = mulscale30(cp->swayV * yamp, Sin(pPlayer->swayPhase * 2));
		pPlayer->swayWidth = mulscale30(cp->swayH * xamp, Sin(pPlayer->swayPhase - kAngle60));
	}
}


void playerFireMissile( PLAYER *pPlayer, long dx, long dy, long dz, int missileType )
{
	actFireMissile(pPlayer->nSprite, pPlayer->sprite->z - pPlayer->weaponAboveZ,
		dx, dy, dz, missileType);
}


int playerFireThing( PLAYER *pPlayer, int relSlope, int thingType, int velocity )
{
	dassert( thingType >= kThingBase && thingType < kThingMax );

	SPRITE *pSprite = pPlayer->sprite;

	return actFireThing( pPlayer->nSprite, pSprite->z - pPlayer->weaponAboveZ,
		pPlayer->slope + relSlope, thingType, velocity );
}


void playerDamageSprite( PLAYER *pPlayer, int nSource, int nDamage )
{
	// if (pPlayer == gMe)
	if ( pPlayer == gView )
		scrDacRelEffect((nDamage>>4)*2, (-nDamage>>4)*3, (-nDamage>>4)*3);

	if ( nDamage > 0 )
		sfxStart3DSound(pPlayer->sprite->extra, kSfxPlayPain, 0x1000);

	// source can be targeted by lackeys?
	if (IsDudeSprite( nSource ))
	{
		for (int i=0; i<kMaxLackeys; i++)
		{
			int nLackey = pPlayer->nLackeySprites[i];
			if (nLackey >= 0 && nLackey != nSource)
				aiAlertLackey( pPlayer, nLackey, nSource, nDamage );
		}
	}
}


BOOL playerAddLackey( PLAYER *pPlayer, int nLackey )
{
	for (int i=0; i<kMaxLackeys; i++)
		if (pPlayer->nLackeySprites[i] == nLackey)
		{
			//dprintf("lackey already in list\n");
			return FALSE;
		}

	for (i=0; i<kMaxLackeys; i++)
		if (pPlayer->nLackeySprites[i] == -1)
		{
			//dprintf("adding lackey %i: %i\n",i,nLackey);
			pPlayer->nLackeySprites[i] = nLackey;
			sprite[nLackey].owner = (short)pPlayer->nSprite;
			break;
		}

	return (i < kMaxLackeys) ? TRUE : FALSE;
}


void playerDeleteLackey( PLAYER *pPlayer, int nLackey )
{
	for (int i=0; i<kMaxLackeys; i++)
		if (pPlayer->nLackeySprites[i] == nLackey)
		{
			pPlayer->nLackeySprites[i] = -1;
			sprite[nLackey].owner = -1;
			break;
		}
}
