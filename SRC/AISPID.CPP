#include <stdio.h>
#include <string.h>

#include "aispid.h"

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

#define kSpiderBiteDist	M2X(1.8)


/****************************************************************************
** LOCAL callback prototypes
****************************************************************************/

static void BiteCallback( int /* type */, int nXIndex );


/****************************************************************************
** LOCAL think/move function prototypes
****************************************************************************/

static void thinkSearch( SPRITE *pSprite, XSPRITE *pXSprite );
static void thinkGoto( SPRITE *pSprite, XSPRITE *pXSprite );
static void thinkChase( SPRITE *pSprite, XSPRITE *pXSprite );


/****************************************************************************
** GLOBAL CONSTANTS
****************************************************************************/

AISTATE spidIdle	= { kSeqDudeIdle,	NULL,			0,		NULL,	NULL,			aiThinkTarget,	NULL };
AISTATE spidChase	= { kSeqSpiderWalk,	NULL,			0,		NULL,	aiMoveForward,	thinkChase,		NULL };
AISTATE spidDodge	= { kSeqSpiderWalk,	NULL,			90,		NULL,	aiMoveDodge,	NULL,			&spidChase };
AISTATE spidGoto	= { kSeqSpiderWalk,	NULL,			3600,	NULL,	aiMoveForward,	thinkGoto,		&spidIdle };
AISTATE spidSearch	= { kSeqSpiderWalk,	NULL,			3600,	NULL,	aiMoveForward,	thinkSearch,	&spidIdle };
AISTATE spidBite	= { kSeqSpiderBite,	BiteCallback,	60,		NULL,	NULL,			NULL,			&spidChase };
AISTATE spidJump	= { kSeqSpiderJump,	BiteCallback,	60,		NULL,	NULL,			NULL,			&spidChase };


/****************************************************************************
** LOCAL FUNCTIONS
****************************************************************************/


/****************************************************************************
** BiteCallback()
**
**
****************************************************************************/
static void BiteCallback( int /* type */, int nXIndex )
{
	XSPRITE *pXSprite = &xsprite[nXIndex];
	int nSprite = pXSprite->reference;
	SPRITE *pSprite = &sprite[nSprite];

	int dx = Cos(pSprite->ang) >> 16;
	int dy = Sin(pSprite->ang) >> 16;
	int dz = 0;

	// dispersal modifiers here
	dx += BiRandom(2000);
	dy += BiRandom(2000);
	dz += BiRandom(2000);

	actFireVector(nSprite, pSprite->z, dx, dy, dz, kVectorSpiderBite);
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
		aiNewState(pSprite, pXSprite, &spidSearch);

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
		aiNewState(pSprite, pXSprite, &spidGoto);
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
		aiNewState(pSprite, pXSprite, &spidSearch);
		return;
	}

	if ( IsPlayerSprite( pTarget ) )
	{
		PLAYER *pPlayer = &gPlayer[ pTarget->type - kDudePlayer1 ];
		if ( powerupCheck( pPlayer, kItemLtdInvisibility - kItemBase ) > 0 )
		{
			aiNewState(pSprite, pXSprite, &spidSearch);
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
				switch ( pSprite->type )
				{
					case kDudeBrownSpider:
					case kDudeRedSpider:
					case kDudeBlackSpider:
					case kDudeMotherSpider:
						if ( dist < kSpiderBiteDist && qabs(losAngle) < kAngle15 )
							aiNewState(pSprite, pXSprite, &spidBite);
						break;
				}
				return;
			}
		}
	}
	dprintf("Spider %d lost sight of target %d\n", pXSprite->reference, pXSprite->target);
	aiNewState(pSprite, pXSprite, &spidGoto);
	pXSprite->target = -1;
}



