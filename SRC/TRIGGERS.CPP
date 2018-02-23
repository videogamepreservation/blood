#include <string.h>
#include <stdio.h>

#include "engine.h"
#include "db.h"
#include "globals.h"
#include "eventq.h"
#include "triggers.h"
#include "debug4g.h"
#include "sectorfx.h"
#include "misc.h"
#include "gameutil.h"
#include "trig.h"
#include "screen.h"
#include "actor.h"
#include "seq.h"
#include "error.h"
#include "names.h"
#include "levels.h"
#include "view.h"
#include "sfx.h"
#include "sound.h"	// can be removed once sfx complete

#define kRemoteDelay	12		// 0.1 second
#define kProxDelay		30		// 0.25 second

static struct
{
	long x, y;
} kwall[kMaxWalls];

static struct
{
	long x, y, z;
} ksprite[kMaxSprites];

// trigger effects
enum
{
	kSeqClearGlass		= 0x80,
	kSeqStainedGlass,
	kSeqWeb,
	kSeqBeam,
	kSeqJar,
	kSeqWaterOff,
	kSeqWaterOn,
	kSeqSpike,
	kSeqMetalGrate1,
	kSeqFireTrap1,
	kSeqFireTrap2,
	kSeqPadlock,
	kSeqMGunOpen,
	kSeqMGunFire,
	kSeqMGunClose,
	kSeqMGunFlare,
	kSeqTriggerMax,
};


/*******************************************************************************
								  DOOR STATES

OFF means closed
ON means open

Doors should be created in map edit in the open position, since it is easier to
computationally determine closed than open.  The trInit function should set
the initial positions of the doors based on their state flags.
*******************************************************************************/



/*******************************************************************************
							 Busy System Constants
*******************************************************************************/
enum {
	kBusyOk = 0,
	kBusyRestore,
	kBusyReverse,
	kBusyComplete,
};

typedef int (*BUSYPROC)( int nIndex, int nBusy );

struct BUSY {
	int nIndex;
	int nDelta;
	int nBusy;
	BUSYPROC busyProc;
};

#define kMaxBusyArray	128

int  gBusyCount = 0;
BUSY gBusy[kMaxBusyArray];

/*******************************************************************************
							LOCAL FUNCTION PROTOTYPES
*******************************************************************************/
static void InitGenerator( int nSprite );
static void ActivateGenerator( int nSprite );
static void FireballTrapCallback( int /* type */, int nXIndex );
static void MGunFireCallback( int /* type */, int nXIndex );
static void MGunOpenCallback( int /* type */, int nXIndex );

/*******************************************************************************
							LOCAL HELPER FUNCTIONS
*******************************************************************************/


/*******************************************************************************
	FUNCTION:		Lin2Sin()

	DESCRIPTION:	Convert a linear busy ramp to a sinusoidal ramp
*******************************************************************************/
inline int Lin2Sin( int nBusy )
{
	return (1 << 15) - (Cos(nBusy * kAngle180 / kMaxBusyValue) >> 15);
}


/*******************************************************************************
	FUNCTION:		SetSpriteState()

	DESCRIPTION:	Change the state of an xsprite, and broadcast message if
					necessary.
*******************************************************************************/
static void SetSpriteState( int nSprite, XSPRITE *pXSprite, int state )
{
	if ( (pXSprite->busy & kFluxMask) == 0 && pXSprite->state == state )
		return;

	pXSprite->busy = state << 16;
	pXSprite->state = state;

	if ( state != pXSprite->restState && pXSprite->waitTime > 0 )
		evPost(nSprite, SS_SPRITE, pXSprite->waitTime * kTimerRate / 10,
			pXSprite->restState ? kCommandOn : kCommandOff);

	if (pXSprite->command == kCommandLink)
		return;

	if ( pXSprite->txID != 0 )
	{
		if (pXSprite->triggerOn && pXSprite->state == 1)
		{
			dprintf( "evSend(%i, SS_SPRITE, %i, %i)\n", nSprite, pXSprite->txID, pXSprite->command );
			evSend(nSprite, SS_SPRITE, pXSprite->txID, pXSprite->command);
		}
		if (pXSprite->triggerOff && pXSprite->state == 0)
		{
			dprintf( "evSend(%i, SS_SPRITE, %i, %i)\n", nSprite, pXSprite->txID, pXSprite->command );
			evSend(nSprite, SS_SPRITE, pXSprite->txID, pXSprite->command);
		}
	}
}


/*******************************************************************************
	FUNCTION:		SetWallState()

	DESCRIPTION:	Change the state of an xwall, and broadcast message if
					necessary.
*******************************************************************************/
static void SetWallState( int nWall, XWALL *pXWall, int state )
{
	if ( (pXWall->busy & kFluxMask) == 0 && pXWall->state == state )
		return;

	pXWall->busy = state << 16;
	pXWall->state = state;

	if ( state != pXWall->restState && pXWall->waitTime > 0 )
		evPost(nWall, SS_WALL, pXWall->waitTime * kTimerRate / 10,
			pXWall->restState ? kCommandOn : kCommandOff);

	if (pXWall->command == kCommandLink)
		return;

	if ( pXWall->txID != 0 )
	{
		if (pXWall->triggerOn && pXWall->state == 1)
		{
			dprintf( "evSend(%i,SS_WALL, %i, %i)\n", nWall, pXWall->txID, pXWall->command );
			evSend(nWall, SS_WALL, pXWall->txID, pXWall->command);
		}
		if (pXWall->triggerOff && pXWall->state == 0)
		{
			dprintf( "evSend(%i,SS_WALL, %i, %i)\n", nWall, pXWall->txID, pXWall->command );
			evSend(nWall, SS_WALL, pXWall->txID, pXWall->command);
		}
	}
}


/*******************************************************************************
	FUNCTION:		SetSectorState()

	DESCRIPTION:	Change the state of an xsector, and broadcast message if
					necessary.
*******************************************************************************/
static void SetSectorState( int nSector, XSECTOR *pXSector, int state )
{
	if ( (pXSector->busy & kFluxMask) == 0 && pXSector->state == state )
		return;

	pXSector->busy = state << 16;
	pXSector->state = state;

	if ( state != pXSector->restState && pXSector->waitTime > 0 )
		evPost(nSector, SS_SECTOR, pXSector->waitTime * kTimerRate / 10,
			pXSector->restState ? kCommandOn : kCommandOff);

	if (pXSector->command == kCommandLink)
		return;

	if ( pXSector->txID != 0 )
	{
		if (pXSector->triggerOn && pXSector->state == 1)
		{
			dprintf( "evSend(%i, SS_SECTOR, %i, %i)\n", nSector, pXSector->txID, pXSector->command );
			evSend(nSector, SS_SECTOR, pXSector->txID, pXSector->command);
		}
		if (pXSector->triggerOff && pXSector->state == 0)
		{
			dprintf( "evSend(%i, SS_SECTOR, %i, %i)\n", nSector, pXSector->txID, pXSector->command );
			evSend(nSector, SS_SECTOR, pXSector->txID, pXSector->command);
		}
	}
}


/*******************************************************************************
	FUNCTION:		AddBusy()

	DESCRIPTION:	Add a busy process.  The callback procedure will be called
					once per frame with the updated busy value.

	PARAMETERS:		nIndex is used to identify the xobject to the callback
					function.

	NOTES:			if nDelta is < 0, the busy value will count down from
					kMaxBusyValue.
*******************************************************************************/
static void AddBusy( int nIndex, BUSYPROC busyProc, int nDelta )
{
	dassert(nDelta != 0);
	dassert(busyProc != NULL);

	// find an existing nIndex busy, or an unused busy slot
	for (int i = 0; i < gBusyCount; i++)
	{
		if ((nIndex == gBusy[i].nIndex) && (busyProc == gBusy[i].busyProc))
			break;
	}

	// adding a new busy?
	if (i == gBusyCount)
	{
		if ( gBusyCount == kMaxBusyArray )
		{
			dprintf("OVERFLOW: AddBusy() ignored\n");
			return;
		};

		gBusy[i].nIndex = nIndex;
		gBusy[i].busyProc = busyProc;
		gBusy[i].nBusy = (nDelta > 0) ? 0 : kMaxBusyValue;
		gBusyCount++;
	}
	gBusy[i].nDelta = nDelta;
}


/*******************************************************************************
	FUNCTION:		ReverseBusy()

	DESCRIPTION:	Reverse the delta of a busy process.  This is used to
					change the direction of a process from outside the
					callback.
*******************************************************************************/
void ReverseBusy( int nIndex, BUSYPROC busyProc )
{
	dassert(busyProc != NULL);

	// find an existing nIndex busy, or an unused busy slot
	for (int i = 0; i < gBusyCount; i++)
	{
		if ( nIndex == gBusy[i].nIndex && busyProc == gBusy[i].busyProc )
		{
			gBusy[i].nDelta = -gBusy[i].nDelta;
			return;
		}
	}
	dprintf("ReverseBusy: matching busy not found!\n");
}


/*******************************************************************************
	FUNCTION:		GetSourceBusy()

	DESCRIPTION:	Get the busy value of the object that originated the event.
					This is used for kCommandLink messages.
*******************************************************************************/
static unsigned GetSourceBusy( EVENT event )
{
	int nXIndex;

	switch ( event.type )
	{
		case SS_SECTOR:
			nXIndex = sector[event.index].extra;
			dassert(nXIndex > 0 && nXIndex < kMaxXSectors);
			return xsector[nXIndex].busy;

		case SS_WALL:
			nXIndex = wall[event.index].extra;
			dassert(nXIndex > 0 && nXIndex < kMaxXWalls);
			return xwall[nXIndex].busy;

		case SS_SPRITE:
			nXIndex = sprite[event.index].extra;
			dassert(nXIndex > 0 && nXIndex < kMaxXSprites);
			return xsprite[nXIndex].busy;
	}

	// shouldn't reach this point
	return 0;
}


/*******************************************************************************
	FUNCTION:		OperateSprite()

	DESCRIPTION:

	PARAMETERS:

	RETURNS:

	NOTES:
*******************************************************************************/
void OperateSprite( int nSprite, XSPRITE *pXSprite, EVENT event )
{
	SPRITE *pSprite = &sprite[nSprite];

	// handle respawns first, to avoid toggling the sprite state
	if ( event.command  == kCommandRespawn)
	{
		actRespawnSprite( (short)nSprite );
		switch (pSprite->type)
		{
			case kThingTNTBarrel:
				pSprite->cstat |= kSpriteBlocking | kSpriteHitscan;
				break;
		}
		return;
	}

	// special handling for lock/unlock commands
	switch ( event.command )
	{
		case kCommandLock:
			pXSprite->locked = 1;
			return;

		case kCommandUnlock:
			pXSprite->locked = 0;
			return;

		case kCommandToggleLock:
			pXSprite->locked ^= 1;
			return;
	}

	switch (pSprite->type)
	{
		case kMissileStarburstFlare:
		{
			if ( sprite[nSprite].statnum != kStatMissile)	// don't trigger if already exploded
				break;

			pSprite->type = kMissileExplodingFlare;
			int nVel = (M2X(2.0) << 4) / kTimerRate; // meters per second
			int nAngle = getangle(pSprite->xvel, pSprite->yvel);

			for (int i = 0; i < 16; i++)
			{
				int nBurst = actCloneSprite( pSprite );
				dbInsertXSprite( nBurst );

				long dxVel = 0;
				long dyVel = mulscale30r(nVel, Cos(i * kAngle360 / 16));
				long dzVel = mulscale30r(nVel, Sin(i * kAngle360 / 16));

				if (i & 1)
				{
					dyVel >>= 1;
					dzVel >>= 1;
				}

				RotateVector( &dxVel, &dyVel, nAngle );

				SPRITE *pBurst = &sprite[nBurst];
				pBurst->xvel += dxVel;
				pBurst->yvel += dyVel;
				pBurst->zvel += dzVel;
			}
			break;
		}

//		case kMissileExplodingFlare:
		case kMissileFlare:
			actPostSprite( nSprite, kStatFree );
			break;

		case kThingMachineGun:
			if (pXSprite->health > 0)
			{
				if ( event.command == kCommandOn )
				{
					if (pXSprite->state == 0)
					{
						SetSpriteState(nSprite, pXSprite, 1);
						seqSpawn(kSeqMGunOpen, SS_SPRITE, pSprite->extra, MGunOpenCallback);
						//sfxStart3DSound(pSprite->extra, kSfxSwitch2);

						// data1 = max ammo (defaults to infinite)
						// data2 = dynamic ammo if data1 > 0
						if (pXSprite->data1 > 0)
							pXSprite->data2 = pXSprite->data1;
					}
				}
				else if ( event.command == kCommandOff )
				{
					if (pXSprite->state)
					{
						SetSpriteState(nSprite, pXSprite, 0);
						seqSpawn(kSeqMGunClose, SS_SPRITE, pSprite->extra);
						//sfxStart3DSound(pSprite->extra, kSfxSwitch2);
					}
				}
			}
			break;
		case kThingWallCrack:
			SetSpriteState(nSprite, pXSprite, 0);
			actPostSprite( nSprite, kStatFree );
			break;
		case kThingCrateFace:
			SetSpriteState(nSprite, pXSprite, 0);
			actPostSprite( nSprite, kStatFree );
			break;
		case kTrapPoweredZap:
			switch( event.command )
			{
				case kCommandOff:
					dprintf("zap off\n");
					pXSprite->state = 0;
					pSprite->cstat |= kSpriteInvisible;
					pSprite->cstat &= ~kSpriteBlocking;
					break;

				case kCommandOn:
					dprintf("zap on\n");
					pXSprite->state = 1;
					pSprite->cstat &= ~kSpriteInvisible;
					pSprite->cstat |= kSpriteBlocking;
					break;

				case kCommandToggle:
					dprintf("zap toggle\n");
					pXSprite->state ^= 1;
					pSprite->cstat ^= kSpriteInvisible;
					pSprite->cstat ^= kSpriteBlocking;
					break;
			}
			break;

		case kSwitchPadlock:
			switch( event.command )
			{
				case kCommandOff:
					SetSpriteState(nSprite, pXSprite, 0);
					break;

				case kCommandOn:
					SetSpriteState(nSprite, pXSprite, 1);
					seqSpawn(kSeqPadlock, SS_SPRITE, pSprite->extra);
					sfxStart3DSound(pSprite->extra, kSfxChains);
					break;

				default:
					SetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1);
					if (pXSprite->state)
					{
						seqSpawn(kSeqPadlock, SS_SPRITE, pSprite->extra);
						sfxStart3DSound(pSprite->extra, kSfxChains);
					}
					break;
			}
			break;

		case kSwitchMomentary:
			switch( event.command )
			{
				case kCommandOff:
					SetSpriteState(nSprite, pXSprite, 0);
					break;

				case kCommandOn:
					SetSpriteState(nSprite, pXSprite, 1);
					break;

				default:
					SetSpriteState(nSprite, pXSprite, pXSprite->restState ^ 1);
					break;
			}
			sfxStart3DSound(pSprite->extra, kSfxSwitch2);
			break;

		case kSwitchCombination:
			switch( event.command )
			{
				case kCommandOff:
					if (--pXSprite->data1 < 0)				// underflow?
						pXSprite->data1 += pXSprite->data3;
					break;

				default:
					if (++pXSprite->data1 >= pXSprite->data3)	// overflow?
						pXSprite->data1 -= pXSprite->data3;
					break;
			}

			// handle master switches
			if ( pXSprite->command == kCommandLink && pXSprite->txID != 0 )
				evSend(nSprite, SS_SPRITE, pXSprite->txID, kCommandLink);

			// at right combination?
			if (pXSprite->data1 == pXSprite->data2)
				SetSpriteState(nSprite, pXSprite, 1);
			else
				SetSpriteState(nSprite, pXSprite, 0);
			break;

		case kThingTNTStick:
		case kThingTNTBundle:
		case kThingTNTBarrel:
			if ( sprite[nSprite].statnum != kStatRespawn )	// don't trigger if waiting to be respawned!
				actExplodeSprite((short)nSprite);
			break;

		case kThingTNTRemArmed:
			if ( sprite[nSprite].statnum == kStatRespawn )
				break;

			switch ( event.command )
			{
				case kCommandOn:
					sfxStart3DSound(pSprite->extra, kSfxTNTDetRemote);
					evPost(nSprite, SS_SPRITE, kRemoteDelay);
					break;

				default:
					actExplodeSprite((short)nSprite);
					break;
			}
			break;

		case kThingTNTProxArmed:
			if ( sprite[nSprite].statnum == kStatRespawn )
				break;

			switch ( event.command )
			{
				case kCommandSpriteProximity:
					if ( pXSprite->state )						// don't trigger it if already triggered
						break;

					sfxStart3DSound(pSprite->extra, kSfxTNTDetProx);
					evPost(nSprite, SS_SPRITE, kProxDelay, kCommandOff);
					pXSprite->state = 1;
					break;

				case kCommandCallback:
					sfxStart3DSound(pSprite->extra, kSfxTNTArmProx);
					actPostSprite(nSprite, kStatProximity);		// activate it by changing list
					break;

				default:
					actExplodeSprite((short)nSprite);
					break;
			}
			break;

		case kGenTrigger:
		case kGenWaterDrip:
		case kGenBloodDrip:
		case kGenFireball:
		case kGenEctoSkull:
		case kGenDart:
		case kGenSound:
			switch( event.command )
			{
				case kCommandCallback:
					pXSprite->data3 = FALSE;	// prevent callback reentrancy
					if ( pXSprite->state )
					{
						if ( pSprite->type != kGenTrigger )
							ActivateGenerator( nSprite );
						if ( pXSprite->txID != 0 )
							evSend(nSprite, SS_SPRITE, pXSprite->txID, pXSprite->command);
						if ( pXSprite->busyTime > 0 )
						{
							pXSprite->data3 = TRUE;
							evPost(nSprite, SS_SPRITE, (pXSprite->busyTime + BiRandom(pXSprite->data1)) * kTimerRate / 10);
						}
					}
					break;

				case kCommandOff:
					SetSpriteState(nSprite, pXSprite, 0);
					break;

				default:
					if ( event.command == kCommandOn )
						SetSpriteState(nSprite, pXSprite, 1);
					else
						SetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1);

					if ( pXSprite->state && pXSprite->data3 == FALSE )
					{
						pXSprite->data3 = TRUE;
						evPost(nSprite, SS_SPRITE, 0);	// trigger immediately
					}
					break;
			}
			break;

		default:
			switch( event.command )
			{
				case kCommandOff:
					SetSpriteState(nSprite, pXSprite, 0);
					break;

				case kCommandOn:
					SetSpriteState(nSprite, pXSprite, 1);
					break;

				default:
					SetSpriteState(nSprite, pXSprite, pXSprite->state ^ 1);
					break;
			}
			break;
	}
}


/*******************************************************************************
	FUNCTION:		OperateWall()

	DESCRIPTION:

	PARAMETERS:

	RETURNS:

	NOTES:
*******************************************************************************/
void OperateWall( int nWall, XWALL *pXWall, EVENT event )
{
	WALL *pWall = &wall[nWall];

	// special handling for lock/unlock commands
	switch ( event.command )
	{
		case kCommandLock:
			pXWall->locked = 1;
			return;

		case kCommandUnlock:
			pXWall->locked = 0;
			return;

		case kCommandToggleLock:
			pXWall->locked ^= 1;
			return;
	}

	switch (pWall->type)
	{
		case kWallClearGlass:
			if ( pXWall->state == 0 )
			{
				SetWallState(nWall, pXWall, 1);
				seqSpawn(kSeqClearGlass, SS_MASKED, wall[nWall].extra);
				sndStartSample( "glasshit2", 64 );
			}
			break;
		case kWallStainedGlass:
			if ( pXWall->state == 0 )
			{
				SetWallState(nWall, pXWall, 1);
				seqSpawn(kSeqStainedGlass, SS_MASKED, wall[nWall].extra);
				sndStartSample( "glasshit2", 64 );
			}
			break;
		case kWallWoodBeams:
			if ( pXWall->state == 0 )
			{
				SetWallState(nWall, pXWall, 1);
				seqSpawn(kSeqBeam, SS_MASKED, wall[nWall].extra);
			}
			break;
		case kWallWeb:
			if ( pXWall->state == 0 )
			{
				SetWallState(nWall, pXWall, 1);
				seqSpawn(kSeqWeb, SS_MASKED, wall[nWall].extra);
			}
			break;
		case kWallMetalGrate1:
			if ( pXWall->state == 0 )
			{
				SetWallState(nWall, pXWall, 1);
				seqSpawn(kSeqMetalGrate1, SS_MASKED, wall[nWall].extra);
			}
			break;
		default:
			switch( event.command )
			{
				case kCommandOff:
					SetWallState(nWall, pXWall, 0);
					break;

				case kCommandOn:
					SetWallState(nWall, pXWall, 1);
					break;

				default:
					SetWallState(nWall, pXWall, pXWall->state ^ 1);
					break;
			}
			break;
	}
}


void TranslateSector( int nSector, int i0, int i1, int x0, int y0, int a0,
	int x1, int y1, int a1, BOOL fAllWalls )
{
	XSECTOR *pXSector = &xsector[sector[nSector].extra];
	int vX = x1 - x0;
	int vY = y1 - y0;
	int vA = a1 - a0;
	int Xi0 = mulscale16(i0, vX);
	int Xi1 = mulscale16(i1, vX);
	int dX = Xi1 - Xi0;
	int Yi0 = mulscale16(i0, vY);
	int Yi1 = mulscale16(i1, vY);
	int dY = Yi1 - Yi0;
	int Ai0 = mulscale16(i0, vA);
	int Ai1 = mulscale16(i1, vA);
	int dA = Ai1 - Ai0;
	long x, y;

	short nWall = sector[nSector].wallptr;
	if ( fAllWalls )
	{
		for (int i = 0; i < sector[nSector].wallnum; nWall++, i++)
		{
			x = kwall[nWall].x;
			y = kwall[nWall].y;

			if ( Ai1 )
				RotatePoint(&x, &y, Ai1, x0, y0);

			// move vertex
			dragpoint(nWall, x + Xi1, y + Yi1);
		}
	}
	else
	{
		for (int i = 0; i < sector[nSector].wallnum; nWall++, i++)
		{
			short nWall2 = wall[nWall].point2;

			x = kwall[nWall].x;
			y = kwall[nWall].y;

			if ( wall[nWall].cstat & kWallMoveForward )
			{
				if ( Ai1 )
					RotatePoint(&x, &y, Ai1, x0, y0);

				dragpoint(nWall, x + Xi1, y + Yi1);

				// move next vertex if not explicitly tagged
				if ( !(wall[nWall2].cstat & kWallMoveMask) )
				{
					x = kwall[nWall2].x;
					y = kwall[nWall2].y;

					if ( Ai1 )
						RotatePoint(&x, &y, Ai1, x0, y0);

					dragpoint(nWall2, x + Xi1, y + Yi1);
				}
			}
			else if ( wall[nWall].cstat & kWallMoveBackward )
			{
				if ( Ai1 )
					RotatePoint(&x, &y, -Ai1, x0, y0);

				dragpoint(nWall, x - Xi1, y - Yi1);

				// move next vertex if not explicitly tagged
				if ( !(wall[nWall2].cstat & kWallMoveMask) )
				{
					x = kwall[nWall2].x;
					y = kwall[nWall2].y;

					if ( Ai1 )
						RotatePoint(&x, &y, -Ai1, x0, y0);

					dragpoint(nWall2, x - Xi1, y - Yi1);
				}
			}
		}
	}

	for (short nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
	{
		SPRITE *pSprite = &sprite[nSprite];

		// don't move markers
		if ( pSprite->statnum == kStatMarker )
			continue;

		x = ksprite[nSprite].x;
		y = ksprite[nSprite].y;

		if ( sprite[nSprite].cstat & kSpriteMoveForward )
		{
			if ( Ai1 )
				RotatePoint(&x, &y, Ai1, x0, y0);
			pSprite->ang = (short)((pSprite->ang + dA) & kAngleMask);
			pSprite->x = x + Xi1;
			pSprite->y = y + Yi1;
		}
		else if ( sprite[nSprite].cstat & kSpriteMoveReverse )
		{
			if ( Ai1 )
				RotatePoint(&x, &y, -Ai1, x0, y0);
			pSprite->ang = (short)((pSprite->ang - dA) & kAngleMask);
			pSprite->x = x - Xi1;
			pSprite->y = y - Yi1;
		}
		else if ( pXSector->drag )
		{
			int zTop, zBot;
			GetSpriteExtents(pSprite, &zTop, &zBot);

			if ( !(pSprite->cstat & kSpriteRMask) && zBot >= sector[nSector].floorz )
			{
				// translate relatively (degenerative)
				if ( dA )
					RotatePoint(&pSprite->x, &pSprite->y, dA, x0 + Xi0, y0 + Yi0);

				pSprite->ang = (short)((pSprite->ang + dA) & kAngleMask);
				pSprite->x += dX;
				pSprite->y += dY;
			}
		}
	}
}


/*******************************************************************************
	FUNCTION:		GetHighestSprite()

	DESCRIPTION:	Helper function to return the highest sprite with the
					specified statnum equal to nStatus in the sector specified
					by nSector.

	PARAMETERS:		nSector = the sector to search
					nStatus = the status number to match, or kMaxStatus = ALL

	RETURNS:		The sprite number with the highest Z, or -1 = none

	NOTES:
*******************************************************************************/
int GetHighestSprite( int nSector, int nStatus, int *hiz )
{
	*hiz = sector[nSector].floorz;
	int retSprite = -1;

	for (short nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
	{
		if ( sprite[nSprite].statnum == nStatus || nStatus == kMaxStatus)
		{
			SPRITE *pSprite = &sprite[nSprite];
			int zUp, zDown;
			GetSpriteExtents(pSprite, &zUp, &zDown);

			if ( pSprite->z - zUp < *hiz )
			{
				*hiz = pSprite->z - zUp;
				retSprite = nSprite;
			}
		}
	}
	return retSprite;
}


/*******************************************************************************
	FUNCTION:		VCrushBusy()

	DESCRIPTION:	Called by trProcessBusy : calculates the new floorz and
					ceilingz height based on the nBusy value

	PARAMETERS:

	RETURNS:

	NOTES:			This busy proc works for lifts, Top doors, Bottom doors,
					and HSplit doors
*******************************************************************************/
int VCrushBusy( int nSector, int nBusy )
{
	dassert( nSector >= 0 && nSector < numsectors);
	int nXSector = sector[nSector]. extra;
	dassert(nXSector > 0 && nXSector < kMaxXSectors);
	XSECTOR *pXSector = &xsector[nXSector];

	int newCeilZ, newFloorZ;

	int dCeilZ = pXSector->onCeilZ - pXSector->offCeilZ;
	if ( dCeilZ )
		newCeilZ = pXSector->offCeilZ + mulscale16(dCeilZ, Lin2Sin(nBusy));

	int dFloorZ = pXSector->onFloorZ - pXSector->offFloorZ;
	if ( dFloorZ )
		newFloorZ = pXSector->offFloorZ + mulscale16(dFloorZ, Lin2Sin(nBusy));

	int z;
	int nSprite = GetHighestSprite( nSector, kStatDude, &z );
	if ( nSprite >= 0 && newCeilZ >= z )
		return kBusyRestore;

	if (dCeilZ)
		sector[nSector].ceilingz = newCeilZ;
	if (dFloorZ)
		sector[nSector].floorz = newFloorZ;

	pXSector->busy = nBusy;

	if ( pXSector->command == kCommandLink && pXSector->txID != 0 )
		evSend(nSector, SS_SECTOR, pXSector->txID, kCommandLink);

	if ( (nBusy & kFluxMask) == 0 )
	{
	 	SetSectorState(nSector, pXSector, nBusy >> 16);
		return kBusyComplete;
	}

 	return kBusyOk;
}


/*******************************************************************************
	FUNCTION:		VSpriteBusy()

	DESCRIPTION:	Called by trProcessBusy : calculates the new sprite z
					based on the nBusy value

	PARAMETERS:

	RETURNS:

	NOTES:			This busy proc works for z motion sprites only. The sector
					floor and ceiling heights are unaffected.
*******************************************************************************/
int VSpriteBusy( int nSector, int nBusy )
{
	dassert( nSector >= 0 && nSector < numsectors);
	int nXSector = sector[nSector]. extra;
	dassert(nXSector > 0 && nXSector < kMaxXSectors);
	XSECTOR *pXSector = &xsector[nXSector];

	int floorZRange = pXSector->onFloorZ - pXSector->offFloorZ;
	if ( floorZRange != 0 )
	{
		// adjust the z for any floor relative sprites or face sprites in the floor
		for (short nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
		{
			SPRITE *pSprite = &sprite[nSprite];

			if ( pSprite->cstat & kSpriteMoveFloor )
				pSprite->z = ksprite[nSprite].z + mulscale16(floorZRange, Lin2Sin(nBusy));
		}
	}

	int ceilZRange = pXSector->onCeilZ - pXSector->offCeilZ;
	if ( ceilZRange != 0 )
	{
		// adjust the z for any ceiling relative sprites or face sprites in the ceiling
		for (short nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
		{
			SPRITE *pSprite = &sprite[nSprite];

			if ( pSprite->cstat & kSpriteMoveCeiling )
				pSprite->z = ksprite[nSprite].z + mulscale16(ceilZRange, Lin2Sin(nBusy));
		}
	}

	pXSector->busy = nBusy;

	if ( pXSector->command == kCommandLink && pXSector->txID != 0 )
		evSend(nSector, SS_SECTOR, pXSector->txID, kCommandLink);

	if ( (nBusy & kFluxMask) == 0 )
	{
	 	SetSectorState(nSector, pXSector, nBusy >> 16);
		return kBusyComplete;
	}

 	return kBusyOk;
}


/*******************************************************************************
	FUNCTION:		VDoorBusy()

	DESCRIPTION:	Called by trProcessBusy : calculates the new floorz and
					ceilingz height based on the nBusy value

	PARAMETERS:

	RETURNS:

	NOTES:			This busy proc works for lifts, Top doors, Bottom doors,
					and HSplit doors
*******************************************************************************/
int VDoorBusy( int nSector, int nBusy )
{
	dassert( nSector >= 0 && nSector < numsectors);
	int nXSector = sector[nSector]. extra;
	dassert(nXSector > 0 && nXSector < kMaxXSectors);
	XSECTOR *pXSector = &xsector[nXSector];

	int floorZRange = pXSector->onFloorZ - pXSector->offFloorZ;
	if ( floorZRange != 0 )
	{
		int oldFloorZ = sector[nSector].floorz;
		sector[nSector].floorz = pXSector->offFloorZ + mulscale16(floorZRange, Lin2Sin(nBusy));

		// adjust the z for any floor relative sprites or face sprites in the floor
		for (short nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
		{
			SPRITE *pSprite = &sprite[nSprite];
			int zTop, zBot;

			GetSpriteExtents(pSprite, &zTop, &zBot);

			if ( pSprite->cstat & kSpriteMoveFloor ||
				( !(pSprite->cstat & kSpriteRMask) && zBot >= oldFloorZ) )
				pSprite->z += sector[nSector].floorz - oldFloorZ;
		}
	}

	int ceilZRange = pXSector->onCeilZ - pXSector->offCeilZ;
	if ( ceilZRange != 0 )
	{
		int oldCeilZ = sector[nSector].ceilingz;
		sector[nSector].ceilingz = pXSector->offCeilZ + mulscale16(ceilZRange, Lin2Sin(nBusy));

		// adjust the z for any ceiling relative sprites or face sprites in the ceiling
		for (short nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
		{
			SPRITE *pSprite = &sprite[nSprite];
			int zTop, zBot;

			GetSpriteExtents(pSprite, &zTop, &zBot);

			if ( pSprite->cstat & kSpriteMoveCeiling ||
				( !(pSprite->cstat & kSpriteRMask) && zTop <= oldCeilZ) )
				pSprite->z += sector[nSector].ceilingz - oldCeilZ;
		}
	}

	pXSector->busy = nBusy;

	if ( pXSector->command == kCommandLink && pXSector->txID != 0 )
		evSend(nSector, SS_SECTOR, pXSector->txID, kCommandLink);

	if ( (nBusy & kFluxMask) == 0 )
	{
	 	SetSectorState(nSector, pXSector, nBusy >> 16);
		return kBusyComplete;
	}

 	return kBusyOk;
}


/*******************************************************************************
	FUNCTION:		HDoorBusy()

	DESCRIPTION:	Called by trProcessBusy : calculates the new wall x,y
					based on the nBusy value

	RETURNS:

	NOTES:			This busy proc works for slide doors
*******************************************************************************/
int HDoorBusy( int nSector, int nBusy )
{
	dassert( nSector >= 0 && nSector < numsectors);
	SECTOR *pSector = &sector[nSector];
	int nXSector = pSector->extra;
	dassert(nXSector > 0 && nXSector < kMaxXSectors);
	XSECTOR *pXSector = &xsector[nXSector];

	SPRITE *pMark0 = &sprite[pXSector->marker0];
	SPRITE *pMark1 = &sprite[pXSector->marker1];
	TranslateSector(nSector, Lin2Sin(pXSector->busy), Lin2Sin(nBusy),
		pMark0->x, pMark0->y, pMark0->ang, pMark1->x, pMark1->y, pMark1->ang,
		pSector->type == kSectorSlide);
//	SlideSector(nSector, mulscale16(dx, nAmount), mulscale16(dy, nAmount));

	pXSector->busy = nBusy;

	if ( pXSector->command == kCommandLink && pXSector->txID != 0 )
		evSend(nSector, SS_SECTOR, pXSector->txID, kCommandLink);

	if ( (nBusy & kFluxMask) == 0 )
	{
	 	SetSectorState(nSector, pXSector, nBusy >> 16);
		return kBusyComplete;
	}

	return kBusyOk;
}


/*******************************************************************************
	FUNCTION:		RDoorBusy()

	DESCRIPTION:	Called by trProcessBusy : calculates the new rotating door
					wall angles based on the nBusy value.

	PARAMETERS:

	RETURNS:

	NOTES:
*******************************************************************************/
int RDoorBusy( int nSector, int nBusy )
{
	dassert( nSector >= 0 && nSector < numsectors);
	SECTOR *pSector = &sector[nSector];
	int nXSector = pSector->extra;
	dassert(nXSector > 0 && nXSector < kMaxXSectors);
	XSECTOR *pXSector = &xsector[nXSector];

	SPRITE *pMark = &sprite[pXSector->marker0];
	TranslateSector(nSector, Lin2Sin(pXSector->busy), Lin2Sin(nBusy),
		pMark->x, pMark->y, 0, pMark->x, pMark->y, pMark->ang,
		pSector->type == kSectorRotate);

	pXSector->busy = nBusy;

	if ( pXSector->command == kCommandLink && pXSector->txID != 0 )
		evSend(nSector, SS_SECTOR, pXSector->txID, kCommandLink);

	if ( (nBusy & kFluxMask) == 0 )
	{
	 	SetSectorState(nSector, pXSector, nBusy >> 16);
		return kBusyComplete;
	}

	return kBusyOk;
}


/*******************************************************************************
	FUNCTION:		OperateDoor()

	DESCRIPTION:

	PARAMETERS:

	RETURNS:

	NOTES:
*******************************************************************************/
void OperateDoor( int nSector, EVENT event, BUSYPROC busyProc )
{
	dassert( nSector >= 0 && nSector < numsectors);
	int nXSector = sector[nSector].extra;
	dassert(nXSector > 0 && nXSector < kMaxXSectors);
	XSECTOR *pXSector = &xsector[nXSector];

	int nDelta = kMaxBusyValue / ClipLow(pXSector->busyTime * kTimerRate / 10, 1);

	switch (event.command)
	{
		case kCommandOff:
			if ( pXSector->busy != 0x0000 )
				AddBusy(nSector, busyProc, -nDelta);
			break;

		case kCommandOn:
			if ( pXSector->busy != 0x10000 )
				AddBusy(nSector, busyProc, nDelta);
			break;

		default:
			if ( (pXSector->busy & kFluxMask) && pXSector->interruptable )
				ReverseBusy(nSector, busyProc);
			else
				AddBusy(nSector, busyProc, pXSector->state ? -nDelta : nDelta);
			break;
	}
}


/*******************************************************************************
	FUNCTION:		OperateTeleport()

	DESCRIPTION:

	PARAMETERS:

	RETURNS:

	NOTES:
*******************************************************************************/
void OperateTeleport( int nSector, EVENT event )
{
	dassert( nSector >= 0 && nSector < numsectors );
	int nXSector = sector[nSector].extra;
	dassert( nXSector > 0 && nXSector < kMaxXSectors );
	XSECTOR *pXSector = &xsector[nXSector];

	BOOL bChanged = FALSE;

	switch (event.command)
	{
		case kCommandOff:
			bChanged = (pXSector->state == 1);
			SetSectorState(nSector, pXSector, 0);
			break;

		case kCommandOn:
			bChanged = (pXSector->state == 0);
			SetSectorState(nSector, pXSector, 1);
			break;

		default:
			bChanged = (pXSector->state == pXSector->restState);
			SetSectorState(nSector, pXSector, pXSector->restState ^ 1);
			break;
	}

	if ( bChanged && (pXSector->state != pXSector->restState) )
	{
		int nDest = pXSector->marker0;
		dassert( nDest < kMaxSprites );

		SPRITE *pDest = &sprite[nDest];
		dassert( pDest->statnum == kStatMarker);
		dassert( pDest->type == kMarkerWarpDest );

		for (int nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
		{
			SPRITE *pSprite = &sprite[nSprite];
			if ( pSprite->statnum == kStatDude )
			{
				pSprite->x = pDest->x;
				pSprite->y = pDest->y;
				pSprite->z += sector[pDest->sectnum].floorz - sector[nSector].floorz;
				pSprite->ang = pDest->ang;
				changespritesect( (short)nSprite, pDest->sectnum );

				if (pSprite->type >= kDudePlayer1 && pSprite->type <= kDudePlayer8)
				{
					// fix backup position for interpolation to work properly
					viewBackupPlayerLoc(pSprite->type - kDudePlayer1);
				}
			}
		}
	}

}

/*******************************************************************************
	FUNCTION:		OperateSector()

	DESCRIPTION:

	PARAMETERS:

	RETURNS:

	NOTES:
*******************************************************************************/
void OperateSector( int nSector, XSECTOR *pXSector, EVENT event )
{
	SECTOR *pSector = &sector[nSector];

	// special handling for lock/unlock commands
	switch ( event.command )
	{
		case kCommandLock:
			pXSector->locked = 1;
			return;

		case kCommandUnlock:
			pXSector->locked = 0;
			return;

		case kCommandToggleLock:
			pXSector->locked ^= 1;
			return;
	}

	switch ( pSector->type)
	{
		case kSectorZSprite:
			OperateDoor(nSector, event, VSpriteBusy);
			break;

		case kSectorZMotion:
			OperateDoor(nSector, event, VDoorBusy);
			break;

		case kSectorZCrusher:
			OperateDoor(nSector, event, VCrushBusy);
			break;

		case kSectorSlideMarked:
		case kSectorSlide:
		case kSectorSlideCrush:
			OperateDoor(nSector, event, HDoorBusy);
			break;

		case kSectorRotateMarked:
		case kSectorRotate:
		case kSectorRotateCrush:
			OperateDoor(nSector, event, RDoorBusy);
			break;

		case kSectorTeleport:
			OperateTeleport( nSector, event );
			break;

		default:
			switch( event.command )
			{
				case kCommandOff:
					SetSectorState(nSector, pXSector, 0);
					break;

				case kCommandOn:
					SetSectorState(nSector, pXSector, 1);
					break;

				case kCommandToggle:
				case kCommandSectorPush:
				case kCommandSectorImpact:
				case kCommandSectorEnter:
				case kCommandSectorExit:
					SetSectorState(nSector, pXSector, pXSector->state ^ 1);
					break;
			}
			break;
 	}
}


/*******************************************************************************
	FUNCTION:		LinkSector()

	DESCRIPTION:

	PARAMETERS:

	RETURNS:

	NOTES:
*******************************************************************************/
void LinkSector( int nSector, XSECTOR *pXSector, EVENT event )
{
	SECTOR *pSector = &sector[nSector];

	int nBusy = GetSourceBusy(event);

	switch ( pSector->type )
	{
		case kSectorZSprite:
			VSpriteBusy( nSector, nBusy );
			break;

		case kSectorZMotion:
			VDoorBusy( nSector, nBusy );
			break;

		case kSectorZCrusher:
			VCrushBusy( nSector, nBusy );
			break;

		case kSectorSlide:
		case kSectorSlideMarked:
		case kSectorSlideCrush:
			HDoorBusy( nSector, nBusy );
			break;

		case kSectorRotate:
		case kSectorRotateMarked:
		case kSectorRotateCrush:
			RDoorBusy( nSector, nBusy );
			break;

		default:
			pXSector->busy = nBusy;
			if ( (nBusy & kFluxMask) == 0 )
				SetSectorState(nSector, pXSector, nBusy >> 16);
			break;
 	}
}


/*******************************************************************************
	FUNCTION:		LinkSprite()

	DESCRIPTION:

	PARAMETERS:

	RETURNS:

	NOTES:
*******************************************************************************/
void LinkSprite( int nSprite, XSPRITE *pXSprite, EVENT event )
{
	SPRITE *pSprite = &sprite[nSprite];
	int nBusy = GetSourceBusy(event);

	switch ( pSprite->type )
	{
		case kSwitchCombination:
			// should only be linked to a master switch
			if (event.type == SS_SPRITE)
			{
				int nXSprite2 = sprite[event.index].extra;
				dassert(nXSprite2 > 0 && nXSprite2 < kMaxXSprites);

				// get master switch selection
				pXSprite->data1 = xsprite[nXSprite2].data1;

				// at right combination?
				if (pXSprite->data1 == pXSprite->data2)
					SetSpriteState(nSprite, pXSprite, 1);
				else
					SetSpriteState(nSprite, pXSprite, 0);
			}
			break;

		default:
			pXSprite->busy = nBusy;
			if ( (nBusy & kFluxMask) == 0 )
			 	SetSpriteState(nSprite, pXSprite, nBusy >> 16);
			break;
 	}
}


/*******************************************************************************
	FUNCTION:		LinkWall()

	DESCRIPTION:

	PARAMETERS:

	RETURNS:

	NOTES:
*******************************************************************************/
void LinkWall( int nWall, XWALL *pXWall, EVENT event )
{
	WALL *pWall = &wall[nWall];

	int nBusy = GetSourceBusy(event);

	switch ( pWall->type )
	{
		default:
			pXWall->busy = nBusy;
			if ( (nBusy & kFluxMask) == 0 )
			 	SetWallState(nWall, pXWall, nBusy >> 16);
			break;
 	}
}


/*******************************************************************************
							  EXPORTED FUNCTIONS
*******************************************************************************/


/*******************************************************************************
	FUNCTION:		trTriggerSector()

	DESCRIPTION:

	PARAMETERS:

	RETURNS:

	NOTES:
*******************************************************************************/
void trTriggerSector( unsigned nSector, XSECTOR *pXSector, int command )
{
	dprintf("TriggerSector: nSector=%i, command=%i\n", nSector, command);

	// bypass locked XObjects
	if ( pXSector->locked )
		return;

	// bypass triggered one-shots
	if ( pXSector->isTriggered )
		return;

	if ( pXSector->triggerOnce )
		pXSector->isTriggered = 1;

	if ( pXSector->decoupled )
	{
		if ( pXSector->txID != 0 )
			evSend(nSector, SS_SECTOR, pXSector->txID, pXSector->command);
	}
	else
	{
		EVENT event;
		event.command  = command;

		// operate the sector
		OperateSector( nSector, pXSector, event );
	}
}


/*******************************************************************************
	FUNCTION:		trMessageSector()

	DESCRIPTION:

	PARAMETERS:

	RETURNS:

	NOTES:
*******************************************************************************/
void trMessageSector( unsigned nSector, EVENT event )
{
	dassert(sector[nSector].extra > 0 && sector[nSector].extra < kMaxXSectors);
	XSECTOR *pXSector = &xsector[sector[nSector].extra];

	// don't send it to the originator
/*
	if (event.type == SS_SECTOR && event.index == nSector && !pXSector->decoupled
		&& event.command != kCommandCallback )
		return;
*/

	// operate the sector
	if ( event.command == kCommandLink )
		LinkSector( nSector, pXSector, event );
	else
		OperateSector( nSector, pXSector, event );
}


/*******************************************************************************
	FUNCTION:		trTriggerWall()

	DESCRIPTION:

	PARAMETERS:

	RETURNS:

	NOTES:
*******************************************************************************/
void trTriggerWall( unsigned nWall, XWALL *pXWall, int command )
{
	dprintf("TriggerWall: nWall=%i, command=%i\n", nWall, command);

	// bypass locked XObjects
	if ( pXWall->locked )
		return;

	// bypass triggered one-shots
	if ( pXWall->isTriggered )
		return;

	if ( pXWall->triggerOnce )
		pXWall->isTriggered = 1;

	if ( pXWall->decoupled )
	{
		if ( pXWall->txID != 0 )
			evSend(nWall, SS_WALL, pXWall->txID, pXWall->command);
	}
	else
	{
		EVENT event;
		event.command  = command;

		// operate the wall
		OperateWall(nWall, pXWall, event);
	}
}


/*******************************************************************************
	FUNCTION:		trMessageWall()

	DESCRIPTION:

	PARAMETERS:

	RETURNS:

	NOTES:
*******************************************************************************/
void trMessageWall( unsigned nWall, EVENT event )
{
	dassert(wall[nWall].extra > 0 && wall[nWall].extra < kMaxXWalls);
	XWALL *pXWall = &xwall[wall[nWall].extra];

	// don't send it to the originator
/*
	if (event.type == SS_WALL && event.index == nWall && !pXWall->decoupled
		&& event.command != kCommandCallback )
		return;
*/

	// operate the wall
	if ( event.command == kCommandLink )
		LinkWall( nWall, pXWall, event );
	else
		OperateWall( nWall, pXWall, event );
}


/*******************************************************************************
	FUNCTION:		trTriggerSprite()

	DESCRIPTION:

	PARAMETERS:

	RETURNS:

	NOTES:
*******************************************************************************/
void trTriggerSprite( unsigned nSprite, XSPRITE *pXSprite, int command )
{
	dprintf("TriggerSprite: nSprite=%i, type= %i command=%i\n", nSprite, sprite[nSprite].type, command);

	// bypass locked XObjects
	if ( pXSprite->locked )
		return;

	// bypass triggered one-shots
	if ( pXSprite->isTriggered )
		return;

	if ( pXSprite->triggerOnce )
		pXSprite->isTriggered = 1;

	if ( pXSprite->decoupled )
	{
		if ( pXSprite->txID != 0 )
			evSend(nSprite, SS_SPRITE, pXSprite->txID, pXSprite->command);
	}
	else
	{
		EVENT event;
		event.command  = command;

		// operate the sprite
		OperateSprite(nSprite, pXSprite, event);
	}
}


/*******************************************************************************
	FUNCTION:		trMessageSprite()

	DESCRIPTION:

	PARAMETERS:

	RETURNS:

	NOTES:
*******************************************************************************/
void trMessageSprite( unsigned nSprite, EVENT event )
{
	// return immediately if sprite has been deleted
	if ( sprite[nSprite].statnum == kMaxStatus )
		return;

	dassert(sprite[nSprite].extra > 0 && sprite[nSprite].extra < kMaxXSprites);
	XSPRITE *pXSprite = &xsprite[sprite[nSprite].extra];

	// don't send it to the originator
/*
	if (event.type == SS_SPRITE && event.index == nSprite && !pXSprite->decoupled
		&& event.command != kCommandCallback )
		return;
*/

	// operate the sprite
	if ( event.command == kCommandLink )
		LinkSprite( nSprite, pXSprite, event );
	else
		OperateSprite( nSprite, pXSprite, event );
}


/*******************************************************************************
	FUNCTION:		trProcessBusy()

	DESCRIPTION:

	PARAMETERS:

	RETURNS:

	NOTES:
*******************************************************************************/
void trProcessBusy( void )
{
	int i;

	for ( i = gBusyCount - 1; i >= 0; i-- )
	{
		// temporarily store nBusy before normalization
		int tempBusy = gBusy[i].nBusy;
		gBusy[i].nBusy = ClipRange(gBusy[i].nBusy + gBusy[i].nDelta * kFrameTicks, 0, kMaxBusyValue);

		int rcode = gBusy[i].busyProc( gBusy[i].nIndex, gBusy[i].nBusy );
		switch (rcode)
		{
			case kBusyRestore:
				gBusy[i].nBusy = tempBusy; // restore previous nBusy
				break;
			case kBusyReverse:
				gBusy[i].nBusy = tempBusy; // restore previous nBusy
				gBusy[i].nDelta = -gBusy[i].nDelta; // reverse delta
				break;
			case kBusyComplete:
				gBusyCount--;
				gBusy[i] = gBusy[gBusyCount];
				break;
		}
	}
}


/*******************************************************************************
	FUNCTION:		trInit()

	DESCRIPTION:	Initialize the trigger system.

	PARAMETERS:

	RETURNS:

	NOTES:			Called in prepareboard, AFTER call to dbLoadMap
*******************************************************************************/
void trInit( void )
{
	int i;
	int nSector, nWall, nSprite;

	gBusyCount = 0;

	// get wall vertice positions
	for (nWall = 0; nWall < numwalls; nWall++)
	{
		kwall[nWall].x = wall[nWall].x;
		kwall[nWall].y = wall[nWall].y;
	}

	// get sprite positions
	for ( nSprite = 0; nSprite < kMaxSprites; nSprite++ )
	{
		if ( sprite[nSprite].statnum < kMaxStatus )
		{
			ksprite[nSprite].x = sprite[nSprite].x;
			ksprite[nSprite].y = sprite[nSprite].y;
			ksprite[nSprite].z = sprite[nSprite].z;
		}
	}

	// init wall trigger masks (must be done first)
	for (nWall = 0; nWall < numwalls; nWall++)
	{
		if ( wall[nWall].extra > 0 )
		{
			int nXWall = wall[nWall].extra;
			dassert(nXWall < kMaxXWalls);

			XWALL *pXWall = &xwall[nXWall];

			if ( pXWall->state )
				pXWall->busy = kMaxBusyValue;

			switch( wall[nWall].type )
			{
				default:
					break;
			}
		}
	}

	// init sector trigger masks
	dassert((numsectors >= 0) && (numsectors < kMaxSectors));
	for ( nSector = 0; nSector < numsectors; nSector++ )
	{
		SECTOR *pSector = &sector[nSector];

		int nXSector = pSector->extra;
		if ( nXSector > 0 )
		{
			dassert(nXSector < kMaxXSectors);
			XSECTOR *pXSector = &xsector[nXSector];

			if ( pXSector->state )
				pXSector->busy = kMaxBusyValue;

			int oldFloorZ, oldCeilZ;
			SPRITE *pMark0 = NULL, *pMark1 = NULL;

			switch( pSector->type )
			{
				case kSectorTeleport:
					pXSector->data = -1;
					break;

				case kSectorZSprite:	// it's probabyl okay to start this one with the on or off z
				case kSectorZMotion:
				case kSectorZCrusher:

					oldFloorZ = pSector->floorz;
					oldCeilZ = pSector->ceilingz;

					// open or close door as necessary for initial state
					if ( pXSector->state == 0 )
					{
						pSector->ceilingz = pXSector->offCeilZ;
						pSector->floorz = pXSector->offFloorZ;
					}
					else
					{
						pSector->ceilingz = pXSector->onCeilZ;
						pSector->floorz = pXSector->onFloorZ;
					}
					// adjust the z for any floor relative sprites or face sprites in the floor
					for (nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
					{
						SPRITE *pSprite = &sprite[nSprite];
						if ( pSprite->cstat & kSpriteMoveFloor ||
							( !(pSprite->cstat & kSpriteRMask) && pSprite->z >= oldFloorZ) )
							pSprite->z += pSector->floorz - oldFloorZ;
					}

					// adjust the z for any ceiling relative sprites or face sprites in the ceiling
					for (nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
					{
						SPRITE *pSprite = &sprite[nSprite];
						if ( pSprite->cstat & kSpriteMoveCeiling ||
							( !(pSprite->cstat & kSpriteRMask) && pSprite->z <= oldCeilZ) )
							pSprite->z += pSector->ceilingz - oldCeilZ;
					}

					break;

				case kSectorSlideMarked:
				case kSectorSlide:
				case kSectorSlideCrush:

					pMark0 = &sprite[pXSector->marker0];
					pMark1 = &sprite[pXSector->marker1];

					// move door to off position by reversing markers
					TranslateSector(nSector, 0, 0x10000,
						pMark1->x, pMark1->y, pMark1->ang, pMark0->x, pMark0->y, pMark0->ang,
						pSector->type == kSectorSlide);

					// grab updated positions of walls
					for (i = 0; i < pSector->wallnum; i++)
					{
						int nWall = pSector->wallptr + i;
						kwall[nWall].x = wall[nWall].x;
						kwall[nWall].y = wall[nWall].y;
					}

					// grab updated positions of sprites
					for (nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
					{
						ksprite[nSprite].x = sprite[nSprite].x;
						ksprite[nSprite].y = sprite[nSprite].y;
						ksprite[nSprite].z = sprite[nSprite].z;
					}

					// open door if necessary
					TranslateSector(nSector, 0, pXSector->busy,
						pMark0->x, pMark0->y, pMark0->ang, pMark1->x, pMark1->y, pMark1->ang,
						pSector->type == kSectorSlide);
					break;

				case kSectorRotateMarked:
				case kSectorRotate:
				case kSectorRotateCrush:

					pMark0 = &sprite[pXSector->marker0];

					// move door to off position
					TranslateSector(nSector, 0, -0x10000,
						pMark0->x, pMark0->y, 0, pMark0->x, pMark0->y, pMark0->ang,
						pSector->type == kSectorRotate);

					// grab updated positions of walls
					for (i = 0; i < pSector->wallnum; i++)
					{
						int nWall = pSector->wallptr + i;
						kwall[nWall].x = wall[nWall].x;
						kwall[nWall].y = wall[nWall].y;
					}

					// grab updated positions of sprites
					for (nSprite = headspritesect[nSector]; nSprite >= 0; nSprite = nextspritesect[nSprite])
					{
						ksprite[nSprite].x = sprite[nSprite].x;
						ksprite[nSprite].y = sprite[nSprite].y;
						ksprite[nSprite].z = sprite[nSprite].z;
					}

					// open door if necessary
					TranslateSector(nSector, 0, pXSector->busy,
						pMark0->x, pMark0->y, 0, pMark0->x, pMark0->y, pMark0->ang,
						pSector->type == kSectorRotate);

					break;

				default:
					break;
			}
		}
	}

	// init sprite trigger masks
	for ( nSprite = 0; nSprite < kMaxSprites; nSprite++ )
	{
		int nXSprite = sprite[nSprite].extra;
		if ( sprite[nSprite].statnum < kMaxStatus && nXSprite > 0 )
		{
			dassert(nXSprite < kMaxXSprites);
			XSPRITE *pXSprite = &xsprite[nXSprite];

			if ( pXSprite->state )
				pXSprite->busy = kMaxBusyValue;

			// special initialization for implicit trigger types
			switch ( sprite[nSprite].type )
			{
				case kSwitchPadlock:
					pXSprite->triggerOnce = 1;	// force trigger once
					break;

				case kGenTrigger:
				case kGenWaterDrip:
				case kGenBloodDrip:
				case kGenFireball:
				case kGenEctoSkull:
				case kGenDart:
				case kGenSound:
					InitGenerator( nSprite );
					break;

				case kThingTNTProxArmed:
					pXSprite->triggerProximity = 1;
					break;
			}

			if ( pXSprite->triggerPush || pXSprite->triggerImpact )
				sprite[nSprite].cstat |= kSpriteHitscan;

/*******************************************************************************
Proximity sensors are being put on a separate list here, but this presents a
problem, since this means they can't be on any other list, such as the Thing
list.  This prevents the proximity bombs from being initialized properly.
How do we make them affected by explosions?
*******************************************************************************/
			if ( pXSprite->triggerProximity )
				actPostSprite( nSprite, kStatProximity );
//				changespritestat((short)nSprite, kStatProximity);

			switch( sprite[nSprite].type )
			{
				default:
					break;
			}
		}
	}

	evSend( 0, 0, kChannelTriggerStart, kCommandOn );

	if (gNetMode == kNetModeCoop)
		evSend( 0, 0, kChannelTriggerCoop, kCommandOn );
	else if (gNetMode == kNetModeBloodBath || gNetMode == kNetModeTeams)
		evSend( 0, 0, kChannelTriggerMatch, kCommandOn );
}


/*******************************************************************************
	FUNCTION:		trTextOver()

	DESCRIPTION:

	PARAMETERS:

	RETURNS:

	NOTES:
*******************************************************************************/
void trTextOver( int textId )
{
	if ( gLevelMessage[textId] != NULL )
		viewSetMessage(gLevelMessage[textId]);
}


/*******************************************************************************
	FUNCTION:		trLightning()

	DESCRIPTION:

	PARAMETERS:

	RETURNS:

	NOTES:
*******************************************************************************/
void trLightning( int nForkID )
{
	dassert( nForkID >= 0 && nForkID < kMaxLightning );

	dprintf( "trLightning: nForkID = %i\n", nForkID );

	int nSlot = gLightningInfo[ nForkID ].slot;
	dprintf( "trLightning: nSlot = %i\n", nSlot );

	if ( nSlot != -1 )
	{
		dassert( nSlot >= 0 && nSlot < kMaxSkyTiles );
		dprintf( "trLightning: gLightningInfo[ %i ].offset = %i\n", nForkID, gLightningInfo[ nForkID ].offset );
		pskyoff[ nSlot ] = (short)gLightningInfo[ nForkID ].offset;
	}
}

/*******************************************************************************
	FUNCTION:		InitGenerator()

	DESCRIPTION:

	PARAMETERS:

	RETURNS:

	NOTES:
*******************************************************************************/
static void InitGenerator( int nSprite )
{
	dassert(nSprite < kMaxSprites);
	SPRITE *pSprite = &sprite[nSprite];
	dassert(pSprite->statnum != kMaxStatus);

	int nXSprite = pSprite->extra;
	dassert(nXSprite > 0);
	XSPRITE *pXSprite = &xsprite[nXSprite];
	dassert(pXSprite->busyTime > 0);	// should be > 0 for timer events

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
	if ( pXSprite->state != pXSprite->restState && pXSprite->busyTime > 0 )
	{
		pXSprite->data3 = TRUE;
		evPost(nSprite, SS_SPRITE, (pXSprite->busyTime + BiRandom(pXSprite->data1)) * kTimerRate / 10);
	}
}


/*******************************************************************************
	FUNCTION:		ActivateGenerator()

	DESCRIPTION:

	PARAMETERS:

	RETURNS:

	NOTES:
*******************************************************************************/
static void ActivateGenerator( int nSprite )
{
	dassert(nSprite < kMaxSprites);
	SPRITE *pSprite = &sprite[nSprite];
	dassert(pSprite->statnum != kMaxStatus);

	int nXSprite = pSprite->extra;
	dassert(nXSprite > 0);
	XSPRITE *pXSprite = &xsprite[nXSprite];
	int zTop, zBot;

	switch ( pSprite->type )
	{
		case kGenWaterDrip:
			GetSpriteExtents(pSprite, &zTop, &zBot);
			actSpawnThing( pSprite->sectnum, pSprite->x, pSprite->y, zBot, kThingWaterDrip);
			break;

		case kGenBloodDrip:
			GetSpriteExtents(pSprite, &zTop, &zBot);
			actSpawnThing( pSprite->sectnum, pSprite->x, pSprite->y, zBot, kThingBloodDrip);
			break;

		case kGenSound:
			sfxCreate3DSound(pSprite->x, pSprite->y, pSprite->z, pXSprite->soundId);
			break;

		case kGenFireball:
			switch(pXSprite->data2)
			{
				case 0:
					FireballTrapCallback( SS_SPRITE, nXSprite );
					break;
				case 1:
					seqSpawn(kSeqFireTrap1, SS_SPRITE, nXSprite, FireballTrapCallback);
					break;
				case 2:
					seqSpawn(kSeqFireTrap2, SS_SPRITE, nXSprite, FireballTrapCallback);
					break;
			}
			break;

		case kGenEctoSkull:
		case kGenDart:
			// spawn a missile that moves and damages accordingly
			break;

		default:
			break;
	}
}


static void FireballTrapCallback( int /* type */, int nXIndex )
{
	XSPRITE *pXSprite = &xsprite[nXIndex];
	int nSprite = pXSprite->reference;
	SPRITE *pSprite = &sprite[nSprite];

	int dx, dy, dz;

	if ( pSprite->cstat & kSpriteFloor )		// floor sprite
	{
		dx = dy = 0;
		if ( pSprite->cstat & kSpriteFlipY )	// face down floor sprite
			dz = 1 << 14;
		else									// face up floor sprite
			dz = -1 << 14;
	}
	else										// wall sprite or face sprite
	{
		dx = Cos(pSprite->ang) >> 16;
		dy = Sin(pSprite->ang) >> 16;
		dz = 0;
	}

	actFireMissile( nSprite, pSprite->z, dx, dy, dz, kMissileFireball);
}


static void MGunFireCallback( int /* type */, int nXIndex )
{
	int nSprite = xsprite[nXIndex].reference;
	SPRITE *pSprite = &sprite[nSprite];
	XSPRITE *pXSprite = &xsprite[nXIndex];

	// if dynamic ammo left or infinite ammo
	if (pXSprite->data2 > 0 || pXSprite->data1 == 0)
	{
		if (pXSprite->data2 > 0)
		{
			pXSprite->data2--;	// subtract ammo
			if (pXSprite->data2 == 0)
				evPost(nSprite, SS_SPRITE, 1, kCommandOff);	// empty! turn it off
		}

		int dx = (Cos(pSprite->ang) >> 16) + BiRandom(1000);
		int dy = (Sin(pSprite->ang) >> 16) + BiRandom(1000);
		int dz = BiRandom(1000);

		actFireVector(nSprite, pSprite->z, dx, dy, dz, kVectorBullet);
		sfxCreate3DSound(pSprite->x, pSprite->y, pSprite->z, kSfxTomFire);
	}
}


static void MGunOpenCallback( int /* type */, int nXIndex )
{
	seqSpawn(kSeqMGunFire, SS_SPRITE, nXIndex, MGunFireCallback);
}

