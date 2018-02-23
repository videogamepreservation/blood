#include "typedefs.h"
#include "engine.h"
#include "gameutil.h"
#include "globals.h"
#include "debug4g.h"
#include "palette.h"
#include "misc.h"
#include "tile.h"


/***********************************************************************
 * AreSectorsNeighbors()
 *
 * Determines if two sectors adjoin
 **********************************************************************/
BOOL AreSectorsNeighbors(int sect1, int sect2)
{
	int i, nWall, num1, num2;

	dassert(sect1 >= 0 && sect1 < kMaxSectors);
	dassert(sect2 >= 0 && sect2 < kMaxSectors);

	num1 = sector[sect1].wallnum;
	num2 = sector[sect2].wallnum;

	//Traverse walls of sector with fewest walls for speed
	if (num1 < num2)
	{
		nWall = sector[sect1].wallptr;
		for( i = 0; i < num1; i++, nWall++ )
			if ( wall[nWall].nextsector == sect2 )
				return TRUE;
	}
	else
	{
		nWall = sector[sect2].wallptr;
		for( i = 0; i < num2; i++, nWall++ )
			if ( wall[nWall].nextsector == sect1 )
				return TRUE;
	}
	return FALSE;
}


/*******************************************************************************
	FUNCTION:		FindSector()

	DESCRIPTION:	This function works like Build's updatesector() function
					except it takes into account z position, which makes it
					give correct values in areas where sectors overlap.

	PARAMETERS:		You should supplies a starting search sector in the nSector
	variable.

	RETURNS:		TRUE if point found and updates nSector, FALSE otherwise.
*******************************************************************************/
BOOL FindSector(int x, int y, int z, short *nSector)
{
	WALL *pWall;
	long ceilz, floorz;

	if ( inside(x, y, *nSector) )
	{
		getzsofslope(*nSector, x, y, &ceilz, &floorz);
		if (z >= ceilz && z <= floorz)
			return TRUE;
	}

	pWall = &wall[sector[*nSector].wallptr];
	for (int i = sector[*nSector].wallnum; i > 0; i--, pWall++)
	{
		short j = pWall->nextsector;
		if ( j >= 0 && inside(x, y, j) )
		{
			getzsofslope(j, x, y, &ceilz, &floorz);
			if (z >= ceilz && z <= floorz)
			{
				*nSector = j;
				return TRUE;
			}
		}
	}

	for (i = 0; i < numsectors; i++)
	{
		if ( inside(x, y, (short)i) )
		{
			getzsofslope((short)i, x, y, &ceilz, &floorz);
			if (z >= ceilz && z <= floorz)
			{
				*nSector = (short)i;
				return TRUE;
			}
		}
	}

	return FALSE;
}




#define FRAMES 64
void CalcFrameRate( void )
{
	static long ticks[FRAMES], index = 0;

	if (gFrameClock != ticks[index])
	{
		gFrameRate = FRAMES * kTimerRate / (gFrameClock - ticks[index]);
		ticks[index] = gFrameClock;
	}
	index = (index + 1) & (FRAMES - 1);
}


/*******************************************************************************
	FUNCTION:		CheckProximity()

	DESCRIPTION:	Test to see if a point is within a given range to a sprite

	RETURNS:        TRUE if the point is in range
*******************************************************************************/
BOOL CheckProximity( SPRITE *pSprite, int x, int y, int z, int nSector, int dist )
{
	dassert(pSprite != NULL);

	int dx = qabs(x - pSprite->x) >> 4;
	if (dx < dist)
	{
		int dy = qabs(y - pSprite->y) >> 4;
		if (dy < dist)
		{
			int dz = qabs(z - pSprite->z) >> 8;
			if ( dz < dist && qdist(dx, dy) < dist )
			{
				if ( cansee(pSprite->x, pSprite->y,  pSprite->z, pSprite->sectnum,
					x, y, z, (short)nSector) )
					return TRUE;
			}
		}
	}
	return FALSE;
}


/*******************************************************************************
	FUNCTION:		GetWallAngle()

	DESCRIPTION:	Get the angle for a wall vector

	RETURNS:		Angle in the range 0 - kMax360

	NOTES:			Add kAngle90 to get the wall Normal
*******************************************************************************/
int GetWallAngle( int nWall )
{
	int dx = wall[wall[nWall].point2].x - wall[nWall].x;
	int dy = wall[wall[nWall].point2].y - wall[nWall].y;
	return getangle( dx, dy );
}


void GetWallNormal( int nWall, int *nx, int *ny )
{
	int dx, dy;
	dx = -(wall[wall[nWall].point2].y - wall[nWall].y) >> 4;
	dy = (wall[wall[nWall].point2].x - wall[nWall].x) >> 4;
	int length = ksqrt(dx * dx + dy * dy);
	dassert(length != 0);
	*nx = divscale16(dx, length);
	*ny = divscale16(dy, length);
}


BOOL IntersectRay(long x1, long y1, long vx, long vy,
	long x3, long y3, long z3, long x4, long y4, long z4,
	long *intx, long *inty, long *intz)
{     //p1 towards p2 is a ray
	long topt, topu, t;

	long x34 = x3 - x4;
	long y34 = y3 - y4;
	long z34 = z3 - z4;
	long bot = vx * y34 - vy * x34;
	if (bot >= 0)
	{
		if (bot == 0) return(0);
		long x31 = x3-x1;
		long y31 = y3-y1;
		topt = x31*y34 - y31*x34;
		if ( topt < 0 ) return FALSE;
		topu = vx*y31 - vy*x31;
		if ( topu < 0 || topu >= bot ) return FALSE;
	}
	else
	{
		long x31 = x3-x1;
		long y31 = y3-y1;
		topt = x31*y34 - y31*x34;
		if (topt > 0) return FALSE;
		topu = vx*y31 - vy*x31;
		if ( topu > 0 || topu <= bot) return FALSE;
	}
	t = divscale16(topu,bot);
	*intx = x3 + mulscale16(x34,t);
	*inty = y3 + mulscale16(y34,t);
	*intz = z3 + mulscale16(z34,t);
	return TRUE;
}




/*******************************************************************************
	FUNCTION:		HitScan()

	DESCRIPTION:	Returns the object hit, prioritized in sprite/wall/object
					order.

	PARAMETERS:

	RETURNS:		SS_SPRITE, SS_WALL, SS_FLOOR, SS_CEILING, or -1

	NOTES:			To simplify return object handling: SS_SPRITE is returned
					for flat and floor sprites. SS_WALL is returned for masked
					and one-way walls. In addition to the standard return
					value, HitScan fills the specified hitInfo structure with
					relevant hit data.
*******************************************************************************/
int HitScan( SPRITE *pSprite, int z, int dx, int dy, int dz, HITINFO *hitInfo )
{
	dassert( pSprite != NULL );
	dassert( hitInfo != NULL );

	hitInfo->hitsect = -1;
	hitInfo->hitwall = -1;
	hitInfo->hitsprite = -1;

	// offset the starting point for the vector so we don't hit the source!
	int x = pSprite->x; // + mulscale30(16, Cos(pSprite->ang));
	int y = pSprite->y; // + mulscale30(16, Sin(pSprite->ang));
	short nSector = pSprite->sectnum;

	ushort oldcstat = pSprite->cstat;
	pSprite->cstat &= ~kSpriteHitscan;

	hitscan(x, y, z, nSector, dx, dy, dz << 4,
		&hitInfo->hitsect, &hitInfo->hitwall, &hitInfo->hitsprite,
		&hitInfo->hitx, &hitInfo->hity, &hitInfo->hitz);

	pSprite->cstat = oldcstat;

	if (hitInfo->hitsprite >= kMaxSprites || hitInfo->hitwall >= kMaxWalls || hitInfo->hitsect >= kMaxSectors)
	{
		dprintf("hitscan failed!\n");
		dprintf("hitscan(%i, %i, %i, %i, %i, %i, %i ...)\n", x, y, z, pSprite->sectnum, dx, dy, dz);
		dprintf("  hitsect   = %i\n", hitInfo->hitsect);
		dprintf("  hitwall   = %i\n", hitInfo->hitwall);
		dprintf("  hitsprite = %i\n", hitInfo->hitsprite);
		dprintf("  hitx      = %i\n", hitInfo->hitx);
		dprintf("  hity      = %i\n", hitInfo->hity);
		dprintf("  hitz      = %i\n", hitInfo->hitz);
		return -1;
	}

	if (hitInfo->hitsprite >= 0)
		return SS_SPRITE;

	if (hitInfo->hitwall >= 0)
	{
		short nNextSector = wall[hitInfo->hitwall].nextsector;

		if ( nNextSector == -1 )	// single-sided wall
			return SS_WALL;
		else						// double-sided wall, check if we hit a masked wall
		{
			long ceilz, floorz;
			getzsofslope(nNextSector, hitInfo->hitx, hitInfo->hity, &ceilz, &floorz);

			if ( hitInfo->hitz <= ceilz || hitInfo->hitz >= floorz )
				return SS_WALL;

			return SS_MASKED;
		}
	}

	if (hitInfo->hitsect >= 0)
		return (hitInfo->hitz > z) ? SS_FLOOR : SS_CEILING;

	return -1;
}


int VectorScan( SPRITE *pSprite, int z, int dx, int dy, int dz, HITINFO *hitInfo )
{
	dassert( pSprite != NULL );
	dassert( hitInfo != NULL );

	hitInfo->hitsect = -1;
	hitInfo->hitwall = -1;
	hitInfo->hitsprite = -1;

	// offset the starting point for the vector so we don't hit the source!
	int x = pSprite->x; // + mulscale30(16, Cos(pSprite->ang));
	int y = pSprite->y; // + mulscale30(16, Sin(pSprite->ang));
	short nSector = pSprite->sectnum;

	ushort oldcstat = pSprite->cstat;
	pSprite->cstat &= ~kSpriteHitscan;

	hitscan(x, y, z, nSector, dx, dy, dz << 4,
		&hitInfo->hitsect, &hitInfo->hitwall, &hitInfo->hitsprite,
		&hitInfo->hitx, &hitInfo->hity, &hitInfo->hitz);

	pSprite->cstat = oldcstat;

retry:
	if (hitInfo->hitsprite >= kMaxSprites || hitInfo->hitwall >= kMaxWalls || hitInfo->hitsect >= kMaxSectors)
	{
		dprintf("hitscan failed!\n");
		dprintf("hitscan(%i, %i, %i, %i, %i, %i, %i ...)\n", x, y, z, pSprite->sectnum, dx, dy, dz);
		dprintf("  hitsect   = %i\n", hitInfo->hitsect);
		dprintf("  hitwall   = %i\n", hitInfo->hitwall);
		dprintf("  hitsprite = %i\n", hitInfo->hitsprite);
		dprintf("  hitx      = %i\n", hitInfo->hitx);
		dprintf("  hity      = %i\n", hitInfo->hity);
		dprintf("  hitz      = %i\n", hitInfo->hitz);
		return -1;
	}

	if (hitInfo->hitsprite >= 0)
	{
		SPRITE *pSprite = &sprite[hitInfo->hitsprite];

		switch ( pSprite->cstat & kSpriteRMask )
		{
			case kSpriteFace:
			{
				int nTile = pSprite->picnum;

				int height = (tilesizy[nTile] * pSprite->yrepeat << 2);
				int zBot = pSprite->z;
				if ( pSprite->cstat & kSpriteOriginAlign )
					zBot += height / 2;

				if ( picanm[nTile].ycenter )
					zBot -= picanm[nTile].ycenter * pSprite->yrepeat << 2;

				int texy = muldiv(zBot - hitInfo->hitz, tilesizy[nTile], height);
				if ( !(pSprite->cstat & kSpriteFlipY) )
					texy = tilesizy[nTile] - texy;

				int width = tilesizx[nTile] * pSprite->xrepeat >> 2;

				width = width * 3 / 4;	// aspect ratio correction?

				int top = dx * (y - pSprite->y) - dy * (x - pSprite->x);
				int bot = ksqrt(dx * dx + dy * dy);
				int dist = top / bot;
				int texx = muldiv(dist, tilesizx[nTile], width);
				texx += tilesizx[nTile] / 2 + picanm[nTile].xcenter;

				dprintf("Hit sprite at texel coor %d, %d\n", texx, texy);

				BYTE *pTile = tileLoadTile(nTile);
				BYTE texel = pTile[texx * tilesizy[nTile] + texy];

				if ( texel == 255 )	// mask color
				{
					dprintf("Texel is clear\n");
					// clear hitscan bits on sprite
					ushort oldcstat = pSprite->cstat;
					pSprite->cstat &= ~kSpriteHitscan;

					hitInfo->hitsect = -1;
					hitInfo->hitwall = -1;
					hitInfo->hitsprite = -1;

					x = hitInfo->hitx;
					y = hitInfo->hity;
					z = hitInfo->hitz;
					nSector = hitInfo->hitsect;

					hitscan(x, y, z, nSector, dx, dy, dz << 4,
						&hitInfo->hitsect, &hitInfo->hitwall, &hitInfo->hitsprite,
						&hitInfo->hitx, &hitInfo->hity, &hitInfo->hitz);

					// restore the hitscan bits
					pSprite->cstat = oldcstat;

					goto retry;
				}
				break;
			}
		}
		return SS_SPRITE;
	}

	if (hitInfo->hitwall >= 0)
	{
		short nNextSector = wall[hitInfo->hitwall].nextsector;

		if ( nNextSector == -1 )	// single-sided wall
			return SS_WALL;
		else						// double-sided wall, check if we hit a masked wall
		{
			long ceilz, floorz;
			getzsofslope(nNextSector, hitInfo->hitx, hitInfo->hity, &ceilz, &floorz);

			if ( hitInfo->hitz <= ceilz || hitInfo->hitz >= floorz )
				return SS_WALL;

			WALL *pWall = &wall[hitInfo->hitwall];
			if ( !(pWall->cstat & kWallMasked) )
			{
				dprintf("Hitscan hit wall which is not masked\n",
					hitInfo->hitwall);
				dprintf("hitscan(%i, %i, %i, %i, %i, %i, %i ...)\n", x, y, z, pSprite->sectnum, dx, dy, dz);
				dprintf("  hitsect   = %i\n", hitInfo->hitsect);
				dprintf("  hitwall   = %i\n", hitInfo->hitwall);
				dprintf("  hitsprite = %i\n", hitInfo->hitsprite);
				dprintf("  hitx      = %i\n", hitInfo->hitx);
				dprintf("  hity      = %i\n", hitInfo->hity);
				dprintf("  hitz      = %i\n", hitInfo->hitz);
				return SS_WALL;
			}

			// must be a masked wall
			int zOrg;
			if ( pWall->cstat & kWallBottomOrg )
				zOrg = ClipHigh(sector[hitInfo->hitsect].floorz, sector[nNextSector].floorz);
			else
				zOrg = ClipLow(sector[hitInfo->hitsect].ceilingz, sector[nNextSector].ceilingz);

			int zOff = (hitInfo->hitz - zOrg) >> 8;
			if ( pWall->cstat & kWallFlipY)
				zOff = -zOff;

			int nTile = pWall->overpicnum;
			int tsizx = tilesizx[nTile];
			int tsizy = tilesizy[nTile];
			BOOL xnice = (1 << (picsiz[nTile] & 15)) == tsizx;
			BOOL ynice = (1 << (picsiz[nTile] >> 4)) == tsizy;

			// calculate y texel coord
			int texy = zOff * pWall->yrepeat / 8 + pWall->ypanning * tsizy / 256;

			int len = qdist(pWall->x - wall[pWall->point2].x, pWall->y - wall[pWall->point2].y);
			int dist;

			if ( pWall->cstat & kWallFlipX )
				dist = qdist(hitInfo->hitx - wall[pWall->point2].x, hitInfo->hity - wall[pWall->point2].y);
			else
				dist = qdist(hitInfo->hitx - pWall->x, hitInfo->hity - pWall->y);

			// calculate x texel coord
			int texx = dist * pWall->xrepeat * 8 / len + pWall->xpanning;

			if ( xnice )
				texx &= (tsizx - 1);
			else
				texx %= tsizx;

			if ( ynice )
				texy &= (tsizy - 1);
			else
				texy %= tsizy;

			dprintf("Hitscan on masked wall %d at texel coord %d, %d\n",
				hitInfo->hitwall, texx, texy);

			BYTE *pTile = tileLoadTile(nTile);
			BYTE texel;

			if ( ynice )
				texel = pTile[(texx << (picsiz[nTile] >> 4)) + texy];
			else
			 	texel = pTile[texx * tilesizy[nTile] + texy];

			if ( texel == 255 )	// mask color
			{
				dprintf("Texel is clear\n");

				// clear hitscan bits on both sides of the wall
				ushort oldcstat1 = pWall->cstat;
				pWall->cstat &= ~kWallHitscan;
				ushort oldcstat2 = wall[pWall->point2].cstat;
				wall[pWall->point2].cstat &= ~kWallHitscan;

				hitInfo->hitsect = -1;
				hitInfo->hitwall = -1;
				hitInfo->hitsprite = -1;

				hitscan(x, y, z, nSector, dx, dy, dz << 4,
					&hitInfo->hitsect, &hitInfo->hitwall, &hitInfo->hitsprite,
					&hitInfo->hitx, &hitInfo->hity, &hitInfo->hitz);

				// restore the hitscan bits
				pWall->cstat = oldcstat1;
				wall[pWall->point2].cstat = oldcstat2;

				goto retry;
			}

			return SS_MASKED;
		}
	}

	if (hitInfo->hitsect >= 0)
		return (hitInfo->hitz > z) ? SS_FLOOR : SS_CEILING;

	return -1;
}



/*******************************************************************************
	FUNCTION:		GetZRange()

	DESCRIPTION:	Cover function for Ken's getzrange

	PARAMETERS:

	RETURNS:

	NOTES:
*******************************************************************************/
void GetZRange( SPRITE *pSprite, long *ceilZ, long *ceilHit, long *floorZ, long *floorHit,
	int clipdist, char cliptype )
{
	dassert(pSprite != NULL);

	short oldcstat = pSprite->cstat;
	pSprite->cstat &= ~kSpriteBlocking & ~kSpriteHitscan;
	getzrange(pSprite->x, pSprite->y, pSprite->z, pSprite->sectnum,
		ceilZ, ceilHit, floorZ, floorHit, clipdist, cliptype);
	pSprite->cstat = oldcstat;
}


unsigned ClipMove( long *px, long *py, long *pz, short *pnSector, long dx, long dy,
	int wallDist, int ceilDist, int floorDist, char clipType )
{
//	return clipmove(px, py, pz, pnSector, dx << 14, dy << 14,
//		wallDist, ceilDist, floorDist, clipType);
	long x = *px;
	long y = *py;
	long z = *pz;
	short nSector = *pnSector;

	unsigned ccode = clipmove(px, py, pz, pnSector, dx << 14, dy << 14,
		wallDist, ceilDist, floorDist, clipType);

	// force temporary fix to ken's inside() bug
	if ( *pnSector == -1)
	{
		// return to last known good location
		*px = x;
		*py = y;
		*pz = z;
		*pnSector = nSector;
	}
	return ccode;
}


#if 0
int rintersect( int x, int y, int z, int vx, int vy, int vz, int xc,
	int yc, int xd, int yd, int *xint, int *yint, int *zint)
{
	int lx = xd - xc;
	int ly = yd - yc;
	int qx = x - xc;
	int qy = y - yc;

	int den = vx * ly - vy * lx;
	if ( den <= 0 )
		return 0;	// lines are parallel or wrong side of line

	int snum = qy * vx - qx * vy;
	if ( snum < 0 || snum >= den )
		return 0;	// not within CD

	int rnum = qy * lx - qx * ly;
	if ( rnum < 0 )
		return 0;	// before AB

	int r = divscale16(rnum, den);
	*xint = x + mulscale16(vx, r);
	*yint = y + mulscale16(vy, r);
	*zint = z + mulscale16(vz, r);
	return 1;
}
#endif


