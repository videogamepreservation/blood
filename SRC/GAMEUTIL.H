/*******************************************************************************
	FILE:			GAMEUTIL.H

	DESCRIPTION:	This header file contains prototypes and inline
					declarations for utilities functions dealing with the
					BUILD engine.

	AUTHOR:			Peter M. Freese
	CREATED:		07-08-95
	COPYRIGHT:		Copyright (c) 1995 Q Studios Corporation
*******************************************************************************/

#ifndef __GAMEUTIL_H
#define __GAMEUTIL_H

#include "engine.h"
#include "typedefs.h"


struct POINT2D
{
	int x, y;
};


struct VECTOR2D
{
	int dx, dy;
};

struct POINT3D
{
	int x, y, z;
};


struct VECTOR3D
{
	long dx, dy, dz;
};


struct HITINFO {
	short	hitsect;
	short	hitwall;
	short	hitsprite;
	long	hitx;
	long	hity;
	long	hitz;
};


/***********************************************************************
 * Inline functions
 **********************************************************************/


// Euclidean 3D distance
inline int Dist3d(int dx, int dy, int dz)
{
	dx >>= 4;
	dy >>= 4;
	dz >>= 8;
	return ksqrt(dx * dx + dy * dy + dz * dz);
}


/***********************************************************************
 * GetSpriteExtents()
 *
 * Report the vertical size of a sprite in z units (works for all
 * sprite types)
 **********************************************************************/
inline void GetSpriteExtents( SPRITE *pSprite, int *zTop, int *zBot )
{
	*zTop = *zBot = pSprite->z;
	if ( (pSprite->cstat & kSpriteRMask) == kSpriteFloor )
		return;

	int nTile = pSprite->picnum;
	*zTop -= (picanm[nTile].ycenter + tilesizy[nTile] / 2) * (pSprite->yrepeat << 2);
	*zBot += (tilesizy[nTile] - (tilesizy[nTile] / 2 + picanm[nTile].ycenter)) * (pSprite->yrepeat << 2);
}


BOOL AreSectorsNeighbors(int sect1, int sect2);
BOOL FindSector(int x, int y, int z, short *nSector);
void CalcFrameRate( void );
BOOL CheckProximity( SPRITE *pSprite, int x, int y, int z, int nSector, int dist );
int GetWallAngle( int nWall );
void GetWallNormal( int nWall, int *nx, int *ny );
int HitScan( SPRITE *pSprite, int startZ, int dx, int dy, int dz, HITINFO *hitInfo );
int VectorScan( SPRITE *pSprite, int z, int dx, int dy, int dz, HITINFO *hitInfo );
void GetZRange( SPRITE *pSprite, long *ceilZ, long *ceilHit, long *floorZ, long *floorHit,
	int clipdist, char cliptype );
unsigned ClipMove( long *px, long *py, long *pz, short *pnSector, long dx, long dy,
	int wallDist, int ceilDist, int floorDist, char clipType );

#endif

