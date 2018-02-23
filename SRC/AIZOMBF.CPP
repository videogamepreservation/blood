#include <stdio.h>
#include <string.h>

#include "aizombf.h"

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
#include "trig.h"

/****************************************************************************
** LOCAL CONSTANTS
****************************************************************************/

#define kFatZombieMeleeDist			M2X(2.0)	//M2X(1.6)
#define kFatZombiePukeDist1			M2X(3)
#define kFatZombiePukeDist2			M2X(5)
#define kFatZombieThrowDist1		M2X(10)
#define kFatZombieThrowDist2		M2X(6)


/****************************************************************************
** LOCAL callback prototypes
****************************************************************************/

static void HackCallback( int /* type */, int nXIndex );
static void PukeCallback( int /* type */, int nXIndex );
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

AISTATE zombieFIdle		= { kSeqDudeIdle,		NULL,			0,		NULL,	NULL,			aiThinkTarget,	NULL };
AISTATE zombieFChase	= { kSeqFatZombieWalk,	NULL,			0,		NULL,	aiMoveForward,	thinkChase,		NULL };
AISTATE zombieFGoto		= { kSeqFatZombieWalk,	NULL,			3600,	NULL,	aiMoveForward,	thinkGoto,		&zombieFIdle };
AISTATE zombieFHack		= { kSeqFatZombieAttack,HackCallback,	120,	NULL,	NULL,			NULL,			&zombieFChase };
AISTATE zombieFPuke		= { kSeqFatZombieAttack,PukeCallback,	120,	NULL,	NULL,			NULL,			&zombieFChase };
AISTATE zombieFThrow	= { kSeqFatZombieAttack,ThrowCallback,	120,	NULL,	NULL,			NULL,			&zombieFChase };
AISTATE zombieFSearch	= { kSeqFatZombieWalk,	NULL,			3600,	NULL,	aiMoveForward,	thinkSearch,	&zombieFIdle };
AISTATE zombieFRecoil	= { kSeqDudeRecoil,		NULL,			0,		NULL,	NULL,			NULL,			&zombieFChase };


/****************************************************************************
** LOCAL FUNCTIONS
****************************************************************************/


/****************************************************************************
** HackCallback()
**
**
****************************************************************************/
static void HackCallback( int /* type */, int nXIndex )
{
	XSPRITE *pXSprite = &xsprite[nXIndex];
	int nSprite = pXSprite->reference;
	SPRITE *pSprite = &sprite[nSprite];

	int dx = Cos(pSprite->ang) >> 16;
	int dy = Sin(pSprite->ang) >> 16;
	int dz = 0;

	actFireVector(nSprite, pSprite->z, dx, dy, dz, kVectorCleaver);
}


/****************************************************************************
** HackCallback()
**
**
****************************************************************************/
static void PukeCallback( int /* type */, int nXIndex )
{
	XSPRITE *pXSprite = &xsprite[nXIndex];
	int nSprite = pXSprite->reference;
	SPRITE *pSprite = &sprite[nSprite];

	int dx = Cos(pSprite->ang) >> 16;
	int dy = Sin(pSprite->ang) >> 16;
	int dz = 0;

	int nThing = actFireThing(nSprite, pSprite->z, 0, kThingTNTStick, (M2X(4.0) << 4) / kTimerRate);
	evPost(nThing, SS_SPRITE, 2 * kTimerRate);		// 2 second burn off
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

	int dx = Cos(pSprite->ang) >> 16;
	int dy = Sin(pSprite->ang) >> 16;
	int dz = 0;

	int nZOffset = dudeInfo[pSprite->type - kDudeBase].eyeHeight;

	actFireMissile( nSprite, pSprite->z - nZOffset, dx, dy, dz, kMissileButcherKnife );
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
		aiNewState(pSprite, pXSprite, &zombieFSearch);

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
		aiNewState(pSprite, pXSprite, &zombieFGoto);
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
		aiNewState(pSprite, pXSprite, &zombieFSearch);
		return;
	}

	if ( IsPlayerSprite( pTarget ) )
	{
		PLAYER *pPlayer = &gPlayer[ pTarget->type - kDudePlayer1 ];
		if ( powerupCheck( pPlayer, kItemLtdInvisibility - kItemBase ) > 0
		|| powerupCheck( pPlayer, kItemDeathMask - kItemBase ) > 0 )
		{
			aiNewState(pSprite, pXSprite, &zombieFSearch);
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

				// check to see if we can attack
				if ( dist < kFatZombieThrowDist1 && dist > kFatZombieThrowDist2 && qabs(losAngle) < kAngle15 )
					aiNewState(pSprite, pXSprite, &zombieFThrow);
				else
				if ( dist < kFatZombiePukeDist1 && dist > kFatZombiePukeDist2 && qabs(losAngle) < kAngle15 )
					aiNewState(pSprite, pXSprite, &zombieFPuke);
				else
				if ( dist < kFatZombieMeleeDist && qabs(losAngle) < kAngle15 )
					aiNewState(pSprite, pXSprite, &zombieFHack);
				return;
			}
		}
	}
	dprintf("Fat zombie %d lost sight of target %d\n", pXSprite->reference, pXSprite->target);
	aiNewState(pSprite, pXSprite, &zombieFSearch);
	pXSprite->target = -1;
}
