#include <stdlib.h>

#include "debug4g.h"
#include "engine.h"
#include "gameutil.h"
#include "mirrors.h"
#include "names.h"
#include "screen.h"
#include "view.h"
#include "globals.h"
#include "error.h"
#include "helix.h"
#include "key.h"
#include "misc.h"

#define MAXMIRRORS 64

/***********************************************************************
 * static variables
 **********************************************************************/
static short mirrorwall[MAXMIRRORS], mirrorcnt;


/*******************************************************************************
	FUNCTION:		InitMirrors()

	DESCRIPTION:

	PARAMETERS:

	NOTES:
		Requires a 64 tile range beginning at MIRRORLABEL, a constant
		aligned on a value of 32.
		Requires MIRROR and MIRRORLABEL constants defined in NAMES.H.

*******************************************************************************/
void InitMirrors( void )
{
	int i, j;

	//Scan wall tags
	mirrorcnt = 0;
	tilesizx[MIRROR] = 0;
	tilesizy[MIRROR] = 0;
	for( i = 0; i < MAXMIRRORS; i++)
	{
		tilesizx[i + MIRRORLABEL] = 0;
		tilesizy[i + MIRRORLABEL] = 0;
	}

	for( i = numwalls - 1; i >= 0; i--)
	{
		if (mirrorcnt == MAXMIRRORS)
		{
			dprintf("Maximum mirror count reached.\n");
			break;
		}

		if ( (wall[i].cstat & kWallOneWay) && wall[i].overpicnum == MIRROR && wall[i].nextwall >= 0 )
		{
			short backSector = wall[i].nextsector;

			sector[backSector].ceilingpicnum = MIRROR;
			sector[backSector].floorpicnum = MIRROR;
//			sector[backSector].floorstat |= kSectorParallax;	// is this needed?
//			sector[backSector].ceilingstat |= kSectorParallax;	// is this needed?

			wall[i].overpicnum = (short)(MIRRORLABEL + mirrorcnt);
//			wall[i].cstat &= ~kWallMasked;
//			wall[i].cstat |= kWallOneWay;

			mirrorwall[mirrorcnt] = (short)i;
			mirrorcnt++;
		}
	}

	for (i = 0; i < mirrorcnt; i++)
	{
		short nSector = wall[mirrorwall[i]].nextsector;
		short wall0 = sector[nSector].wallptr;
		short wallN = (short)(wall0 + sector[nSector].wallnum);
		for (j = wall0; j < wallN; j++)
		{
			wall[j].picnum = MIRROR;
			wall[j].overpicnum = MIRROR;
		}
	}

	dprintf("%d mirrors initialized\n", mirrorcnt);
}


void TranslateMirrorColors( int shade, int pal )
{
	shade = ClipRange(shade, 0, kPalLookups - 1);
	BYTE *lut = palookup[pal] + shade * 256;

	BYTE *p = (BYTE *)gPageTable[0].begin;
	for (int i = 0; i < gPageTable[0].size; i++, p++)
	{
		*p = lut[*p];
	}
}


/*******************************************************************************
	FUNCTION:		DrawMirrors()

	DESCRIPTION:

	PARAMETERS:

	NOTES:
		MUST be called before drawrooms and before gotpic is cleared.
		Assumes MIRRORLABEL is aligned to a value of 32
		Assumes MAXMIRRORS = 64

*******************************************************************************/
void DrawMirrors( long x, long y, long z, short ang, long horiz )
{
	int i;
	long  tx, ty;
	short tang;
	DWORD dw0 = *(long *)&gotpic[MIRRORLABEL >> 3];
	DWORD dw1 = *(long *)&gotpic[(MIRRORLABEL >> 3) + 4];

	if ( !(dw0 | dw1) )
		return;	// no mirror visible

	for( i = MAXMIRRORS - 1; i >= 0; i-- )
	{
		if ( TestBitString(gotpic, i + MIRRORLABEL) )
		{
			ClearBitString(gotpic, MIRRORLABEL + i);

			// Prepare drawrooms for drawing mirror and calculate reflected
			// position into tx, ty, and tang (tz == z)
			// Must call preparemirror before drawrooms and completemirror after drawrooms

			short nWall = mirrorwall[i];
			short nSector = wall[nWall].nextsector;
			preparemirror( x, y, z, ang, horiz, nWall, nSector, &tx, &ty, &tang);

			clearview(0);

			drawrooms( tx, ty, z, tang, horiz, nSector);
			viewProcessSprites(tx, ty, z);
			drawmasks();

			if (keystatus[KEY_M])
			{
				scrNextPage();
				dprintf("Mirror %d\n", i);
				dprintf("  mirrorwall[i]       = %d\n", nWall);
				dprintf("  wall.nextwall       = %d\n", wall[nWall].nextwall);
				dprintf("  wall.nextsector     = %d\n", wall[nWall].nextsector);
				while ( keystatus[KEY_M] );
			}

			completemirror();   // Reverse screen x-wise in this function
			TranslateMirrorColors(wall[nWall].shade, wall[nWall].pal);
			break;
		}
	}
}
