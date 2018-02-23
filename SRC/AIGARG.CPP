#include <stdio.h>
#include <string.h>

#include "aigarg.h"

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

#define kFGargoyleMeleeDist		M2X(2.0)
#define kFGargoyleThrowDist1	M2X(12)	// thrown bone
#define kFGargoyleThrowDist2	M2X(6)

#define kSGargoyleMeleeDist		M2X(2.0)
#define kSGargoyleBlastDist1	M2X(20)	// thrown paralyzing blast
#define kSGargoyleBlastDist2	M2X(14)

#define kSlopeThrow		-7168


/****************************************************************************
** LOCAL callback prototypes
****************************************************************************/

// flesh gargoyle callbacks
static void SlashFCallback( int /* type */, int nXIndex );
static void ThrowFCallback( int /* type */, int nXIndex );

// stone gargoyle callbacks
//static void SlashSCallback( int /* type */, int nXIndex );
//static void ThrowSCallback( int /* type */, int nXIndex );


/****************************************************************************
** LOCAL think/move/entry function prototypes
****************************************************************************/

static void thinkSearch( SPRITE *pSprite, XSPRITE *pXSprite );
static void thinkGoto( SPRITE *pSprite, XSPRITE *pXSprite );
static void thinkChase( SPRITE *pSprite, XSPRITE *pXSprite );

static void entryFStatue( SPRITE *pSprite, XSPRITE */*pXSprite*/ );

/****************************************************************************
** GLOBAL CONSTANTS
****************************************************************************/

AISTATE gargoyleFIdle	= { kSeqDudeIdle,		NULL,			0,		NULL,	NULL,			aiThinkTarget,	NULL };
AISTATE gargoyleFChase	= { kSeqDudeIdle,		NULL,			0,		NULL,	aiMoveForward,	thinkChase,		NULL };
AISTATE gargoyleFGoto	= { kSeqDudeIdle,		NULL,			3600,	NULL,	aiMoveForward,	thinkGoto,		&gargoyleFIdle };
AISTATE gargoyleFSlash	= { kSeqGargoyleAttack,	SlashFCallback,	120,	NULL,	NULL,			NULL,			&gargoyleFChase };
AISTATE gargoyleFThrow	= { kSeqGargoyleAttack,	ThrowFCallback,	120,	NULL,	NULL,			NULL,			&gargoyleFChase };
AISTATE gargoyleFRecoil	= { kSeqDudeRecoil,		NULL,			0,		NULL,	NULL,			NULL,			&gargoyleFChase };
AISTATE gargoyleFSearch	= { kSeqDudeIdle,		NULL,			3600,	NULL,	aiMoveForward,	thinkSearch,	&gargoyleFIdle };
AISTATE gargoyleFMorph2	= { -1,					NULL,			0,		entryFStatue,	NULL,	NULL,			&gargoyleFSearch };
AISTATE gargoyleFMorph	= { kSeqFleshStatueTurn,NULL,			0,		NULL,	NULL,			NULL,			&gargoyleFMorph2 };

//AISTATE fleshGargoyleDodge	= { kSeqDudeIdle, NULL, 90, NULL, aiMoveDodge, NULL, &gargoyleFChase };
//AISTATE fleshGargoyleSwoop	= { kSeqGargoyleFly, NULL, 0, NULL, aiMoveForward, thinkChase, NULL };


/****************************************************************************
** LOCAL FUNCTIONS
****************************************************************************/


/****************************************************************************
** SlashFCallback()
**
**
****************************************************************************/
static void SlashFCallback( int /* type */, int nXIndex )
{
	XSPRITE *pXSprite = &xsprite[nXIndex];
	int nSprite = pXSprite->reference;
	SPRITE *pSprite = &sprite[nSprite];

	int dx = Cos(pSprite->ang) >> 16;
	int dy = Sin(pSprite->ang) >> 16;
	int dz = 0;

	actFireVector(nSprite, pSprite->z, dx, dy, dz, kVectorClaw);
}


/****************************************************************************
** ThrowFCallback()
**
**
****************************************************************************/
static void ThrowFCallback( int /* type */, int nXIndex )
{
	XSPRITE *pXSprite = &xsprite[nXIndex];
	int nSprite = pXSprite->reference;
	SPRITE *pSprite = &sprite[nSprite];

	int nThing = actFireThing(
		nSprite, pSprite->z, gDudeSlope[nXIndex] + kSlopeThrow, kThingBoneClub, (M2X(14.0) << 4) / kTimerRate);
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
		aiNewState(pSprite, pXSprite, &gargoyleFSearch);

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
		aiNewState(pSprite, pXSprite, &gargoyleFGoto);
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
		aiNewState(pSprite, pXSprite, &gargoyleFSearch);
		return;
	}

	if ( IsPlayerSprite( pTarget ) )
	{
		PLAYER *pPlayer = &gPlayer[ pTarget->type - kDudePlayer1 ];
		if ( powerupCheck( pPlayer, kItemLtdInvisibility - kItemBase ) > 0 )
		{
			aiNewState(pSprite, pXSprite, &gargoyleFSearch);
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
					case kDudeFleshGargoyle:
						if ( dist < kFGargoyleThrowDist1 && dist > kFGargoyleThrowDist2 && qabs(losAngle) < kAngle15 )
							aiNewState(pSprite, pXSprite, &gargoyleFThrow);
						else if ( dist < kFGargoyleMeleeDist && qabs(losAngle) < kAngle15 )
						{
							aiNewState(pSprite, pXSprite, &gargoyleFSlash);
							sfxStart3DSound(pSprite->extra, kSfxGargAttack);
						}
						break;
				}
				return;
			}
		}
	}
	dprintf("Gargoyle %d lost sight of target %d\n", pXSprite->reference, pXSprite->target);
	aiNewState(pSprite, pXSprite, &gargoyleFGoto);
	pXSprite->target = -1;
}

static void entryFStatue( SPRITE *pSprite, XSPRITE */*pXSprite*/ )
{
	pSprite->type = kDudeFleshGargoyle;
}

