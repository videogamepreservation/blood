#include <stdio.h>
#include <string.h>

#include "aihand.h"

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

#define kHandChokeDist			M2X(1.8)
#define kHandJumpDist1			M2X(16)
#define kHandJumpDist2			M2X(10)


/****************************************************************************
** LOCAL think/move function prototypes
****************************************************************************/

static void thinkSearch( SPRITE *pSprite, XSPRITE *pXSprite );
static void thinkGoto( SPRITE *pSprite, XSPRITE *pXSprite );
static void thinkChase( SPRITE *pSprite, XSPRITE *pXSprite );


/****************************************************************************
** GLOBAL CONSTANTS
****************************************************************************/

AISTATE handIdle	= { kSeqDudeIdle,	NULL,	0,		NULL,	NULL,			aiThinkTarget,	NULL };
AISTATE handSearch	= { kSeqHandWalk,	NULL,	3600,	NULL,	aiMoveForward,	thinkSearch,	&handIdle };
AISTATE handChase	= { kSeqHandWalk,	NULL,	0,		NULL,	aiMoveForward,	thinkChase,		NULL };
AISTATE handRecoil	= { kSeqDudeRecoil,	NULL,	0,		NULL,	NULL,			NULL,			&handSearch };	//ADD: handDodge
AISTATE handGoto	= { kSeqHandWalk,	NULL,	3600,	NULL,	aiMoveForward,	thinkGoto,		&handIdle };
AISTATE handJump	= { kSeqHandWalk,	NULL,	3600,	NULL,	aiMoveForward,	thinkGoto,		&handIdle };	//ADD: jumping
AISTATE handChoke	= { kSeqHandWalk,	NULL,	3600,	NULL,	aiMoveForward,	thinkGoto,		&handIdle };	//ADD: choking


/****************************************************************************
** LOCAL FUNCTIONS
****************************************************************************/

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
		aiNewState(pSprite, pXSprite, &handSearch);

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
		aiNewState(pSprite, pXSprite, &handGoto);
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
		aiNewState(pSprite, pXSprite, &handSearch);
		return;
	}

	if ( IsPlayerSprite( pTarget ) )
	{
		PLAYER *pPlayer = &gPlayer[ pTarget->type - kDudePlayer1 ];
		if ( powerupCheck( pPlayer, kItemLtdInvisibility - kItemBase ) > 0 )
		{
			aiNewState(pSprite, pXSprite, &handSearch);
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

				if ( dist < kHandJumpDist1 && dist > kHandJumpDist2 && qabs(losAngle) < kAngle15 )
					aiNewState(pSprite, pXSprite, &handJump);
				else if ( dist < kHandChokeDist && qabs(losAngle) < kAngle15 )
					aiNewState(pSprite, pXSprite, &handChoke);

				return;
			}
		}
	}
	dprintf("Hand %d lost sight of target %d\n", pXSprite->reference, pXSprite->target);
	aiNewState(pSprite, pXSprite, &handGoto);
	pXSprite->target = -1;
}
