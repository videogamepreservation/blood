/*******************************************************************************
	FILE:			SEQ.CPP

	DESCRIPTION:	Sequence engine

	AUTHOR:			Peter M. Freese
	CREATED:		06-23-95
	COPYRIGHT:		Copyright (c) 1995 Q Studios Corporation
*******************************************************************************/

#include <string.h>

#include "seq.h"
#include "db.h"
#include "debug4g.h"
#include "globals.h"
#include "tile.h"
#include "error.h"
#include "actor.h"
#include "sfx.h"

#define kMaxSequences	1024

struct ACTIVE
{
	uchar	type;
	ushort	index;
};

struct SEQINST
{
	RESHANDLE	hSeq;
	Seq			*pSequence;
	SEQCALLBACK	callback;
	short		timeCounter;		// age of current frame in ticks
	uchar		frameIndex;			// current frame
	BOOL		isPlaying;
	void		Update( ACTIVE *active );
};


SEQINST siWall[kMaxXWalls], siMasked[kMaxXWalls];
SEQINST siCeiling[kMaxXSectors], siFloor[kMaxXSectors];
SEQINST siSprite[kMaxXSprites];
ACTIVE activeList[kMaxSequences];
static int activeCount = 0;


void Seq::Preload( void )
{
	if ( memcmp(signature, kSEQSig, sizeof(signature)) != 0 )
		ThrowError("Invalid sequence", ES_ERROR);

	if ( (version & 0xFF00) != (kSEQVersion & 0xFF00) )
		ThrowError("Obsolete sequence version", ES_ERROR);

	for (int i = 0; i < nFrames; i++)
		tilePreloadTile(frame[i].nTile);
}


static void UpdateSprite( int nXSprite, SEQFRAME *pFrame )
{
	dassert(nXSprite > 0 && nXSprite < kMaxXSprites);
	int nSprite = xsprite[nXSprite].reference;
	dassert(nSprite >= 0 && nSprite < kMaxSprites);
	SPRITE *pSprite = &sprite[nSprite];
	dassert(pSprite->extra == nXSprite);

	// set the motion bit if the vertical size or position of the sprite changes
	if ( (pSprite->flags & kAttrGravity) &&
		( tilesizy[pSprite->picnum] != tilesizy[pFrame->nTile] ||
		picanm[pSprite->picnum].ycenter != picanm[pFrame->nTile].ycenter ||
		pFrame->yrepeat && pFrame->yrepeat != pSprite->yrepeat) )
		pSprite->flags |= kAttrFalling;

	pSprite->picnum = (short)pFrame->nTile;
	pSprite->pal = (char)pFrame->pal;

//	if (pFrame->shadeRelative)
//		pSprite->shade += (char)pFrame->shade;
//	else
		pSprite->shade = (char)pFrame->shade;

	if (pFrame->xrepeat)
		pSprite->xrepeat = (uchar)pFrame->xrepeat;
	if (pFrame->yrepeat)
		pSprite->yrepeat = (uchar)pFrame->yrepeat;

	if ( pFrame->translucent )
		pSprite->cstat |= kSpriteTranslucent;
	else
		pSprite->cstat &= ~kSpriteTranslucent;

	if ( pFrame->translucentR )
		pSprite->cstat |= kSpriteTranslucentR;
	else
		pSprite->cstat &= ~kSpriteTranslucentR;

	if ( pFrame->blocking )
		pSprite->cstat |= kSpriteBlocking;
	else
		pSprite->cstat &= ~kSpriteBlocking;

	if ( pFrame->hitscan )
		pSprite->cstat |= kSpriteHitscan;
	else
		pSprite->cstat &= ~kSpriteHitscan;

	if ( pFrame->smoke )
		pSprite->flags |= kAttrSmoke;
	else
		pSprite->flags &= ~kAttrSmoke;
}


static void UpdateWall( int nXWall, SEQFRAME *pFrame )
{
	dassert(nXWall > 0 && nXWall < kMaxXWalls);
	int nWall = xwall[nXWall].reference;
	dassert(nWall >= 0 && nWall < kMaxWalls);
	WALL *pWall = &wall[nWall];
	dassert(pWall->extra == nXWall);

	pWall->picnum = (short)pFrame->nTile;
//	pWall->shade = (char)pFrame->shade;
	pWall->pal = (char)pFrame->pal;

	if ( pFrame->translucent )
		pWall->cstat |= kWallTranslucent;
	else
		pWall->cstat &= ~kWallTranslucent;

	if ( pFrame->translucentR )
		pWall->cstat |= kWallTranslucentR;
	else
		pWall->cstat &= ~kWallTranslucentR;

	if ( pFrame->blocking )
		pWall->cstat |= kWallBlocking;
	else
		pWall->cstat &= ~kWallBlocking;

	if ( pFrame->hitscan )
		pWall->cstat |= kWallHitscan;
	else
		pWall->cstat &= ~kWallHitscan;
}


static void UpdateMasked( int nXWall, SEQFRAME *pFrame )
{
	dassert(nXWall > 0 && nXWall < kMaxXWalls);
	int nWall = xwall[nXWall].reference;
	dassert(nWall >= 0 && nWall < kMaxWalls);
	WALL *pWall = &wall[nWall];
	dassert(pWall->extra == nXWall);
	dassert(pWall->nextwall >= 0);		// it must be a 2 sided wall
	WALL *pWall2 = &wall[pWall->nextwall];

	pWall->overpicnum = pWall2->overpicnum = (short)pFrame->nTile;
//	pWall->shade = (char)pFrame->shade;
	pWall->pal = pWall2->pal = (char)pFrame->pal;

	if ( pFrame->translucent )
	{
		pWall->cstat |= kWallTranslucent;
		pWall2->cstat |= kWallTranslucent;
	}
	else
	{
		pWall->cstat &= ~kWallTranslucent;
		pWall2->cstat &= ~kWallTranslucent;
	}

	if ( pFrame->translucentR )
	{
		pWall->cstat |= kWallTranslucentR;
		pWall2->cstat |= kWallTranslucentR;
	}
	else
	{
		pWall->cstat &= ~kWallTranslucentR;
		pWall2->cstat &= ~kWallTranslucentR;
	}

	if ( pFrame->blocking )
	{
		pWall->cstat |= kWallBlocking;
		pWall2->cstat |= kWallBlocking;
	}
	else
	{
		pWall->cstat &= ~kWallBlocking;
		pWall2->cstat &= ~kWallBlocking;
	}

	if ( pFrame->hitscan )
	{
		pWall->cstat |= kWallHitscan;
		pWall2->cstat |= kWallHitscan;
	}
	else
	{
		pWall->cstat &= ~kWallHitscan;
		pWall2->cstat &= ~kWallHitscan;
	}
}


static void UpdateFloor( int nXSector, SEQFRAME *pFrame )
{
	dassert(nXSector > 0 && nXSector < kMaxXSectors);
	int nSector = xsector[nXSector].reference;
	dassert(nSector >= 0 && nSector < kMaxSectors);
	SECTOR *pSector = &sector[nSector];
	dassert(pSector->extra == nXSector);

	pSector->floorpicnum = (short)pFrame->nTile;
	pSector->floorshade = (char)pFrame->shade;
	pSector->floorpal = (char)pFrame->pal;
}


static void UpdateCeiling( int nXSector, SEQFRAME *pFrame )
{
	dassert(nXSector > 0 && nXSector < kMaxXSectors);
	int nSector = xsector[nXSector].reference;
	dassert(nSector >= 0 && nSector < kMaxSectors);
	SECTOR *pSector = &sector[nSector];
	dassert(pSector->extra == nXSector);

	pSector->ceilingpicnum = (short)pFrame->nTile;
	pSector->ceilingshade = (char)pFrame->shade;
	pSector->ceilingpal = (char)pFrame->pal;
}


void SEQINST::Update( ACTIVE * active )
{
	dassert(frameIndex < pSequence->nFrames);
	switch (active->type)
	{
		case SS_WALL:
			UpdateWall(active->index, &pSequence->frame[frameIndex]);
			break;

		case SS_CEILING:
			UpdateCeiling(active->index, &pSequence->frame[frameIndex]);
			break;

		case SS_FLOOR:
			UpdateFloor(active->index, &pSequence->frame[frameIndex]);
			break;

		case SS_SPRITE:
			UpdateSprite(active->index, &pSequence->frame[frameIndex]);
			break;

		case SS_MASKED:
			UpdateMasked(active->index, &pSequence->frame[frameIndex]);
			break;
	}

	if ( pSequence->frame[frameIndex].trigger && callback != NULL )
		callback(active->type, active->index);
}


SEQINST *GetInstance( int type, int nXIndex )
{
	switch ( type )
	{
		case SS_WALL:
			dassert(nXIndex > 0 && nXIndex < kMaxXWalls);
			return &siWall[nXIndex];

		case SS_CEILING:
			dassert(nXIndex > 0 && nXIndex < kMaxXSectors);
			return &siCeiling[nXIndex];

		case SS_FLOOR:
			dassert(nXIndex > 0 && nXIndex < kMaxXSectors);
			return &siFloor[nXIndex];

		case SS_SPRITE:
			dassert(nXIndex > 0 && nXIndex < kMaxXSprites);
			return &siSprite[nXIndex];

		case SS_MASKED:
			dassert(nXIndex > 0 && nXIndex < kMaxXWalls);
			return &siMasked[nXIndex];

	}

	ThrowError("Unexcepted object type", ES_ERROR);
	return NULL;
}


void UnlockInstance( SEQINST *pInst )
{
	dassert( pInst != NULL );
	dassert( pInst->hSeq != NULL );
	dassert( pInst->pSequence != NULL );

	gSysRes.Unlock( pInst->hSeq );
	pInst->hSeq = NULL;
	pInst->pSequence = NULL;
	pInst->isPlaying = FALSE;
}


void seqSpawn( int nSeqID, int type, int nXIndex, SEQCALLBACK callback )
{
	SEQINST *pInst = GetInstance(type, nXIndex);

	RESHANDLE hSeq = gSysRes.Lookup(nSeqID, ".SEQ");
	dassert( hSeq != NULL);

	int i = activeCount;
	if ( pInst->isPlaying )
	{
		// already playing this sequence?
		if (pInst->hSeq == hSeq)
			return;

		UnlockInstance( pInst );

		for (i = 0; i < activeCount; i++)
		{
			if (activeList[i].type == type && activeList[i].index == nXIndex)
				break;
		}

		dassert(i < activeCount);	// should have been found
	}

	Seq *pSequence = (Seq *)gSysRes.Lock(hSeq);

	if ( memcmp(pSequence->signature, kSEQSig, sizeof(pSequence->signature)) != 0 )
		ThrowError("Invalid sequence", ES_ERROR);

	if ( (pSequence->version & 0xFF00) != (kSEQVersion & 0xFF00) )
		ThrowError("Obsolete sequence version", ES_ERROR);

	pInst->hSeq = hSeq;
	pInst->pSequence = pSequence;
	pInst->callback = callback;
	pInst->isPlaying = TRUE;
	pInst->timeCounter = pSequence->ticksPerFrame;
	pInst->frameIndex = 0;

	if (i == activeCount)
	{
		dassert(activeCount < kMaxSequences);
		activeList[activeCount].type = (uchar)type;
		activeList[activeCount].index = (short)nXIndex;
		activeCount++;
	}

	pInst->Update(&activeList[i]);
}


void seqKill( int type, int nXIndex )
{
	SEQINST *pInst = GetInstance(type, nXIndex);

	if ( pInst->isPlaying )
	{
		for (int i = 0; i < activeCount; i++)
		{
			if (activeList[i].type == type && activeList[i].index == nXIndex)
				break;
		}

		dassert(i < activeCount);

		// remove it from the active list
		activeList[i] = activeList[--activeCount];
		pInst->isPlaying = FALSE;

		UnlockInstance( pInst );
	}
}


void seqKillAll( void )
{
	int i;

	for (i = 0; i < kMaxXWalls; i++)
	{
		if ( siWall[i].isPlaying )
			UnlockInstance( &siWall[i] );

		if ( siMasked[i].isPlaying )
			UnlockInstance( &siMasked[i] );
	}

	for (i = 0; i < kMaxXSectors; i++)
	{
		if ( siCeiling[i].isPlaying )
			UnlockInstance( &siCeiling[i] );
		if ( siFloor[i].isPlaying )
			UnlockInstance( &siFloor[i] );
	}

	for (i = 0; i < kMaxXSprites; i++)
	{
		if ( siSprite[i].isPlaying )
			UnlockInstance( &siSprite[i] );
	}

	activeCount = 0;
}


int seqGetStatus( int type, int nXIndex )
{
	SEQINST *pInst = GetInstance(type, nXIndex);
	if ( pInst->isPlaying )
		return pInst->frameIndex;
	return -1;
}


void seqProcess( int nTicks )
{
	for (int i = 0; i < activeCount; i++)
	{
		SEQINST *pInst = GetInstance(activeList[i].type, activeList[i].index);
		Seq *pSeq = pInst->pSequence;

		dassert(pInst->frameIndex < pSeq->nFrames);

		pInst->timeCounter -= nTicks;
		while (pInst->timeCounter < 0)
		{
			pInst->timeCounter += pSeq->ticksPerFrame;
			++pInst->frameIndex;

			if (pInst->frameIndex == pSeq->nFrames)
			{
				if ( pSeq->flags & kSeqLoop )
					pInst->frameIndex = 0;
				else
				{
					UnlockInstance( pInst );

					if ( pSeq->flags & kSeqRemove )
					{
						short nSprite, nWall, nNextWall;
						switch (activeList[i].type)
						{
							case SS_SPRITE:
								nSprite = (short)xsprite[activeList[i].index].reference;
								dassert(nSprite >= 0 && nSprite < kMaxSprites);
								deletesprite(nSprite);	// safe to not use actPostSprite here
								break;
							case SS_MASKED:
								nWall = (short)xwall[activeList[i].index].reference;
								dassert(nWall >= 0 && nWall < kMaxWalls);
 								wall[nWall].cstat &= ~kWallMasked & ~kWallFlipX & ~kWallOneWay;
								if ((nNextWall = wall[nWall].nextwall) != -1)
	 								wall[nNextWall].cstat &= ~kWallMasked & ~kWallFlipX & ~kWallOneWay;
								break;
						}
					}

					// remove it from the active list
					activeList[i--] = activeList[--activeCount];
					break;
				}
			}
			pInst->Update(&activeList[i]);
		}
	}
}
