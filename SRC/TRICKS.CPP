#include "actor.h"
#include "db.h"
#include "debug4g.h"
#include "engine.h"
#include "eventq.h"
#include "misc.h"
#include "trig.h"
#include "tricks.h"

void InitGenerator( int nSprite )
{
	dassert(nSprite < kMaxSprites);
	SPRITE *pSprite = &sprite[nSprite];
	dassert(pSprite->statnum != kMaxStatus);

	int nXSprite = pSprite->extra;
	dassert(nXSprite > 0);
	XSPRITE *pXSprite = &xsprite[nXSprite];

	switch ( sprite[nSprite].type )
	{
		case kGenTrigger:
			pSprite->cstat &= ~kSpriteBlocking;
			pSprite->cstat |= kSpriteInvisible;
			break;

		default:
			break; //return;
	}

	pXSprite->data3 = FALSE;
	if ( pXSprite->state != pXSprite->restState && pXSprite->data1 > 0 )
	{
		pXSprite->data3 = TRUE;
		evPost(nSprite, SS_SPRITE, (pXSprite->data1 + BiRandom(pXSprite->data2)) * kTimerRate / 10);
	}
}


void ActivateGenerator( int nSprite )
{
	dassert(nSprite < kMaxSprites);
	SPRITE *pSprite = &sprite[nSprite];
	dassert(pSprite->statnum != kMaxStatus);

	int nXSprite = pSprite->extra;
	dassert(nXSprite > 0);
	XSPRITE *pXSprite = &xsprite[nXSprite];

	switch ( pSprite->type )
	{
		case kGenWaterDrip:
		case kGenBloodDrip:
			// spawn a drip sprite that drops to the ground and turns to a splash
			break;

		case kGenFireball:
			dprintf("kGenFireball calling actFireMissile\n");
			actFireMissile( nSprite, pSprite->z, Cos(pSprite->ang) >> 16, Sin(pSprite->ang) >> 16, 0, kMissileFireball);
			break;

		case kGenEctoSkull:
		case kGenDart:
			// spawn a missile that moves and damages accordingly
			break;

		default:
			break;
	}
}
