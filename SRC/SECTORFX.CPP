#include <stdlib.h>

#include "engine.h"

#include "sectorfx.h"
#include "globals.h"
#include "db.h"
#include "misc.h"
#include "trig.h"
#include "debug4g.h"

#include <memcheck.h>

#define kPanScale	8

short shadeList[kMaxXSectors];
short panList[kMaxXSectors];
int shadeCount = 0, panCount = 0;

short wallPanList[kMaxXSectors];
int wallPanCount = 0;

int gDepth[] =
{
	0,			// kDepthWalk
	15 << 8,	// kDepthTread
	30 << 8,	// kDepthWade
	40 << 8		// kDepthSwim
};

// monotonic flicker -- very doom like
static char flicker1[] = {
	0, 0, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0, 1, 0,
	1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 1,
	0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1,
	0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1,
};

// organic flicker -- good for torches
static char flicker2[] = {
	1, 2, 4, 2, 3, 4, 3, 2, 0, 0, 1, 2, 4, 3, 2, 0,
	2, 1, 0, 1, 0, 2, 3, 4, 3, 2, 1, 1, 2, 0, 0, 1,
	1, 2, 3, 4, 4, 3, 2, 1, 2, 3, 4, 4, 2, 1, 0, 1,
	0, 0, 0, 0, 1, 2, 3, 4, 3, 2, 1, 2, 3, 4, 3, 2,
};

// mostly on flicker -- good for flaky fluourescents
static char flicker3[] = {
	4, 4, 4, 4, 3, 4, 4, 4, 4, 4, 4, 2, 4, 3, 4, 4,
	4, 4, 2, 1, 3, 3, 3, 4, 3, 4, 4, 4, 4, 4, 2, 4,
	4, 4, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 2, 1, 0, 1,
	0, 1, 0, 1, 0, 2, 3, 4, 4, 4, 4, 4, 4, 4, 3, 4,
};

// not yet done (same as 2 for now)
static char flicker4[] = {
	1, 2, 4, 2, 3, 4, 3, 2, 0, 0, 1, 2, 4, 3, 2, 0,
	2, 1, 0, 1, 0, 2, 3, 4, 3, 2, 1, 1, 2, 0, 0, 1,
	1, 2, 3, 4, 4, 3, 2, 1, 2, 3, 4, 4, 2, 1, 0, 1,
	0, 0, 0, 0, 1, 2, 3, 4, 3, 2, 1, 2, 3, 4, 3, 2,
};

static char strobe[] = {
	64, 64, 64, 48, 36, 27, 20, 15, 11, 9, 6, 5, 4, 3, 2, 2,
	1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

static int GetWaveValue( int nWave, long time, int freq, int amplitude )
{
	switch (nWave)
	{
		case kWaveNone:
			return amplitude;

		case kWaveSquare:
			return ((time * freq >> 11) & 1) * amplitude;

		case kWaveSaw:
			return abs(0x80 - ((time * freq >> 4) & 0xFF)) * amplitude >> 7;

		case kWaveRampup:
			return ((time * freq >> 4) & 0xFF) * amplitude >> 8;

		case kWaveRampdown:
			return (0xFF - ((time * freq >> 4) & 0xFF)) * amplitude >> 8;

		case kWaveSine:
			return (amplitude + mulscale30(amplitude, Sin(time * freq >> 1))) >> 1;

		case kWaveFlicker1:
			return flicker1[(time * freq >> 6) & 0x3F] * amplitude;

		case kWaveFlicker2:
			return flicker2[(time * freq >> 6) & 0x3F] * amplitude >> 2;

		case kWaveFlicker3:
			return flicker3[(time * freq >> 6) & 0x3F] * amplitude >> 2;

		case kWaveFlicker4:
			return flicker4[(time * freq >> 6) & 0x3F] * amplitude >> 2;

		case kWaveStrobe:
			return strobe[(time * freq >> 6) & 0x3F] * amplitude >> 6;

		case kWaveSearch:
		{
			int phi = ((time * freq >> 1) & kAngleMask) << 2;
			if (phi > kAngle360)
				return 0;
			return (amplitude - mulscale30(amplitude, Cos(phi))) >> 1;
		}
	}
	return 0;
};


void DoSectorLighting( void )
{
	int i, nXSector, nSector;
	int nWave, freq, amplitude, value;
	int nWall, startwall, endwall;

	for (i = 0; i < shadeCount; i++)
	{
		nXSector = shadeList[i];
		XSECTOR *pXSector = &xsector[nXSector];
		nSector = pXSector->reference;
		dassert(sector[nSector].extra == nXSector);

		// undo previous lighting
		if ( pXSector->shade != 0 )
		{
			value = pXSector->shade;

			if ( pXSector->shadeFloor )
				sector[nSector].floorshade = (schar)(sector[nSector].floorshade - value);

			if ( pXSector->shadeCeiling )
				sector[nSector].ceilingshade = (schar)(sector[nSector].ceilingshade - value);

			if ( pXSector->shadeWalls )
			{
				startwall = sector[nSector].wallptr;
				endwall = startwall + sector[nSector].wallnum-1;
				for (nWall = startwall; nWall <= endwall; nWall++)
					wall[nWall].shade = (schar)(wall[nWall].shade - value);
			}

			pXSector->shade = 0;
		}

		if ( pXSector->shadeAlways || pXSector->busy )
		{
			freq = pXSector->freq;
			nWave = pXSector->wave;
			amplitude = pXSector->amplitude;

			if ( !pXSector->shadeAlways && pXSector->busy )
				amplitude = mulscale16(amplitude, pXSector->busy);

			value = GetWaveValue(nWave, gFrameClock + pXSector->phase, freq, amplitude);

			if ( pXSector->shadeFloor )
				sector[nSector].floorshade = (schar)ClipRange(sector[nSector].floorshade + value, -128, 127);

			if ( pXSector->shadeCeiling )
				sector[nSector].ceilingshade = (schar)ClipRange(sector[nSector].ceilingshade + value, -128, 127);

			if ( pXSector->shadeWalls )
			{
				startwall = sector[nSector].wallptr;
				endwall = startwall + sector[nSector].wallnum-1;
				for (nWall = startwall; nWall <= endwall; nWall++)
					wall[nWall].shade = (schar)ClipRange(wall[nWall].shade + value, -128, 127);
			}

			pXSector->shade = value;
		}
	}
}


void UndoSectorLighting( void )		// should be used only in mapedit
{
	int nXSector, nSector;
	int value;
	int nWall, startwall, endwall;

	for (nSector = 0; nSector < numsectors; nSector++)
	{
		nXSector = sector[nSector].extra;
		if (nXSector <= 0)
			continue;

		XSECTOR *pXSector = &xsector[nXSector];

		if ( pXSector->shade != 0 )
		{
			value = pXSector->shade;

			if ( pXSector->shadeFloor )
				sector[nSector].floorshade = (schar)(sector[nSector].floorshade - value);

			if ( pXSector->shadeCeiling )
				sector[nSector].ceilingshade = (schar)(sector[nSector].ceilingshade - value);

			if ( pXSector->shadeWalls )
			{
				startwall = sector[nSector].wallptr;
				endwall = startwall + sector[nSector].wallnum-1;
				for (nWall = startwall; nWall <= endwall; nWall++)
					wall[nWall].shade = (schar)(wall[nWall].shade - value);
			}

			pXSector->shade = 0;
		}
	}
}


void DoSectorPanning( void )
{
	int i;

	for (i = 0; i < panCount; i++)
	{
		int nXSector = panList[i];
		XSECTOR *pXSector = &xsector[nXSector];
		int nSector = pXSector->reference;
		dassert(nSector >= 0 && nSector < kMaxSectors);
		SECTOR *pSector = &sector[nSector];

		dassert(pSector->extra == nXSector);

		if ( pXSector->panAlways || pXSector->busy )
		{
			int panAngle = (pXSector->panAngle - kAngle90) & kAngleMask;
			int panVel = pXSector->panVel << kPanScale;

			if ( !pXSector->panAlways && (pXSector->busy & kFluxMask) )
				panVel = mulscale16(panVel, pXSector->busy);

			if ( pXSector->panFloor )
			{
				int nTile = pSector->floorpicnum;

				int panX = (pSector->floorxpanning << 8) + pXSector->floorxpanFrac;
				int panY = (pSector->floorypanning << 8) + pXSector->floorypanFrac;

				panX += mulscale30(kFrameTicks * panVel, Cos(panAngle)) >>
					((picsiz[nTile] & 0xF) - (pSector->floorstat & kSectorExpand ? 1 : 0));
				panY += mulscale30(kFrameTicks * panVel, Sin(panAngle)) >>
					((picsiz[nTile] / 16) - (pSector->floorstat & kSectorExpand ? 1 : 0));

				pSector->floorxpanning = (char)(panX >> 8);
				pSector->floorypanning = (char)(panY >> 8);
				pXSector->floorxpanFrac = panX & 0xFF;
				pXSector->floorypanFrac = panY & 0xFF;
			}

			if ( pXSector->panCeiling )
			{
				int nTile = pSector->ceilingpicnum;

				int panX = (pSector->ceilingxpanning << 8) + pXSector->ceilxpanFrac;
				int panY = (pSector->ceilingypanning << 8) + pXSector->ceilypanFrac;

				panX += mulscale30(kFrameTicks * panVel, Cos(panAngle)) >>
					((picsiz[nTile] & 0xF) - (pSector->ceilingstat & kSectorExpand ? 1 : 0));
				panY += mulscale30(kFrameTicks * panVel, Sin(panAngle)) >>
					((picsiz[nTile] / 16) - (pSector->ceilingstat & kSectorExpand ? 1 : 0));

				pSector->ceilingxpanning = (char)(panX >> 8);
				pSector->ceilingypanning = (char)(panY >> 8);
				pXSector->ceilxpanFrac = panX & 0xFF;
				pXSector->ceilypanFrac = panY & 0xFF;
			}
		}
	}

	for (i = 0; i < wallPanCount; i++)
	{
		int nXWall = wallPanList[i];
		XWALL *pXWall = &xwall[nXWall];
		int nWall = pXWall->reference;
		dassert(wall[nWall].extra == nXWall);

		if ( pXWall->panAlways || pXWall->busy )
		{
			int panXVel = pXWall->panXVel << 8;
			int panYVel = pXWall->panYVel << 8;

			if ( !pXWall->panAlways && (pXWall->busy & kFluxMask) )
			{
				panXVel = mulscale16(panXVel, pXWall->busy);
				panYVel = mulscale16(panYVel, pXWall->busy);
			}

			int nTile = wall[nWall].picnum;

			int panX = (wall[nWall].xpanning << 8) + pXWall->xpanFrac;
			int panY = (wall[nWall].ypanning << 8) + pXWall->ypanFrac;

			panX += kFrameTicks * panXVel >> (picsiz[nTile] & 0xF);
			panY += kFrameTicks * panYVel >> (picsiz[nTile] / 16);

			wall[nWall].xpanning = (char)(panX >> 8);
			wall[nWall].ypanning = (char)(panY >> 8);
			pXWall->xpanFrac = panX & 0xFF;
			pXWall->ypanFrac = panY & 0xFF;
		}
	}
}


void InitSectorFX( void )
{
	int i;
	shadeCount = 0;
	panCount = 0;
	wallPanCount = 0;

	for (i = 0; i < numsectors; i++)
	{
		short nXSector = sector[i].extra;
		if ( nXSector > 0 )
		{
			XSECTOR *pXSector = &xsector[nXSector];
			if ( pXSector->amplitude != 0 )
				shadeList[shadeCount++] = nXSector;
			if ( pXSector->panVel != 0 )
				panList[panCount++] = nXSector;
		}
	}

	for (i = 0; i < numwalls; i++)
	{
		short nXWall = wall[i].extra;
		if ( nXWall > 0 )
		{
			XWALL *pXWall = &xwall[nXWall];
			if ( pXWall->panXVel != 0 || pXWall->panYVel != 0)
				wallPanList[wallPanCount++] = nXWall;
		}
	}
}
