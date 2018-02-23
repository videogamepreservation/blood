#include <stdlib.h>

#include "engine.h"
#include "debug4g.h"
#include "error.h"
#include "eventq.h"
#include "db.h"
#include "pqueue.h"
#include "triggers.h"
#include "globals.h"
#include "levels.h"

#define kMaxChannels		4096
#define kMaxID			1024


static PriorityQueue eventQ;


/*******************************************************************************
Each channel bucket is a contiguous range in a rxBucket[] array.  The
bucketHead[] array points to the start index for each channel id.  Creating the
buckets is not particularly fast, but using them is very fast and storage
efficient.
*******************************************************************************/

struct RXBUCKET
{
	unsigned index	: 13; 	// object array index (sprite[], sector[], wall[])
	unsigned type	: 3;	// 0=sprite, 1=sector, 2=wall
} rxBucket[kMaxChannels];

ushort bucketHead[kMaxID + 1];


static int GetBucketChannel( RXBUCKET *pBucket )
{
	int nXIndex;

	switch (pBucket->type)
	{
		case SS_SECTOR:
			nXIndex = sector[pBucket->index].extra;
			dassert(nXIndex > 0);
			return xsector[nXIndex].rxID;

		case SS_WALL:
			nXIndex = wall[pBucket->index].extra;
			dassert(nXIndex > 0);
			return xwall[nXIndex].rxID;

		case SS_SPRITE:
			nXIndex = sprite[pBucket->index].extra;
			dassert(nXIndex > 0);
			return xsprite[nXIndex].rxID;
	}

	ThrowError("Unexpected rxBucket type", ES_ERROR);
	return 0;
}


int CompareChannels( const void *ref1, const void *ref2 )
{
	return GetBucketChannel((RXBUCKET *)ref1) - GetBucketChannel((RXBUCKET *)ref2);
}



/*******************************************************************************
	FUNCTION:		evInit()

	DESCRIPTION:	Initialize the event queue

	NOTES:			The map should be loaded so that all the rxIDs can be
					scanned
*******************************************************************************/
void evInit( void )
{
	int i, j;
	int nCount = 0;

	eventQ.Flush();

	// add all the tags to the bucket array
	for (i = 0; i < kMaxSectors; i++)
	{
		int nXSector = sector[i].extra;
		if (nXSector > 0 && xsector[nXSector].rxID > 0)
		{
			dassert(nCount < kMaxChannels);
			rxBucket[nCount].type = SS_SECTOR;
			rxBucket[nCount].index = i;
			nCount++;
		}
	}

	for (i = 0; i < kMaxWalls; i++)
	{
		int nXWall = wall[i].extra;
		if (nXWall > 0 && xwall[nXWall].rxID > 0)
		{
			dassert(nCount < kMaxChannels);
			rxBucket[nCount].type = SS_WALL;
			rxBucket[nCount].index = i;
			nCount++;
		}
	}

	for (i = 0; i < kMaxSprites; i++)
	{
		if (sprite[i].statnum < kMaxStatus)
		{
			int nXSprite = sprite[i].extra;
			if (nXSprite > 0 && xsprite[nXSprite].rxID > 0)
			{
				dassert(nCount < kMaxChannels);
				rxBucket[nCount].type = SS_SPRITE;
				rxBucket[nCount].index = i;
				nCount++;
			}
		}
	}

	// sort the array on rx tags
	qsort(rxBucket, nCount, sizeof(RXBUCKET), &CompareChannels);

	// create the list of header indices
	j = 0;
	for (i = 0; i < kMaxID; i++)
	{
		bucketHead[i] = (short)j;
		while ( j < nCount && GetBucketChannel(&rxBucket[j]) == i)
			j++;
	}

	bucketHead[i] = (short)j;
}


/***********************************************************************
 * evSourceState()
 *
 * Return the operational state of the event's source.
 *
 **********************************************************************/
static BOOL evGetSourceState( int type, int nIndex )
{
	int nXIndex;

	switch ( type )
	{
		case SS_SECTOR:
			nXIndex = sector[nIndex].extra;
			dassert(nXIndex > 0 && nXIndex < kMaxXSectors);
			return (BOOL)xsector[nXIndex].state;

		case SS_WALL:
			nXIndex = wall[nIndex].extra;
			dassert(nXIndex > 0 && nXIndex < kMaxXWalls);
			return (BOOL)xwall[nXIndex].state;

		case SS_SPRITE:
			nXIndex = sprite[nIndex].extra;
			dassert(nXIndex > 0 && nXIndex < kMaxXSprites);
			return (BOOL)xsprite[nXIndex].state;
	}

	// shouldn't reach this point
	return FALSE;
}


/***********************************************************************
 * evSend()
 *
 **********************************************************************/
void evSend( int index, int type, int to, int command )
{
	if ( command == kCommandState )
		command = evGetSourceState(type, index) ? kCommandOn : kCommandOff;
	else if ( command == kCommandNotState )
		command = evGetSourceState(type, index) ? kCommandOff : kCommandOn;

	EVENT event;

	event.index = index;
	event.type = type;
	event.to = to;
	event.command = command;

	// handle transmit-only system triggers
	if (to > kChannelNull)
	{
		switch (to)
		{
//			case kChannelNull:
//				ThrowError("Event with null tag encountered", ES_ERROR);

			case kChannelEndLevelA:
				gEndLevelFlag = gEndingA;
				// Hooray.  You finished the level.
				return;

			case kChannelEndLevelB:
				gEndLevelFlag = gEndingB;
				// Hooray.  You finished the level via the secret ending.
				return;

			case kChannelTextOver:
				if (command < kCommandNumbered)
					ThrowError("Non-numbered command triggered for TextOver", ES_ERROR);
				trTextOver(command - kCommandNumbered);
				return;

			case kChannelLightning:
				if (command < kCommandNumbered)
					ThrowError("Non-numbered command triggered for Lightning", ES_ERROR);
				trLightning(command - kCommandNumbered);
				return;

			case kChannelTriggerStart:
				dprintf("Trigger start broadcast\n");
				break;

			case kChannelTriggerMatch:
				dprintf("BloodBath start broadcast\n");
				break;

			case kChannelTriggerCoop:
				dprintf("Coop start broadcast\n");
				break;

			case kChannelRemoteFire1:
			case kChannelRemoteFire2:
			case kChannelRemoteFire3:
			case kChannelRemoteFire4:
			case kChannelRemoteFire5:
			case kChannelRemoteFire6:
			case kChannelRemoteFire7:
			case kChannelRemoteFire8:
				// these can't use the rx buckets since they are dynamically created
				for (short nSprite = headspritestat[kStatThing]; nSprite >= 0; nSprite = nextspritestat[nSprite] )
				{
					SPRITE *pSprite = &sprite[nSprite];

					if ( pSprite->extra > 0 )
					{
						XSPRITE *pXSprite = &xsprite[pSprite->extra];
						if ( pXSprite->rxID == to )
				 			trMessageSprite( nSprite, event );
					}
			 	}
				return;
		}
	}

	// the event is a broadcast message
	for (int i = bucketHead[event.to]; i < bucketHead[event.to + 1]; i++)
	{
		// don't send it to the originator
		if (rxBucket[i].type == event.type && rxBucket[i].index == event.index)
			continue;

		switch ( rxBucket[i].type )
		{
			case SS_SECTOR:
	 			trMessageSector( rxBucket[i].index, event );
				break;

	 		case SS_WALL:
	  			trMessageWall( rxBucket[i].index, event );
				break;

			case SS_SPRITE:
	 			trMessageSprite( rxBucket[i].index, event );
				break;

			default:
	 			break;
		}
	}
}


/***********************************************************************
 * evPost()
 *
 **********************************************************************/
void evPost( int index, int type, ulong time, int command )
{
	if ( command == kCommandState )
		command = evGetSourceState(type, index) ? kCommandOn : kCommandOff;
	else if ( command == kCommandNotState )
		command = evGetSourceState(type, index) ? kCommandOff : kCommandOn;

	EVENT event;

	event.index = index;
	event.type = type;
	event.command = command;

	eventQ.Insert(gFrameClock + time, (void *&)event);
}


/***********************************************************************
 * evProcess()
 *
 * check for and process broadcast commands to objects
 *
 **********************************************************************/
void evProcess( ulong time )
{
	// while there are events to be processed
	while ( eventQ.Check(time) )
	{
		void *p = eventQ.Remove();
		EVENT event = (EVENT &)p;

		// is it a special callback event?
//		if (event.command != kCommandCallback)
//			ThrowError("Non-callback in event queue", ES_ERROR);

		dprintf("Dispatching callback event ");
	 	switch ( event.type )
		{
			case SS_SECTOR:
				dprintf("to sector %d\n", event.index);
	 			trMessageSector( event.index, event );
				break;

	 		case SS_WALL:
				dprintf("to wall %d\n", event.index);
	  			trMessageWall( event.index, event );
				break;

			case SS_SPRITE:
				dprintf("to sprite %d\n", event.index);
	 			trMessageSprite( event.index, event );
				break;
		}
	}
}
