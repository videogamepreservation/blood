#include <stdio.h>
#include <string.h>

#include "aiburn.h"

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

#define kBurnDist	M2X(1.6)


/****************************************************************************
** LOCAL callback prototypes
****************************************************************************/

//static void ExplodeCallback( int /* type */, int nXIndex );
static void BurnCallback( int /* type */, int nXIndex );


/****************************************************************************
** LOCAL think/move/entry function prototypes
****************************************************************************/

static void entryTorchy( SPRITE *pSprite, XSPRITE *pXSprite );

static void thinkSearch( SPRITE *pSprite, XSPRITE *pXSprite );
static void thinkGoto( SPRITE *pSprite, XSPRITE *pXSprite );
static void thinkChase( SPRITE *pSprite, XSPRITE *pXSprite );


/****************************************************************************
** GLOBAL CONSTANTS
****************************************************************************/
AISTATE cultistBurnChase	= { kSeqCultistWalk,	NULL,			0,		NULL,	aiMoveForward,	thinkChase,		NULL };
AISTATE cultistBurnGoto		= { kSeqCultistWalk,	NULL,			3600,	NULL,	aiMoveForward,	thinkGoto,		&cultistBurnSearch };
AISTATE cultistBurnSearch	= { kSeqCultistWalk,	NULL,			3600,	NULL,	aiMoveForward,	thinkSearch,	&cultistBurnSearch };
AISTATE cultistBurnAttack	= { kSeqCultistWalk,	BurnCallback,	120,	NULL,	NULL,			NULL,			&cultistBurnChase };

AISTATE playerBurnChase		= { kSeqCultistWalk,	NULL,			0,		NULL,	aiMoveForward,	thinkChase,		NULL };
AISTATE playerBurnGoto		= { kSeqCultistWalk,	NULL,			3600,	NULL,	aiMoveForward,	thinkGoto,		&playerBurnSearch };
AISTATE playerBurnSearch	= { kSeqCultistWalk,	NULL,			3600,	NULL,	aiMoveForward,	thinkSearch,	&playerBurnSearch };
AISTATE playerBurnAttack	= { kSeqCultistWalk,	BurnCallback,	120,	NULL,	NULL,			NULL,			&playerBurnChase };

AISTATE zombieABurnChase	= { kSeqAxeZombieWalk,	NULL,			0,		NULL,	aiMoveForward,	thinkChase,		NULL };
AISTATE zombieABurnGoto		= { kSeqAxeZombieWalk,	NULL,			3600,	NULL,	aiMoveForward,	thinkGoto,		&zombieABurnSearch };
AISTATE zombieABurnSearch	= { kSeqAxeZombieWalk,	NULL,			3600,	NULL,	aiMoveForward,	thinkSearch,	&zombieABurnSearch };
AISTATE zombieABurnAttack	= { kSeqAxeZombieWalk,	BurnCallback,	120,	NULL,	NULL,			NULL,			&zombieABurnChase };

AISTATE zombieFBurnChase	= { kSeqFatZombieWalk,	NULL,			0,		NULL,	aiMoveForward,	thinkChase,		NULL };
AISTATE zombieFBurnGoto		= { kSeqFatZombieWalk,	NULL,			3600,	NULL,	aiMoveForward,	thinkGoto,		&zombieFBurnSearch };
AISTATE zombieFBurnSearch	= { kSeqFatZombieWalk,	NULL,			3600,	NULL,	aiMoveForward,	thinkSearch,	&zombieFBurnSearch };
AISTATE zombieFBurnAttack	= { kSeqFatZombieWalk,	BurnCallback,	120,	NULL,	NULL,			NULL,			&zombieFBurnChase };


/****************************************************************************
** LOCAL FUNCTIONS
****************************************************************************/


/****************************************************************************
** entryTorchy()
**
**
****************************************************************************/
static void entryCultist( SPRITE *pSprite, XSPRITE */*pXSprite*/ )
{
	pSprite->type =

	switch( pSprite->type )
	{
		case kDudePlayerBurning:
			aiNewState(pSprite, pXSprite, &playerBurnSearch);
			break;
		case kDudeCultistBurning:
			aiNewState(pSprite, pXSprite, &cultistBurnSearch);
			break;
		case kDudeAxeZombieBurning:
			aiNewState(pSprite, pXSprite, &zombieABurnSearch);
			break;
		case kDudeFatZombieBurning:
			aiNewState(pSprite, pXSprite, &zombieFBurnSearch);
			break;
	}
}

/****************************************************************************
** ExplodeCallback()
**
** This should be called when a burning cultist or burning player with
** dynamite randomly explodes.
****************************************************************************/
//static void ExplodeCallback( int /* type */, int nXIndex )
//{
//	XSPRITE *pXSprite = &xsprite[nXIndex];
//	int nSprite = pXSprite->reference;
//	SPRITE *pSprite = &sprite[nSprite];
//
//	(void)pSprite;
//}


/****************************************************************************
** BurnCallback()
**
** This should be called when a burning cultist or burning player touches
** any other dudes or explodable objects, like barrels. Basically, a close
** hitscan that further immolates whatever it hits with burn damage.
****************************************************************************/
static void BurnCallback( int /* type */, int nXIndex )
{
	XSPRITE *pXSprite = &xsprite[nXIndex];
	int nSprite = pXSprite->reference;
	SPRITE *pSprite = &sprite[nSprite];

	(void)pSprite;
}


/****************************************************************************
** CheckNearBarrel()
**
**
****************************************************************************/
//static int CheckNearBarrel( SPRITE *pTarget )
//{
//	for (short nSprite = headspritesect[pTarget->sectnum]; nSprite >= 0; nSprite = nextspritesect[nSprite])
//	{
//		// check for TNT sticks or explosions in the same sector as the target
//		if (sprite[nSprite].type == kThingTNTBarrel || sprite[nSprite].statnum == kStatExplosion)
//			return nSprite; // indicate new target
//	}
//	return -1;
//}


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
	{
		switch( pSprite->type )
		{
			case kDudePlayerBurning:
				aiNewState(pSprite, pXSprite, &playerBurnSearch);
				break;
			case kDudeCultistBurning:
				aiNewState(pSprite, pXSprite, &cultistBurnSearch);
				break;
			case kDudeAxeZombieBurning:
				aiNewState(pSprite, pXSprite, &zombieABurnSearch);
				break;
			case kDudeFatZombieBurning:
				aiNewState(pSprite, pXSprite, &zombieFBurnSearch);
				break;
		}
	}

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
		switch( pSprite->type )
		{
			case kDudePlayerBurning:
				aiNewState(pSprite, pXSprite, &playerBurnGoto);
				break;
			case kDudeCultistBurning:
				aiNewState(pSprite, pXSprite, &cultistBurnGoto);
				break;
			case kDudeAxeZombieBurning:
				aiNewState(pSprite, pXSprite, &zombieABurnGoto);
				break;
			case kDudeFatZombieBurning:
				aiNewState(pSprite, pXSprite, &zombieFBurnGoto);
				break;
		}
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
		switch( pSprite->type )
		{
			case kDudePlayerBurning:
				aiNewState(pSprite, pXSprite, &playerBurnSearch);
				break;
			case kDudeCultistBurning:
				aiNewState(pSprite, pXSprite, &cultistBurnSearch);
				break;
			case kDudeAxeZombieBurning:
				aiNewState(pSprite, pXSprite, &zombieABurnSearch);
				break;
			case kDudeFatZombieBurning:
				aiNewState(pSprite, pXSprite, &zombieFBurnSearch);
				break;
		}
		return;
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
				if ( dist < kBurnDist && qabs(losAngle) < kAngle15 )
				{
					switch ( pSprite->type )
					{
						case kDudePlayerBurning:
							aiNewState(pSprite, pXSprite, &playerBurnAttack);
							break;
						case kDudeCultistBurning:
							aiNewState(pSprite, pXSprite, &cultistBurnAttack);
							break;
						case kDudeAxeZombieBurning:
							aiNewState(pSprite, pXSprite, &zombieABurnAttack);
							break;
						case kDudeFatZombieBurning:
							aiNewState(pSprite, pXSprite, &zombieFBurnAttack);
							break;
					}
				}
				return;
			}
		}
	}
	dprintf("Torchy %d lost sight of target %d\n", pXSprite->reference, pXSprite->target);

	switch ( pSprite->type )
	{
		case kDudePlayerBurning:
			aiNewState(pSprite, pXSprite, &playerBurnGoto);
			break;
		case kDudeCultistBurning:
			aiNewState(pSprite, pXSprite, &cultistBurnGoto);
			break;
		case kDudeAxeZombieBurning:
			aiNewState(pSprite, pXSprite, &zombieABurnGoto);
			break;
		case kDudeFatZombieBurning:
			aiNewState(pSprite, pXSprite, &zombieFBurnGoto);
			break;
	}

	pXSprite->target = -1;
}

