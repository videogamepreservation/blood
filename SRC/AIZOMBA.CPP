#include <stdio.h>
#include <string.h>

#include "aizomba.h"

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

#define kAxeZombieMeleeDist			M2X(2.0)	//M2X(1.6)


/****************************************************************************
** LOCAL callback prototypes
****************************************************************************/

static void HackCallback( int /* type */, int nXIndex );


/****************************************************************************
** LOCAL think/move/entry function prototypes
****************************************************************************/

static void thinkSearch( SPRITE *pSprite, XSPRITE *pXSprite );
static void thinkGoto( SPRITE *pSprite, XSPRITE *pXSprite );
static void thinkChase( SPRITE *pSprite, XSPRITE *pXSprite );
static void thinkPonder( SPRITE *pSprite, XSPRITE *pXSprite );

static void entryEZombie( SPRITE *pSprite, XSPRITE *pXSprite );
static void entryAIdle( SPRITE */*pSprite*/, XSPRITE *pXSprite );

static void myThinkTarget( SPRITE *pSprite, XSPRITE *pXSprite );
static void myThinkSearch( SPRITE *pSprite, XSPRITE *pXSprite );


/****************************************************************************
** GLOBAL CONSTANTS
****************************************************************************/

AISTATE zombieAIdle		= { kSeqDudeIdle,			NULL,			0,		entryAIdle,	NULL,		aiThinkTarget,	NULL };
AISTATE zombieAChase	= { kSeqAxeZombieWalk,		NULL,			0,		NULL,	aiMoveForward,	thinkChase,		NULL };
AISTATE zombieAPonder	= { kSeqDudeIdle,			NULL,			0,		NULL, aiMoveTurn, thinkPonder, NULL };
AISTATE zombieAGoto		= { kSeqAxeZombieWalk,		NULL,			3600,	NULL,	aiMoveForward,	thinkGoto,		&zombieAIdle };
AISTATE zombieAHack		= { kSeqAxeZombieAttack,	HackCallback,	120,	NULL,	NULL,			NULL,			&zombieAPonder };
AISTATE zombieASearch	= { kSeqAxeZombieWalk,		NULL,			3600,	NULL,	aiMoveForward,	thinkSearch,	&zombieAIdle };
AISTATE zombieARecoil	= { kSeqDudeRecoil,			NULL,			0,		NULL,	NULL,			NULL,			&zombieAPonder };

AISTATE zombieEIdle	= { kSeqEarthZombieIdle,		NULL,			0,		NULL,	NULL,			aiThinkTarget,	NULL };
AISTATE zombieEUp2	= { kSeqDudeIdle,				NULL,			1,		entryEZombie,	NULL,	NULL,			&zombieASearch };
AISTATE zombieEUp	= { kSeqEarthZombieUp,			NULL,			180,	NULL,	NULL,			NULL,			&zombieEUp2 };

AISTATE zombie2Idle		= { kSeqDudeIdle,			NULL,			0,		entryAIdle,	NULL,		myThinkTarget,	NULL };
AISTATE zombie2Search	= { kSeqAxeZombieWalk,		NULL,			3600,	NULL,	aiMoveForward,	myThinkSearch,	&zombie2Idle };


/****************************************************************************
** LOCAL FUNCTIONS
****************************************************************************/


/****************************************************************************
** TommyCallback()
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

	actFireVector(nSprite, pSprite->z, dx, dy, dz, kVectorAxe);
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
	{
		dprintf("Axe zombie %d switching to search mode\n", pXSprite->reference);
		aiNewState(pSprite, pXSprite, &zombieASearch);
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
		aiNewState(pSprite, pXSprite, &zombieASearch);
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
		aiNewState(pSprite, pXSprite, &zombieASearch);
		return;
	}

	if ( IsPlayerSprite( pTarget ) )
	{
		PLAYER *pPlayer = &gPlayer[ pTarget->type - kDudePlayer1 ];
		if ( powerupCheck( pPlayer, kItemLtdInvisibility - kItemBase ) > 0
		|| powerupCheck( pPlayer, kItemDeathMask - kItemBase ) > 0 )
		{
			aiNewState(pSprite, pXSprite, &zombieAGoto);
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
			if ( qabs(losAngle) <= pDudeInfo->periphery )
			{
				aiSetTarget(pXSprite, pXSprite->target);

				// check to see if we can attack
				if ( dist < kAxeZombieMeleeDist && qabs(losAngle) < kAngle15 )
				{
					sfxCreate3DSound(pSprite->x, pSprite->y, pSprite->z, kSfxAxeAir);
					aiNewState(pSprite, pXSprite, &zombieAHack);
				}
				return;
			}
		}
	}
	dprintf("Axe zombie %d lost sight of target %d\n", pXSprite->reference, pXSprite->target);
	aiNewState(pSprite, pXSprite, &zombieAGoto);
	pXSprite->target = -1;
}


/****************************************************************************
** thinkPonder()
**
**
****************************************************************************/
static void thinkPonder( SPRITE *pSprite, XSPRITE *pXSprite )
{
	if ( pXSprite->target == -1)
	{
		aiNewState(pSprite, pXSprite, &zombieASearch);
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
		aiNewState(pSprite, pXSprite, &zombieASearch);
		return;
	}

	if ( IsPlayerSprite( pTarget ) )
	{
		PLAYER *pPlayer = &gPlayer[ pTarget->type - kDudePlayer1 ];
		if ( powerupCheck( pPlayer, kItemLtdInvisibility - kItemBase ) > 0
		|| powerupCheck( pPlayer, kItemDeathMask - kItemBase ) > 0 )
		{
			aiNewState(pSprite, pXSprite, &zombieAGoto);
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
			if ( qabs(losAngle) <= pDudeInfo->periphery )
			{
				aiSetTarget(pXSprite, pXSprite->target);

				// check to see if we can attack
				if ( dist < kAxeZombieMeleeDist )
				{
					if ( qabs(losAngle) < kAngle15 )
					{
						sfxCreate3DSound(pSprite->x, pSprite->y, pSprite->z, kSfxAxeAir);
						aiNewState(pSprite, pXSprite, &zombieAHack);
					}
					return;
				}
			}
		}
	}
	aiNewState(pSprite, pXSprite, &zombieAChase);
}


/****************************************************************************
** myThinkTarget()
**
**
****************************************************************************/
static void myThinkTarget( SPRITE *pSprite, XSPRITE *pXSprite )
{
	dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];

	for (int i = 0; i < numplayers; i++)
	{
		PLAYER *pPlayer = &gPlayer[i];

		// skip this player if the he owns the dude or is invisible
		if ( pSprite->owner == pPlayer->nSprite
		|| pPlayer->xsprite->health == 0
		|| powerupCheck( pPlayer, kItemLtdInvisibility - kItemBase ) > 0 )
			continue;

		int x = pPlayer->sprite->x;
		int y = pPlayer->sprite->y;
		int z = pPlayer->sprite->z;
		short nSector = pPlayer->sprite->sectnum;

		int dx = x - pSprite->x;
		int dy = y - pSprite->y;

		int dist = qdist(dx, dy);

		if ( dist <= pDudeInfo->seeDist || dist <= pDudeInfo->hearDist )
		{
			int eyeAboveZ = pDudeInfo->eyeHeight * pSprite->yrepeat << 2;

			// is there a line of sight to the player?
			if ( cansee(x, y, z, nSector, pSprite->x, pSprite->y, pSprite->z - eyeAboveZ,
				pSprite->sectnum) )
			{
				int nAngle = getangle(dx, dy);
				int losAngle = ((kAngle180 + nAngle - pSprite->ang) & kAngleMask) - kAngle180;

				// is the player visible?
				if ( dist < pDudeInfo->seeDist && qabs(losAngle) <= pDudeInfo->periphery )
				{
					aiSetTarget( pXSprite, pPlayer->nSprite );
					aiActivateDude( pSprite, pXSprite );
					return;
				}

				// we may want to make hearing a function of sensitivity, rather than distance
				if ( dist < pDudeInfo->hearDist )
				{
					aiSetTarget(pXSprite, x, y, z);
					aiActivateDude( pSprite, pXSprite );
					return;
				}
			}
		}
	}
}


/****************************************************************************
** myThinkSearch()
**
**
****************************************************************************/
static void myThinkSearch( SPRITE *pSprite, XSPRITE *pXSprite )
{
	aiChooseDirection(pSprite, pXSprite, pXSprite->goalAng);
	myThinkTarget(pSprite, pXSprite);
}


/****************************************************************************
** entryEZombie()
**
**
****************************************************************************/
static void entryEZombie( SPRITE *pSprite, XSPRITE */*pXSprite*/ )
{
	pSprite->flags |= kAttrMove;
	pSprite->type = kDudeAxeZombie;
}

/****************************************************************************
** entryAZombieIdle()
**
**
****************************************************************************/
static void entryAIdle( SPRITE */*pSprite*/, XSPRITE *pXSprite )
{
	pXSprite->target = -1;
}


/****************************************************************************
** aiAlertLackey()
**
**
****************************************************************************/
void aiAlertLackey( PLAYER *pPlayer, int nLackey, int nSource, int nDamage )
{
	// tell lackeys all about your troubles...
	SPRITE *pLackey = &sprite[nLackey];
	XSPRITE *pXLackey = &xsprite[pLackey->extra];

	if ( pXLackey->target == -1 || pXLackey->target == pPlayer->nSprite )
	{
		// give a dude a target
		//dprintf("giving dude a new target\n");
		aiSetTarget(pXLackey, nSource);
		aiActivateDude(pLackey, pXLackey);
	}
	else if (nSource != pXLackey->target)
	{
		// retarget
		SPRITE *pSource = &sprite[nSource];
		int nThresh = nDamage * dudeInfo[pSource->type - kDudeBase].changeTarget;

		if ( Chance(nThresh) )
		{
			//dprintf("aiSetTarget lackey #%i to %i\n",i,nSource);
			aiSetTarget(pXLackey, nSource);
			aiActivateDude(pLackey, pXLackey);
		}
	}
}
