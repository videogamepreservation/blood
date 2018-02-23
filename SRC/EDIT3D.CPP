#include <string.h>
#include <stdio.h>

#include "typedefs.h"
#include "engine.h"
#include "debug4g.h"
#include "bstub.h"
#include "misc.h"
#include "gameutil.h"
#include "build.h"
#include "trig.h"
#include "names.h"
#include "db.h"
#include "tile.h"
#include "screen.h"
#include "gui.h"
#include "key.h"
#include "globals.h"
#include "gfx.h"
#include "mouse.h"
#include "sectorfx.h"

typedef void HSECTORFUNC( short nSector );
typedef void HSPRITEFUNC( SPRITE *pSprite );


/*******************************************************************************
									Defines
*******************************************************************************/
#define kPlayerRadius		200
#define kMaxVisited			8192	// set to larger of kMaxSectors and kMaxWalls


/*******************************************************************************
								Local Variables
*******************************************************************************/
static char buffer[256] = "";
static char *gZModeMsg[3] =
{
	"Gravity",
	"Locked/Step",
	"Locked/Free"
};

static BOOL visited[kMaxVisited];
static int gStep;

static ushort WallShadeFrac[kMaxWalls];
static ushort FloorShadeFrac[kMaxSectors];
static ushort CeilShadeFrac[kMaxSectors];
static int WallArea[kMaxWalls];
static int SectorArea[kMaxSectors];
static int WallNx[kMaxWalls], WallNy[kMaxWalls];
static int FloorNx[kMaxSectors], FloorNy[kMaxSectors], FloorNz[kMaxSectors];
static int CeilNx[kMaxSectors], CeilNy[kMaxSectors], CeilNz[kMaxSectors];


/*******************************************************************************
These are globals that should be referenced in other parts of build.c.
Ken should ADD these to BUILD and make them non-static.
*******************************************************************************/
char tempvisibility = 0;


/***********************************************************************
 * SetCeilingZ()
 *
 * Change the z of a sector ceiling and all sprites on the ceiling
 **********************************************************************/
static void SetCeilingZ( int nSector, long z )
{
	dassert(nSector >= 0 && nSector < kMaxSectors);

	// don't allow to go through the floor
	z = ClipHigh(z, sector[nSector].floorz);

	for (int i = headspritesect[searchsector]; i != -1; i = nextspritesect[i])
	{
		int zTop, zBot;
		SPRITE *pSprite = &sprite[i];
		GetSpriteExtents(pSprite, &zTop, &zBot);
		if ( zTop <= getceilzofslope((short)nSector, pSprite->x, pSprite->y) )
			pSprite->z += z - sector[nSector].ceilingz;
	}
	sector[nSector].ceilingz = z;
}


/***********************************************************************
 * SetFloorZ()
 *
 * Change the z of a sector floor and all sprites on the floor
 **********************************************************************/
static void SetFloorZ( int nSector, long z )
{
	dassert(nSector >= 0 && nSector < kMaxSectors);

	// don't allow to go through the ceiling
	z = ClipLow(z, sector[nSector].ceilingz);

	for (int i = headspritesect[searchsector]; i != -1; i = nextspritesect[i])
	{
		int zTop, zBot;
		SPRITE *pSprite = &sprite[i];
		GetSpriteExtents(pSprite, &zTop, &zBot);
		if (zBot >= getflorzofslope((short)nSector, pSprite->x, pSprite->y) )
			pSprite->z += z - sector[nSector].floorz;
	}
	sector[nSector].floorz = z;
}


/***********************************************************************
 * SetCeilingSlope()
 *
 * Change the slope of a sector ceiling and all sprites on the ceiling
 **********************************************************************/
static void SetCeilingSlope( short nSector, short nSlope )
{
	dassert(nSector >= 0 && nSector < kMaxSectors);

	sector[nSector].ceilingslope = nSlope;

	if ( sector[nSector].ceilingslope == 0 )
		sector[nSector].ceilingstat &= ~kSectorSloped;
	else
		sector[nSector].ceilingstat |= kSectorSloped;

	for (int i = headspritesect[searchsector]; i != -1; i = nextspritesect[i])
	{
		SPRITE *pSprite = &sprite[i];
		int zTop, zBot;
		GetSpriteExtents(pSprite, &zTop, &zBot);
		long z = getceilzofslope(nSector, pSprite->x, pSprite->y);
		if ( zTop < z )
			sprite[i].z += z - zTop;
	}
}


/***********************************************************************
 * SetFloorSlope()
 *
 * Change the slope of a sector floor and all sprites on the floor
 **********************************************************************/
static void SetFloorSlope( short nSector, short nSlope )
{
	dassert(nSector >= 0 && nSector < kMaxSectors);

	sector[nSector].floorslope = nSlope;

	if ( sector[nSector].floorslope == 0 )
		sector[nSector].floorstat &= ~kSectorSloped;
	else
		sector[nSector].floorstat |= kSectorSloped;

	for (int i = headspritesect[searchsector]; i != -1; i = nextspritesect[i])
	{
		SPRITE *pSprite = &sprite[i];
		int zTop, zBot;
		GetSpriteExtents(pSprite, &zTop, &zBot);
		long z = getflorzofslope(nSector, pSprite->x, pSprite->y);
		if ( zBot > z )
			sprite[i].z += z - zBot;
	}
}


/***********************************************************************
 * IsSectorHighlight()
 *
 * Determines if a sector is in the sector highlight list
 **********************************************************************/
static BOOL IsSectorHighlight( int nSector )
{
	dassert(nSector >= 0 && nSector < kMaxSectors);

	for (int i = 0; i < highlightsectorcnt; i++)
		if (highlightsector[i] == nSector)
			return TRUE;

	return FALSE;
}


inline int NextCCW( int nWall )
{
	dassert( wall[nWall].nextwall >= 0 );
	return wall[wall[nWall].nextwall].point2;
}


/*******************************************************************************
	FUNCTION:		GetWallZPeg()

	DESCRIPTION:	Calculate the z position that the wall texture is relative
					to.
*******************************************************************************/
inline int GetWallZPeg( int nWall )
{
	dassert(nWall >= 0 && nWall < kMaxWalls);
	int z;

	int nSector = sectorofwall((short)nWall);
	dassert(nSector >= 0 && nSector < kMaxSectors);

	int nNextSector = wall[nWall].nextsector;

	if (nNextSector == -1)
	{
		// one sided wall
		if ( wall[nWall].cstat & kWallBottomOrg )
			z = sector[nSector].floorz;
		else
			z = sector[nSector].ceilingz;
	}
	else
	{
		// two sided wall
		if ( wall[nWall].cstat & kWallOutsideOrg )
			z = sector[nSector].ceilingz;
		else
		{
			// top step
			if (sector[nNextSector].ceilingz > sector[nSector].ceilingz)
				z = sector[nNextSector].ceilingz;
			// bottom step
			if (sector[nNextSector].floorz < sector[nSector].floorz)
				z = sector[nNextSector].floorz;
		}
	}
	return z;
}


static void AlignWalls( int nWall0, int z0, int nWall1, int z1, int nTile )
{
	dassert(nWall0 >= 0 && nWall0 < kMaxWalls);
	dassert(nWall1 >= 0 && nWall1 < kMaxWalls);

	dprintf("¯%d", nWall1);

	// do the x alignment
	wall[nWall1].cstat &= ~kWallFlipMask;    // set to non-flip
	wall[nWall1].xpanning = (uchar)((wall[nWall0].xpanning + (wall[nWall0].xrepeat << 3)) % tilesizx[nTile]);

	z1 = GetWallZPeg(nWall1);

	int n = picsiz[nTile] >> 4;
	if ( (1 << n) != tilesizy[nTile] )
		n++;

	wall[nWall1].yrepeat = wall[nWall0].yrepeat;
	wall[nWall1].ypanning = (uchar)(wall[nWall0].ypanning + (((z1 - z0) * wall[nWall0].yrepeat) >> (n + 3)));
}


#define kMaxAlign	64
static void AutoAlignWalls( int nWall0, int ply = 0 )
{
	dassert(nWall0 >= 0 && nWall0 < kMaxWalls);

	int z0, z1;
	int nTile = wall[nWall0].picnum;
	int nWall1;
	int branch = 0;

	if ( ply == kMaxAlign )
	{
		dprintf("\nAlignment aborted by ply -- possible node problem!\n");
		return;
	}

	if ( ply == 0 )
	{
		// clear visited bits
		memset(visited, FALSE, sizeof(visited));
		visited[nWall0] = TRUE;
		dprintf("Aligning walls: %d", nWall0);
	}

	z0 = GetWallZPeg(nWall0);

	nWall1 = wall[nWall0].point2;
	dassert(nWall1 >= 0 && nWall1 < kMaxWalls);

	// loop through walls at this vertex in CCW order
	while (1)
	{
		// break if this wall would connect us in a loop
		if ( visited[nWall1] )
			break;

		visited[nWall1] = TRUE;

		// break if reached back of left wall
		if ( wall[nWall1].nextwall == nWall0 )
			break;

		if ( wall[nWall1].picnum == nTile )
		{
			z1 = GetWallZPeg(nWall1);
			BOOL visible = FALSE;

			int nNextSector = wall[nWall1].nextsector;
			if ( nNextSector < 0 )
				visible = TRUE;
			else
			{
				// ignore two sided walls that have no visible face
				int nSector = wall[wall[nWall1].nextwall].nextsector;
				if ( getceilzofslope((short)nSector, wall[nWall1].x, wall[nWall1].y) <
					getceilzofslope((short)nNextSector, wall[nWall1].x, wall[nWall1].y) )
					visible = TRUE;

				if ( getflorzofslope((short)nSector, wall[nWall1].x, wall[nWall1].y) >
					getflorzofslope((short)nNextSector, wall[nWall1].x, wall[nWall1].y) )
					visible = TRUE;
			}

			if ( visible )
			{
				if ( branch++ )
					dprintf(" %d", nWall0);
				AlignWalls(nWall0, z0, nWall1, z1, nTile);

				int nNextWall = wall[nWall1].nextwall;

				// if wall was 1-sided, no need to recurse
				if ( nNextWall < 0 )
				{
					nWall0 = nWall1;
					z0 = GetWallZPeg(nWall0);
					nWall1 = wall[nWall0].point2;
					branch = 0;
					continue;
				}
				else
				{
					if ( wall[nWall1].cstat & kWallBottomSwap && wall[nNextWall].picnum == nTile )
						AlignWalls(nWall0, z0, nNextWall, z1, nTile);
					AutoAlignWalls(nWall1, ply + 1);
				}
			}
		}

		if (wall[nWall1].nextwall < 0)
			break;

		nWall1 = wall[wall[nWall1].nextwall].point2;
	}

	if ( ply == 0 )
		dprintf("\n");
}


static void BuildStairsF( int nSector )
{
	int i, j;

	dassert(nSector >= 0 && nSector < kMaxSectors);

	// mark this sector as visited
	visited[nSector] = TRUE;

	for (i = 0; i < sector[nSector].wallnum; i++)
	{
		j = wall[sector[nSector].wallptr + i].nextsector;
		if (j != -1)
		{
			if ( IsSectorHighlight(j) && !visited[j] )
			{
				SetFloorZ(j, sector[nSector].floorz - (gStairHeight << 8));
				BuildStairsF(j);
			}
		}
	}
}


static void BuildStairsC( int nSector )
{
	int i, j;

	dassert(nSector >= 0 && nSector < kMaxSectors);

	// mark this sector as visited
	visited[nSector] = TRUE;

	for (i = 0; i < sector[nSector].wallnum; i++)
	{
		j = wall[sector[nSector].wallptr + i].nextsector;
		if (j != -1)
		{
			if ( IsSectorHighlight(j) && !visited[j] )
			{
				SetCeilingZ(j, sector[nSector].ceilingz - (gStairHeight << 8));
				BuildStairsC(j);
			}
		}
	}
}


static void ShootRay(int x, int y, int z, short nSector, int dx, int dy, int dz, int nIntensity, int nReflect, int dist)
{
	short hitsect = -1, hitwall = -1, hitsprite = -1;
	long hitx, hity, hitz;
	int x2, y2, z2;
	int dotProduct;
	int E;

	hitscan(x, y, z, nSector, dx, dy, dz << 4, &hitsect, &hitwall, &hitsprite, &hitx, &hity, &hitz);

	x2 = qabs(x - hitx) >> 4;
	y2 = qabs(y - hity) >> 4;
	z2 = qabs(z - hitz) >> 8;
	dist += ksqrt(x2 * x2 + y2 * y2 + z2 * z2);

	if (hitwall >= 0)
	{
		E = divscale16(nIntensity, ClipLow(dist + gLBRampDist, 1));
		if (E <= 0)
			return;		// too small to deal with

		// distribute over area of wall
		E = divscale16(E, ClipLow(WallArea[hitwall], 1));

		int n = (wall[hitwall].shade << 16) | WallShadeFrac[hitwall];
		n -= E;
		wall[hitwall].shade = (schar)ClipLow(n >> 16, gLBMaxBright);
		WallShadeFrac[hitwall] = (ushort)(n & 0xFFFF);

		if ( nReflect < gLBReflections )
		{
			int Nx = WallNx[hitwall];
			int Ny = WallNy[hitwall];

			// dotProduct is cos of angle of intersection
			dotProduct = dmulscale16(dx, Nx, dy, Ny);
			if (dotProduct < 0)
				return;		// bogus intersection

			// reflect vector
			dx -= mulscale16(2 * dotProduct, Nx);
			dy -= mulscale16(2 * dotProduct, Ny);

			hitx += dx >> 12;
			hity += dy >> 12;
			hitz += dz >> 8;

			nIntensity -= mulscale16(nIntensity, gLBAttenuation);

			ShootRay(hitx, hity, hitz, hitsect, dx, dy, dz, nIntensity, nReflect + 1, dist);
		}
	}
	else if (hitsprite >= 0)
	{
	}
	else if (dz > 0)	// hit floor
	{
		E = divscale16(nIntensity, ClipLow(dist + gLBRampDist, 1));
		if (E <= 0)
			return;		// too small to deal with

		// distribute over area of floor
		E = divscale16(E, ClipLow(SectorArea[hitsect], 1));

		int n = (sector[hitsect].floorshade << 16) | FloorShadeFrac[hitsect];
		n -= E;
		sector[hitsect].floorshade = (schar)ClipLow(n >> 16, gLBMaxBright);
		FloorShadeFrac[hitsect] = (ushort)(n & 0xFFFF);

		if ( nReflect < gLBReflections )
		{
			if ( sector[hitsect].floorstat & kSectorSloped )
			{
				int Nx = FloorNx[hitsect];
				int Ny = FloorNy[hitsect];
				int Nz = FloorNz[hitsect];

				// dotProduct is cos of angle of intersection
				dotProduct = tmulscale16(dx, Nx, dy, Ny, dz, Nz);
				if (dotProduct < 0)
					return;		// bogus intersection

				dx -= mulscale16(2 * dotProduct, Nx);
				dy -= mulscale16(2 * dotProduct, Ny);
				dz -= mulscale16(2 * dotProduct, Nz);
			}
			else
				dz = -dz;

			nIntensity -= mulscale16(nIntensity, gLBAttenuation);
			hitx += dx >> 12;
			hity += dy >> 12;
			hitz += dz >> 8;
			ShootRay(hitx, hity, hitz, hitsect, dx, dy, dz, nIntensity, nReflect + 1, dist);
		}
	}
	else				// hit ceiling
	{
		E = divscale16(nIntensity, ClipLow(dist + gLBRampDist, 1));
		if (E <= 0)
			return;		// too small to deal with

		// distribute over area of ceiling
		E = divscale16(E, ClipLow(SectorArea[hitsect], 1));

		int n = (sector[hitsect].ceilingshade << 16) | CeilShadeFrac[hitsect];
		n -= E;
		sector[hitsect].ceilingshade = (schar)ClipLow(n >> 16, gLBMaxBright);
		CeilShadeFrac[hitsect] = (ushort)(n & 0xFFFF);

		if ( nReflect < gLBReflections )
		{
			// reflect vector
			if ( sector[hitsect].ceilingstat & kSectorSloped )
			{
				int Nx = CeilNx[hitsect];
				int Ny = CeilNy[hitsect];
				int Nz = CeilNz[hitsect];

				// dotProduct is cos of angle of intersection
				dotProduct = tmulscale16(dx, Nx, dy, Ny, dz, Nz);
				if (dotProduct < 0)
					return;		// bogus intersection

				dx -= mulscale16(2 * dotProduct, Nx);
				dy -= mulscale16(2 * dotProduct, Ny);
				dz -= mulscale16(2 * dotProduct, Nz);
			}
			else
				dz = -dz;

			nIntensity -= mulscale16(nIntensity, gLBAttenuation);
			hitx += dx >> 12;
			hity += dy >> 12;
			hitz += dz >> 8;
			ShootRay(hitx, hity, hitz, hitsect, dx, dy, dz, nIntensity, nReflect + 1, dist);
		}
	}
}


static int AreaOfSector( SECTOR *pSector )
{
	int area = 0;
	int startwall = pSector->wallptr;
	int endwall = startwall + pSector->wallnum;
	for (int i = startwall; i < endwall; i++)
	{
		int x1 = wall[i].x >> 4;
		int y1 = wall[i].y >> 4;
		int x2 = wall[wall[i].point2].x >> 4;
		int y2 = wall[wall[i].point2].y >> 4;
		area += (x1+x2) * (y2-y1);	// add area of this trapezoid
	}
	area >>= 1;

	return area;
}


static void SetupLightBomb( void )
{
	SECTOR *pSector;
	WALL *pWall;
	int nSector, nWall;

	pSector = &sector[0];
	for ( nSector = 0; nSector < numsectors; nSector++, pSector++ )
	{
		FloorShadeFrac[nSector] = 0;
		CeilShadeFrac[nSector] = 0;
		SectorArea[nSector] = AreaOfSector(pSector);

		pWall = &wall[pSector->wallptr];
		for (nWall = pSector->wallptr; nWall < pSector->wallptr + pSector->wallnum; nWall++, pWall++)
		{
			WallShadeFrac[nWall] = 0;

			int Nx, Ny;

			// calculate normal for wall
			Nx = (wall[pWall->point2].y - pWall->y) >> 4;
			Ny = -(wall[pWall->point2].x - pWall->x) >> 4;

			int length = ksqrt(Nx * Nx + Ny * Ny);

			WallNx[nWall] = divscale16(Nx, length);
			WallNy[nWall] = divscale16(Ny, length);

			long ceilZ, ceilZ1, ceilZ2, floorZ, floorZ1, floorZ2;
			getzsofslope((short)nSector, pWall->x, pWall->y, &ceilZ1, &floorZ1);
			getzsofslope((short)nSector, wall[pWall->point2].x, wall[pWall->point2].y, &ceilZ2, &floorZ2);
			ceilZ = (ceilZ1 + ceilZ2) >> 1;
			floorZ = (floorZ1 + floorZ2) >> 1;

			// calculate the area of the wall
			int height = floorZ - ceilZ;

			// red wall?
			if ( pWall->nextsector >= 0 )
			{
				height = 0;

				long nextCeilZ, nextCeilZ1, nextCeilZ2, nextFloorZ, nextFloorZ1, nextFloorZ2;
				getzsofslope(pWall->nextsector, pWall->x, pWall->y, &nextCeilZ1, &nextFloorZ1);
				getzsofslope(pWall->nextsector, wall[pWall->point2].x, wall[pWall->point2].y, &nextCeilZ2, &nextFloorZ2);
				nextCeilZ = (nextCeilZ1 + nextCeilZ2) >> 1;
				nextFloorZ = (nextFloorZ1 + nextFloorZ2) >> 1;

				// floor step up?
				if ( nextFloorZ < floorZ)
					height += floorZ - nextFloorZ;

				// ceiling step down?
				if ( nextCeilZ > ceilZ )
					height += nextCeilZ - ceilZ;
			}

			WallArea[nWall] = length * height >> 8;
		}

		FloorNx[nSector] = 0;
		FloorNy[nSector] = 0;
		FloorNz[nSector] = -0x10000;

		CeilNx[nSector] = 0;
		CeilNy[nSector] = 0;
		CeilNz[nSector] = 0x10000;
	}
}


static void LightBomb( int x, int y, int z, short nSector )
{
	int dx, dy, dz;
	for (int a = kAngle90 - kAngle60; a <= kAngle90 + kAngle60; a += kAngle15)
	{
		for (int i = 0; i < kAngle360; i += kAngle360 / 256)
		{
			dx = mulscale30(Cos(i), Sin(a)) >> 16;
			dy = mulscale30(Sin(i), Sin(a)) >> 16;
			dz = Cos(a) >> 16;

			ShootRay(x, y, z, nSector, dx, dy, dz, gLBIntensity, 0, 0);
		}
	}
}


static void SetFirstWall( int nSector, int nWall )
{
	int start, length, shift;
	int i, j, k;
	WALL tempWall;

	// rotate the walls using the shift copy algorithm

	start = sector[nSector].wallptr;
	length = sector[nSector].wallnum;

	dassert(nWall >= start && nWall < start + length);
	shift = nWall - start;

	if (shift == 0)
		return;

	i = k = start;

	for (int n = length; n > 0; n--)
	{
		if (i == k)
			tempWall = wall[i];

		j = i + shift;
		while (j >= start + length)
			j -= length;

		if (j == k)
		{
			wall[i] = tempWall;
			i = ++k;
			continue;
		}
		wall[i] = wall[j];
		i = j;
	}

	for (i = start; i < start + length; i++)
	{
		if ( (wall[i].point2 -= shift) < start )
			wall[i].point2 += length;

		if ( wall[i].nextwall >= 0 )
			wall[wall[i].nextwall].nextwall = (short)i;
	}

	CleanUp();
}


static void TranslateWallToSector( void )
{
	short hitsect = -1, hitwall = -1, hitsprite = -1;
	long hitx, hity, hitz;
	long x, y;

	x = 1 << 14;
	y = divscale(searchx - xdim / 2, xdim / 2, 14);
	RotateVector(&x, &y, ang);
	hitscan(posx, posy, posz, cursectnum,        //Start position
		x, y, (searchy - horiz) * 2000,          //vector of 3D ang
		&hitsect, &hitwall, &hitsprite, &hitx, &hity, &hitz);

	if (hitwall == searchwall)						// front of wall?
		searchsector = wall[searchwall].nextsector;
	else if (hitwall == wall[searchwall].nextwall)	// back of 2 texture wall?
		searchsector = wall[hitwall].nextsector;
	else
	{
		dprintf("Can't figure out what you are pointing at\n");
		return;
	}

	if (hitz > getflorzofslope(searchsector, hitx, hity))
	{
		searchstat = SS_FLOOR;
	}
	else
	{
		searchstat = SS_CEILING;
	}
}



/*******************************************************************************
	FUNCTION:		InsertMisc()

	DESCRIPTION:	Displays a tile menu allowing user to insert
					a miscellaneous game object.

	PARAMETERS:

	NOTES:			This routine should offer a list box rather than tiles.
*******************************************************************************/
static BOOL InsertMisc( int where, int nSector, long x, long y, long z, int nAngle )
{
	static short miscTiles[] =
	{
		kPicSwitch1Off,
		kPicSwitch2Off,
		kPicSwitch3Off,
		kPicSwitch4Off,
		kPicSwitch5Off,
		kPicSwitch6Off,
		kPicSwitch7Off,
		kPicWallCrack,
		kPicFluorescent,
		kPicGlass,
		kPicBlockWeb,
		kPicWoodBeam,
//		kPicMetalGrate1,
		kPicCrateFace,
		kPicVase1,
		kPicVase2,
		kPicStart1,
		kPicStart2,
		kPicStart3,
		kPicStart4,
		kPicStart5,
		kPicStart6,
		kPicStart7,
		kPicStart8,
	};

	short nType;
	short nStat = kStatDefault;
	tileIndexCount = LENGTH(miscTiles);
	for ( int i = 0; i < tileIndexCount; i++ )
		tileIndex[i] =  miscTiles[i];  // gMiscInfo[i].picnum;

	short picnum = tilePick( -1 /*gMiscInfo[0].picnum*/, -1, SS_CUSTOM );

	if ( picnum == -1 )
		return FALSE;

	switch( picnum )
	{
		case kPicSwitch1Off:
		case kPicSwitch2Off:
		case kPicSwitch3Off:
		case kPicSwitch4Off:
		case kPicSwitch5Off:
		case kPicSwitch6Off:
		case kPicSwitch7Off:
			nType = kSwitchToggle;
			break;
		case kPicWallCrack:
			nType = kThingWallCrack;
			break;
		case kPicFluorescent:
			nType = kThingFluorescent;
			break;
		case kPicGlass:
			nType = kThingClearGlass;
			break;
		case kPicBlockWeb:
			nType = kThingWeb;
			break;
		case kPicWoodBeam:
			nType = kThingWoodBeam;
			break;
//		case kPicMetalGrate1:
//			nType = kThingMetalGrate1;
//			break;
		case kPicCrateFace:
			nType = kThingCrateFace;
			break;
		case kPicVase1:
			nType = kThingBlueVase;
			break;
		case kPicVase2:
			nType = kThingBrownVase;
			break;
		case kPicStart1:
		case kPicStart2:
		case kPicStart3:
		case kPicStart4:
		case kPicStart5:
		case kPicStart6:
		case kPicStart7:
		case kPicStart8:
			nType = kMarkerPlayerStart;
			break;
	}

	int nSprite = insertsprite( (short)nSector, nStat );
	dassert( nSprite >= 0 && nSprite < kMaxSprites );
	sprite[nSprite].type = nType;

	switch( picnum )
	{
		case kPicStart1:
		case kPicStart2:
		case kPicStart3:
		case kPicStart4:
		case kPicStart5:
		case kPicStart6:
		case kPicStart7:
		case kPicStart8:
		{
			int nXSprite = GetXSprite( nSprite );
			dassert(nXSprite > 0 && nXSprite < kMaxXSprites);
			xsprite[nXSprite].data1 = picnum - kPicStart1;
			break;
		}
	}

	AutoAdjustSprites();

	SPRITE *pSprite = &sprite[nSprite];
	pSprite->x = x;
	pSprite->y = y;
	pSprite->z = z;
	pSprite->ang = (short)nAngle;
	pSprite->shade = -8;

	int zTop, zBot;
	GetSpriteExtents(pSprite, &zTop, &zBot);

	if ( where == SS_FLOOR)
		pSprite->z += getflorzofslope((short)nSector, pSprite->x, pSprite->y) - zBot;
	else
		pSprite->z += getceilzofslope((short)nSector, pSprite->x, pSprite->y) - zTop;

	updatenumsprites();

	BeepOkay();

	return TRUE;
}


/*******************************************************************************
	FUNCTION:		InsertHazard()

	DESCRIPTION:	Displays a tile menu allowing user to insert a hazard.

	PARAMETERS:

	NOTES:			This routine should offer a list box rather than tiles.
*******************************************************************************/
static BOOL InsertHazard( int where, int nSector, long x, long y, long z, int nAngle )
{
	static short hazardTiles[] =
	{
		kAnmCircSaw1,
		kAnmTNTProxArmed,
		kPicTNTBarrel,
		kAnmFloorSpike,
		kAnmPendulum,
		kPicGuillotine,
		kAnmElectrify,
		kPicMachineGun,
	};

	short nType;
	short nStat = kStatDefault;
	tileIndexCount = LENGTH(hazardTiles);
	for ( int i = 0; i < tileIndexCount; i++ )
		tileIndex[i] =  hazardTiles[i];  // gItemInfo[i].picnum;

	short picnum = tilePick( -1 /*gHazardInfo[0].picnum*/, -1, SS_CUSTOM );
	if ( picnum == -1 )
		return FALSE;

	switch( picnum )
	{
		case kAnmCircSaw1:
			nType = kTrapSawBlade;
			break;
		case kAnmTNTProxArmed:
			nType = kThingTNTProxArmed;
			nStat = kStatProximity;
			break;
		case kPicTNTBarrel:
			nType = kThingTNTBarrel;
			break;
//		case kAnmRollBarrel:
//			nType = kThingTNTBarrel;
//			break;
		case kAnmFloorSpike:
			nType = kTrapSpiketrap;
			break;
		case kAnmPendulum:
			nType = kTrapPendulum;
			break;
		case kPicGuillotine:
			nType = kTrapGuillotine;
			break;
		case kAnmElectrify:
			nType = kTrapPoweredZap;
			break;
		case kPicMachineGun:
			nType = kThingMachineGun;
			break;
		default:
			scrSetMessage("Invalid hazard tile selected!");
			BeepError();
			return FALSE;
	}

	int nSprite = insertsprite( (short)nSector, nStat );
	dassert( nSprite >= 0 && nSprite < kMaxSprites );
	sprite[nSprite].type = nType;

	AutoAdjustSprites();

	SPRITE *pSprite = &sprite[nSprite];
	pSprite->x = x;
	pSprite->y = y;
	pSprite->z = z;
	pSprite->ang = (short)nAngle;
	pSprite->shade = -8;

	int zTop, zBot;
	GetSpriteExtents(pSprite, &zTop, &zBot);

	if ( where == SS_FLOOR)
		pSprite->z += getflorzofslope((short)nSector, pSprite->x, pSprite->y) - zBot;
	else
		pSprite->z += getceilzofslope((short)nSector, pSprite->x, pSprite->y) - zTop;

	updatenumsprites();

	BeepOkay();

	return TRUE;
}


/*******************************************************************************
	FUNCTION:		InsertItem()

	DESCRIPTION:	Displays a tile menu allowing user to insert an item
					on the floor.

	PARAMETERS:

	NOTES:			This routine should offer a list box rather than tiles.
*******************************************************************************/
static BOOL InsertItem( int where, int nSector, long x, long y, long z, int nAngle )
{
	static short itemTiles[] =
	{
		kPicKey1,				//kItemKey1
		kPicKey2,				//kItemKey2
		kPicKey3,				//kItemKey3
		kPicKey4,				//kItemKey4
		kPicKey5,				//kItemKey5
		kPicKey6,				//kItemKey6
		kPicDocBag,				//kItemDoctorBag
		kPicMedPouch,			//kItemMedPouch
		kPicEssence,			//kItemLifeEssence
		kAnmLifeSeed,			//kItemLifeSeed
		kPicPotion,				//kItemPotion1
		kAnmFeather,			//kItemFeatherFall
		kAnmInviso,				//kItemLtdInvisibility
		kPicInvulnerable,		//kItemInvulnerability
		kPicJumpBoots,			//kItemJumpBoots
		kPicRavenFlight,		//kItemRavenFlight
		kPicGunsAkimbo,			//kItemGunsAkimbo
		kPicDivingSuit,			//kItemDivingSuit
		kPicGasMask,			//kItemGasMask
		kAnmClone,				//kItemClone
		kPicCrystalBall,		//kItemCrystalBall
		kPicDecoy,				//kItemDecoy
		kAnmDoppleganger,		//kItemDoppleganger
		kAnmReflectShots,		//kItemReflectiveShots
		kPicRoseGlasses,		//kItemRoseGlasses
		kAnmCloakNDagger,		//kItemShadowCloak
		kPicShroom1,			//kItemShroomRage
		kPicShroom2,			//kItemShroomDelirium
		kPicShroom3,			//kItemShroomGrow
		kPicShroom4,			//kItemShroomShrink
		kPicDeathMask,			//kItemDeathMask
		kPicGoblet,				//kItemWineGoblet
		kPicBottle1,			//kItemWineBottle
		kPicSkullGrail,			//kItemSkullGrail
		kPicSilverGrail,		//kItemSilverGrail
		kPicTome1,				//kItemTome
		kPicBlackChest,			//kItemBlackChest
		kPicWoodChest,			//kItemWoodenChest
		kPicAsbestosSuit,		//kItemAsbestosArmor
		kPicRandomUp			//kItemRandom
	};

	short nType;
	tileIndexCount = LENGTH(itemTiles);
	for ( int i = 0; i < tileIndexCount; i++ )
		tileIndex[i] =  itemTiles[i];  // gItemInfo[i].picnum;

	short picnum = tilePick( -1 /*gItemInfo[0].picnum*/, -1, SS_CUSTOM );
	if ( picnum == -1 )
		return FALSE;

	switch( picnum )
	{
		case kPicKey1:
			nType = kItemKey1;
			break;
		case kPicKey2:
			nType = kItemKey2;
			break;
		case kPicKey3:
			nType = kItemKey3;
			break;
		case kPicKey4:
			nType = kItemKey4;
			break;
		case kPicKey5:
			nType = kItemKey5;
			break;
		case kPicKey6:
			nType = kItemKey6;
			break;
		case kPicDocBag:
			nType = kItemDoctorBag;
			break;
		case kPicMedPouch:
			nType = kItemMedPouch;
			break;
		case kPicEssence:
			nType = kItemLifeEssence;
			break;
		case kAnmLifeSeed:
			nType = kItemLifeSeed;
			break;
		case kPicPotion:
			nType = kItemPotion1;
			break;
		case kAnmFeather:
			nType = kItemFeatherFall;
			break;
		case kAnmInviso:
			nType = kItemLtdInvisibility;
			break;
		case kPicInvulnerable:
			nType = kItemInvulnerability;
			break;
		case kPicJumpBoots:
			nType = kItemJumpBoots;
			break;
		case kPicRavenFlight:
			nType = kItemRavenFlight;
			break;
		case kPicGunsAkimbo:
			nType = kItemGunsAkimbo;
			break;
		case kPicDivingSuit:
			nType = kItemDivingSuit;
			break;
		case kPicGasMask:
			nType = kItemGasMask;
			break;
		case kAnmClone:
			nType = kItemClone;
			break;
		case kPicCrystalBall:
			nType = kItemCrystalBall;
			break;
		case kPicDecoy:
			nType = kItemDecoy;
			break;
		case kAnmDoppleganger:
			nType = kItemDoppleganger;
			break;
		case kAnmReflectShots:
			nType = kItemReflectiveShots;
			break;
		case kPicRoseGlasses:
			nType = kItemRoseGlasses;
			break;
		case kAnmCloakNDagger:
			nType = kItemShadowCloak;
			break;
		case kPicShroom1:
			nType = kItemShroomRage;
			break;
		case kPicShroom2:
			nType = kItemShroomDelirium;
			break;
		case kPicShroom3:
			nType = kItemShroomGrow;
			break;
		case kPicShroom4:
			nType = kItemShroomShrink;
			break;
		case kPicDeathMask:
			nType = kItemDeathMask;
			break;
		case kPicGoblet:
			nType = kItemWineGoblet;
			break;
		case kPicBottle1:
			nType = kItemWineBottle;
			break;
		case kPicSkullGrail:
			nType = kItemSkullGrail;
			break;
		case kPicSilverGrail:
			nType = kItemSilverGrail;
			break;
		case kPicTome1:
			nType = kItemTome;
			break;
		case kPicBlackChest:
			nType = kItemBlackChest;
			break;
		case kPicWoodChest:
			nType = kItemWoodenChest;
			break;
		case kPicAsbestosSuit:
			nType = kItemAsbestosArmor;
			break;
		case kPicRandomUp:
			nType = kItemRandom;
			break;
		default:
			scrSetMessage("Invalid item tile selected!");
			BeepError();
			return FALSE;
	}

	int nSprite = insertsprite( (short)nSector, kStatItem );
	dassert( nSprite >= 0 && nSprite < kMaxSprites );
	sprite[nSprite].type = nType;

	AutoAdjustSprites();

	SPRITE *pSprite = &sprite[nSprite];
	pSprite->x = x;
	pSprite->y = y;
	pSprite->z = z;
	pSprite->ang = (short)nAngle;
	pSprite->shade = -8;	// gItemInfo[nItemType].shade;

	int zTop, zBot;
	GetSpriteExtents(pSprite, &zTop, &zBot);

	if ( where == SS_FLOOR)
		pSprite->z += getflorzofslope((short)nSector, pSprite->x, pSprite->y) - zBot;
	else
		pSprite->z += getceilzofslope((short)nSector, pSprite->x, pSprite->y) - zTop;

	updatenumsprites();

	BeepOkay();

	return TRUE;
}


/*******************************************************************************
	FUNCTION:		InsertAmmo()

	DESCRIPTION:	Displays a tile menu allowing user to insert an ammo
					object on the floor.

	PARAMETERS:

	NOTES:			This routine should offer a list box rather than tiles.
*******************************************************************************/
static BOOL InsertAmmo( int where, int nSector, long x, long y, long z, int nAngle )
{
	static short ammoTiles[] =
	{
		kPicSpear,
		kPicSpears,
		kPicTNTStick,
		kPicTNTPak,
		kPicSprayCan,
		kPicShotShells,
		kPicTNTBox,
		kPicTNTRemote,
		kPicTNTProx,
		kPicShellBox,
		kPicBullets,
		kPicBulletBox,
		kPicBulletBoxAP,
		kPicTommyDrum,
		kPicFlares,
		kPicFlareHE,
		kPicFlareBurst,
		kPicSpearExplode,
		kPicRandomUp
	};

	short nType;
	tileIndexCount = LENGTH(ammoTiles);
	for ( int i = 0; i < tileIndexCount; i++ )
		tileIndex[i] =  ammoTiles[i];  // gAmmoInfo[i].picnum;

	short picnum = tilePick( -1 /*gAmmoInfo[0].picnum*/, -1, SS_CUSTOM );
	if ( picnum == -1 )
		return FALSE;

	switch( picnum )
	{
		case kPicSpear:
			nType = kAmmoItemSpear;
			break;
		case kPicSpears:
			nType = kAmmoItemSpearPack;
			break;
		case kPicTNTStick:
			nType = kAmmoItemTNTStick;
			break;
		case kPicTNTPak:
			nType = kAmmoItemTNTBundle;
			break;
		case kPicSprayCan:
			nType = kAmmoItemSprayCan;
			break;
		case kPicShotShells:
			nType = kAmmoItemShells;
			break;
		case kPicTNTBox:
			nType = kAmmoItemTNTCase;
			break;
		case kPicTNTRemote:
			nType = kAmmoItemTNTRemote;
			break;
		case kPicTNTProx:
			nType = kAmmoItemTNTProximity;
			break;
		case kPicShellBox:
			nType = kAmmoItemShellBox;
			break;
		case kPicBullets:
			nType = kAmmoItemBullets;
			break;
		case kPicBulletBox:
			nType = kAmmoItemBulletBox;
			break;
		case kPicBulletBoxAP:
			nType = kAmmoItemAPBullets;
			break;
		case kPicTommyDrum:
			nType = kAmmoItemTommyDrum;
			break;
		case kPicFlareHE:
			nType = kAmmoItemHEFlares;
			break;
		case kPicFlareBurst:
			nType = kAmmoItemStarFlares;
			break;
		case kPicSpearExplode:
			nType = kAmmoItemHESpears;
			break;
		case kPicFlares:
			nType = kAmmoItemFlares;
			break;
		case kPicRandomUp:
			nType = kAmmoItemRandom;
			break;
		default:
			scrSetMessage("Invalid ammo tile selected!");
			BeepError();
			return FALSE;
	}

	int nSprite = insertsprite( (short)nSector, kStatItem );
	dassert( nSprite >= 0 && nSprite < kMaxSprites );
	sprite[nSprite].type = nType;

	AutoAdjustSprites();

	SPRITE *pSprite = &sprite[nSprite];
	pSprite->x = x;
	pSprite->y = y;
	pSprite->z = z;
	pSprite->ang = (short)nAngle;
	pSprite->shade = -8;	// gAmmoInfo[nAmmoType].shade;

	int zTop, zBot;
	GetSpriteExtents(pSprite, &zTop, &zBot);

	if ( where == SS_FLOOR)
		pSprite->z += getflorzofslope((short)nSector, pSprite->x, pSprite->y) - zBot;
	else
		pSprite->z += getceilzofslope((short)nSector, pSprite->x, pSprite->y) - zTop;

	updatenumsprites();

	BeepOkay();

	return TRUE;
}


/*******************************************************************************
	FUNCTION:		InsertWeapon()

	DESCRIPTION:	Displays a tile menu allowing user to insert a weapon
					on the floor.

	PARAMETERS:

	NOTES:			This routine should offer a list box rather than tiles.
*******************************************************************************/
static BOOL InsertWeapon( int where, int nSector, long x, long y, long z, int nAngle )
{
	static short weaponTiles[] =
	{
		kPicRandomUp, kPicShotgun, kPicTommyGun, kPicFlareGun, kPicVoodooDoll, kPicSpearGun, kPicShadowGun
	};

	short nType;

	tileIndexCount = LENGTH(weaponTiles);
	for ( int i = 0; i < tileIndexCount; i++ )
		tileIndex[i] =  weaponTiles[i]; // gWeaponInfo[i].picnum;

	short picnum = tilePick( -1 /*gWeaponInfo[0].picnum*/, -1, SS_CUSTOM );
	if ( picnum == -1 )
		return FALSE;

	switch( picnum )
	{
		case kPicRandomUp:
			nType = kWeaponItemRandom;
			break;
		case kPicShotgun:
			nType = kWeaponItemShotgun;
			break;
		case kPicTommyGun:
			nType = kWeaponItemTommyGun;
			break;
		case kPicFlareGun:
			nType = kWeaponItemFlareGun;
			break;
		case kPicVoodooDoll:
			nType = kWeaponItemVoodooDoll;
			break;
		case kPicSpearGun:
			nType = kWeaponItemSpearGun;
			break;
		case kPicShadowGun:
			nType = kWeaponItemShadowGun;
			break;
		default:
			scrSetMessage("Invalid weapon tile selected!");
			BeepError();
			return FALSE;
	}

	int nSprite = insertsprite( (short)nSector, kStatItem );
	dassert( nSprite >= 0 && nSprite < kMaxSprites );
	sprite[nSprite].type = nType;

	AutoAdjustSprites();

	SPRITE *pSprite = &sprite[nSprite];
	pSprite->x = x;
	pSprite->y = y;
	pSprite->z = z;
	pSprite->ang = (short)nAngle;
	pSprite->shade = -8;

	int zTop, zBot;
	GetSpriteExtents(pSprite, &zTop, &zBot);

	if ( where == SS_FLOOR)
		pSprite->z += getflorzofslope((short)nSector, pSprite->x, pSprite->y) - zBot;
	else
		pSprite->z += getceilzofslope((short)nSector, pSprite->x, pSprite->y) - zTop;

	updatenumsprites();

	BeepOkay();

	return TRUE;
}


/*******************************************************************************
	FUNCTION:		InsertEnemy()

	DESCRIPTION:	Displays a tile menu allowing user to insert an enemy
					on the ceiling or floor.

	PARAMETERS:

	NOTES:			This routine should offer a list box rather than tiles.
					At present, only base enemies may be added, but may be
					"extended" in 2D mode to other types in the same class.
*******************************************************************************/
static BOOL InsertEnemy( int where, int nSector, long x, long y, long z, int nAngle )
{
	static short dudeTiles[] =
	{
		kAnmCultistM1, kPicShotgunIdle, kAnmZomb1M1, kPicEarthIdle, kAnmZomb2M1, kAnmGargoyleM1, kAnmGargStatue,
		kAnmPhantasmM1, kAnmHellM2, kAnmHandM1, kAnmSpiderM1, kAnmSpiderM1B, kAnmSpiderM1C, kAnmSpiderM1D,
		kAnmGillM1, kAnmEelM1, kAnmBatM1, kAnmRatM1, kAnmPodM1, kAnmTentacleM1,
		kAnmCerberusM1, kPicRandomUp, kAnmTchernobogM1, kAnmRachelM1,
	};

	short nType;

	tileIndexCount = LENGTH(dudeTiles);
	for ( int i = 0; i < tileIndexCount; i++ )
		tileIndex[i] = dudeTiles[i];

	short picnum = tilePick( -1, -1, SS_CUSTOM );
	if ( picnum == -1 )
		return FALSE;

	switch( picnum )
	{
		case kAnmCultistM1:
			nType = kDudeTommyCultist;
			break;
		case kPicShotgunIdle:
			nType = kDudeShotgunCultist;
			break;
		case kAnmZomb1M1:
			nType = kDudeAxeZombie;
			break;
		case kPicEarthIdle:
			nType = kDudeEarthZombie;
			break;
		case kAnmZomb2M1:
			nType = kDudeFatZombie;
			break;
		case kAnmGargoyleM1:
			nType = kDudeFleshGargoyle;
			break;
		case kAnmGargStatue:
			nType = kDudeFleshStatue;
			break;
		case kAnmPhantasmM1:
			nType = kDudePhantasm;
			break;
		case kAnmHellM2:
			nType = kDudeHound;
			break;
		case kAnmHandM1:
			nType = kDudeHand;
			break;
		case kAnmSpiderM1:
			nType = kDudeBrownSpider;
			break;
		case kAnmSpiderM1B:
			nType = kDudeRedSpider;
			break;
		case kAnmSpiderM1C:
			nType = kDudeBlackSpider;
			break;
		case kAnmSpiderM1D:
			nType = kDudeMotherSpider;
			break;
		case kAnmGillM1:
			nType = kDudeGillBeast;
			break;
		case kAnmEelM1:
			nType = kDudeEel;
			break;
		case kAnmBatM1:
			nType = kDudeBat;
			break;
		case kAnmRatM1:
			nType = kDudeRat;
			break;
		case kAnmPodM1:
			nType = kDudeGreenPod;
			break;
		case kAnmTentacleM1:
			nType = kDudeGreenTentacle;
			break;
		case kAnmCerberusM1:
			nType = kDudeCerberus;
			break;
		case kPicRandomUp:
			nType = kDudeRandom;
			break;
		case kAnmTchernobogM1:
			nType = kDudeTchernobog;
			break;
		case kAnmRachelM1:
			nType = kDudeRachel;
			break;
		default:
			scrSetMessage("Invalid dude tile selected!");
			BeepError();
			return FALSE;
	}

	int nSprite = insertsprite( (short)nSector, kStatDude );
	dassert( nSprite >= 0 && nSprite < kMaxSprites );
	sprite[nSprite].type = nType;

	AutoAdjustSprites();

	SPRITE *pSprite = &sprite[nSprite];
	pSprite->x = x;
	pSprite->y = y;
	pSprite->z = z;
	pSprite->ang = (short)nAngle;
	pSprite->shade = -8;

	int zTop, zBot;
	GetSpriteExtents(pSprite, &zTop, &zBot);

	if ( where == SS_FLOOR)
		pSprite->z += getflorzofslope((short)nSector, pSprite->x, pSprite->y) - zBot;
	else
		pSprite->z += getceilzofslope((short)nSector, pSprite->x, pSprite->y) - zTop;

	updatenumsprites();

	int nXSprite = GetXSprite(nSprite);
	dassert(nXSprite > 0 && nXSprite < kMaxXSprites);

	// handle any special setups here
	switch( nType )
	{
		case kDudeBrownSpider:
		case kDudeRedSpider:
		case kDudeBlackSpider:
		case kDudeMotherSpider:
			if (where == SS_CEILING && !(sector[nSector].ceilingstat & kSectorParallax) )
				pSprite->cstat |= kSpriteFlipY;  // invert ceiling spiders
			break;
		case kDudeBat:
			if ( where == SS_CEILING && !(sector[nSector].ceilingstat & kSectorParallax) )
				pSprite->picnum = kAnmBatM2;	// make picnum ceiling bat
			else
				xsprite[nXSprite].state = 1;	// they must be flying, better turn them on
			break;
	}

	BeepOkay();

	return TRUE;
}


// modal results for InsertGameObject
enum {
	mrEnemy = mrUser,
	mrWeapon,
	mrAmmo,
	mrItem,
	mrHazard,
	mrMisc,
};

/*******************************************************************************
	FUNCTION:		InsertGameObject()

	DESCRIPTION:	Displays a menu allowing user to insert a game-specific
					object. Only ornaments can be placed as wall sprites.

	PARAMETERS:

	NOTES:
*******************************************************************************/
static BOOL InsertGameObject( int where, int nSector, long x, long y, long z, int nAngle )
{
	Window dialog(0, 0, 80, 182, "Insert");

	TextButton *pbEnemy   = new TextButton( 4,   4, 60, 20,  "&Enemy",   (MODAL_RESULT)mrEnemy );
	TextButton *pbWeapon  = new TextButton( 4,  26, 60, 20,  "&Weapon",  (MODAL_RESULT)mrWeapon );
	TextButton *pbAmmo    = new TextButton( 4,  48, 60, 20,  "&Ammo",    (MODAL_RESULT)mrAmmo );
	TextButton *pbItem    = new TextButton( 4,  70, 60, 20,  "&Item",    (MODAL_RESULT)mrItem );
	TextButton *pbHazard  = new TextButton( 4,  92, 60, 20,  "&Hazard",  (MODAL_RESULT)mrHazard );
	TextButton *pbMisc    = new TextButton( 4, 114, 60, 20,  "&Misc",    (MODAL_RESULT)mrMisc );
	TextButton *pbCancel  = new TextButton( 4, 136, 60, 20,  "&Cancel",  (MODAL_RESULT)mrCancel );

	dialog.Insert(pbEnemy);
	dialog.Insert(pbWeapon);
	dialog.Insert(pbAmmo);
	dialog.Insert(pbItem);
	dialog.Insert(pbHazard);
	dialog.Insert(pbMisc);
	dialog.Insert(pbCancel);

	ShowModal(&dialog);
	switch ( dialog.endState )
	{
		case mrEnemy:
			return InsertEnemy( where, nSector, x, y, z, (nAngle + kAngle180) & kAngleMask );

		case mrWeapon:
			return InsertWeapon( where, nSector, x, y, z, 0 );

		case mrAmmo:
			return InsertAmmo( where, nSector, x, y, z, 0 );

		case mrItem:
			return InsertItem( where, nSector, x, y, z, 0);

		case mrHazard:
			return InsertHazard( where, nSector, x, y, z, nAngle );

		case mrMisc:
			return InsertMisc( where, nSector, x, y, z, nAngle );

		case mrCancel:
			// pressed escape
			return FALSE;
	}

	return TRUE;
}


static BOOL CompareXSectors( XSECTOR *pXModel, XSECTOR *pXSector )
{
	return memcmp(pXModel, pXSector, sizeof(XSECTOR)) == 0;
}

static BOOL Confirm( char *zMessage )
{
	// create the dialog
	Window dialog(59, 80, 202, 46, zMessage);

	dialog.Insert(new TextButton(4, 4, 60,  20, "&Yes", mrYes));
	dialog.Insert(new TextButton(68, 4, 60, 20, "&No", mrNo));

	ShowModal(&dialog);

	return (dialog.endState == mrYes) ? TRUE : FALSE;
}

/*******************************************************************************
	FUNCTION:		OptionsMenu()

	DESCRIPTION:

	PARAMETERS:

	NOTES:
*******************************************************************************/
static BOOL OptionsMenu( void )
{
	Window dialog(0, 0, 80, 182, "Options");

	TextButton *pbClean		= new TextButton( 4,   4, 60, 20,  "C&lean",   (MODAL_RESULT)mrUser );
	TextButton *pbCancel	= new TextButton( 4, 136, 60, 20,  "&Cancel",  (MODAL_RESULT)mrCancel );

	dialog.Insert(pbClean);
	dialog.Insert(pbCancel);

	ShowModal(&dialog);
	switch ( dialog.endState )
	{
		case mrUser:
		{
			XSECTOR model;

			if (somethingintab == SS_FLOOR
			&& sector[searchsector].extra >= 0
			&& xsector[sector[searchsector].extra].reference == searchsector)
				memcpy(&model, &xsector[sector[searchsector].extra], sizeof(XSECTOR));

			int nCount = 1;
			for (int i = 0; i < kMaxXSectors; i++)
			{
				int nSector = xsector[i].reference;

				// compare with all other valid xsectors except self
				if ((nSector != -1) && sector[nSector].extra == i && nSector != searchsector)
				{
					// fake exceptions in model
					model.reference = xsector[i].reference;

					// compare the xsectors
					if ( CompareXSectors(&model, &xsector[i]) )
						nCount++;
				}
			}

			char message[40];
			sprintf(message, "Clean %i common sectors?", nCount);
			if (Confirm(message))
			{
				for (int i = 0; i < kMaxXSectors; i++)
				{
					int nSector = xsector[i].reference;

					// compare with all other valid xsectors
					if ((nSector != -1) && sector[nSector].extra == i)
					{
						// fake exceptions in model
						model.reference = xsector[i].reference;

						// compare the xsectors
						if ( CompareXSectors(&model, &xsector[i]) )
							dbDeleteXSector( i );
					}
				}
			}
			InitSectorFX();
			break;
		}

		case mrCancel:
			// pressed escape
			return FALSE;
	}

	return TRUE;
}


/***********************************************************************
 * SetupSky()
 *
 * Setup the sky based on the width of the first sky tile.
 * Called by ExtCheckKeys, when Alt-1 is pressed.
 *
 * Sky		Tile	1024	Tile	Engine
 * Width	Count	Shift	Shift	pskybits
 * -----------------------------------------
 * 1024		1       10		10		    0
 * 512		2       10		9		    1
 * 256		4       10		8		    2
 * 128		8       10		7		    3
 * 64		16      10		6		    4
 * 32		32      10		5			5
 * 16		64      10		4			6
 * 8		128     10		3			7
 **********************************************************************/
static void SetupSky( int nTile )
{
	dassert(nTile >= 0 && nTile < kMaxTiles);

	pskybits = (short)(10 - (picsiz[nTile] & 0x0F));
	gSkyCount = 1 << pskybits;
	sector[searchsector].ceilingpicnum = (short)nTile;

	dprintf("nTile = %i\n",nTile);
	dprintf("pskybits = %i\n",pskybits);
	dprintf("gSkyCount = %i\n",gSkyCount);

	// setup the sky tile offset table
	for (short i = 0; i < gSkyCount; i++)
	{
		pskyoff[i] = i;
		dprintf("pskyoff[%i] = %i\n",i,pskyoff[i]);
	}

	// reset the base tile for all the parallaxed skies (ceiling & floor!)
	for (i = 0; i < numsectors; i++)
	{
		if ( sector[i].ceilingstat & kSectorParallax )
			sector[i].ceilingpicnum = (short)nTile;
		if ( sector[i].floorstat & kSectorParallax )
			sector[i].floorpicnum = (short)nTile;
	}
}

static void PutSpriteOnCeiling( SPRITE *pSprite )
{
	int zTop, zBot;
	GetSpriteExtents(pSprite, &zTop, &zBot);
	pSprite->z += getceilzofslope(pSprite->sectnum, pSprite->x, pSprite->y) - zTop;

}


static void PutSpriteOnFloor( SPRITE *pSprite )
{
	int zTop, zBot;
	GetSpriteExtents(pSprite, &zTop, &zBot);
	pSprite->z += getflorzofslope(pSprite->sectnum, pSprite->x, pSprite->y) - zBot;
}


static void RaiseSprite( SPRITE *pSprite )
{
	pSprite->z = DecNext(pSprite->z, gStep);
}


static void LowerSprite( SPRITE *pSprite )
{
	pSprite->z = IncNext(pSprite->z, gStep);
}


static void ProcessHighlightSprites( HSPRITEFUNC SpriteFunc )
{
	if ( TestBitString(show2dsprite, searchwall) )	// highlighted?
	{
		for (int i = 0; i < highlightcnt; i++)
		{
			if ( (highlight[i] & 0xC000) == 0x4000 )
			{
				short nSprite = (short)(highlight[i] & 0x3FFF);
				SpriteFunc(&sprite[nSprite]);
			}
		}
	}
	else
		SpriteFunc(&sprite[searchwall]);
}


static void RaiseCeiling( short nSector)
{
	SetCeilingZ(nSector, DecNext(sector[nSector].ceilingz, gStep));
}


static void RaiseFloor( short nSector)
{
	SetFloorZ(nSector, DecNext(sector[nSector].floorz, gStep));
}


static void LowerCeiling( short nSector)
{
	SetCeilingZ(nSector, IncNext(sector[nSector].ceilingz, gStep));
}


static void LowerFloor( short nSector)
{
	SetFloorZ(nSector, IncNext(sector[nSector].floorz, gStep));
}


static void ProcessHighlightSectors( HSECTORFUNC FloorFunc )
{
	if ( IsSectorHighlight(searchsector) )
	{
		for (short i = 0; i < highlightsectorcnt; i++)
			FloorFunc(highlightsector[i]);
	}
	else
		FloorFunc(searchsector);
}


/*******************************************************************************
	FUNCTION:		ProcessKeys3D()

	DESCRIPTION:

	PARAMETERS:

	NOTES:
*******************************************************************************/
void ProcessKeys3D( void )
{
	int startwall, endwall;
	short nSector;
	long i, j, doubvel, changedir;
	long goalz, xvect, yvect, hiz, loz;
	short hitsect, hitwall, hitsprite;
	long hitx, hity, hitz, hihit, lohit;
	long x, y;
	int nXSector, nXWall, nXSprite;
	BYTE key;
	static short tabang;	// tawhat?
	static short temptype;
	int zTop, zBot;

	// these were globals not referenced elsewhere
	static long hvel = 0;

	BYTE shift, ctrl, alt, pad5;

	shift = keystatus[KEY_LSHIFT] | keystatus[KEY_RSHIFT];
	ctrl = keystatus[KEY_LCTRL] | keystatus[KEY_RCTRL];
	alt = keystatus[KEY_LALT] | keystatus[KEY_RALT];
	pad5 = keystatus[KEY_PAD5];

	if (ctrl && pad5)
		horiz = 100;

	if ( angvel )
	{
		doubvel = gFrameTicks;
		if ( shift )
			doubvel += gFrameTicks / 2;
		ang = (short)((ang + (angvel * doubvel >> 4)) & kAngleMask);
	}
	if ( vel | svel )
	{
		doubvel = gFrameTicks;
		if ( shift )
			doubvel += gFrameTicks;
		xvect = 0, yvect = 0;
		if ( vel )
		{
			xvect += mulscale30(vel * doubvel >> 2, Cos(ang));
			yvect += mulscale30(vel * doubvel >> 2, Sin(ang));
		}
		if (svel != 0)
		{
			xvect += mulscale30(svel * doubvel >> 2, Sin(ang));
			yvect -= mulscale30(svel * doubvel >> 2, Cos(ang));
		}
		ClipMove(&posx, &posy, &posz, &cursectnum, xvect, yvect, kPlayerRadius, 4<<8, 4<<8, 0);
	}

	getzrange(posx, posy, posz, cursectnum, &hiz, &hihit, &loz, &lohit, kPlayerRadius, 0);

	// ceiling hit?
	if ( (hihit & kHitTypeMask) == kHitSector && (hihit & kHitIndexMask) == cursectnum)
	{
		if (sector[cursectnum].ceilingstat & kSectorParallax)
			hiz = 0x80000000;	// largest negative number
	}

	if ( keystatus[KEY_H] )
	{
		dprintf("kensplayerheight = %i, current height = %i\n", kensplayerheight, loz - posz);
		keystatus[KEY_H] = 0;
	}

	if (zmode == 0)
	{
		goalz = loz - kensplayerheight;   // playerheight pixels above floor
		if (goalz < hiz + (16<<8))   //ceiling&floor too close
			goalz = (loz + hiz) / 2;
		if ( keystatus[KEY_A] )
		{
			if ( ctrl )
			{
				if (horiz > 0) horiz -= 4;
			}
			else
			{
				goalz -= (16<<8);
				if ( shift )
					goalz -= (24<<8);
			}
		}
		if (keystatus[KEY_Z])                            //Z (stand low)
		{
			if ( ctrl )
			{
				if (horiz < 200) horiz += 4;
			}
			else
			{
				goalz += (12<<8);
				if ( shift )
					goalz += 12 << 8;
			}
		}

		if (goalz != posz)
		{
			if (posz < goalz) hvel += 32;
			if (posz > goalz) hvel = ((goalz-posz)>>3);

			posz += hvel;
			if (posz > loz-(4<<8)) posz = loz-(4<<8), hvel = 0;
			if (posz < hiz+(4<<8)) posz = hiz+(4<<8), hvel = 0;
		}
	}
	else
	{
		goalz = posz;
		if (keystatus[KEY_A])
		{
			if ( keystatus[KEY_LCTRL] )
			{
				if (horiz > 0) horiz -= 4;
			}
			else
			{
				if (zmode != 1)
					goalz -= (8<<8);
				else
				{
					zlock += (4<<8);
					keystatus[KEY_A] = 0;
				}
			}
		}
		if (keystatus[KEY_Z])                            //Z (stand low)
		{
			if ( keystatus[KEY_LCTRL] )
			{
				if (horiz < 200) horiz += 4;
			}
			else
			{
				if (zmode != 1)
					goalz += (8<<8);
				else if (zlock > 0)
				{
					zlock -= (4<<8);
					keystatus[KEY_Z] = 0;
				}
			}
		}

		if (goalz < hiz+(4<<8))
			goalz = hiz+(4<<8);
		if (goalz > loz-(4<<8))
			goalz = loz-(4<<8);
		if (zmode == 1)
			goalz = loz-zlock;

		if (goalz < hiz+(4<<8))   //ceiling&floor too close
			goalz = (loz + hiz) / 2;

		if (zmode == 1)
			posz = goalz;

		if (goalz != posz)
		{
			if (posz < goalz) hvel += 32;
			if (posz > goalz) hvel -= 32;

			posz += hvel;

			if (posz > loz-(4<<8)) posz = loz-(4<<8), hvel = 0;
			if (posz < hiz+(4<<8)) posz = hiz+(4<<8), hvel = 0;
		}
		else
			hvel = 0;
	}

	Mouse::Read(gFrameTicks);

	searchx = ClipRange(Mouse::X, 1, xdim - 2);
	searchy = ClipRange(Mouse::Y, 1, ydim - 2);

	searchit = 2;

	// keep search vars if left mouse button pressed
	if (searchstat >= 0 && (Mouse::buttons & 1) )
		searchit = 0;

	// draw mouse
	Video.SetColor(gStdColor[23 + mulscale30(8, Sin(gFrameClock * kAngle360 / kTimerRate))]);
	gfxHLine(searchy, searchx - 6, searchx - 2);
	gfxHLine(searchy, searchx + 2, searchx + 6);
	gfxVLine(searchx, searchy - 5, searchy - 2);
	gfxVLine(searchx, searchy + 2, searchy + 5);

	if (searchstat < 0)
		return;

	key = keyGet();

	switch ( key )
	{
		case KEY_PAGEUP:
			gStep = 0x400;
			if ( shift )
				gStep = 0x100;

			if (( searchstat == SS_WALL ) && (wall[searchwall].nextsector != -1) )
				TranslateWallToSector();

			if ( searchstat == SS_WALL )
				searchstat = SS_CEILING; // adjust ceilings when pointing at white walls

			if (ctrl)
			{
				switch (searchstat)
				{
					case SS_CEILING:
						{
							int nNeighbor = nextsectorneighborz( searchsector, sector[searchsector].ceilingz, -1, -1);
//							if (nNeighbor == -1)
//								nNeighbor = nextsectorneighborz( searchsector, sector[searchsector].ceilingz, -1, 1);
							if (nNeighbor != -1)
								SetCeilingZ( searchsector, sector[nNeighbor].ceilingz );
							sprintf(buffer, "sector[%i].ceilingz: %i", searchsector, sector[searchsector].ceilingz);
							scrSetMessage(buffer);
						}
						break;
					case SS_FLOOR:
						{
							int nNeighbor = nextsectorneighborz( searchsector, sector[searchsector].floorz, 1, -1);
//							if (nNeighbor == -1)
//								nNeighbor = nextsectorneighborz( searchsector, sector[searchsector].floorz, 1, 1);
							if (nNeighbor != -1)
								SetFloorZ( searchsector, sector[nNeighbor].floorz );
							sprintf(buffer, "sector[%i].floorz: %i", searchsector, sector[searchsector].floorz);
							scrSetMessage(buffer);
						}
						break;
					case SS_SPRITE:
						ProcessHighlightSprites(PutSpriteOnCeiling);
						sprintf(buffer, "sprite[%i].z: %i", searchwall, sprite[searchwall].z);
						scrSetMessage(buffer);
						break;
				}
			}
			else if (alt)
			{
				switch (searchstat)
				{
					case SS_CEILING:
					{
						int dzInPixels = (sector[searchsector].floorz - sector[searchsector].ceilingz) / 256;
						gStep = GetNumberBox("height off floor", dzInPixels, dzInPixels) * 256;
						SetCeilingZ(searchsector, sector[searchsector].floorz - gStep);
						sprintf(buffer, "sector[%i].ceilingz: %i", searchsector, sector[searchsector].ceilingz);
						scrSetMessage(buffer);
						break;
					}
					case SS_FLOOR:
					{
						int dzInPixels = (sector[searchsector].floorz - sector[searchsector].ceilingz) / 256;
						gStep = GetNumberBox("height off ceiling", dzInPixels, dzInPixels) * 256;
						SetFloorZ(searchsector, sector[searchsector].ceilingz + gStep);
						sprintf(buffer, "sector[%i].floorz: %i", searchsector, sector[searchsector].floorz);
						scrSetMessage(buffer);
						break;
					}
				}
			}
			else if (shift)
			{
				switch (searchstat)
				{
					case SS_CEILING:
						gStep = GetNumberBox("shift ceiling up", 128, 128) * 256;
						SetCeilingZ(searchsector, sector[searchsector].ceilingz - gStep);
						break;

					case SS_FLOOR:
						gStep = GetNumberBox("shift floor up", 128, 128) * 256;
						SetFloorZ(searchsector, sector[searchsector].floorz - gStep);
						break;
				}
			}
			else
			{
				switch (searchstat)
				{
					case SS_CEILING:
						ProcessHighlightSectors(RaiseCeiling);
						sprintf(buffer, "sector[%i].ceilingz: %i", searchsector, sector[searchsector].ceilingz);
						scrSetMessage(buffer);
						break;

					case SS_FLOOR:
						ProcessHighlightSectors(RaiseFloor);
						sprintf(buffer, "sector[%i].floorz: %i", searchsector, sector[searchsector].floorz);
						scrSetMessage(buffer);
						break;

					case SS_SPRITE:
						ProcessHighlightSprites(RaiseSprite);
						sprintf(buffer, "sprite[%i].z: %i", searchwall, sprite[searchwall].z);
						scrSetMessage(buffer);
						break;
				}
			}
			BeepOkay();
			break;

		case KEY_PAGEDN:
			gStep = 0x400;
			if ( shift )
				gStep = 0x100;

			if (( searchstat == SS_WALL ) && (wall[searchwall].nextsector != -1) )
				TranslateWallToSector();

			if ( searchstat == SS_WALL )
				searchstat = SS_CEILING; // adjust ceilings when pointing at white walls

			if (ctrl)
			{
				switch (searchstat)
				{
					case SS_CEILING:
						{
							int nNeighbor = nextsectorneighborz( searchsector, sector[searchsector].ceilingz, -1, 1);
//							if (nNeighbor == -1)
//								nNeighbor = nextsectorneighborz( searchsector, sector[searchsector].ceilingz, -1, -1);
							if (nNeighbor != -1)
								SetCeilingZ( searchsector, sector[nNeighbor].ceilingz );
							sprintf(buffer, "sector[%i].ceilingz: %i", searchsector, sector[searchsector].ceilingz);
							scrSetMessage(buffer);
						}
						break;
					case SS_FLOOR:
						{
							int nNeighbor = nextsectorneighborz( searchsector, sector[searchsector].floorz, 1, 1);
//							if (nNeighbor == -1)
//								nNeighbor = nextsectorneighborz( searchsector, sector[searchsector].floorz, 1, -1);
							if (nNeighbor != -1)
								SetFloorZ( searchsector, sector[nNeighbor].floorz );
							sprintf(buffer, "sector[%i].floorz: %i", searchsector, sector[searchsector].floorz);
							scrSetMessage(buffer);
						}
						break;
					case SS_SPRITE:
						ProcessHighlightSprites(PutSpriteOnFloor);
						sprintf(buffer, "sprite[%i].z: %i", searchwall, sprite[searchwall].z);
						scrSetMessage(buffer);
						break;
				}
			}
			else if (alt)
			{
				switch (searchstat)
				{
					case SS_CEILING:
					{
						int dzInPixels = (sector[searchsector].floorz - sector[searchsector].ceilingz) / 256;
						gStep = GetNumberBox("height off floor", dzInPixels, dzInPixels) * 256;
						SetCeilingZ(searchsector, sector[searchsector].floorz - gStep);
						sprintf(buffer, "sector[%i].ceilingz: %i", searchsector, sector[searchsector].ceilingz);
						scrSetMessage(buffer);
						break;
					}
					case SS_FLOOR:
					{
						int dzInPixels = (sector[searchsector].floorz - sector[searchsector].ceilingz) / 256;
						gStep = GetNumberBox("height off ceiling", dzInPixels, dzInPixels) * 256;
						SetFloorZ(searchsector, sector[searchsector].ceilingz + gStep);
						sprintf(buffer, "sector[%i].floorz: %i", searchsector, sector[searchsector].floorz);
						scrSetMessage(buffer);
						break;
					}
				}
			}
			else if (shift)
			{
				switch (searchstat)
				{
					case SS_CEILING:
						gStep = GetNumberBox("shift ceiling down", 128, 128) * 256;
						SetCeilingZ(searchsector, sector[searchsector].ceilingz + gStep);
						break;

					case SS_FLOOR:
						gStep = GetNumberBox("shift floor down", 128, 128) * 256;
						SetFloorZ(searchsector, sector[searchsector].floorz + gStep);
						break;
				}
			}
			else
			{
				switch (searchstat)
				{
					case SS_CEILING:
						ProcessHighlightSectors(LowerCeiling);
						sprintf(buffer, "sector[%i].ceilingz: %i", searchsector, sector[searchsector].ceilingz);
						scrSetMessage(buffer);
						break;

					case SS_FLOOR:
						ProcessHighlightSectors(LowerFloor);
						sprintf(buffer, "sector[%i].floorz: %i", searchsector, sector[searchsector].floorz);
						scrSetMessage(buffer);
						break;

					case SS_SPRITE:
						ProcessHighlightSprites(LowerSprite);
						sprintf(buffer, "sprite[%i].z: %i", searchwall, sprite[searchwall].z);
						scrSetMessage(buffer);
						break;
				}
			}
			BeepOkay();
			break;

		case KEY_DELETE:
			if (searchstat == 3)
			{
				deletesprite(searchwall);
				updatenumsprites();
				BeepOkay();
			}
			else
				BeepError();
			break;

		case KEY_TAB:
			switch (searchstat)
			{
				case SS_WALL:
					dprintf("Wall %i copied\n", searchwall);
					temppicnum = wall[searchwall].picnum;
					tempshade = wall[searchwall].shade;
					temppal = wall[searchwall].pal;
					tempxrepeat = wall[searchwall].xrepeat;
					tempyrepeat = wall[searchwall].yrepeat;
					tempcstat = wall[searchwall].cstat;
					tempextra = wall[searchwall].extra;
					temptype = wall[searchwall].type;
					break;

				case SS_CEILING:
					dprintf("Ceiling %i copied\n", searchsector);
					temppicnum = sector[searchsector].ceilingpicnum;
					tempshade = sector[searchsector].ceilingshade;
					temppal = sector[searchsector].ceilingpal;
					tempxrepeat = sector[searchsector].ceilingxpanning;
					tempyrepeat = sector[searchsector].ceilingypanning;
					tempcstat = sector[searchsector].ceilingstat;
					tempextra = sector[searchsector].extra;
					tempvisibility = sector[searchsector].visibility;
					temptype = sector[searchsector].type;
					break;

				case SS_FLOOR:
					dprintf("Floor %i copied\n", searchsector);
					temppicnum = sector[searchsector].floorpicnum;
					tempshade = sector[searchsector].floorshade;
					temppal = sector[searchsector].floorpal;
					tempxrepeat = sector[searchsector].floorxpanning;
					tempyrepeat = sector[searchsector].floorypanning;
					tempcstat = sector[searchsector].floorstat;
					tempextra = sector[searchsector].extra;
					tempvisibility = sector[searchsector].visibility;
					temptype = sector[searchsector].type;
					break;

				case SS_SPRITE:
					dprintf("Sprite %i copied\n", searchwall);
					temppicnum = sprite[searchwall].picnum;
					tempshade = sprite[searchwall].shade;
					temppal = sprite[searchwall].pal;
					tempxrepeat = sprite[searchwall].xrepeat;
					tempyrepeat = sprite[searchwall].yrepeat;
					tempcstat = sprite[searchwall].cstat;
					tempextra = sprite[searchwall].extra;
					tabang = sprite[searchwall].ang;
					temptype = sprite[searchwall].type;
					break;

				case SS_MASKED:
					dprintf("Masked wall %i copied\n", searchwall);
					temppicnum = wall[searchwall].overpicnum;
					tempshade = wall[searchwall].shade;
					temppal = wall[searchwall].pal;
					tempxrepeat = wall[searchwall].xrepeat;
					tempyrepeat = wall[searchwall].yrepeat;
					tempcstat = wall[searchwall].cstat;
					tempextra = wall[searchwall].extra;
					temptype = wall[searchwall].type;
					break;
			}
			somethingintab = (char)searchstat;
			break;

		case KEY_CAPSLOCK:
			zmode = IncRotate(zmode, 3);
			if (zmode == 1)
				zlock = (loz-posz) & ~0x3FF;
			sprintf(buffer, "ZMode = %s", gZModeMsg[zmode]);
			scrSetMessage(buffer);
			break;

		case KEY_ENTER:
			if (somethingintab == 255)		// must have something to paste
			{
				BeepError();
				break;
			}

			if ( ctrl )
			{
				switch (searchstat)
				{
					// Paste to every wall in loop
					case SS_WALL:
					case SS_MASKED:
						i = searchwall;
						do
						{
							if ( shift )
							{
								wall[i].shade = tempshade;
								wall[i].pal = temppal;
							}
							else
							{
								wall[i].picnum = temppicnum;

								if (somethingintab == SS_WALL || somethingintab == SS_MASKED)
								{
									wall[i].xrepeat = tempxrepeat;
									wall[i].yrepeat = tempyrepeat;
									wall[i].cstat = tempcstat;

								}
								fixrepeats((short)i);
							}
							i = wall[i].point2;
						}
						while (i != searchwall);
						BeepOkay();
						break;

					// paste to all parallax ceilings
					case SS_CEILING:
						for (i = 0; i < numsectors; i++)
						{
							if ( sector[i].ceilingstat & kSectorParallax )
							{
								sector[i].ceilingpicnum = temppicnum;
								sector[i].ceilingshade = tempshade;
								sector[i].ceilingpal = temppal;
								if (somethingintab == SS_CEILING || somethingintab == SS_FLOOR)
								{
									sector[i].ceilingxpanning = tempxrepeat;
									sector[i].ceilingypanning = tempyrepeat;
									sector[i].ceilingstat = (uchar)(tempcstat | kSectorParallax);
								}
							}
						}
						BeepOkay();
						break;

					// paste to all parallax floors
					case SS_FLOOR:
						for (i = 0; i < numsectors; i++)
						{
							if ( sector[i].floorstat & kSectorParallax )
							{
								sector[i].floorpicnum = temppicnum;
								sector[i].floorshade = tempshade;
								sector[i].floorpal = temppal;
								if (somethingintab == SS_CEILING || somethingintab == SS_FLOOR)
								{
									sector[i].floorxpanning = tempxrepeat;
									sector[i].floorypanning = tempyrepeat;
									sector[i].floorstat = (uchar)(tempcstat | kSectorParallax);
								}
							}
						}
						BeepOkay();
						break;

					default:
						BeepError();
						break;
				}
				break;
			}

			if ( shift )	// paste shade and palette
			{
				switch (searchstat)
				{
					case SS_WALL:
						wall[searchwall].shade = tempshade;
						wall[searchwall].pal = temppal;
						break;

					case SS_CEILING:
						if ( IsSectorHighlight(searchsector) )
						{
							for(i = 0; i < highlightsectorcnt; i++)
							{
								sector[i].ceilingshade = tempshade;
								sector[i].ceilingpal = temppal;
								sector[i].visibility = tempvisibility;
							}
						}
						else
						{
							sector[searchsector].ceilingshade = tempshade;
							sector[searchsector].ceilingpal = temppal;
							sector[searchsector].visibility = tempvisibility;

							// if this sector is a parallaxed sky...
							if ( sector[searchsector].ceilingstat & kSectorParallax )
							{
								// propagate shade data on all parallaxed sky sectors
								for (i = 0; i < numsectors; i++)
									if (sector[i].ceilingstat & kSectorParallax)
									{
										sector[i].ceilingshade = tempshade;
										sector[i].ceilingpal = temppal;
										sector[i].visibility = tempvisibility;
									}
							}
						}
						break;

					case SS_FLOOR:
						if ( IsSectorHighlight(searchsector) )
						{
							for(i = 0; i < highlightsectorcnt; i++)
							{
								sector[i].floorshade = tempshade;
								sector[i].floorpal = temppal;
								sector[i].visibility = tempvisibility;
							}
						}
						else
						{
							sector[searchsector].floorshade = tempshade;
							sector[searchsector].floorpal = temppal;
							sector[searchsector].visibility = tempvisibility;
						}
						break;

					case SS_SPRITE:
						sprite[searchwall].shade = tempshade;
						sprite[searchwall].pal = temppal;
						break;

					case SS_MASKED:
						wall[searchwall].shade = tempshade;
						wall[searchwall].pal = temppal;
						break;
				}
				BeepOkay();
				break;
			}

			if ( alt )	// copy extra structures
			{
				switch (searchstat)
				{
					case SS_WALL:
					case SS_MASKED:
						if (somethingintab == SS_WALL || somethingintab == SS_MASKED)
						{
							wall[searchwall].extra = tempextra;
							wall[searchwall].type = temptype;
							CleanUp();
							BeepOkay();
						}
						else
							BeepError();
						break;

					case SS_CEILING:
					case SS_FLOOR:
						if (somethingintab == SS_CEILING || somethingintab == SS_FLOOR)
						{
							if ( IsSectorHighlight(searchsector) )
							{
								for(i = 0; i < highlightsectorcnt; i++)
								{
									sector[i].extra = tempextra;
									sector[i].type = temptype;
								}
							}
							else
							{
								sector[searchsector].extra = tempextra;
								sector[searchsector].type = temptype;
							}
							CleanUp();
							BeepOkay();
						}
						else
							BeepError();
						break;

					case SS_SPRITE:
						if (somethingintab == SS_SPRITE)
						{
							sprite[searchwall].type = temptype;
							sprite[searchwall].extra = tempextra;
							CleanUp();
							BeepOkay();
						}
						else
							BeepError();
						break;
				}
				break;
			}

			switch (searchstat)
			{
				case SS_WALL:
					wall[searchwall].picnum = temppicnum;
					wall[searchwall].shade = tempshade;
					wall[searchwall].pal = temppal;
					if (somethingintab == SS_WALL)
					{
						wall[searchwall].xrepeat = tempxrepeat;
						wall[searchwall].yrepeat = tempyrepeat;
						wall[searchwall].cstat = tempcstat;
					}
					fixrepeats(searchwall);
					break;

				case SS_CEILING:
					sector[searchsector].ceilingpicnum = temppicnum;
					sector[searchsector].ceilingshade = tempshade;
					sector[searchsector].ceilingpal = temppal;
					if (somethingintab == SS_CEILING || somethingintab == SS_FLOOR)
					{
						sector[searchsector].ceilingxpanning = tempxrepeat;
						sector[searchsector].ceilingypanning = tempyrepeat;
						sector[searchsector].ceilingstat = (uchar)tempcstat;
						sector[searchsector].visibility = tempvisibility;
					}
					break;

				case SS_FLOOR:
					sector[searchsector].floorpicnum = temppicnum;
					sector[searchsector].floorshade = tempshade;
					sector[searchsector].floorpal = temppal;
					if (somethingintab == SS_CEILING || somethingintab == SS_FLOOR)
					{
						sector[searchsector].floorxpanning= tempxrepeat;
						sector[searchsector].floorypanning= tempyrepeat;
						sector[searchsector].floorstat = (uchar)tempcstat;
						sector[searchsector].visibility = tempvisibility;
					}
					break;

				case SS_SPRITE:
					sprite[searchwall].picnum = temppicnum;
					sprite[searchwall].shade = tempshade;
					sprite[searchwall].pal = temppal;
					if (somethingintab == SS_SPRITE)
					{
						sprite[searchwall].xrepeat = tempxrepeat;
						sprite[searchwall].yrepeat = tempyrepeat;
						if (sprite[searchwall].xrepeat < 1) sprite[searchwall].xrepeat = 1;
						if (sprite[searchwall].yrepeat < 1) sprite[searchwall].yrepeat = 1;
						sprite[searchwall].cstat = tempcstat;
					}
					GetSpriteExtents(&sprite[searchwall], &zTop, &zBot);
					if ( !(sector[sprite[searchwall].sectnum].ceilingstat & kSectorParallax) )
						sprite[searchwall].z += ClipLow(sector[sprite[searchwall].sectnum].ceilingz - zTop, 0);
					if ( !(sector[sprite[searchwall].sectnum].floorstat & kSectorParallax) )
						sprite[searchwall].z += ClipHigh(sector[sprite[searchwall].sectnum].floorz - zBot, 0);
					break;

				case SS_MASKED:
					wall[searchwall].overpicnum = temppicnum;
					if (wall[searchwall].nextwall >= 0)
						wall[wall[searchwall].nextwall].overpicnum = temppicnum;
					wall[searchwall].shade = tempshade;
					wall[searchwall].pal = temppal;
					if (somethingintab == SS_MASKED)
					{
						wall[searchwall].xrepeat = tempxrepeat;
						wall[searchwall].yrepeat = tempyrepeat;
						wall[searchwall].cstat = tempcstat;
					}
					fixrepeats(searchwall);
					break;
			}
			BeepOkay();
			break;

		case KEY_LBRACE:
			gStep = 256;
			if ( shift )
				gStep = 32;

			switch (searchstat)
			{
				case SS_CEILING:
					if ( IsSectorHighlight(searchsector) )
					{
						for (i = 0; i < highlightsectorcnt; i++)
							SetCeilingSlope(highlightsector[i], (short)DecNext(sector[highlightsector[i]].ceilingslope, gStep));
						sprintf(buffer, "adjusted %i ceilings by %i", highlightsectorcnt, gStep);
						scrSetMessage(buffer);
					}
					else
					{
						SetCeilingSlope(searchsector, (short)DecNext(sector[searchsector].ceilingslope, gStep));
						sprintf(buffer, "sector[%i].ceilingslope: %i", searchsector, sector[searchsector].ceilingslope);
						scrSetMessage(buffer);
					}
					break;
				case SS_FLOOR:
					if ( IsSectorHighlight(searchsector) )
					{
						for(i = 0; i < highlightsectorcnt; i++)
							SetFloorSlope(highlightsector[i], (short)DecNext(sector[highlightsector[i]].floorslope, gStep));
						sprintf(buffer, "adjusted %i floors by %i", highlightsectorcnt, gStep);
						scrSetMessage(buffer);
					}
					else
					{
						SetFloorSlope(searchsector, (short)DecNext(sector[searchsector].floorslope, gStep));
						sprintf(buffer, "sector[%i].floorslope: %i", searchsector, sector[searchsector].floorslope);
						scrSetMessage(buffer);
					}
					break;
			}
			BeepOkay();
			break;

		case KEY_RBRACE:
			gStep = 256;
			if ( shift )
				gStep = 32;

			switch (searchstat)
			{
				case SS_CEILING:
					if ( IsSectorHighlight(searchsector) )
					{
						for (i = 0; i < highlightsectorcnt; i++)
							SetCeilingSlope(highlightsector[i], (short)IncNext(sector[highlightsector[i]].ceilingslope, gStep));
						sprintf(buffer, "adjusted %i ceilings by %i", highlightsectorcnt, gStep);
						scrSetMessage(buffer);
					}
					else
					{
						SetCeilingSlope(searchsector, (short)IncNext(sector[searchsector].ceilingslope, gStep));
						sprintf(buffer, "sector[%i].ceilingslope: %i", searchsector, sector[searchsector].ceilingslope);
						scrSetMessage(buffer);
					}
					break;
				case SS_FLOOR:
					if ( IsSectorHighlight(searchsector) )
					{
						for(i = 0; i < highlightsectorcnt; i++)
							SetFloorSlope(highlightsector[i], (short)IncNext(sector[highlightsector[i]].floorslope, gStep));
						sprintf(buffer, "adjusted %i floors by %i", highlightsectorcnt, gStep);
						scrSetMessage(buffer);
					}
					else
					{
						SetFloorSlope(searchsector, (short)IncNext(sector[searchsector].floorslope, gStep));
						sprintf(buffer, "sector[%i].floorslope: %i", searchsector, sector[searchsector].floorslope);
						scrSetMessage(buffer);
					}
					break;
			}
			BeepOkay();
			break;

		case KEY_COMMA:
			switch (searchstat)
			{
				// rotate sprite ccw
				case SS_SPRITE:
					gStep = shift ? 16 : 256;
					i = searchwall;
					sprite[i].ang = (short)(IncNext(sprite[i].ang, gStep) & kAngleMask);
					sprintf(buffer, "sprite[%i].ang: %i", searchwall, sprite[searchwall].ang);
					scrSetMessage(buffer);
					BeepOkay();
					break;

				default:
					BeepError();
			}
			break;

		case KEY_PERIOD:
			switch (searchstat)
			{
				// rotate sprite cw
				case SS_SPRITE:
					gStep = shift ? 16 : 256;
					i = searchwall;
					sprite[i].ang = (short)(DecNext(sprite[i].ang, gStep) & kAngleMask);
					sprintf(buffer, "sprite[%i].ang: %i", searchwall, sprite[searchwall].ang);
					scrSetMessage(buffer);
					BeepOkay();
					break;

				// search and fix panning to the right
				case SS_WALL:
				case SS_MASKED:
					AutoAlignWalls(searchwall);
					BeepOkay();
					break;

				default:
					BeepError();
			}
			break;

		case KEY_BACKSLASH:		// Reset slope to 0
			switch (searchstat)
			{
				case SS_CEILING:
					sector[searchsector].ceilingslope = 0;
					sector[searchsector].ceilingstat &= ~kSectorSloped;

					sprintf(buffer, "sector[%i] ceiling slope reset", searchsector);
					scrSetMessage(buffer);
					break;

				case SS_FLOOR:
					sector[searchsector].floorslope = 0;
					sector[searchsector].floorstat &= ~kSectorSloped;

					sprintf(buffer, "sector[%i] floor slope reset", searchsector);
					scrSetMessage(buffer);
					break;
			}
			BeepOkay();
			break;

		case KEY_SLASH:		// Reset panning, repeat, and flip
			switch (searchstat)
			{
				case SS_WALL:
				case SS_MASKED:
					if ( shift )
						wall[searchwall].xrepeat = wall[searchwall].yrepeat;
					else
					{
						wall[searchwall].xpanning = 0;
						wall[searchwall].ypanning = 0;
						wall[searchwall].xrepeat = 8;
						wall[searchwall].yrepeat = 8;
						wall[searchwall].cstat = 0;
					}
					fixrepeats((short)searchwall);
					sprintf(buffer, "wall[%i] pan/repeat reset", searchwall);
					scrSetMessage(buffer);
					break;

				case SS_CEILING:
					sector[searchsector].ceilingxpanning = 0;
					sector[searchsector].ceilingypanning = 0;
					sector[searchsector].ceilingstat &= ~kSectorFlipMask;
					sprintf(buffer, "sector[%i] ceiling pan reset", searchsector);
					scrSetMessage(buffer);
					break;

				case SS_FLOOR:
					sector[searchsector].floorxpanning = 0;
					sector[searchsector].floorypanning = 0;
					sector[searchsector].floorstat &= ~kSectorFlipMask;
					sprintf(buffer, "sector[%i] floor pan reset", searchsector);
					scrSetMessage(buffer);
					break;

				case SS_SPRITE:
					if ( shift )
						sprite[searchwall].xrepeat = sprite[searchwall].yrepeat;
					else
						sprite[searchwall].xrepeat = sprite[searchwall].yrepeat = 64;
					sprintf(buffer, "sprite[%i].xrepeat: %i yrepeat: %i", searchwall,
						sprite[searchwall].xrepeat, sprite[searchwall].yrepeat );
					scrSetMessage(buffer);
					break;
			}
			BeepOkay();
			break;

		case KEY_MINUS:
			// decrease sector visibility
			if (ctrl && alt)
			{
				if ( IsSectorHighlight(searchsector) )
				{
					for (i = 0; i < highlightsectorcnt; i++)
					{
						// higher numbers are less visibility
						sector[highlightsector[i]].visibility++;
					}
					scrSetMessage("highlighted sectors less visible");
				}
				else
				{
					// higher numbers are less visibility
					sector[searchsector].visibility++;
					sprintf(buffer, "Visibility: %i", sector[searchsector].visibility);
					scrSetMessage(buffer);
				}
				BeepOkay();
				break;
			}

			// decrease lighting effect amplitude
			if ( ctrl )
			{
				nXSector = GetXSector(searchsector);
				xsector[nXSector].amplitude++;
				sprintf(buffer, "Amplitude: %i", xsector[nXSector].amplitude);
				scrSetMessage(buffer);
				BeepOkay();
				break;
			}

			// decrease lighting effect phase
			if ( shift )
			{
				nXSector = GetXSector(searchsector);
				xsector[nXSector].phase -= 6;
				sprintf(buffer, "Phase: %i", xsector[nXSector].phase);
				scrSetMessage(buffer);
				BeepOkay();
				break;
			}

			// adjust gamma level
			if ( keystatus[KEY_G] )
			{
				gGamma = ClipLow(gGamma - 1, 0);
				BloodINI.PutKeyInt("View", "Gamma", gGamma);
				BloodINI.Save();
				sprintf(buffer, "Gamma correction level %i", gGamma);
				scrSetMessage(buffer);
				scrSetGamma(gGamma);
				scrSetDac(0);
				break;
			}

			if ( keystatus[KEY_D] )
			{
				visibility = ClipHigh(visibility + 16, 4096);
				sprintf(buffer, "Depth cueing level %i", visibility);
				scrSetMessage(buffer);
				BeepOkay();
				break;
			}
			break;

		case KEY_PLUS:
			// increase sector visibility
			if ( ctrl && alt )
			{
				if ( IsSectorHighlight(searchsector) )
				{
					for (i = 0; i < highlightsectorcnt; i++)
					{
						sector[highlightsector[i]].visibility--;
					}
					scrSetMessage("highlighted sectors more visible");
				}
				else
				{
					// lower numbers are more visibility
					sector[searchsector].visibility--;
					sprintf(buffer, "Visibility: %i", sector[searchsector].visibility);
					scrSetMessage(buffer);
				}
				BeepOkay();
				break;
			}

			// increase lighting effect amplitude
			if ( ctrl )
			{
				nXSector = GetXSector(searchsector);
				xsector[nXSector].amplitude--;
				sprintf(buffer, "Amplitude: %i", xsector[nXSector].amplitude);
				scrSetMessage(buffer);
				BeepOkay();
				break;
			}

			// increae lighting effect phase
			if ( shift )
			{
				nXSector = GetXSector(searchsector);
				xsector[nXSector].phase += 6;
				sprintf(buffer, "Phase: %i", xsector[nXSector].phase);
				scrSetMessage(buffer);
				BeepOkay();
				break;
			}

			// adjust gamma level
			if ( keystatus[KEY_G] )
			{
				gGamma = ClipHigh(gGamma + 1, gGammaLevels - 1);
				BloodINI.PutKeyInt("View", "Gamma", gGamma);
				BloodINI.Save();
				sprintf(buffer, "Gamma correction level %i", gGamma);
				scrSetMessage(buffer);
				scrSetGamma(gGamma);
				scrSetDac(0);
				break;
			}

			if ( keystatus[KEY_D] )
			{
				visibility = ClipLow(visibility - 16, 128);
				sprintf(buffer, "Depth cueing level %i", visibility);
				scrSetMessage(buffer);
				BeepOkay();
				break;
			}
			break;

		case KEY_B:		// B (clip Blocking xor) (3D)
			switch (searchstat)
			{
				case SS_WALL:
				case SS_MASKED:
					if ( TestBitString(show2dwall, searchwall) )	// highlighted?
					{
						for (int i = 0; i < highlightcnt; i++)
							if ( (highlight[i] & 0xC000) == 0 )
							{
								int nWall = highlight[ i ];
 								wall[ nWall ].cstat ^= kWallBlocking;
							}
					} else {
						wall[searchwall].cstat ^= kWallBlocking;
						if (wall[searchwall].nextwall >= 0)
						{
							wall[wall[searchwall].nextwall].cstat &= ~kWallBlocking;
							wall[wall[searchwall].nextwall].cstat |= (short)(wall[searchwall].cstat & kWallBlocking);
						}
					}
					sprintf(buffer, "Wall %i blocking flag is %s", searchwall,
						gBoolNames[ wall[searchwall].cstat & kWallBlocking  ? 1 : 0 ]);
					scrSetMessage(buffer);
					BeepOkay();
					break;

				case SS_SPRITE:
					sprite[searchwall].cstat ^= kSpriteBlocking;
					sprintf(buffer, "sprite[%i] %s blocking", searchwall,
						(sprite[searchwall].cstat & kSpriteBlocking) ? "is" : "not" );
					scrSetMessage(buffer);
					BeepOkay();
					break;

				default:
					BeepError();
			}
			break;

		case KEY_C:
			if (alt)	// change all tiles matching target to tab picnum
			{
				if (somethingintab != searchstat)
				{
					BeepError();
					break;
				}

				switch (searchstat)
				{
					case SS_WALL:
						j = wall[searchwall].picnum;
						if ( TestBitString(show2dwall, searchwall) )	// highlighted?
						{
							for (int i = 0; i < highlightcnt; i++)
								if ( (highlight[i] & 0xC000) == 0 )
								{
									int nWall = highlight[ i ];
									if (wall[nWall].picnum == j)
									{
										if (wall[nWall].picnum != temppicnum)
											wall[nWall].picnum = temppicnum;
										else if (wall[nWall].pal != temppal)
											wall[nWall].pal = temppal;
									}
								}
						} else {
							for ( i = 0; i < numwalls; i++)
								if (wall[i].picnum == j)
								{
									if (wall[i].picnum != temppicnum)
										wall[i].picnum = temppicnum;
									else if (wall[i].pal != temppal)
										wall[i].pal = temppal;
								}
						}
						break;

					case SS_CEILING:
						j = sector[searchsector].ceilingpicnum;
						for (i = 0; i < numsectors; i++)
							if (sector[i].ceilingpicnum == j)
							{
								if (sector[i].ceilingpicnum != temppicnum)
									sector[i].ceilingpicnum = temppicnum;
								else if (sector[i].ceilingpal != temppal)
									sector[i].ceilingpal = temppal;
							}
						break;

					case SS_FLOOR:
						j = sector[searchsector].floorpicnum;
						for (i = 0; i < numsectors; i++)
							if (sector[i].floorpicnum == j)
							{
								if (sector[i].floorpicnum != temppicnum)
									sector[i].floorpicnum = temppicnum;
								else if (sector[i].floorpal != temppal)
									sector[i].floorpal = temppal;
							}
						break;

					case SS_SPRITE:
						j = sprite[searchwall].picnum;
						for (i = 0; i < kMaxSprites; i++)
							if (sprite[i].statnum < kMaxStatus && sprite[i].picnum == j)
							{
								sprite[i].picnum = temppicnum;
								sprite[i].type = temptype;
							}
						break;

					case SS_MASKED:
						j = wall[searchwall].overpicnum;
						for (i = 0; i < numwalls; i++)
							if (wall[i].overpicnum == j)
							{
								wall[i].overpicnum = temppicnum;
							}
						break;
				}
				BeepOkay();
				break;
			}

/*
			switch (searchstat)
			{
				case SS_SPRITE:
					sprite[searchwall].cstat ^= kSpriteOriginAlign;
					BeepOkay();
					break;
			}
*/
			break;

		case KEY_E:		// E (expand)
			switch (searchstat)
			{
				case SS_CEILING:
					sector[searchsector].ceilingstat ^= kSectorExpand;
					sprintf(buffer, "ceiling texture %s expanded",
						(sector[searchsector].ceilingstat & kSectorExpand) ? "is" : "not");
					scrSetMessage(buffer);
					BeepOkay();
					break;

				case SS_FLOOR:
					sector[searchsector].floorstat ^= kSectorExpand;
					sprintf(buffer, "floor texture %s expanded",
						(sector[searchsector].floorstat & kSectorExpand) ? "is" : "not");
					scrSetMessage(buffer);
					BeepOkay();
					break;

				default:
					BeepError();
			}
			break;

		case KEY_F:
			if ( alt )
			{
				switch ( searchstat )
				{
					case SS_WALL:
					case SS_MASKED:
						nSector = sectorofwall(searchwall);
						sector[searchsector].ceilingstat &= ~kSectorFlipMask;
						sector[searchsector].ceilingstat |= kSectorRelAlign;
						SetFirstWall(nSector, searchwall);
						BeepOkay();
						break;

					case SS_CEILING:
						sector[searchsector].ceilingstat &= ~kSectorFlipMask;
						sector[searchsector].ceilingstat |= kSectorRelAlign;
						SetFirstWall(searchsector, sector[searchsector].wallptr + 1);
						BeepOkay();
						break;

					case SS_FLOOR:
						sector[searchsector].floorstat &= ~kSectorFlipMask;
						sector[searchsector].floorstat |= kSectorRelAlign;
						SetFirstWall(searchsector, sector[searchsector].wallptr + 1);
						BeepOkay();
						break;

					default:
						BeepError();
				}
				break;
			}

			switch (searchstat)
			{
				case SS_WALL:
				case SS_MASKED:

					i = wall[searchwall].cstat & kWallFlipMask;
					switch(i)
					{
						case 0: i = kWallFlipX; break;
						case kWallFlipX: i = kWallFlipX | kWallFlipY; break;
						case kWallFlipX | kWallFlipY: i = kWallFlipY; break;
						case kWallFlipY: i = 0; break;
					}
					wall[searchwall].cstat &= ~kWallFlipMask;
					wall[searchwall].cstat |= (short)i;
					sprintf(buffer, "wall[%i]",searchwall);
					if (wall[searchwall].cstat & kWallFlipX)
						strcat(buffer," x-flipped");
					if (wall[searchwall].cstat & kWallFlipY)
					{
						if (wall[searchwall].cstat & kWallFlipX)
							strcat(buffer," and");
						strcat(buffer," y-flipped");
					}
					scrSetMessage(buffer);
					BeepOkay();
					break;

				// 8-way ceiling flipping (bits 2,4,5)
				case SS_CEILING:
					i = sector[searchsector].ceilingstat & kSectorFlipMask;
					switch (i)
					{
						case 0x00: i = 0x10; break;
						case 0x10: i = 0x30; break;
						case 0x30: i = 0x20; break;
						case 0x20: i = 0x04; break;
						case 0x04: i = 0x14; break;
						case 0x14: i = 0x34; break;
						case 0x34: i = 0x24; break;
						case 0x24: i = 0x00; break;
					}
					sector[searchsector].ceilingstat &= ~kSectorFlipMask;
					sector[searchsector].ceilingstat |= (uchar)i;
 					break;

				// 8-way floor flipping (bits 2,4,5)
				case SS_FLOOR:
					i = sector[searchsector].floorstat & kSectorFlipMask;
					switch (i)
					{
						case 0x00: i = 0x10; break;
						case 0x10: i = 0x30; break;
						case 0x30: i = 0x20; break;
						case 0x20: i = 0x04; break;
						case 0x04: i = 0x14; break;
						case 0x14: i = 0x34; break;
						case 0x34: i = 0x24; break;
						case 0x24: i = 0x00; break;
					}
					sector[searchsector].floorstat &= ~kSectorFlipMask;
					sector[searchsector].floorstat |= (uchar)i;
					break;

				case SS_SPRITE:
					i = sprite[searchwall].cstat;

					// two-sided floor sprite?
					if ( (i & kSpriteRMask) == kSpriteFloor && !(i & kSpriteOneSided) )
					{
						// what the hell is this supposed to be doing?
						sprite[searchwall].cstat &= ~kSpriteFlipY;
						sprite[searchwall].cstat ^= kSpriteFlipX;
					}
					else
					{
						i = i & 0xC;
						switch(i)
						{
							case 0x0: i = 0x4; break;
							case 0x4: i = 0xC; break;
							case 0xC: i = 0x8; break;
							case 0x8: i = 0x0; break;
						}
						sprite[searchwall].cstat &= ~0xC;
						sprite[searchwall].cstat |= (short)i;
					}
					break;
			}
			BeepOkay();
			break;

		case KEY_G:
			if ( alt )
			{
				// change global palette
				gPalette = IncRotate(gPalette, kMaxPalettes);
				scrSetPalette(gPalette);
				scrSetDac(0);			// force update of DAC registers
				switch (gPalette)
				{
					case kPalNormal:
						scrSetMessage("Normal palette");
						break;
					case kPalWater:
						scrSetMessage("Water palette");
						break;
					case kPalBeast:
						scrSetMessage("Beast palette");
						break;
				}
			}
			break;

		case KEY_H:		// H (hitscan sensitivity)
			switch (searchstat)
			{
				case SS_WALL:
				case SS_MASKED:
					wall[searchwall].cstat ^= kWallHitscan;
					if (wall[searchwall].nextwall >= 0)
					{
						wall[wall[searchwall].nextwall].cstat &= ~kWallHitscan;
						wall[wall[searchwall].nextwall].cstat |= (short)(wall[searchwall].cstat & kWallHitscan);
					}
					sprintf(buffer, "wall[%i] %s hitscan sensitive", searchwall,
						(wall[searchwall].cstat & kWallHitscan) ? "is" : "not");
					scrSetMessage(buffer);
					BeepOkay();
					break;

				case SS_SPRITE:
					sprite[searchwall].cstat ^= kSpriteHitscan;
					sprintf(buffer, "sprite[%i] %s hitscan sensitive", searchwall,
						(sprite[searchwall].cstat & kSpriteHitscan) ? "is" : "not");
					scrSetMessage(buffer);
					BeepOkay();
					break;
				default:
					BeepError();
			}
			break;

		case KEY_L:

			switch (searchstat)
			{
				case SS_SPRITE:
				{
					GetSpriteExtents(&sprite[searchwall], &zTop, &zBot);
					ushort oldcstat = sprite[searchwall].cstat;
					sprite[searchwall].cstat &= ~kSpriteHitscan;
					SetupLightBomb();
					LightBomb(sprite[searchwall].x, sprite[searchwall].y, zTop,
						sprite[searchwall].sectnum);
					sprite[searchwall].cstat = oldcstat;
					BeepOkay();
					break;
				}

				case SS_FLOOR:
					if (sector[searchsector].ceilingstat & kSectorParallax)
					{
						sector[searchsector].floorstat ^= kSectorFloorShade;
						sprintf(buffer, "Forced floor shading is %s",
							gBoolNames[ sector[searchsector].floorstat & kSectorFloorShade  ? 1 : 0 ]);
						scrSetMessage(buffer);
					}
					else
						scrSetMessage("Sky must be parallaxed to force floorshade");
					break;

				default:
					BeepError();
			}
			break;

		case KEY_M:		// M (masking walls)
			switch (searchstat)
			{
				case SS_WALL:
				case SS_MASKED:
				case SS_CEILING:
				case SS_FLOOR:
					i = wall[searchwall].nextwall;

					if (i < 0)
					{
						BeepError();
						break;
					}

					wall[searchwall].cstat ^= kWallMasked;
					if (wall[searchwall].cstat & kWallMasked)
					{
						wall[searchwall].cstat &= ~kWallFlipX;
						if ( !shift )
						{
							wall[i].cstat |= kWallFlipX;           //auto other-side flip
							wall[i].cstat |= kWallMasked;
							if (wall[searchwall].overpicnum < 0)
								wall[searchwall].overpicnum = 0;
							wall[i].overpicnum = wall[searchwall].overpicnum;
						}
					}
					else
					{
						wall[searchwall].cstat &= ~kWallFlipX;
						if ( !shift )
						{
							wall[i].cstat &= ~kWallFlipX;         //auto other-side unflip
							wall[i].cstat &= ~kWallMasked;
						}
					}
					wall[searchwall].cstat &= ~kWallOneWay;
					if ( !shift )
						wall[i].cstat &= ~kWallOneWay;
					sprintf(buffer, "wall[%i] %s masked", searchwall,
						(wall[searchwall].cstat & kWallMasked) ? "is" : "not");
					scrSetMessage(buffer);
					BeepOkay();
					break;

				default:
					BeepError();
			}
			break;

		case KEY_O:
			if (alt)
			{
				asksave = OptionsMenu();
			}
			else
			{
				switch (searchstat)
				{
					// O (top/bottom orientation - for doors)
					case SS_WALL:
					case SS_MASKED:
						wall[searchwall].cstat ^= kWallBottomOrg;
						if ( wall[searchwall].nextwall == -1 )
							sprintf(buffer, "Texture pegged at %s", (wall[searchwall].cstat & kWallBottomOrg) ? "bottom" : "top");
						else
							sprintf(buffer, "Texture pegged at %s", (wall[searchwall].cstat & kWallOutsideOrg) ? "outside" : "inside");
						scrSetMessage(buffer);
						BeepOkay();
						break;

					// O (ornament onto wall)
					case SS_SPRITE:
					{
						if (sprite[searchwall].z > sector[sprite[searchwall].sectnum].floorz)
						{
							scrSetMessage("Sprite z is below floor");
							BeepError();
							break;
						}

						if (sprite[searchwall].z < sector[sprite[searchwall].sectnum].ceilingz)
						{
							scrSetMessage("Sprite z is above ceiling");
							BeepError();
							break;
						}

						HITINFO hitInfo;
						int hitType = HitScan(&sprite[searchwall], sprite[searchwall].z,
							Cos(sprite[searchwall].ang + kAngle180) >> 16,
							Sin(sprite[searchwall].ang + kAngle180) >> 16,
							0, &hitInfo);

						if ( hitType == SS_WALL || hitType == SS_MASKED )
						{
							int nx, ny;
							GetWallNormal(hitInfo.hitwall, &nx, &ny);
							sprite[searchwall].x = hitInfo.hitx + (nx >> 14);
							sprite[searchwall].y = hitInfo.hity + (ny >> 14);
							sprite[searchwall].z = hitInfo.hitz;
							sprite[searchwall].cstat &= ~kSpriteBlocking;
							sprite[searchwall].cstat |= kSpriteOneSided;
							changespritesect(searchwall, hitInfo.hitsect);
							sprite[searchwall].ang = (short)((GetWallAngle(hitInfo.hitwall) + kAngle90) & kAngleMask);
							dprintf("Sprite ornamented onto wall %d\n", hitInfo.hitwall);
							BeepOkay();
						}
						else
							BeepError();
						break;
					}

					default:
						BeepError();
				}
			}
			break;

		case KEY_P:
			if ( ctrl )
			{
				parallaxtype = (char)IncRotate(parallaxtype, 3);
				sprintf(buffer, "Parallax type: %i", parallaxtype);
				scrSetMessage(buffer);
				BeepOkay();
				break;
			}

			if ( alt )
			{
				switch (searchstat)
				{
					case SS_WALL:
					case SS_MASKED:
						wall[searchwall].pal = (char)GetNumberBox("Wall palookup", wall[searchwall].pal, wall[searchwall].pal);
						break;
					case SS_CEILING:
						sector[searchsector].ceilingpal = (char)GetNumberBox("Ceiling palookup", sector[searchsector].ceilingpal, sector[searchsector].ceilingpal);
						break;
					case SS_FLOOR:
						sector[searchsector].floorpal = (char)GetNumberBox("Floor palookup", sector[searchsector].floorpal, sector[searchsector].floorpal);
						break;
					case SS_SPRITE:
						sprite[searchwall].pal = (char)GetNumberBox("Sprite palookup", sprite[searchwall].pal, sprite[searchwall].pal);
						break;
				}
				BeepOkay();
				break;
			}

			switch (searchstat)
			{
				case SS_CEILING:
					sector[searchsector].ceilingstat ^= kSectorParallax;
					sprintf(buffer, "sector[%i] ceiling %s parallaxed", searchsector,
						(sector[searchsector].ceilingstat & kSectorParallax) ? "is" : "not");
					scrSetMessage(buffer);
					if ( sector[searchsector].ceilingstat & kSectorParallax )
						SetupSky(sector[searchsector].ceilingpicnum);
					else
						sector[searchsector].floorstat &= ~kSectorFloorShade;	// clear forced floor shading bit
					BeepOkay();
					break;

				case SS_FLOOR:
					sector[searchsector].floorstat ^= kSectorParallax;
					sprintf(buffer, "sector[%i] floor %s parallaxed", searchsector,
						(sector[searchsector].floorstat & kSectorParallax) ? "is" : "not");
					scrSetMessage(buffer);
					BeepOkay();
					break;

				default:
					BeepError();
			}
			break;

		case KEY_R:		// R (relative alignment, rotation)
			switch (searchstat)
			{
				case SS_CEILING:
					sector[searchsector].ceilingstat ^= kSectorRelAlign;
					sprintf(buffer, "sector[%i] ceiling %s relative", searchsector,
						(sector[searchsector].ceilingstat & kSectorRelAlign) ? "is" : "not");
					scrSetMessage(buffer);
					BeepOkay();
					break;

				case SS_FLOOR:
					sector[searchsector].floorstat ^= kSectorRelAlign;
					sprintf(buffer, "sector[%i] floor %s relative", searchsector,
						(sector[searchsector].floorstat & kSectorRelAlign) ? "is" : "not");
					scrSetMessage(buffer);
					BeepOkay();
					break;

				case SS_SPRITE:
					i = sprite[searchwall].cstat & kSpriteRMask;
					switch (i)
					{
						case 0x00:
							i = 0x10;
							sprintf( buffer, "sprite[%i] is wall sprite", searchwall );
							break;
						case 0x10:
							i = 0x20;
							sprintf( buffer, "sprite[%i] is floor sprite", searchwall );
							break;
						case 0x20:
							i = 0x00;
							sprintf( buffer, "sprite[%i] is face sprite", searchwall );
							break;
						default:
							i = 0x00;
							sprintf( buffer, "sprite[%i] is face sprite", searchwall );
							break;
					}
					scrSetMessage(buffer);
					sprite[searchwall].cstat &= ~kSpriteRMask;
					sprite[searchwall].cstat |= (ushort)i;
					BeepOkay();
					break;

				default:
					BeepError();
			}
			break;

		case KEY_S:
			if ( alt && (searchstat == SS_CEILING || searchstat == SS_FLOOR) )
			{
				x = 1 << 14;
				y = divscale(searchx - xdim / 2, xdim / 2, 14);

				RotateVector(&x, &y, ang);

				hitsect = hitwall = hitsprite = -1;
				hitscan(posx, posy, posz, cursectnum,        //Start position
					x, y, (searchy-horiz) * 2000,          //vector of 3D ang
					&hitsect, &hitwall, &hitsprite, &hitx, &hity, &hitz);

				if (hitsect >= 0)
				{
					x = hitx;
					y = hity;
					if ((gridlock > 0) && (grid > 0))
					{
						x = (x+(1024 >> grid)) & (0xffffffff<<(11-grid));
						y = (y+(1024 >> grid)) & (0xffffffff<<(11-grid));
					}
 					asksave = InsertGameObject( searchstat, hitsect, x, y, hitz, ang );
				}
				else
					BeepError();
				break;
			}
			switch (searchstat)
			{
				case SS_WALL:
				case SS_MASKED:
				{
					x = 16384;
					y = divscale(searchx - xdim / 2, xdim / 2, 14);
					RotateVector(&x, &y, ang);
					hitsect = hitwall = hitsprite = -1;
					hitscan(posx, posy, posz, cursectnum,        //Start position
						x, y, (searchy - horiz) * 2000,          //vector of 3D ang
						&hitsect, &hitwall, &hitsprite, &hitx, &hity, &hitz);

					// NOTE: Unfortunately this case short-circuits masked walls that are not hitscan sensitive.
					if (hitwall != searchwall)
						break;

					int nx, ny;
					short picnum;
					if (somethingintab != SS_SPRITE)
					{
						picnum = tilePick(-1, -1, SS_FLATSPRITE);
						if (picnum == -1)
							break;
					}

					i = insertsprite(searchsector, kStatDefault);
					sprite[i].ang = (short)((GetWallAngle( hitwall ) + kAngle90) & kAngleMask);
	 				GetWallNormal( hitwall, &nx, &ny );
					sprite[i].x = hitx + (nx >> 14);
					sprite[i].y = hity + (ny >> 14);
					sprite[i].z = hitz;

					if (somethingintab == SS_SPRITE)
					{
						sprite[i].picnum = temppicnum;
						sprite[i].shade = tempshade;
						sprite[i].pal = temppal;
						sprite[i].xrepeat = tempxrepeat;
						sprite[i].yrepeat = tempyrepeat;
						if (sprite[i].xrepeat < 1) sprite[i].xrepeat = 1;
						if (sprite[i].yrepeat < 1) sprite[i].yrepeat = 1;
						sprite[i].cstat = (short)(tempcstat | kSpriteWall | kSpriteOneSided);
					}
					else
					{
						sprite[i].cstat |= kSpriteWall | kSpriteOneSided;
						sprite[i].shade = -8;
						sprite[i].pal = 0;
						sprite[i].picnum = picnum;

						if (tilesizy[sprite[i].picnum] >= 32)
							sprite[i].cstat |= kSpriteBlocking;
					}

					updatenumsprites();
					BeepOkay();
					break;
				}

				case SS_CEILING:
				case SS_FLOOR:
					x = 1 << 14;
					y = divscale(searchx - xdim / 2, xdim / 2, 14);

					RotateVector(&x, &y, ang);

					hitsect = hitwall = hitsprite = -1;
					hitscan(posx, posy, posz, cursectnum,        //Start position
						x, y, (searchy-horiz) * 2000,          //vector of 3D ang
						&hitsect, &hitwall, &hitsprite, &hitx, &hity, &hitz);

					if (hitsect >= 0)
					{
						x = hitx;
						y = hity;
						if ((gridlock > 0) && (grid > 0))
						{
							x = (x+(1024 >> grid)) & (0xffffffff<<(11-grid));
							y = (y+(1024 >> grid)) & (0xffffffff<<(11-grid));
						}

						short picnum;
						if (somethingintab != SS_SPRITE)
						{
							picnum = tilePick(-1, -1, SS_SPRITE);
							if (picnum == -1)
								break;
						}

						i = insertsprite(hitsect, kStatDefault);
						sprite[i].x = x, sprite[i].y = y;

						if (somethingintab == SS_SPRITE)
						{
							sprite[i].picnum = temppicnum;
							sprite[i].shade = tempshade;
							sprite[i].pal = temppal;
							sprite[i].xrepeat = tempxrepeat;
							sprite[i].yrepeat = tempyrepeat;
							sprite[i].ang = tabang;
							if (sprite[i].xrepeat < 1) sprite[i].xrepeat = 1;
							if (sprite[i].yrepeat < 1) sprite[i].yrepeat = 1;
							sprite[i].cstat = tempcstat;
						}
						else
						{
							sprite[i].shade = -8;
							sprite[i].pal = 0;
							sprite[i].picnum = picnum;

							if (tilesizy[sprite[i].picnum] >= 32)
								sprite[i].cstat |= kSpriteBlocking;
						}

						GetSpriteExtents(&sprite[i], &zTop, &zBot);

						if ( searchstat == SS_FLOOR)
							sprite[i].z += getflorzofslope(hitsect, x, y) - zBot;
						else
							sprite[i].z += getceilzofslope(hitsect, x, y) - zTop;

						// fix sprite type attributes
						AutoAdjustSprites();

						updatenumsprites();
						BeepOkay();
					}
					else
						BeepError();
					break;

				default:
					BeepError();
			}
			break;

		case KEY_T:
			switch (searchstat)
			{
				case SS_MASKED:
				{
					int level = 0;
					if ( wall[searchwall].cstat & kWallTranslucent )
					{
						level = 1;
						if ( wall[searchwall].cstat & kWallTranslucentR )
							level = 2;
					}

					level = IncRotate(level, 3);

					switch ( level )
					{
						case 0:
							wall[searchwall].cstat &= ~kWallTranslucent & ~kWallTranslucentR;
							break;

						case 1:
							wall[searchwall].cstat &= ~kWallTranslucentR;
							wall[searchwall].cstat |= kWallTranslucent;
							break;

						case 2:
							wall[searchwall].cstat |= kWallTranslucent | kWallTranslucentR;
							break;
					}

					if (wall[searchwall].nextwall >= 0)
					{
						wall[wall[searchwall].nextwall].cstat &= ~kWallTranslucent & kWallTranslucentR;
						wall[wall[searchwall].nextwall].cstat |= (short)(wall[searchwall].cstat & (kWallTranslucent | kWallTranslucentR));
					}

					sprintf(buffer, "wall[%i] translucent type %d", searchsector, level);
					scrSetMessage(buffer);
					BeepOkay();
					break;
				}

				case SS_SPRITE:
				{
					int level = 0;
					if ( sprite[searchwall].cstat & kSpriteTranslucent )
					{
						level = 1;
						if ( sprite[searchwall].cstat & kSpriteTranslucentR )
							level = 2;
					}

					level = IncRotate(level, 3);

					switch ( level )
					{
						case 0:
							sprite[searchwall].cstat &= ~kSpriteTranslucent & ~kSpriteTranslucentR;
							break;

						case 1:
							sprite[searchwall].cstat &= ~kSpriteTranslucentR;
							sprite[searchwall].cstat |= kSpriteTranslucent;
							break;

						case 2:
							sprite[searchwall].cstat |= kSpriteTranslucent | kSpriteTranslucentR;
							break;
					}

					sprintf(buffer, "sprite[%i] translucent type %d", searchsector, level);
					scrSetMessage(buffer);
					BeepOkay();
					break;
				}

				default:
					BeepError();
			}
			break;

		case KEY_U:
			for (i = 0; i < numsectors; i++)
			{
				sector[i].visibility = 0;
			}
			scrSetMessage("All sector visibility values set to 0");
			break;

		case KEY_V:
			switch (searchstat)
			{
				case SS_WALL:
				{
					short oldPicnum = wall[searchwall].picnum;
					wall[searchwall].picnum =
						tilePick(wall[searchwall].picnum, wall[searchwall].picnum, SS_WALL);
					vel = svel = angvel = 0;
					if ( wall[searchwall].picnum != oldPicnum )
						BeepOkay();
					break;
				}

				case SS_CEILING:
				{
					short oldPicnum = sector[searchsector].ceilingpicnum;
					sector[searchsector].ceilingpicnum =
						tilePick(sector[searchsector].ceilingpicnum, sector[searchsector].ceilingpicnum, SS_CEILING);
					vel = svel = angvel = 0;
					if (sector[searchsector].ceilingstat & kSectorParallax)
						SetupSky( sector[searchsector].ceilingpicnum );
					else
						sector[searchsector].floorstat &= ~kSectorFloorShade;	// clear forced floor shading bit
					if ( sector[searchsector].ceilingpicnum != oldPicnum )
						BeepOkay();
					break;
				}

				case SS_FLOOR:
				{
					short oldPicnum = sector[searchsector].floorpicnum;
					sector[searchsector].floorpicnum =
						tilePick(sector[searchsector].floorpicnum, sector[searchsector].floorpicnum, SS_FLOOR);
					vel = svel = angvel = 0;
					if ( sector[searchsector].floorpicnum != oldPicnum )
						BeepOkay();
					break;
				}

				case SS_SPRITE:
				{
					if ( (sprite[searchwall].cstat & kSpriteRMask) != kSpriteFace )
						searchstat = SS_FLATSPRITE;
					int oldPicnum = sprite[searchwall].picnum;
					sprite[searchwall].picnum =
						tilePick(sprite[searchwall].picnum, sprite[searchwall].picnum, searchstat);
					GetSpriteExtents(&sprite[searchwall], &zTop, &zBot);
					if ( !(sector[sprite[searchwall].sectnum].ceilingstat & kSectorParallax) )
						sprite[searchwall].z += ClipLow(sector[sprite[searchwall].sectnum].ceilingz - zTop, 0);
					if ( !(sector[sprite[searchwall].sectnum].floorstat & kSectorParallax) )
						sprite[searchwall].z += ClipHigh(sector[sprite[searchwall].sectnum].floorz - zBot, 0);
					vel = svel = angvel = 0;
					if ( sprite[searchwall].picnum != oldPicnum )
						BeepOkay();
					break;
				}

				case SS_MASKED:
				{
					dprintf("wall[%d].overpicnum = %d\n",searchwall,wall[searchwall].overpicnum);
					short oldPicnum = wall[searchwall].overpicnum;
					wall[searchwall].overpicnum =
						tilePick(wall[searchwall].overpicnum, wall[searchwall].overpicnum, SS_MASKED);
					vel = svel = angvel = 0;
					if ( wall[searchwall].nextwall >= 0 )
						wall[wall[searchwall].nextwall].overpicnum = wall[searchwall].overpicnum;
					if ( wall[searchwall].overpicnum != oldPicnum )
						BeepOkay();
					break;
				}
			}
			break;

		case KEY_W:
			// change lighting waveform
			nXSector = GetXSector(searchsector);
			do {
				xsector[nXSector].wave = IncRotate(xsector[nXSector].wave, 15);
			} while ( gWaveNames[xsector[nXSector].wave] == NULL );
			scrSetMessage(gWaveNames[xsector[nXSector].wave]);
			break;

		case KEY_PADMINUS:
			gStep = ctrl ? 256 : 1;

			if ( IsSectorHighlight(searchsector) )
			{
				for (i = 0; i < highlightsectorcnt; i++)
				{
					nSector = highlightsector[i];

					sector[nSector].ceilingshade = (schar)ClipHigh(sector[nSector].ceilingshade + gStep, kPalLookups - 1);
					sector[nSector].floorshade = (schar)ClipHigh(sector[nSector].floorshade + gStep, kPalLookups - 1);

					startwall = sector[nSector].wallptr;
					endwall = startwall + sector[nSector].wallnum - 1;
					for (j = startwall; j <= endwall; j++)
						wall[j].shade = (schar)ClipHigh(wall[j].shade + gStep, kPalLookups - 1);
				}
			}
			else
			{
				signed char newshade;
				switch (searchstat)
				{
					case SS_WALL:
						newshade = wall[searchwall].shade = (schar)ClipHigh(wall[searchwall].shade + gStep, kPalLookups - 1);
						break;
					case SS_CEILING:
						newshade = sector[searchsector].ceilingshade = (schar)ClipHigh(sector[searchsector].ceilingshade + gStep, kPalLookups - 1);
						break;
					case SS_FLOOR:
						newshade = sector[searchsector].floorshade = (schar)ClipHigh(sector[searchsector].floorshade + gStep, kPalLookups - 1);
						break;
					case SS_SPRITE:
						newshade = sprite[searchwall].shade = (schar)ClipHigh(sprite[searchwall].shade + gStep, kPalLookups - 1);
						break;
					case SS_MASKED:
						newshade = wall[searchwall].shade = (schar)ClipHigh(wall[searchwall].shade + gStep, kPalLookups - 1);
						break;
				}
				sprintf(buffer, "shade: %i", newshade);
				scrSetMessage(buffer);
			}
			BeepOkay();
			break;

		case KEY_PADPLUS:
			gStep = ctrl ? 256 : 1;

			if ( IsSectorHighlight(searchsector) )
			{
				for( i = 0; i < highlightsectorcnt; i++)
				{
					nSector = highlightsector[i];

					sector[nSector].ceilingshade = (schar)ClipLow(sector[nSector].ceilingshade - gStep, -128);
					sector[nSector].floorshade = (schar)ClipLow(sector[nSector].floorshade - gStep, -128);

					startwall = sector[nSector].wallptr;
					endwall = startwall + sector[nSector].wallnum - 1;
					for(j=startwall;j<=endwall;j++)
						wall[j].shade = (schar)ClipLow(wall[j].shade - 1, -128);
				}
			}
			else
			{
				signed char newshade;
				switch (searchstat)
				{
					case SS_WALL:
						newshade = wall[searchwall].shade = (schar)ClipLow(wall[searchwall].shade - gStep, -128);
						break;
					case SS_CEILING:
						newshade = sector[searchsector].ceilingshade = (schar)ClipLow(sector[searchsector].ceilingshade - gStep, -128);
						break;
					case SS_FLOOR:
						newshade = sector[searchsector].floorshade = (schar)ClipLow(sector[searchsector].floorshade - gStep, -128);
						break;
					case SS_SPRITE:
						newshade = sprite[searchwall].shade = (schar)ClipLow(sprite[searchwall].shade - gStep, -128);
						break;
					case SS_MASKED:
						newshade = wall[searchwall].shade = (schar)ClipLow(wall[searchwall].shade - gStep, -128);
						break;
				}
				sprintf(buffer, "shade: %i", newshade);
				scrSetMessage(buffer);
			}
			BeepOkay();
			break;

		case KEY_PAD0:
			// set the shade of something to 0 brightness
			if ( IsSectorHighlight(searchsector) )
			{
				for( i = 0; i < highlightsectorcnt; i++)
				{
					nSector = highlightsector[i];

					sector[nSector].ceilingshade = 0;
					sector[nSector].floorshade = 0;

					startwall = sector[nSector].wallptr;   //wall shade
					endwall = startwall + sector[nSector].wallnum - 1;
					for(j=startwall;j<=endwall;j++)
						wall[j].shade = 0;
				}
			}
			else
			{
				switch (searchstat)
				{
					case SS_WALL:
						wall[searchwall].shade = 0; break;
					case SS_CEILING:
						sector[searchsector].ceilingshade = 0; break;
					case SS_FLOOR:
						sector[searchsector].floorshade = 0; break;
					case SS_SPRITE:
						sprite[searchwall].shade = 0; break;
					case SS_MASKED:
						wall[searchwall].shade = 0; break;
				}
			}
			BeepOkay();
			break;

		case KEY_PADLEFT:
		case KEY_PADRIGHT:
		{
			BOOL bCoarse = gOldKeyMapping ? pad5 : (BOOL)!shift;
			BOOL bPan = gOldKeyMapping ? shift : ctrl;

			changedir = (key == KEY_PADLEFT) ? 1 : -1;
			switch (searchstat)
			{
				case SS_WALL:
				case SS_MASKED:
					if ( bPan )
					{
						wall[searchwall].xpanning = changechar(wall[searchwall].xpanning, changedir, bCoarse, 0);
						sprintf(buffer, "wall %i xpanning: %i ypanning: %i", searchwall,
							wall[searchwall].xpanning, wall[searchwall].ypanning );
					}
					else
					{
						wall[searchwall].xrepeat = changechar(wall[searchwall].xrepeat, changedir, bCoarse, 1);
						sprintf(buffer, "wall %i xrepeat: %i yrepeat: %i", searchwall,
							wall[searchwall].xrepeat, wall[searchwall].yrepeat );
					}
					scrSetMessage(buffer);
					break;

				case SS_CEILING:
					sector[searchsector].ceilingxpanning = changechar(sector[searchsector].ceilingxpanning, changedir, bCoarse, 0);
					break;

				case SS_FLOOR:
					sector[searchsector].floorxpanning = changechar(sector[searchsector].floorxpanning, changedir, bCoarse, 0);
					break;

				case SS_SPRITE:
					if ( bPan )
					{
						sprite[searchwall].xoffset = changechar(sprite[searchwall].xoffset, changedir, bCoarse, 0);
						sprintf(buffer, "sprite %i xoffset: %i yoffset: %i", searchwall,
							sprite[searchwall].xoffset, sprite[searchwall].yoffset);
					}
					else
					{
						sprite[searchwall].xrepeat = (uchar)ClipLow(changechar(sprite[searchwall].xrepeat, -changedir, bCoarse, 1), 4);
						sprintf(buffer, "sprite %i xrepeat: %i yrepeat: %i", searchwall,
							sprite[searchwall].xrepeat, sprite[searchwall].yrepeat);
					}
					scrSetMessage(buffer);
					break;
			}
			BeepOkay();
			break;
		}

		case KEY_PADUP:
		case KEY_PADDOWN:
		{
			BOOL bCoarse = gOldKeyMapping ? pad5 : (BOOL)!shift;
			BOOL bPan = gOldKeyMapping ? shift : ctrl;

			changedir = (key == KEY_PADUP) ? -1 : 1;
			switch (searchstat)
			{
				case SS_WALL:
				case SS_MASKED:
					if ( bPan )
					{
						wall[searchwall].ypanning = changechar(wall[searchwall].ypanning, changedir, bCoarse, 0);
						sprintf(buffer, "wall %i xpanning: %i ypanning: %i", searchwall,
							wall[searchwall].xpanning, wall[searchwall].ypanning );
					}
					else
					{
						wall[searchwall].yrepeat = changechar(wall[searchwall].yrepeat, changedir, bCoarse, 1);
						sprintf(buffer, "wall %i xrepeat: %i yrepeat: %i", searchwall,
							wall[searchwall].xrepeat, wall[searchwall].yrepeat );
					}
					scrSetMessage(buffer);
					break;

				case SS_CEILING:
					sector[searchsector].ceilingypanning = changechar(sector[searchsector].ceilingypanning, changedir, bCoarse, 0);
					break;

				case SS_FLOOR:
					sector[searchsector].floorypanning = changechar(sector[searchsector].floorypanning, changedir, bCoarse, 0);
					break;

				case SS_SPRITE:
					if ( bPan )
					{
						sprite[searchwall].yoffset = changechar(sprite[searchwall].yoffset, changedir, bCoarse, 0);
						sprintf(buffer, "sprite %i xoffset: %i yoffset: %i", searchwall,
							sprite[searchwall].xoffset, sprite[searchwall].yoffset);
					}
					else
					{
						sprite[searchwall].yrepeat = (uchar)ClipLow(changechar(sprite[searchwall].yrepeat, -changedir, bCoarse, 1), 4);
						sprintf(buffer, "sprite %i xrepeat: %i yrepeat: %i", searchwall,
							sprite[searchwall].xrepeat, sprite[searchwall].yrepeat);
					}
					scrSetMessage(buffer);
					break;
			}
			BeepOkay();
			break;
		}

		case KEY_PADENTER:
			overheadeditor();
			clearview(0);
			scrNextPage();
			keyFlushStream();
			CleanUp();
			break;

	 	case KEY_F2:
			// toggle xstructure state
			switch (searchstat)
			{
				case SS_WALL:
				case SS_MASKED:
					nXWall = wall[searchwall].extra;
					if (nXWall > 0)
					{
						xwall[nXWall].state ^= 1;
						xwall[nXWall].busy = xwall[nXWall].state << 16;
						scrSetMessage(gBoolNames[xwall[nXWall].state]);
						BeepOkay();
					}
					else
						BeepError();
					break;

				case SS_CEILING:
				case SS_FLOOR:
					nXSector = sector[searchsector].extra;
					if (nXSector > 0)
					{
						xsector[nXSector].state ^= 1;
						xsector[nXSector].busy = xsector[nXSector].state << 16;
						scrSetMessage(gBoolNames[xsector[nXSector].state]);
						BeepOkay();
					}
					else
						BeepError();
					break;

				case SS_SPRITE:
					nXSprite = sprite[searchwall].extra;
					if (nXSprite > 0)
					{
						xsprite[nXSprite].state ^= 1;
						xsprite[nXSprite].busy = xsprite[nXSprite].state << 16;
						scrSetMessage(gBoolNames[xsprite[nXSprite].state]);
						BeepOkay();
					}
					else
						BeepError();
					break;
			}
			break;

	 	case KEY_F3:
			nSector = searchsector;
			if ( alt )
			{
				// capture off positions for sector ceiling and floor height
				switch (searchstat)
				{
					case SS_WALL:
						if (wall[searchwall].nextsector != -1)
							nSector = wall[searchwall].nextsector;
						// fall through to nXSector processing

					case SS_CEILING:
					case SS_FLOOR:
						nXSector = sector[nSector].extra;
						if ( nXSector > 0 )
						{
							xsector[nXSector].offFloorZ = sector[nSector].floorz;
							xsector[nXSector].offCeilZ = sector[nSector].ceilingz;
							sprintf( buffer, "SET offFloorZ= %i  offCeilZ= %i",
								xsector[nXSector].offFloorZ, xsector[nXSector].offCeilZ );
							scrSetMessage(buffer);
							BeepOkay();
							asksave = 1;
						} else {
							scrSetMessage("Sector type must first be set in 2D mode.");
							BeepError();
						}
						break;
				}
				break;

			}

			// set sector height to captured off positions
			switch (searchstat)
			{
				case SS_WALL:
					if (wall[searchwall].nextsector != -1)
						nSector = wall[searchwall].nextsector;
					// fall through to nXSector processing

				case SS_CEILING:
				case SS_FLOOR:
					nXSector = sector[nSector].extra;
					if ( nXSector > 0 )
					{
						switch( sector[nSector].type )
						{
							case kSectorZMotion:
							case kSectorZCrusher:
								sector[nSector].floorz = xsector[nXSector].offFloorZ;
								sector[nSector].ceilingz = xsector[nXSector].offCeilZ;
								sprintf( buffer, "SET floorz= %i  ceilingz= %i",
									sector[nSector].floorz, sector[nSector].ceilingz );
								scrSetMessage(buffer);
								BeepOkay();
								asksave = 1;
								break;
						}
					} else {
						scrSetMessage("Sector type must first be set in 2D mode.");
						BeepError();
					}
					break;
			}
			break;

	 	case KEY_F4:
			nSector = searchsector;
			if ( alt )
			{
				// capture on positions for sector ceiling and floor height
				switch (searchstat)
				{
					case SS_WALL:
						if (wall[searchwall].nextsector != -1)
							nSector = wall[searchwall].nextsector;
						// fall through to nXSector processing

					case SS_CEILING:
					case SS_FLOOR:
						nXSector = sector[nSector].extra;
						if ( nXSector > 0 )
						{
							xsector[nXSector].onFloorZ = sector[nSector].floorz;
							xsector[nXSector].onCeilZ = sector[nSector].ceilingz;
							sprintf( buffer, "SET onFloorZ= %i  onCeilZ= %i",
								xsector[nXSector].onFloorZ, xsector[nXSector].onCeilZ );
							scrSetMessage(buffer);
							BeepOkay();
							asksave = 1;
						} else {
							scrSetMessage("Sector type must first be set in 2D mode.");
							BeepError();
						}
						break;
				}
				break;
			}
			// set sector height to captured on positions
			switch (searchstat)
			{
				case SS_WALL:
					if (wall[searchwall].nextsector != -1)
						nSector = wall[searchwall].nextsector;
					// fall through to nXSector processing

				case SS_CEILING:
				case SS_FLOOR:
					nXSector = sector[nSector].extra;
					if ( nXSector > 0 )
					{
						switch( sector[nSector].type )
						{
							case kSectorZMotion:
							case kSectorZCrusher:
								sector[nSector].floorz = xsector[nXSector].onFloorZ;
								sector[nSector].ceilingz = xsector[nXSector].onCeilZ;
								sprintf( buffer, "SET floorz= %i  ceilingz= %i",
									sector[nSector].floorz, sector[nSector].ceilingz );
								scrSetMessage(buffer);
								BeepOkay();
								asksave = 1;
								break;
						}
					} else {
						scrSetMessage("Sector type must first be set in 2D mode.");
						BeepError();
					}
					break;
			}
			break;

		case KEY_F9:
			// create a set of stairs
			switch (searchstat)
			{
				case SS_FLOOR:
					// clear visited bits
					memset(visited, FALSE, sizeof(visited));

					sector[searchsector].floorstat &= ~0x80;

					gStairHeight = GetNumberBox("Step height", 8, 8);
					BuildStairsF(searchsector);
					break;

				case SS_CEILING:
					// clear visited bits
					memset(visited, FALSE, sizeof(visited));

					sector[searchsector].ceilingstat &= ~0x80;

					gStairHeight = GetNumberBox("Step height", 8, 8);
					BuildStairsC(searchsector);
					break;

			}
			break;

		case KEY_F11:
			gPanning = !gPanning;
			sprintf(buffer, "Global panning is %s", gBoolNames[gPanning]);
			scrSetMessage(buffer);
			BeepOkay();
			break;

		case KEY_F12:
			gBeep = !gBeep;
			sprintf(buffer, "Beeps are %s", gBoolNames[gBeep]);
			scrSetMessage(buffer);
			BeepOkay();
			break;

		case KEY_1:
			switch (searchstat)
			{
				case SS_WALL:
				case SS_MASKED:
					wall[searchwall].cstat ^= kWallOneWay;
					sprintf(buffer, "Wall %i one-way flag is %s", searchwall,
						gBoolNames[wall[searchwall].cstat & kWallOneWay ? 1 : 0 ]);
					scrSetMessage(buffer);
					BeepOkay();
					break;

				case SS_SPRITE:
					sprite[searchwall].cstat ^= kSpriteOneSided;
					i = sprite[searchwall].cstat;
					if ((i & kSpriteRMask) == kSpriteFloor)		// floor sprite
					{
						// use y-flip bit to indicate whether its visible from above or below
						sprite[searchwall].cstat &= ~kSpriteFlipY;
						if (i & kSpriteOneSided)
							if (posz > sprite[searchwall].z)
								sprite[searchwall].cstat |= kSpriteFlipY;
					}
					sprintf(buffer, "Sprite %i one-sided flag is %s", searchwall,
						gBoolNames[sprite[searchwall].cstat & kSpriteOneSided ? 1 : 0 ]);
					scrSetMessage(buffer);
					BeepOkay();
					break;

				default:
					BeepError();
			}
			break;

		case KEY_2:		// 2 (bottom wall swapping)
			switch (searchstat)
			{
				case SS_WALL:
				case SS_MASKED:
					wall[searchwall].cstat ^= kWallBottomSwap;
					sprintf(buffer, "Wall %i bottom swap flag is %s", searchwall,
						gBoolNames[wall[searchwall].cstat & kWallBottomSwap ? 1 : 0 ]);
					scrSetMessage(buffer);
					BeepOkay();
					break;

				default:
					BeepError();
			}
			break;

		case KEY_PRINTSCREEN:
			screencapture("captxxxx.pcx");
			BeepOkay();
			break;

	}

	// let's see if this fixes the input buffering problem
	if (key != 0)
		keyFlushStream();
}
