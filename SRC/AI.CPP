#include <stdio.h>
#include <string.h>

#include "engine.h"
#include "ai.h"
#include "db.h"
#include "trig.h"
#include "misc.h"
#include "actor.h"
#include "player.h"
#include "globals.h"
#include "gameutil.h"
#include "multi.h"
#include "dude.h"
#include "debug4g.h"
#include "eventq.h"

#include "aicult.h"
#include "aigarg.h"
#include "aihand.h"
#include "aihound.h"
#include "airat.h"
#include "aispid.h"
#include "aizomba.h"
#include "aizombf.h"

/****************************************************************************
** LOCAL CONSTANTS
****************************************************************************/

#define kAIThinkRate		8	// how often high level AI is sampled (in frames)
#define kAIThinkMask		(kAIThinkRate - 1)
#define kAIThinkTime		(kAIThinkRate * kFrameTicks)

#define kSGargoyleMeleeDist		M2X(2.0)	//M2X(1.6)
#define kSGargoyleBlastDist1	M2X(20)	// used for paralyzing blast
#define kSGargoyleBlastDist2	M2X(14)

#define kTentacleActivateDist	M2X(5)	// activates and stays on until target reaches deactivate distance?
#define kTentacleDeactivateDist	M2X(9)
#define kTentacleMeleeDist		M2X(2)

#define kPodActivateDist		M2X(8)
#define kPodDeactivateDist		M2X(14)
#define kPodFireDist1			M2X(12)
#define kPodFireDist2			M2X(8)

#define kGillBeastMeleeDist		M2X(1.6)
#define kGillBeastSummonDist1	M2X(16)
#define kGillBeastSummonDist2	M2X(12)

#define kEelMeleeDist		M2X(1)
#define kRatMeleeDist		M2X(1)
#define kHandMeleeDist		M2X(1)

#define kCerberusMeleeDist	M2X(2)
#define kCerberusBlastDist1	M2X(14)	// used for fireball
#define kCerberusBlastDist2	M2X(10)

#define kPhantasmMeleeDist	M2X(1.6)
#define kPhantasmThrowDist1	M2X(16)
#define kPhantasmThrowDist2	M2X(12)


static int cumulDamage[kMaxXSprites];	// cumulative damage per frame
int gDudeSlope[kMaxXSprites];


static AISTATE genIdle		= { kSeqDudeIdle,	NULL,	0,	NULL,	NULL,	NULL,	NULL };
static AISTATE genRecoil	= { kSeqDudeRecoil,	NULL,	20,	NULL,	NULL,	NULL,	&genIdle };

//struct AISOUND
//{
//	short sightID;
//	short painID;
//	short attackID;
//	short altAttackID;
//	short deathID;
//};


/*******************************************************************************

AI design idea: p-code kernel which a short program for each dude type.

P-code primitives:

PlaySeq [id] [callback]
PlaySound [id]
*******************************************************************************/



/****************************************************************************
** GLOBALS
****************************************************************************/


/****************************************************************************
** LOCALS
****************************************************************************/

void aiNewState( SPRITE *pSprite, XSPRITE *pXSprite, AISTATE *pState )
{
	DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];
	pXSprite->stateTimer = pState->ticks;
	pXSprite->aiState = pState;

	if ( pState->seqId >= 0 )
	{
		if ( gSysRes.Lookup(pDudeInfo->seqStartID + pState->seqId,".SEQ") == NULL )
		{
			dprintf("NULL sequence, dudeType = %d, seqId = %d\n", pSprite->type, pState->seqId);
			return;
		}
		seqSpawn(pDudeInfo->seqStartID + pState->seqId, SS_SPRITE, pSprite->extra, pState->seqCallback);
	}

	// call the enter function if defined
	if ( pState->enter )
		pState->enter(pSprite, pXSprite);
}


BOOL CanMove( SPRITE *pSprite, int nTarget, int ang, int dist )
{
	int zTop, zBot;
	GetSpriteExtents(pSprite, &zTop, &zBot);

	int dx = mulscale30(dist, Cos(ang));
	int dy = mulscale30(dist, Sin(ang));

	int x = pSprite->x;
	int y = pSprite->y;
	int z = pSprite->z;

	HITINFO hitInfo;
	HitScan(pSprite, z, dx, dy, 0, &hitInfo);
	int hitDist = qdist(x - hitInfo.hitx, y - hitInfo.hity);
	if ( hitDist - (pSprite->clipdist << 2) < dist )
	{
		// okay to be blocked by target
		if ( hitInfo.hitsprite >= 0 && hitInfo.hitsprite == nTarget )
			return TRUE;
		return FALSE;
	}

	x += dx;
	y += dy;
	short nSector = pSprite->sectnum;
	if ( !FindSector(x, y, z, &nSector) )
		return FALSE;

	long floorZ = getflorzofslope(nSector, x, y);

	// this should go into the dude table and be time relative
	if ( floorZ - zBot > M2Z(1.0) )
		return FALSE;

	return TRUE;
}


void aiChooseDirection( SPRITE *pSprite, XSPRITE *pXSprite, int ang )
{
	dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];

	int dang = ((kAngle180 + ang - pSprite->ang) & kAngleMask) - kAngle180;

	long sin = Sin(pSprite->ang);
	long cos = Cos(pSprite->ang);

	// find vel and svel relative to current angle
//	long vel = dmulscale30(pSprite->xvel, cos, pSprite->yvel, sin);
//	long svel = dmulscale30(pSprite->xvel, sin, -pSprite->yvel, cos);

	// look 1.0 second ahead
	int avoidDist = pDudeInfo->frontSpeed * kTimerRate >> 4;
	int turnTo = kAngle60;
	if (dang < 0 )
		turnTo = -turnTo;

	// clear movement toward target?
	if ( CanMove(pSprite, pXSprite->target, pSprite->ang + dang, avoidDist) )
	{
		pXSprite->goalAng = (pSprite->ang + dang) & kAngleMask;
	}
	// clear movement partially toward target?
	else if ( CanMove(pSprite, pXSprite->target, pSprite->ang + dang / 2, avoidDist) )
	{
		pXSprite->goalAng = (pSprite->ang + dang / 2) & kAngleMask;
	}
	// try turning in target direction
	else if ( CanMove(pSprite, pXSprite->target, pSprite->ang + turnTo, avoidDist) )
	{
		pXSprite->goalAng = (pSprite->ang + turnTo) & kAngleMask;
	}
	// clear movement straight?
	else if ( CanMove(pSprite, pXSprite->target, pSprite->ang, avoidDist) )
	{
		pXSprite->goalAng = pSprite->ang;
	}
	// try turning away
	else if ( CanMove(pSprite, pXSprite->target, pSprite->ang - turnTo, avoidDist) )
	{
		pXSprite->goalAng = (pSprite->ang - turnTo) & kAngleMask;
	}
	else
	{
	// just turn around
		pXSprite->goalAng = (pSprite->ang + kAngle180) & kAngleMask;
	}

	// choose dodge direction
	pXSprite->dodgeDir = Chance(0x8000) ? 1 : -1;
	if ( !CanMove(pSprite, pXSprite->target, pSprite->ang + kAngle90 * pXSprite->dodgeDir,
		pDudeInfo->sideSpeed * 90 >> 4) )
	{
		pXSprite->dodgeDir = -pXSprite->dodgeDir;
		if ( !CanMove(pSprite, pXSprite->target, pSprite->ang + kAngle90 * pXSprite->dodgeDir,
			pDudeInfo->sideSpeed * 90 >> 4) )
			pXSprite->dodgeDir = 0;
	}

//	pSprite->zvel = (short)(dz >> 8);
}


void aiMoveForward( SPRITE *pSprite, XSPRITE *pXSprite )
{
	dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];

	pXSprite->moveState = kMoveWalk;

	int dang = ((kAngle180 + pXSprite->goalAng - pSprite->ang) & kAngleMask) - kAngle180;
	int maxTurn = pDudeInfo->angSpeed * kFrameTicks >> 4;

	pSprite->ang = (short)((pSprite->ang + ClipRange(dang, -maxTurn, maxTurn)) & kAngleMask);
	int accel = (pDudeInfo->frontAccel + kGroundFriction) * kFrameTicks;

	// don't move forward if trying to turn around
	if ( qabs(dang) > kAngle60 )
		return;

	long sin = Sin(pSprite->ang);
	long cos = Cos(pSprite->ang);

	// find vel and svel relative to current angle
	long vel = dmulscale30(pSprite->xvel, cos, pSprite->yvel, sin);
	long svel = dmulscale30(pSprite->xvel, sin, -pSprite->yvel, cos);

	// acceleration
	if ( accel > 0 )
	{
		if ( vel < pDudeInfo->frontSpeed )
			vel = ClipHigh(vel + accel, pDudeInfo->frontSpeed);
	}
	else
	{
		if ( vel > -pDudeInfo->backSpeed )
			vel = ClipLow(vel + accel, pDudeInfo->backSpeed);
	}

	// reconstruct x and y velocities
	pSprite->xvel = (short)dmulscale30(vel, cos, svel, sin);
	pSprite->yvel = (short)dmulscale30(vel, sin, -svel, cos);
}


void aiMoveTurn( SPRITE *pSprite, XSPRITE *pXSprite )
{
	dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];

	pXSprite->moveState = kMoveWalk;

	int dang = ((kAngle180 + pXSprite->goalAng - pSprite->ang) & kAngleMask) - kAngle180;
	int maxTurn = pDudeInfo->angSpeed * kFrameTicks >> 4;

	pSprite->ang = (short)((pSprite->ang + ClipRange(dang, -maxTurn, maxTurn)) & kAngleMask);
}


void aiMoveDodge( SPRITE *pSprite, XSPRITE *pXSprite )
{
	dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];

	pXSprite->moveState = kMoveWalk;

	int dang = ((kAngle180 + pXSprite->goalAng - pSprite->ang) & kAngleMask) - kAngle180;
	int maxTurn = pDudeInfo->angSpeed * kFrameTicks >> 4;
	pSprite->ang = (short)((pSprite->ang + ClipRange(dang, -maxTurn, maxTurn)) & kAngleMask);

	if ( pXSprite->dodgeDir == 0 )
		return;

	int accel = (pDudeInfo->sideAccel + kGroundFriction) * kFrameTicks;
	long sin = Sin(pSprite->ang);
	long cos = Cos(pSprite->ang);

	// find vel and svel relative to current angle
	long vel = dmulscale30(pSprite->xvel, cos, pSprite->yvel, sin);
	long svel = dmulscale30(pSprite->xvel, sin, -pSprite->yvel, cos);

	if ( pXSprite->dodgeDir > 0 )
	{
		if ( svel < pDudeInfo->sideSpeed )
			svel = ClipHigh(svel + accel, pDudeInfo->sideSpeed);
	}
	else
	{
		if ( svel > -pDudeInfo->sideSpeed )
			svel = ClipLow(svel - accel, -pDudeInfo->sideSpeed);
	}

	// reconstruct x and y velocities
	pSprite->xvel = (short)dmulscale30(vel, cos, svel, sin);
	pSprite->yvel = (short)dmulscale30(vel, sin, -svel, cos);
}


void aiActivateDude( SPRITE *pSprite, XSPRITE *pXSprite )
{
	dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];


	if ( pXSprite->state == 0 )
	{
		// this doesn't take into account sprites triggered w/o a target location....
		int nAngle = getangle(pXSprite->targetX - pSprite->x, pXSprite->targetY - pSprite->y);
		aiChooseDirection(pSprite, pXSprite, nAngle);
		pXSprite->state = 1;
	}

	switch ( pSprite->type )
	{
		case kDudeTommyCultist:
		case kDudeShotgunCultist:
			if (pXSprite->target == -1)
				aiNewState(pSprite, pXSprite, &cultistSearch);
			else
				aiNewState(pSprite, pXSprite, &cultistChase);
			break;

		case kDudeAxeZombie:
			if (pXSprite->target == -1)
				aiNewState(pSprite, pXSprite, &zombieASearch);
			else
				aiNewState(pSprite, pXSprite, &zombieAChase);
			break;

		case kDudeEarthZombie:
			if (pXSprite->aiState == &zombieEIdle)
				aiNewState(pSprite, pXSprite, &zombieEUp);
			break;

		case kDudeFatZombie:
			if (pXSprite->target == -1)
				aiNewState(pSprite, pXSprite, &zombieFSearch);
			else
				aiNewState(pSprite, pXSprite, &zombieFChase);
			break;

		case kDudeFleshStatue:
			aiNewState(pSprite, pXSprite, &gargoyleFMorph);
			break;

		case kDudeStoneStatue:
//			seqSpawn(pDudeInfo->seqStartID + kSeqStoneStatueTurn, SS_SPRITE, pSprite->extra);
			pSprite->type = kDudeStoneGargoyle;
			break;

		case kDudeHound:
			if (pXSprite->target == -1)
				aiNewState(pSprite, pXSprite, &houndSearch);
			else
				aiNewState(pSprite, pXSprite, &houndChase);
			break;

		case kDudeHand:
			if (pXSprite->target == -1)
				aiNewState(pSprite, pXSprite, &handSearch);
			else
				aiNewState(pSprite, pXSprite, &handChase);
			break;

		case kDudeRat:
			if (pXSprite->target == -1)
				aiNewState(pSprite, pXSprite, &ratSearch);
			else
				aiNewState(pSprite, pXSprite, &ratChase);
			break;

		case kDudeBrownSpider:
		case kDudeRedSpider:
		case kDudeBlackSpider:
		case kDudeMotherSpider:
			pSprite->flags |= kAttrGravity;
			pSprite->cstat &= ~kSpriteFlipY;
			if (pXSprite->target == -1)
				aiNewState(pSprite, pXSprite, &spidSearch);
			else
				aiNewState(pSprite, pXSprite, &spidChase);
			break;
	}
}


/*******************************************************************************
	FUNCTION:		aiSetTarget()

	DESCRIPTION:	Target a location (as opposed to a sprite)
*******************************************************************************/
void aiSetTarget( XSPRITE *pXSprite, int x, int y, int z )
{
	pXSprite->target = -1;
	pXSprite->targetX = x;
	pXSprite->targetY = y;
	pXSprite->targetZ = z;
}


void aiSetTarget( XSPRITE *pXSprite, int nTarget )
{
	dassert(nTarget >= 0 && nTarget < kMaxSprites);
	SPRITE *pTarget = &sprite[nTarget];

	if ( pTarget->type < kDudeBase || pTarget->type >= kDudeMax )
		return;

	if ( nTarget == sprite[pXSprite->reference].owner )
	{
		dprintf("aiSetTarget: skipping owner\n");
		return;
	}

//	dassert(pTarget->type >= kDudeBase && pTarget->type < kDudeMax);
	DUDEINFO *pTargetInfo = &dudeInfo[pTarget->type - kDudeBase];

	pXSprite->target = nTarget;
	pXSprite->targetX = pTarget->x;
	pXSprite->targetY = pTarget->y;
	pXSprite->targetZ = pTarget->z - (pTargetInfo->eyeHeight * pTarget->yrepeat << 2);
}


void aiDamageSprite( SPRITE *pSprite, XSPRITE *pXSprite, int nSource, DAMAGE_TYPE nDamageType, int nDamage )
{
	(void)nDamageType;

	dassert(nSource < kMaxSprites);

	if (pXSprite->health == 0)
		return;

	cumulDamage[pSprite->extra] += nDamage;	// add to cumulative damage

	DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];

	if (nSource >= 0 && nSource != pXSprite->reference)
	{
		if (pXSprite->target == -1)
		{
			// give a dude a target
			aiSetTarget(pXSprite, nSource);
			aiActivateDude(pSprite, pXSprite);
		}
		else if (nSource != pXSprite->target)
		{
			// retarget
			int nThresh = nDamage;

			if ( sprite[nSource].type == pSprite->type )
				nThresh *= pDudeInfo->changeTargetKin;
			else
				nThresh *= pDudeInfo->changeTarget;

			if ( Chance(nThresh) )
			{
				aiSetTarget(pXSprite, nSource);
				aiActivateDude(pSprite, pXSprite);
			}
		}

		// you DO need special processing here or somewhere else (your choice) for dodging
		switch ( pSprite->type )
		{
			case kDudeTommyCultist:
			case kDudeShotgunCultist:
//					if (nDamage >= (pDudeInfo->hinderDamage << 2))
					aiNewState(pSprite, pXSprite, &cultistDodge);
				break;

			case kDudeFleshGargoyle:
				aiNewState(pSprite, pXSprite, &gargoyleFChase);
				break;

			default:
				break;
		}
	}
}


void RecoilDude( SPRITE *pSprite, XSPRITE *pXSprite )
{
	dprintf("Recoiling dude\n");
	switch ( pSprite->type )
	{
		case kDudeTommyCultist:
		case kDudeShotgunCultist:
			aiNewState(pSprite, pXSprite, &cultistRecoil);
			break;

		case kDudeFatZombie:
			aiNewState(pSprite, pXSprite, &zombieFRecoil);
			break;

		case kDudeAxeZombie:
		case kDudeEarthZombie:
			aiNewState(pSprite, pXSprite, &zombieARecoil);
			break;

		case kDudeFleshGargoyle:
			aiNewState(pSprite, pXSprite, &gargoyleFRecoil);
			break;

		case kDudeHound:
			aiNewState(pSprite, pXSprite, &houndRecoil);
			break;

		case kDudeHand:
			aiNewState(pSprite, pXSprite, &handRecoil);
			break;

		case kDudeRat:
			aiNewState(pSprite, pXSprite, &handRecoil);
			break;

		case kDudeBrownSpider:
		case kDudeRedSpider:
		case kDudeBlackSpider:
		case kDudeMotherSpider:
			aiNewState(pSprite, pXSprite, &spidDodge);
			break;

		default:
			aiNewState(pSprite, pXSprite, &genRecoil);
			break;
	}
}


void aiThinkTarget( SPRITE *pSprite, XSPRITE *pXSprite )
{
	dassert(pSprite->type >= kDudeBase && pSprite->type < kDudeMax);
	DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];

	if ( !Chance(pDudeInfo->alertChance) )
		return;

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


void aiProcessDudes( void )
{
	// process active sprites
	for (short nSprite = headspritestat[kStatDude]; nSprite >= 0; nSprite = nextspritestat[nSprite])
	{
		SPRITE *pSprite = &sprite[nSprite];
		int nXSprite = pSprite->extra;
		XSPRITE *pXSprite = &xsprite[nXSprite];
	 	DUDEINFO *pDudeInfo = &dudeInfo[pSprite->type - kDudeBase];

		// don't manipulate players or dead guys
		if (( pSprite->type >= kDudePlayer1 && pSprite->type <= kDudePlayer8 ) || ( pXSprite->health == 0 ))
			continue;

		pXSprite->stateTimer = ClipLow(pXSprite->stateTimer - kFrameTicks, 0);

		if ( pXSprite->aiState->move )
			pXSprite->aiState->move(pSprite, pXSprite);

		if ( pXSprite->aiState->think && (gFrame & kAIThinkMask) == (nSprite & kAIThinkMask) )
			pXSprite->aiState->think(pSprite, pXSprite);

		if ( pXSprite->stateTimer == 0 && pXSprite->aiState->next != NULL )
		{
			if ( pXSprite->aiState->ticks > 0 )
				aiNewState(pSprite, pXSprite, pXSprite->aiState->next);
			else if ( seqGetStatus(SS_SPRITE, nXSprite) < 0 )
				aiNewState(pSprite, pXSprite, pXSprite->aiState->next);
		}

		// process dudes for recoil
		if ( cumulDamage[nXSprite] >= pDudeInfo->hinderDamage << 4 )
			RecoilDude(pSprite, pXSprite);
	}

	// reset the cumulative damages for the next frame
	memset(cumulDamage, 0, sizeof(cumulDamage));

/*
		// special processing for converting dude types
		switch ( pSprite->type )
		{
			case kDudeCerberus:
				if ( pXSprite->health <= 0 && seqGetStatus(SS_SPRITE, nXSprite) < 0 )	// head #1 finished dying?
				{
					pXSprite->health = dudeInfo[kDudeCerberus2 - kDudeBase].startHealth << 4;
					pSprite->type = kDudeCerberus2;		// change his type
//						aiActivateDude(pSprite, pXSprite);	// reactivate him
				}
				break;
		}
*/
}


/*******************************************************************************
	FUNCTION:		aiInit()

	DESCRIPTION:

	PARAMETERS:		void

	RETURNS:		void

	NOTES:
*******************************************************************************/
void aiInit( void )
{
	for (short nSprite = headspritestat[kStatDude]; nSprite >= 0; nSprite = nextspritestat[nSprite])
	{
		SPRITE *pDude = &sprite[nSprite];
		XSPRITE *pXDude = &xsprite[pDude->extra];

		switch ( pDude->type )
		{
			case kDudeTommyCultist:
			case kDudeShotgunCultist:
				aiNewState(pDude, pXDude, &cultistIdle);
				break;

			case kDudeFatZombie:
				aiNewState(pDude, pXDude, &zombieFIdle);
				break;

			case kDudeAxeZombie:
				aiNewState(pDude, pXDude, &zombieAIdle);
				break;

			case kDudeEarthZombie:
				aiNewState(pDude, pXDude, &zombieEIdle);
				pDude->flags &= ~kAttrMove;
				break;

			case kDudeFleshGargoyle:
				aiNewState(pDude, pXDude, &gargoyleFIdle);
				break;

			case kDudeHound:
				aiNewState(pDude, pXDude, &houndIdle);
				break;

			case kDudeHand:
				aiNewState(pDude, pXDude, &handIdle);
				break;

			case kDudeRat:
				aiNewState(pDude, pXDude, &ratIdle);
				break;

			case kDudeBrownSpider:
			case kDudeRedSpider:
			case kDudeBlackSpider:
			case kDudeMotherSpider:
				aiNewState(pDude, pXDude, &spidIdle);
				break;

			default:
				aiNewState(pDude, pXDude, &genIdle);
				break;
		}

		aiSetTarget(pXDude, 0, 0, 0);

		pXDude->stateTimer	= 0;

		switch ( pDude->type )
		{
			case kDudeBrownSpider:
			case kDudeRedSpider:
			case kDudeBlackSpider:
				if ( pDude->cstat & kSpriteFlipY )
					pDude->flags &= ~kAttrGravity;
				break;

			case kDudeBat:
			case kDudeFleshGargoyle:
			case kDudeStoneGargoyle:
			case kDudePhantasm:
				pDude->flags &= ~kAttrGravity;
				break;
		}
	}
}

