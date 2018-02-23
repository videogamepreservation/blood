#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dos.h>
#include <conio.h>

#include "debug4g.h"
#include "engine.h"
#include "globals.h"
#include "resource.h"
#include "misc.h"
#include "gameutil.h"
#include "screen.h"
#include "tile.h"

#include <memcheck.h>


// empty function stubs
void allocache(long *, long, char *) {}
void initcache(long, long) {}


void * kmalloc( size_t size )
{
	void *ptr = Resource::Alloc( (long)size );
	dprintf( "kmalloc(%i) = %P\n", size, ptr );
	return ptr;
}


void kfree( void * ptr )
{
	dprintf( "kfree(%p)\n", ptr );
	Resource::Free( ptr );
}


int loadpics( char * )
{
	if (tileInit() == 0)
		return -1;
	return 0;
}


void loadtile( short nTile )
{
	tileLoadTile(nTile);
}


ulong allocatepermanenttile( short nTile, long nSizeX, long nSizeY )
{
	return (ulong)tileAllocTile(nTile, nSizeX, nSizeY);
}

void overwritesprite( long thex, long they, short tilenum, schar shade, char flags, char dapalnum )
{
	rotatesprite(thex << 16, they << 16, 0x10000, (short)((flags & 8) << 7), tilenum, shade, dapalnum,
		(char)(((flags & 1 ^ 1) << 4) + (flags & 2) + ((flags & 4) >> 2) + ((flags & 16) >> 2) ^ ((flags & 8) >> 1)),
		windowx1, windowy1, windowx2, windowy2);
}

short animateoffs( short nTile, ushort nInfo )
{
	int index = 0, frames, clock;

	dassert(nTile >= 0 && nTile < kMaxTiles);

	if ( (nInfo & 0xC000) == 0x8000)	// sprite
	{
		// hash sprite frame by info variable
		clock = (gFrameClock + CRC32(&nInfo, 2)) >> picanm[nTile].speed;
	}
	else
		clock = gFrameClock >> picanm[nTile].speed;

	frames = picanm[nTile].frames;

	if (frames > 0)
	{
		switch(picanm[nTile].type)
		{
			case 1:     // Oscil
				index = clock % (frames * 2);
				if (index >= frames)
					index = frames * 2 - index;
				break;

			case 2:		// Forward
				index = clock % (frames + 1);
				break;

			case 3:		// Backward
				index = -(clock % (frames + 1));
				break;
		}
	}
	return (short)index;
}


/*
inline BOOL flintersect(long x1, long y1, long z1, long x2, long y2, long z2, long x3,
			  long y3, long x4, long y4, long *intx, long *inty, long *intz)
{
	long x21 = x2 - x1;
	long x34 = x3 - x4;
	long y21 = y2 - y1;
	long y34 = y3 - y4;
	long bot = x21 * y34 - y21 * x34;

	if ( bot >= 0 )
		return FALSE;

	long x31 = x3 - x1;
	long y31 = y3 - y1;
	long topt = x31 * y34 - y31 * x34;
	if ( topt > 0 || topt <= bot)
		return FALSE;
	long topu = x21 * y31 - y21 * x31;
	if ( topu > 0 || topu <= bot)
		return FALSE;

	long t = divscale24(topt, bot);
	*intx = x1 + mulscale24(x21, t);
	*inty = y1 + mulscale24(y21, t);
	*intz = z1 + mulscale24(z2 - z1, t);
	return TRUE;
}



BOOL cansee(long x0, long y0, long z0, short sect0, long x1, long y1, long z1, short sect1)
{
	SECTOR *pSector;
	WALL *pWall, *pWall2;
	int i, nSector, nCount, nWalls;
	long nNextSector, x, y, z, daz, daz2;
	short clipsectorlist[512];

	int qx = x1 - x0;
	int qy = y1 - y0;

	if ( x0 == x1 && y0 == y1 )
		return (sect0 == sect1);

	clipsectorlist[0] = sect0;
	nCount = 1;

	for( i = 0; i < nCount; i++ )
	{
		nSector = clipsectorlist[i];
		pSector = &sector[nSector];

		for( nWalls = pSector->wallnum, pWall = &wall[pSector->wallptr]; nWalls > 0; nWalls--, pWall++)
		{
			pWall2 = &wall[pWall->point2];

			if ( flintersect(x0, y0, z0, x1, y1, z1,
				pWall->x, pWall->y, pWall2->x, pWall2->y, &x, &y, &z) != 0)
			{
				nNextSector = pWall->nextsector;
				if (nNextSector < 0)
					return FALSE;

				getzsofslope((short)nSector, x, y, &daz, &daz2);
				if ( z <= daz || z >= daz2 )
					return FALSE;

				getzsofslope((short)nNextSector, x, y, &daz, &daz2);
				if ( z <= daz || z >= daz2 )
					return FALSE;

				for (int j = nCount - 1; j >= 0; j--)
				{
					if (clipsectorlist[j] == nNextSector)
						break;
				}

				if ( j < 0 )
					clipsectorlist[nCount++] = (short)nNextSector;
				dassert(nCount < 512);
			}
		}

		if ( clipsectorlist[i] == sect1 )
			return TRUE;
	}
	return FALSE;
}
*/


unsigned movesprite(short nSprite, long dx, long dy, long dz, long ceildist, long flordist, char cliptype, long nTicks)
{
	int retval;
	short nSector;
	int zTop, zBot;
	long zCeil, zFloor;

	dassert(nSprite >= 0 && nSprite < kMaxSprites);
	SPRITE *pSprite = &sprite[nSprite];

	nSector = pSprite->sectnum;
	dassert(nSector>= 0 && nSector < kMaxSectors);

	retval = ClipMove(&pSprite->x, &pSprite->y, &pSprite->z, &nSector,
		dx * nTicks >> 4, dy * nTicks >> 4, pSprite->clipdist << 2,
		ceildist, flordist, cliptype);

	if ((nSector != pSprite->sectnum) && (nSector >= 0))
		changespritesect(nSprite, nSector);

	int z = pSprite->z + dz * nTicks;
	GetSpriteExtents(pSprite, &zTop, &zBot);
	pSprite->z = z;

	getzsofslope(nSector, pSprite->x, pSprite->y, &zCeil, &zFloor);

	if ( !(sector[nSector].floorstat & kSectorParallax) )
		pSprite->z += ClipHigh(zFloor - zBot, 0);
	if ( !(sector[nSector].ceilingstat & kSectorParallax) )
		pSprite->z += ClipLow(zCeil - zTop, 0);

	if (retval != 0)
		return retval;

	if ( pSprite->z < z )
		return kHitFloor | nSector;

	if ( pSprite->z > z )
		return kHitCeiling | nSector;

	return 0;
}


void uninitengine()
{
	tileTerm();
}


void loadpalette()
{
	scrLoadPalette();
}


int getpalookup(long nVis, long nShade)
{
	if ( gFogMode )
		return ClipHigh(nVis >> 8, 15) * 16 + ClipRange(nShade >> 2, 0, 15);
	else
		return ClipRange(nShade + (nVis >> 8), 0, kPalLookups - 1);
}
