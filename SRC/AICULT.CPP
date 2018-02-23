#include <stdio.h>
#include <string.h>

#include "aicult.h"

#include "actor.h"
#include "db.h"
#include "debug4g.h"
#include "dude.h"
#include "engine.h"
#include "eventq.h"
#include "gameutil.h"
#include "globals.h"
#include "misc.h"
#include "multi.h"
#include "player.h"
#include "sfx.h"
#include "trig.h"

/****************************************************************************
** LOCAL CONSTANTS
****************************************************************************/

#define kCultistTFireDist			M2X(100)
#define kCultistSFireDist			M2X(10)
#define kCultistThrowDist1			M2X(16)
#define kCultistThrowDist2			M2X(10)

#define kSlopeThrow		-8192	// tan(26)

#define kVectorsPerBarrel	8

/****************************************************************************
** LOCAL callback prototypes
****************************************************************************/

static void TommyCallback( int /* type */, int nXIndex );
static void ShotCallback( int /* type */, int nXIndex );
static void ThrowCallback( int /* type */, int nXIndex );


/****************************************************************************
** LOCAL think/move function prototypes
****************************************************************************/

static void thinkSearch( SPRITE *pSprite, XSPRITE *pXSprite );
static void thinkGoto( SPRITE *pSprite, XSPRITE *pXSprite );
static void thinkChase( SPRITE *pSprite, XSPRITE *pXSprite );


/****************************************************************************
** GLOBAL CONSTANTS
****************************************************************************/

AISTATE cultistIdle		= { kSeqDudeIdle,		NULL,			0,		NULL,	NULL,			aiThinkTarget,	NULL };
AISTATE cultistChase	= { kSeqCultistWalk,	NULL,			0,		NULL,	aiMoveForward,	thinkChase,		NULL };
AISTATE cultistDodge	= { kSeqCultistWalk,	NULL,			90,		NULL,	aiMoveDodge,	NULL,			&cultistChase };
AISTATE cultistGoto		= { kSeqCultistWalk,	NULL,			3600,	NULL,	aiMoveForward,	thinkGoto,		&cultistIdle };
AISTATE cultistTThrow	= { kSeqCultistAttack2,	ThrowCallback,	120,	NULL,	NULL,			NULL,			&cultistTFire };
AISTATE cultistSThrow	= { kSeqCultistAttack2,	ThrowCallback,	120,	NULL,	NULL,			NULL,			&cultistSFire };
AISTATE cultistSearch	= { kSeqCultistWalk,	NULL,			3600,	NULL,	aiMoveForward,	thinkSearch,	&cultistIdle };
AISTATE cultistSFire	= { kSeqCultistAttack1,	ShotCallback,	60,		NULL,	NULL,			NULL,			&cultistChase };
AISTATE cultistTFire	= { kSeqCultistAttack1,	TommyCallback,	0,		NULL,	aiMoveTurn,		thinkChase, 	&cultistTFire };
AISTATE cultistRecoil	= { kSeqDudeRecoil,		NULL,			0,		NULL,	NULL,			NULL,			&cultistDodge };


/****************************************************************************
** LOCAL FUNCTIONS
****************************************************************************/


/****************************************************************************
** TommyCallback()
**
**
****************************************************************************/
static void TommyCallback( int /* type */, int nXIndex )
{
	XSPRITE *pXSprite = &xsprite[nXIndex];
	int nSprite = pXSprite->reference;
	SPRITE *pSprite = &sprite[nSprite];

	int dx = Cos(pSprite->ang) >> 16;
	int dy = Sin(pSprite->ang) >> 16;
	int dz = gDudeSlope[nXIndex];

	// dispersal modifiers here
	dx += BiRandom(1000);
	dy += BiRandom(1000);
	dz += BiRandom(1000);

	actFireVector(nSprite, pSprite->z, dx, dy, dz, kVectorBullet);
	sfxCreate3DSound(pSprite->x, pSprite->y, pSprite->z, kSfxTomFire);
}


/****************************************************************************
** ShotCallback()
**
**
****************************************************************************/
static void ShotCallback( int /* type */, int nXIndex )
{
	XSPRITE *pXSprite = &xsprite[nXIndex];
	int nSprite = pXSprite->reference;
	SPRITE *pSprite = &sprite[nSprite];

	int dx = Cos(pSprite->ang) >> 16;
	int dy = Sin(pSprite->ang) >> 16;
	int dz = gDudeSlope[nXIndex];

	// aim modifiers
	dx += BiRandom(4000);
	dy += BiRandom(4000);
	dz += BiRandom(4000);

	for ( int i = 0; i < kVectorsPerBarrel; i++ )
	{
		actFireVector(nSprite, pSprite->z, dx + BiRandom(3000), dy + BiRandom(3000),
			dz + BiRandom(3000), kVectorShell);
	}
	sfxCreate3DSound(pSprite->x, pSprite->y, pSprite->z, kSfxShotFire);
}


/****************************************************************************
** ThrowCallback()
**
**
****************************************************************************/
static void ThrowCallback( int /* type */, int nXIndex )
{
	XSPRITE *pXSprite = &xsprite[nXIndex];
	int nSprite = pXSprite->reference;
	SPRITE *pSprite = &sprite[nSprite];

	sfxCreate3DSound(pSprite->x, pSprite->y, pSprite->z, kSfxTNTToss);
	int nThing = actFireThing(
		nSprite, pSprite->z, gDudeSlope[nXIndex] + kSlopeThrow, kThingTNTStick, (M2X(10.0) << 4) / kTimerRate);
	evPost(nThing, SS_SPRITE, 2 * kTimerRate);		// 2 second burn off
}


/****************************************************************************
** TargetNearExplosion()
**
**
****************************************************************************/
static BOOL TargetNearExplosion( SPRITE *pTarget )
{
	for (short nSprite = headspritesect[pTarget->sectnum]; nSprite >= 0; nSprite = nextspritesect[nSprite])
	{
		// check for TNT sticks or explosions in the same sector as the target
		if (sprite[nSprite].type == kThingTNTStick || sprite[nSprite].statnum == kStatExplosion)
			return TRUE; // indicate danger
	}
	return FALSE;
}


/****************************************************************************
** thinkSearch()
**
**
****************************************************************************/
static void thinkSearch( SPRITE *pSprite, XSPRITE *pXSprite )
{
	aiChooseDirection(pSprite, pXSprite, pXSprite->goalAng);
	aiThinkTarget(pSprite, pXSprite);
}


/****************************************************************************
** thinkGoto()
**
**
****************************************************************************/
static void thinkGoto( SPRITE *pSprite, XSPRITE *pXSprite )
{
	int dx, dy, dist;

	dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];

	SPRITE *pTarget = &sprite[pXSprite->target];
	XSPRITE *pXTarget = &xsprite[pTarget->extra];

	dx = pXSprite->targetX - pSprite->x;
	dy = pXSprite->targetY - pSprite->y;

	int nAngle = getangle(dx, dy);
	dist = qdist(dx, dy);

	aiChooseDirection(pSprite, pXSprite, nAngle);

	// if reached target, change to search mode
	if ( dist < M2X(1.0) && qabs(pSprite->ang - nAngle) < pDudeInfo->periphery )
		aiNewState(pSprite, pXSprite, &cultistSearch);

	aiThinkTarget(pSprite, pXSprite);
}


/****************************************************************************
** thinkChase()
**
**
****************************************************************************/
static void thinkChase( SPRITE *pSprite, XSPRITE *pXSprite )
{
	if ( pXSprite->target == -1)
	{
		aiNewState(pSprite, pXSprite, &cultistGoto);
		return;
	}

	int dx, dy, dist;

	dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];

	dassert(pXSprite->target >= 0 && pXSprite->target < kMaxSprites);
	SPRITE *pTarget = &sprite[pXSprite->target];
	XSPRITE *pXTarget = &xsprite[pTarget->extra];

	// check target
	dx = pTarget->x - pSprite->x;
	dy = pTarget->y - pSprite->y;

	aiChooseDirection(pSprite, pXSprite, getangle(dx, dy));

	if ( pXTarget->health == 0 )
	{
		// target is dead
		aiNewState(pSprite, pXSprite, &cultistSearch);
		return;
	}

	if ( IsPlayerSprite( pTarget ) )
	{
		PLAYER *pPlayer = &gPlayer[ pTarget->type - kDudePlayer1 ];
		if ( powerupCheck( pPlayer, kItemLtdInvisibility - kItemBase ) > 0 )
		{
			aiNewState(pSprite, pXSprite, &cultistSearch);
			return;
		}
	}

	dist = qdist(dx, dy);
	if ( dist <= pDudeInfo->seeDist )
	{
		int nAngle = getangle(dx, dy);
		int losAngle = ((kAngle180 + nAngle - pSprite->ang) & kAngleMask) - kAngle180;
		int eyeAboveZ = pDudeInfo->eyeHeight * pSprite->yrepeat << 2;

		// is there a line of sight to the target?
		if ( cansee(pTarget->x, pTarget->y, pTarget->z, pTarget->sectnum,
			pSprite->x, pSprite->y, pSprite->z - eyeAboveZ, pSprite->sectnum) )
		{
			// is the target visible?
			if ( dist < pDudeInfo->seeDist && qabs(losAngle) <= pDudeInfo->periphery )
			{
				aiSetTarget(pXSprite, pXSprite->target);

				int nXSprite = sprite[pXSprite->reference].extra;
				gDudeSlope[nXSprite] = divscale(pTarget->z - pSprite->z, dist, 10);

				// check to see if we can attack
				switch ( pSprite->type )
				{
					case kDudeTommyCultist:
						if ( dist < kCultistThrowDist1 && dist > kCultistThrowDist2 && qabs(losAngle) < kAngle15
						&& !TargetNearExplosion(pTarget) && (pTarget->flags & kAttrGravity)
						&& !( IsPlayerSprite(pXSprite->target) && gPlayer[pTarget->type - kDudePlayer1].run )
						&& Chance(0x4000) )
							aiNewState(pSprite, pXSprite, &cultistTThrow);
						else if ( dist < kCultistTFireDist && qabs(losAngle) < kAngle15 )
							aiNewState(pSprite, pXSprite, &cultistTFire);
						break;

					case kDudeShotgunCultist:
						if ( dist < kCultistThrowDist1 && dist > kCultistThrowDist2 && qabs(losAngle) < kAngle15
						&& !TargetNearExplosion(pTarget) && (pTarget->flags & kAttrGravity)
						&& !(IsPlayerSprite(pXSprite->target) && gPlayer[pTarget->type - kDudePlayer1].run)
						&& Chance(0x4000) )
							aiNewState(pSprite, pXSprite, &cultistSThrow);
						else if ( dist < kCultistSFireDist && qabs(losAngle) < kAngle15 )
							aiNewState(pSprite, pXSprite, &cultistSFire);
						break;
				}
				return;
			}
		}
	}
	dprintf("Cultist %d lost sight of target %d\n", pXSprite->reference, pXSprite->target);
	aiNewState(pSprite, pXSprite, &cultistGoto);
	pXSprite->target = -1;
}



