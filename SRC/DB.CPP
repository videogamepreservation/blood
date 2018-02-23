#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>

#include "engine.h"

#include "debug4g.h"
#include "db.h"
#include "error.h"
#include "misc.h"
#include "resource.h"
#include "iob.h"
#include "names.h"
#include "globals.h"
#include "trig.h"

#include <memcheck.h>


/***********************************************************************
 * Constants
 **********************************************************************/
#define kBloodMapExt		".MAP"
#define kBloodMapSig		"BLM\x1A"
#define kBloodMapVersion	0x0600
#define kMajorVersionMask	0xFF00
#define kMinorVersionMask	0x00FF

#define kFreeHead			0

/***********************************************************************
 * Structures and typedefs
 **********************************************************************/
struct HEADER
{
	char signature[4];
	short version;
};

// version 6.x header
struct INFO
{
	long	x, y, z;
	ushort	angle;
	ushort	sector;
	ushort 	pskybits;
	long	visibility;
	int		songId;
	uchar	parallax;
	int 	mapRevisions;
	ushort	numsectors;
	ushort	numwalls;
	ushort	numsprites;
};

/*******************************************************************************
File format:

	HEADER
	INFO
	name[]
	author[]
	sector[]
	wall[]
	sprite[]
	xsprite[]
	xwall[]
	xsector[]
	CRC
*******************************************************************************/


/***********************************************************************
 * Global Variables / MPX related strings
 **********************************************************************/
char *gItemText[ kItemMax - kItemBase ] = {
							 // items which will probably not change
	"Key 1",                 // kItemKey1
	"Key 2",                 // kItemKey2
	"Key 3",                 // kItemKey3
	"Key 4",                 // kItemKey4
	"Key 5",                 // kItemKey5
	"Key 6",                 // kItemKey6
	"Key 7",                 // kItemKey7
	"Doctor's Bag",          // kItemDoctorBag
	"Medicine Pouch",        // kItemMedPouch
	"Life Essence",          // kItemLifeEssence
	"Life Seed",             // kItemLifeSeed
	"Red Potion",            // kItemPotion1
	"Feather Fall",          // kItemFeatherFall
	"Limited Invisibility",  // kItemLtdInvisibility
	"INVULNERABILITY",       // kItemInvulnerability
	"Boots of Jumping",      // kItemJumpBoots
	"Raven Flight",          // kItemRavenFlight
	"Guns Akimbo",           // kItemGunsAkimbo
	"Diving Suit",           // kItemDivingSuit
	"Gas mask",              // kItemGasMask
	"Clone",                 // kItemClone
	"Crystal Ball",          // kItemCrystalBall
	"Decoy",                 // kItemDecoy
	"Doppleganger",          // kItemDoppleganger
	"Reflective shots",      // kItemReflectiveShots
	"Rose colored glasses",  // kItemRoseGlasses
	"ShadowCloak",           // kItemShadowCloak
 	"Rage shroom",           // kItemShroomRage
	"Delirium Shroom",       // kItemShroomDelirium
	"Grow shroom",           // kItemShroomGrow
	"Shrink shroom",		 // kItemShroomShrink
	"Death mask",			// kItemDeathMask
	"Wine Goblet",          // kItemWineGoblet
	"Wine Bottle",          // kItemWineBottle
	"Skull Grail",          // kItemSkullGrail
	"Silver Grail",         // kItemSilverGrail
	"Tome",                 // kItemTome
	"Black Chest",          // kItemBlackChest
	"Wooden Chest",         // kItemWoodenChest
	"Asbestos Armor",		// kItemAsbestosArmor
};

char *gAmmoText[kAmmoItemMax - kAmmoItemBase] = {
	"Spray can",
	"Stick of TNT",
	"Bundle of TNT",
	"Case of TNT",
	"Proximity Detonator",
	"Remote Detonator",
	"Timed Detonator",
	"4 shotgun shells",
	"Box of shotgun shells",
	"A few bullets",
	"Box of bullets",
	"Armor piercing bullets",
	"Full drum of bullets",
	"Speargun spear",
	"6 speargun spears",
	"Explosive spears",
	"Flares",
	"Explosive flares",
	"Burst flares",
};

char *gWeaponText[kWeaponItemMax - kWeaponItemBase] =
{
	"RandomWeapon",	// kWeaponItemRandom
	"SawedOff",		// kWeaponItemShotgun
	"TommyGun",		// kWeaponItemTommyGun
	"FlareGun",		// kWeaponItemFlareGun
	"VoodooDoll",	// kWeaponItemVoodooDoll
	"SpearGun",		// kWeaponItemSpearGun
	"EctoBlaster",	// kWeaponItemShadowGun
	"Pitchfork",	// kWeaponItemPitchfork
	"SprayCan",		// kWeaponItemSprayCan
	"TNT"			// kWeaponItemTNT
};

/***********************************************************************
 * Variables
 **********************************************************************/

XSPRITE 	xsprite[kMaxXSprites];
XWALL 		xwall[kMaxXWalls];
XSECTOR 	xsector[kMaxXSectors];

ushort 		nextXSprite[kMaxXSprites];
ushort 		nextXWall[kMaxXWalls];
ushort 		nextXSector[kMaxXSectors];

int			gMapRev;
int			gSongId;
ulong 		gMapCRC;
int 		gSkyCount;


/*******************************************************************************
Replace engine sprite list functions
*******************************************************************************/

void InsertSpriteSect( short nSprite, short nSector )
{
	dassert(nSprite >= 0 && nSprite < kMaxSprites);
	dassert(nSector >= 0 && nSector < kMaxSectors);

	int nOldHead = headspritesect[nSector];

	if (nOldHead >= 0)
	{
		// insert sprite at the tail of the list
		prevspritesect[nSprite] = prevspritesect[nOldHead];
		nextspritesect[nSprite] = -1;
		nextspritesect[prevspritesect[nOldHead]] = nSprite;
		prevspritesect[nOldHead] = nSprite;
	}
	else
	{
		prevspritesect[nSprite] = nSprite;
		nextspritesect[nSprite] = -1;
		headspritesect[nSector] = nSprite;
	}
	sprite[nSprite].sectnum = nSector;
}


void RemoveSpriteSect( int nSprite )
{
	dassert(nSprite >= 0 && nSprite < kMaxSprites);
	int nSector = sprite[nSprite].sectnum;

	dassert(nSector >= 0 && nSector < kMaxSectors);

	if ( nextspritesect[nSprite] >= 0 )
		prevspritesect[nextspritesect[nSprite]] = prevspritesect[nSprite];
	else
		prevspritesect[headspritesect[nSector]] = prevspritesect[nSprite];

	if ( headspritesect[nSector] != nSprite )
		nextspritesect[prevspritesect[nSprite]] = nextspritesect[nSprite];
	else
		headspritesect[nSector] = nextspritesect[nSprite];

	sprite[nSprite].sectnum = -1;
}


void InsertSpriteStat( short nSprite, short nStat )
{
	dassert(nSprite >= 0 && nSprite < kMaxSprites);
	dassert(nStat >= 0 && nStat <= kMaxStatus);

	int nHead = headspritestat[nStat];

	if (nHead >= 0)
	{
		nextspritestat[nSprite] = -1;
		prevspritestat[nSprite] = prevspritestat[nHead];
		nextspritestat[prevspritestat[nHead]] = nSprite;
		prevspritestat[nHead] = nSprite;
	}
	else
	{
		prevspritestat[nSprite] = nSprite;
		nextspritestat[nSprite] = -1;
		headspritestat[nStat] = nSprite;
	}
	sprite[nSprite].statnum = nStat;
}


void RemoveSpriteStat( int nSprite )
{
	dassert(nSprite >= 0 && nSprite < kMaxSprites);
	int nStat = sprite[nSprite].statnum;
	dassert(nStat >= 0 && nStat <= kMaxStatus);

	if ( nextspritestat[nSprite] >= 0 )
		prevspritestat[nextspritestat[nSprite]] = prevspritestat[nSprite];
	else
		prevspritestat[headspritestat[nStat]] = prevspritestat[nSprite];

	if (headspritestat[nStat] != nSprite )
		nextspritestat[prevspritestat[nSprite]] = nextspritestat[nSprite];
	else
		headspritestat[nStat] = nextspritestat[nSprite];

	sprite[nSprite].statnum = -1;
}


void initspritelists()
{
	short i;

     //Init doubly-linked sprite sector lists
	for (i = 0; i <= kMaxSectors; i++)
		headspritesect[i] = -1;

	for (i = 0; i <= kMaxStatus; i++)
		headspritestat[i] = -1;

	for (i = 0; i < kMaxSprites; i++)
	{
		sprite[i].sectnum = -1;
		InsertSpriteStat(i, kStatFree);	// add to free list
	}
}

//long debugNumSprites = 0;

short insertsprite( short nSector, short nStatus )
{
	short nSprite;

//	debugNumSprites++;
//	dprintf("insertsprite: debugNumSprites=%i\n",debugNumSprites);

	nSprite = headspritestat[kStatFree];
	if (nSprite >= 0)
	{
		RemoveSpriteStat(nSprite);
		InsertSpriteStat(nSprite, nStatus);
		InsertSpriteSect(nSprite, nSector);

		sprite[nSprite].cstat = kSpriteOriginAlign;
		sprite[nSprite].shade = 0;
		sprite[nSprite].pal = 0;
		sprite[nSprite].clipdist = 32;
		sprite[nSprite].xrepeat = 64;
		sprite[nSprite].yrepeat = 64;
		sprite[nSprite].xoffset = 0;
		sprite[nSprite].yoffset = 0;
		sprite[nSprite].picnum = 0;
		sprite[nSprite].ang = 0;
		sprite[nSprite].xvel = 0;
		sprite[nSprite].yvel = 0;
		sprite[nSprite].zvel = 0;
		sprite[nSprite].owner = -1;
		sprite[nSprite].type = 0;
		sprite[nSprite].flags = 0;
		sprite[nSprite].extra = -1;
	}

	if ( nSprite < 0 )
	{
		dprintf("sprite status linked list:\n");
		for ( int i = 0; i <= kMaxStatus; i++ )
			dprintf("head[%4d]=%4d\n", i, headspritestat[i]);
		for ( i = 0; i <= kMaxSprites; i++ )
			dprintf("%4d: prev=%4d next=%4d\n", i, prevspritestat[i], nextspritestat[i]);
		dassert(nSprite >= 0);
	}

	return nSprite;
}


void deletesprite( short nSprite )
{
//	debugNumSprites--;
//	dprintf("deletesprite: debugNumSprites=%i\n",debugNumSprites);

	if ( sprite[nSprite].extra > 0 )
		dbDeleteXSprite(sprite[nSprite].extra);

	dassert(sprite[nSprite].statnum >= 0 && sprite[nSprite].statnum < kMaxStatus);
	RemoveSpriteStat(nSprite);

	dassert(sprite[nSprite].sectnum >= 0 && sprite[nSprite].sectnum < kMaxSectors);
	RemoveSpriteSect(nSprite);

	InsertSpriteStat(nSprite, kStatFree);
}


int changespritesect( short nSprite, short nSector )
{
	dassert(nSprite >= 0 && nSprite < kMaxSprites);
	dassert(nSector >= 0 && nSector < kMaxSectors);

	dassert(sprite[nSprite].statnum >= 0 && sprite[nSprite].statnum < kMaxStatus);
	dassert(sprite[nSprite].sectnum >= 0 && sprite[nSprite].sectnum < kMaxSectors);

	// changing to same sector is a valid operation, and will put sprite at tail of list
	RemoveSpriteSect(nSprite);
	InsertSpriteSect(nSprite, nSector);
	return 0;
}


int changespritestat( short nSprite, short nStatus )
{
	dassert(nSprite >= 0 && nSprite < kMaxSprites);
	dassert(nStatus >= 0 && nStatus < kMaxStatus);
	dassert(sprite[nSprite].statnum >= 0 && sprite[nSprite].statnum < kMaxStatus);
	dassert(sprite[nSprite].sectnum >= 0 && sprite[nSprite].sectnum < kMaxSectors);

	// changing to same status is a valid operation, and will put sprite at tail of list
	RemoveSpriteStat(nSprite);
	InsertSpriteStat(nSprite, nStatus);
	return 0;
}


/*******************************************************************************
These functions are for manipulating the xobject lists
*******************************************************************************/

void InitFreeList( ushort xlist[], int xlistSize )
{
	for (int i = 1; i < xlistSize; i++)
		xlist[i] = (ushort)(i - 1);
	xlist[kFreeHead] = (ushort)(xlistSize - 1);
}


inline ushort RemoveFree( ushort next[] )
{
	ushort n = next[kFreeHead];
	next[kFreeHead] = next[n];
	return n;
}


inline void InsertFree( ushort next[], int n )
{
	next[n] = next[kFreeHead];
	next[kFreeHead] = (ushort)n;
}


ushort dbInsertXSprite( int nSprite )
{
	ushort nXSprite = RemoveFree(nextXSprite);
	if (nXSprite == 0)
		ThrowError("Out of free XSprites", ES_ERROR);

	memset(&xsprite[nXSprite], 0, sizeof(XSPRITE));
	sprite[nSprite].extra = nXSprite;
	xsprite[nXSprite].reference = nSprite;

	return nXSprite;
}


void dbDeleteXSprite( int nXSprite )
{
	dassert(xsprite[nXSprite].reference >= 0);
	dassert(sprite[xsprite[nXSprite].reference].extra == nXSprite);
	InsertFree( nextXSprite, nXSprite );

	// clear the references
	sprite[xsprite[nXSprite].reference].extra = -1;
	xsprite[nXSprite].reference = -1;
}


ushort dbInsertXWall( int nWall )
{
	ushort nXWall = RemoveFree(nextXWall);
	if (nXWall == 0)
		ThrowError("Out of free XWalls", ES_ERROR);

	memset(&xwall[nXWall], 0, sizeof(XWALL));
	wall[nWall].extra = nXWall;
	xwall[nXWall].reference = nWall;

	return nXWall;
}


void dbDeleteXWall( int nXWall )
{
	dassert(xwall[nXWall].reference >= 0);
	InsertFree( nextXWall, nXWall );

	// clear the references
	wall[xwall[nXWall].reference].extra = -1;
	xwall[nXWall].reference = -1;
}


ushort dbInsertXSector( int nSector )
{
	ushort nXSector = RemoveFree(nextXSector);
	if (nXSector == 0)
		ThrowError("Out of free XSectors", ES_ERROR);

	memset(&xsector[nXSector], 0, sizeof(XSECTOR));
	sector[nSector].extra = nXSector;
	xsector[nXSector].reference = nSector;

	return nXSector;
}


void dbDeleteXSector( int nXSector )
{
	dassert(xsector[nXSector].reference >= 0);
	InsertFree( nextXSector, nXSector );

	// clear the references
	sector[xsector[nXSector].reference].extra = -1;
	xsector[nXSector].reference = -1;	// clear the reference
}


/***********************************************************************
 * dbXSpriteClean()
 *
 * Clean up the xsprite list: clone duplicate references & delete orphans
 **********************************************************************/
void dbXSpriteClean(void)
{
	int nSprite, nXSprite;

	// clone duplicate references
	for ( nSprite = 0; nSprite < kMaxSprites; nSprite++ )
	{
		nXSprite = sprite[nSprite].extra;
		if (nXSprite == 0)
			sprite[nSprite].extra = -1;

		if ( sprite[nSprite].statnum < kMaxStatus && nXSprite > 0 )
		{
			dassert(nXSprite < kMaxXSprites);

			if ( xsprite[nXSprite].reference != nSprite )
			{
				// make a copy of the xsprite entry
				int newXSprite = dbInsertXSprite(nSprite);
				xsprite[newXSprite] = xsprite[nXSprite];
				xsprite[newXSprite].reference = nSprite;	// fix reference after copying
			}
		}
	}

	// remove orphans
	for ( nXSprite = 1; nXSprite < kMaxXSprites; nXSprite++ )
	{
		nSprite = xsprite[nXSprite].reference;
		if (nSprite >= 0)
		{
			dassert(nSprite < kMaxSprites);

			if (sprite[nSprite].statnum >= kMaxStatus || sprite[nSprite].extra != nXSprite)
			{
				// delete manually so we don't assert from a mismatched reference
				InsertFree( nextXSprite, nXSprite );
				xsprite[nXSprite].reference = -1;
			}
		}
	}
}

/***********************************************************************
 * dbXWallClean()
 *
 * Clean up the xwall list: clone duplicate references & delete orphans
 **********************************************************************/
void dbXWallClean(void)
{
	int nWall, nXWall;

	// back propagate references
	for ( nWall = 0; nWall < numwalls; nWall++ )
	{
   		nXWall = wall[nWall].extra;
		if (nXWall == 0)
			wall[nWall].extra = -1;

		if ( nXWall > 0 )
		{
			dassert(nXWall < kMaxXWalls);

			// wall points to a freed xwall
			if (xwall[nXWall].reference == -1)
			{
				wall[nWall].extra = -1;
				continue;
			}

			// probably just moved the wall around
			xwall[nXWall].reference = nWall;
		}
	}

	// clone duplicate references
	for ( nWall = 0; nWall < numwalls; nWall++ )
	{
		if ( wall[nWall].extra > 0 )
		{
			nXWall = wall[nWall].extra;
			dassert(nXWall < kMaxXWalls);

			if ( xwall[nXWall].reference != nWall )
			{
				// make a copy of the xwall entry
				int newXWall = dbInsertXWall(nWall);
				xwall[newXWall] = xwall[nXWall];
				xwall[newXWall].reference = nWall;	// fix reference after copying
			}

		}
	}

	// remove orphans
	for ( nXWall = 1; nXWall < kMaxXWalls; nXWall++ )
	{
		nWall = xwall[nXWall].reference;
		if (nWall >= 0)
		{
			dassert(nWall < kMaxWalls);

			if ( nWall >= numwalls || wall[nWall].extra != nXWall )
			{
				// delete manually so we don't assert from a mismatched reference
				InsertFree( nextXWall, nXWall );
				xwall[nXWall].reference = -1;
			}
		}
	}
}

/***********************************************************************
 * dbXSectorClean()
 *
 * Clean up the xsector list: clone duplicate references & delete orphans
 **********************************************************************/
void dbXSectorClean(void)
{
	int nSector, nXSector;

	// back propagate references
	for ( nSector = 0; nSector < numsectors; nSector++ )
	{
   		nXSector = sector[nSector].extra;
		if (nXSector == 0)
			sector[nSector].extra = -1;

		if ( nXSector > 0 )
		{
			dassert(nXSector < kMaxXSectors);

			// sector points to a freed xsector
			if (xsector[nXSector].reference == -1)
			{
				sector[nSector].extra = -1;
				continue;
			}

			// probably just moved the sector around
			xsector[nXSector].reference = nSector;
		}
	}

	// clone duplicate references
	for ( nSector = 0; nSector < numsectors; nSector++ )
	{
		if ( sector[nSector].extra > 0 )
		{
			nXSector = sector[nSector].extra;
			dassert(nXSector < kMaxXSectors);

			if ( xsector[nXSector].reference != nSector )
			{
				// make a copy of the xsector entry
				int newXSector = dbInsertXSector(nSector);
				xsector[newXSector] = xsector[nXSector];
				xsector[newXSector].reference = nSector;	// fix reference after copying
			}

		}
	}

	// remove orphans
	for ( nXSector = 1; nXSector < kMaxXSectors; nXSector++ )
	{
		nSector = xsector[nXSector].reference;
		if (nSector >= 0)
		{
			dassert(nSector < kMaxSectors);

			if ( nSector >= numsectors || sector[nSector].extra != nXSector )
			{
				// delete manually so we don't assert from a mismatched reference
				InsertFree( nextXSector, nXSector );
				xsector[nXSector].reference = -1;
			}
		}
	}
}


void dbInit( void )
{
	int i;

//	memset(sprite, 0, sizeof(sprite));
//	memset(wall, 0, sizeof(wall));
//	memset(sector, 0, sizeof(sector));

	InitFreeList(nextXSprite, kMaxXSprites);
	for (i = 1; i < kMaxXSprites; i++)
		xsprite[i].reference = -1;

	InitFreeList(nextXWall, kMaxXWalls);
	for (i = 1; i < kMaxXWalls; i++)
		xwall[i].reference = -1;

	InitFreeList(nextXSector, kMaxXSectors);
	for (i = 1; i < kMaxXSectors; i++)
		xsector[i].reference = -1;

	initspritelists();

	// initialize default cstat for sprites
	for (i = 0; i < kMaxSprites; i++)
		sprite[i].cstat = kSpriteOriginAlign;
}



/*******************************************************************************
	FUNCTION:		PropagateMarkerReferences()

	DESCRIPTION:	Fixed sector references based on marker sprite references.

	NOTES:			This function assumes that the owner references in the
					marker sprites are correct.  This function should be called
					from loadboard, where we know that sprites are rearranged.
*******************************************************************************/
void PropagateMarkerReferences( void )
{
	int nSprite, j;
	int nSector, nXSector;

	for (nSprite = headspritestat[kStatMarker]; nSprite != -1; nSprite = j)
	{
		j = nextspritestat[nSprite];

		switch (sprite[nSprite].type)
		{
			case kMarkerWarpDest:
				nSector = sprite[nSprite].owner;
				if (nSector < 0 || nSector >= numsectors)
					break;
				nXSector = sector[nSector].extra;
				if (nXSector <= 0 || nXSector >= kMaxXSectors)
					break;

				xsector[nXSector].marker0 = nSprite;
				continue;

			case kMarkerOff:
				nSector = sprite[nSprite].owner;
				if (nSector < 0 || nSector >= numsectors)
					break;
				nXSector = sector[nSector].extra;
				if (nXSector <= 0 || nXSector >= kMaxXSectors)
					break;

				xsector[nXSector].marker0 = nSprite;
				continue;

			case kMarkerOn:
				nSector = sprite[nSprite].owner;
				if (nSector < 0 || nSector >= numsectors)
					break;
				nXSector = sector[nSector].extra;
				if (nXSector <= 0 || nXSector >= kMaxXSectors)
					break;

				xsector[nXSector].marker1 = nSprite;
				continue;

			case kMarkerAxis:
				nSector = sprite[nSprite].owner;
				if (nSector < 0 || nSector >= numsectors)
					break;
				nXSector = sector[nSector].extra;
				if (nXSector <= 0 || nXSector >= kMaxXSectors)
					break;

				xsector[nXSector].marker0 = nSprite;
				continue;
		}

		dprintf("Deleting invalid marker sprite\n");
		deletesprite((short)nSprite);
	}
}


/***********************************************************************
 * dbLoadMap()
 **********************************************************************/
void dbLoadMap(const char *mapname, long *x, long *y, long *z, short *angle, short *nSector)
{
	char filename[_MAX_PATH];
	int i, numsprites;
	int length;
	HEADER header;
	INFO info;
	BYTE *buffer;

	// these eventually need to be added to the save game format
//	memset(show2dsector, 0, sizeof(show2dsector));
//	memset(show2dwall, 0, sizeof(show2dwall));
//	memset(show2dsprite, 0, sizeof(show2dsprite));

	strcpy(filename, mapname);

	// clear any extension
	char *p = strchr(filename, '.');
	if ( p != NULL)
		*p = '\0';

	RESHANDLE hMap = gSysRes.Lookup(filename, ".MAP");
	if ( hMap == NULL )
		ThrowError("Error opening map file", ES_ERROR);

	length = gSysRes.Size(hMap);
	buffer = (BYTE *)gSysRes.Lock(hMap);

	IOBuffer iob(buffer, length);

	iob.Read(&header, sizeof(HEADER));

	if (memcmp(header.signature, kBloodMapSig, sizeof(header.signature)) != 0)
		ThrowError("Map file corrupted", ES_ERROR);

	if ( (header.version & kMajorVersionMask) != (kBloodMapVersion & kMajorVersionMask) )
		ThrowError("Map file is wrong version", ES_ERROR);

	iob.Read(&info, sizeof(INFO));

	*x = 			info.x;
	*y = 			info.y;
	*z = 			info.z;
	*angle = 		info.angle;
	*nSector = 		info.sector;
	pskybits = 		info.pskybits;
	visibility = 	info.visibility;
	gSongId	=		info.songId;
	parallaxtype = 	info.parallax;
	gMapRev =		info.mapRevisions;
	numsectors =	info.numsectors;
	numwalls =		info.numwalls;
	numsprites =	info.numsprites;

	dbInit();

	gSkyCount = 1 << pskybits;
	iob.Read(pskyoff, gSkyCount * sizeof(pskyoff[0]));

	for (i = 0; i < numsectors; i++)
	{
		SECTOR *pSector = &sector[i];
		iob.Read(&sector[i], sizeof(SECTOR));

		if (sector[i].extra > 0)
		{
			iob.Read(&xsector[dbInsertXSector(i)], sizeof(XSECTOR));
			xsector[sector[i].extra].reference = i;
			xsector[sector[i].extra].busy = xsector[sector[i].extra].state << 16;
		}
	}

	for (i = 0; i < numwalls; i++)
	{
		iob.Read(&wall[i], sizeof(WALL));

		if (wall[i].extra > 0)
		{
			iob.Read(&xwall[dbInsertXWall(i)], sizeof(XWALL));
			xwall[wall[i].extra].reference = i;
			xwall[wall[i].extra].busy = xwall[wall[i].extra].state << 16;
		}
	}

	initspritelists();
	for (i = 0; i < numsprites; i++)
	{
		RemoveSpriteStat((short)i);		// remove it from the free list

		iob.Read(&sprite[i], sizeof(SPRITE));

		// insert sprite on appropriate lists
		InsertSpriteSect((short)i, sprite[i].sectnum);
		InsertSpriteStat((short)i, sprite[i].statnum);
//		debugNumSprites++;

		if (sprite[i].extra > 0)
		{
			iob.Read(&xsprite[dbInsertXSprite(i)], sizeof(XSPRITE));
			xsprite[sprite[i].extra].reference = i;
			xsprite[sprite[i].extra].busy = xsprite[sprite[i].extra].state << 16;
		}
	}

	iob.Read(&gMapCRC, sizeof(gMapCRC));

	if ( gMapCRC != CRC32(buffer, length - sizeof(gMapCRC)) )
		ThrowError("File does not match CRC", ES_ERROR);

	gSysRes.Unlock(hMap);

	PropagateMarkerReferences();
}


/***********************************************************************
 * dbSaveMap()
 **********************************************************************/
void dbSaveMap(const char *mapname, long x, long y, long z, short angle, short nSector)
{
	char filename[_MAX_PATH];
	char bakfilename[_MAX_PATH];
	int hFile;
	HEADER header;
	INFO info;
	int length;
	BYTE *buffer;
	int numsprites = 0;
	int i;

	gSkyCount = 1 << pskybits;
	gMapRev++;

	strcpy(filename, mapname);
	strcpy(bakfilename, mapname);
	ChangeExtension(filename, kBloodMapExt);
	ChangeExtension(bakfilename, ".BAK");

	// add up all the elements in the file to determine the size of the buffer
	length = 0;
	length += sizeof(HEADER);
	length += sizeof(INFO);
	length += gSkyCount * sizeof(pskyoff[0]);
	length += numsectors * sizeof(SECTOR);
	for (i = 0; i < numsectors; i++)
	{
		if (sector[i].extra > 0)
			length += sizeof(XSECTOR);
	}
	length += numwalls * sizeof(WALL);
	for (i = 0; i < numwalls; i++)
	{
		if (wall[i].extra > 0)
			length += sizeof(XWALL);
	}
	for (i = 0; i < kMaxSprites; i++)
	{
		if (sprite[i].statnum < kMaxStatus)
		{
			numsprites++;
			if (sprite[i].extra > 0)
				length += sizeof(XSPRITE);
		}
	}
	length += numsprites * sizeof(SPRITE);
	length += 4;	// CRC

	buffer = (BYTE *)Resource::Alloc(length);
	IOBuffer iob(buffer, length);

	memcpy(header.signature, kBloodMapSig, sizeof(header.signature));
	header.version = kBloodMapVersion;

	iob.Write(&header, sizeof(header));

	info.x = x;
	info.y = y;
	info.z = z;
	info.angle = angle;
	info.sector = nSector;
	info.pskybits = pskybits;
	info.visibility = visibility;
	info.songId = gSongId;
	info.parallax = parallaxtype;
	info.mapRevisions = gMapRev;
	info.numsectors = numsectors;
	info.numwalls = numwalls;
	info.numsprites = (short)numsprites;

	iob.Write(&info, sizeof(INFO));
	iob.Write(pskyoff, gSkyCount * sizeof(pskyoff[0]));

	for (i = 0; i < numsectors; i++)
	{
		iob.Write(&sector[i], sizeof(SECTOR));
		if (sector[i].extra > 0)
			iob.Write(&xsector[sector[i].extra], sizeof(XSECTOR));
	}

	for (i = 0; i < numwalls; i++)
	{
		iob.Write(&wall[i], sizeof(WALL));
		if (wall[i].extra > 0)
			iob.Write(&xwall[wall[i].extra], sizeof(XWALL));
	}

	for (i = 0; i < kMaxSprites; i++)
	{
		if (sprite[i].statnum < kMaxStatus)
		{
			iob.Write(&sprite[i], sizeof(SPRITE));
			if (sprite[i].extra > 0)
				iob.Write(&xsprite[sprite[i].extra], sizeof(XSPRITE));
		}
	}

	gMapCRC = CRC32(buffer, length - sizeof(gMapCRC));
	iob.Write(&gMapCRC, sizeof(gMapCRC));

	// backup the map file
	unlink(bakfilename);
	rename(filename, bakfilename);

	hFile = open(filename, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, S_IWUSR);
	if ( hFile == -1 )
		ThrowError("Error opening MAP file", ES_ERROR);

	if (write(hFile, buffer, length) != length)
		ThrowError("Error writing MAP file", ES_ERROR);

	close(hFile);
	Resource::Free(buffer);

	// make sure we don't leave a stale copy of the map on the heap
 	char *p = strchr(filename, '.');
	if ( p != NULL)
		*p = '\0';
	gSysRes.AddExternalResource(filename, ".MAP", length);
}
