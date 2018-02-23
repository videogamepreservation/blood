#ifndef __WARP_H
#define __WARP_H

#include "typedefs.h"
#include "engine.h"

/***********************************************************************
 * Global Structures
 **********************************************************************/

struct ZONE {
	long x;
	long y;
	long z;
	short sector;
	short angle;
};


/***********************************************************************
 * Global Data
 **********************************************************************/

extern ZONE gStartZone[kMaxPlayers];
extern short gUpperLink[kMaxSectors], gLowerLink[kMaxSectors];


/***********************************************************************
 * Function Prototypes
 **********************************************************************/

extern void InitPlayerStartZones( void );

#endif // __WARP_H
