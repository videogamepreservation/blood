#include "db.h"
#include "engine.h"
#include "globals.h"
#include "options.h"
#include "warp.h"
#include "error.h"
#include "debug4g.h"

/***********************************************************************
 * Global Data
 **********************************************************************/
ZONE gStartZone[kMaxPlayers];
short gUpperLink[kMaxSectors], gLowerLink[kMaxSectors];

/***********************************************************************
 * InitPlayerStartZones()
 *
 **********************************************************************/
void InitPlayerStartZones( void )
{
	int nSprite, nXSprite;

	// clear link values
	for ( int nSector = 0; nSector < kMaxSectors; nSector++ )
	{
		gUpperLink[nSector] = -1;
		gLowerLink[nSector] = -1;
	}

	for ( nSprite = 0; nSprite < kMaxSprites; nSprite++ )
	{
		if (sprite[nSprite].statnum < kMaxStatus)
		{
			SPRITE *pSprite = &sprite[nSprite];
			nXSprite = pSprite->extra;
			if ( nXSprite > 0 )
			{
				XSPRITE *pXSprite = &xsprite[nXSprite];
				switch( pSprite->type )
				{
					case kMarkerPlayerStart:
					{
						if ( (gNetMode == kNetModeOff || gNetMode == kNetModeCoop)
						&& (pXSprite->data1 >= 0 && pXSprite->data1 < kMaxPlayers) )
						{
							ZONE *pZone = &gStartZone[pXSprite->data1];
							pZone->x = pSprite->x;
							pZone->y = pSprite->y;
							pZone->z = pSprite->z;
							pZone->sector = pSprite->sectnum;
							pZone->angle = pSprite->ang;
						}
						deletesprite( (short)nSprite );
						break;
					}

					case kMarkerDeathStart:
						if ( gNetMode == kNetModeBloodBath
						&& (pXSprite->data1 >= 0 && pXSprite->data1 < kMaxPlayers) )
						{
							ZONE *pZone = &gStartZone[pXSprite->data1];
							pZone->x = pSprite->x;
							pZone->y = pSprite->y;
							pZone->z = pSprite->z;
							pZone->sector = pSprite->sectnum;
							pZone->angle = pSprite->ang;
						}
						deletesprite( (short)nSprite );
						break;

					case kMarkerUpperLink:
						gUpperLink[pSprite->sectnum] = (short)nSprite;
						pSprite->cstat |= kSpriteInvisible | kSpriteMapNever;
						pSprite->cstat &= ~kSpriteBlocking & ~kSpriteHitscan;
						break;

					case kMarkerLowerLink:
						gLowerLink[pSprite->sectnum] = (short)nSprite;
						pSprite->cstat |= kSpriteInvisible | kSpriteMapNever;
						pSprite->cstat &= ~kSpriteBlocking & ~kSpriteHitscan;
						break;
				}
			}
		}
	}

	// verify links have mates and connect them
	for (int nFrom = 0; nFrom < kMaxSectors; nFrom++)
	{
		if ( gUpperLink[nFrom] >= 0 )
		{
			SPRITE *pFromSprite = &sprite[gUpperLink[nFrom]];
			nXSprite = pFromSprite->extra;
			dassert(nXSprite > 0 && nXSprite < kMaxXSprites);
			XSPRITE *pXSprite = &xsprite[nXSprite];
			int nID = pXSprite->data1;

			for (int nTo = 0; nTo < kMaxSectors; nTo++)
			{
				if ( gLowerLink[nTo] >= 0 )
				{
					SPRITE *pToSprite = &sprite[gLowerLink[nTo]];
					nXSprite = pToSprite->extra;
					dassert(nXSprite > 0 && nXSprite < kMaxXSprites);
					XSPRITE *pXSprite = &xsprite[nXSprite];

					if ( pXSprite->data1 == nID )
					{
						pFromSprite->owner = (short)gLowerLink[nTo];
						pToSprite->owner = (short)gUpperLink[nFrom];
					}
				}
			}
		}
	}
}
