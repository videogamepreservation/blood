#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <dos.h>
#include <io.h>
#include <fcntl.h>

#include "db.h"
#include "getopt.h"
#include "engine.h"
#include "error.h"
#include "misc.h"
#include "iob.h"
#include "tile.h"

#include <memcheck.h>

#define kBloodMapSig		"BLM\x1A"
#define kBloodMapVersion	0x0501
#define kBloodMapOldVersion	0x0401

struct FNODE
{
	FNODE *next;
	char name[1];
};


FNODE head = { &head, "" };
FNODE *tail = &head;

// Build globals
SECTOR 		sector[kMaxSectors];
WALL 		wall[kMaxWalls];
SPRITE 		sprite[kMaxSprites];

// Q globals
XSECTOR 	xsector[kMaxXSectors];
XWALL 		xwall[kMaxXWalls];
XSPRITE 	xsprite[kMaxXSprites];

short 		pskyoff[kMaxSkyTiles];
PICANM 		picanm[kMaxTiles];
uchar 		picsiz[kMaxTiles];
short 		tilesizx[kMaxTiles];
short 		tilesizy[kMaxTiles];

short pskybits;
ulong gMapCRC;
int   gSkyCount;
short numsectors, numwalls;

struct HEADER4
{
	char signature[4];
	short version;
};

struct HEADER5
{
	char signature[4];
	short version;
};

// version 4.x header
struct INFO4
{
	long	x, y, z;
	ushort	angle;
	ushort	sector;
	ushort 	pskybits;
	long    visibility;
	int		songId;
	uchar	parallax;
	int 	mapRevisions;
	uchar 	lenName;
	uchar 	lenAuthor;
	ushort	numsectors;
	ushort	numwalls;
	ushort	numsprites;
};

struct INFO5
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


/***********************************************************************
 * Old version structures and typedefs
 **********************************************************************/
struct XSPRITE4
{
	signed reference :			16;
	unsigned state :			1;

	// trigger data
	unsigned busy :				17;
	unsigned type :				10;
	signed data :				16;
	unsigned txID :				10;
	unsigned rxID :				10;
	unsigned command : 			3;
	unsigned triggerOn : 		1;
	unsigned triggerOff : 		1;
	unsigned triggerOnce :		1;
	unsigned restState :		1; // state to return to on callback
	unsigned key :				3;

	unsigned difficulty :		2;
	unsigned detail :			2;
	unsigned map : 				2;
	unsigned view :				4;	// OBSOLETE
	unsigned soundKit :			8;

	unsigned isTriggered :      1; // used for triggerOnce objects
	unsigned triggerPush :      1; // gates?
	unsigned triggerImpact :    1; // vector hits

	unsigned triggerPickup :    1; // secrets
	unsigned triggerTouch :     1; // sawblades, spikes, zaps?
	unsigned triggerSight :     1; // dunno, yet.
	unsigned triggerProximity : 1; // proximity bombs

	unsigned decoupled :		1;
	unsigned waitTime : 		8;

	unsigned permanent :		1; // 0=not permanent, 1=permanent (respawn ignored)
	unsigned respawn :			2; // 0=never, 1=always, 2=optional never, 3=optional always
	unsigned respawnTime :		8; // 0=instant, >0=time in tenths of a second,
	unsigned launchMode :		2; // 0=all, 1=bloodbath, 2=ally, 3=ally&bloodbath,

	unsigned pad1 : 32;
	unsigned pad2 : 32;
};


struct XWALL4
{
	signed reference :			16;
	unsigned state : 			1;
	unsigned map :				2;

	// trigger data
	unsigned busy : 			17;
	unsigned type :				10;
	signed data :				16;
	unsigned txID :				10;
	unsigned rxID :				10;
	unsigned command : 			3;
	unsigned triggerOn : 		1;
	unsigned triggerOff : 		1;
	unsigned triggerOnce :		1;
	unsigned key :				3;

	// panning data
	signed panXVel :			8;
	signed panYVel :			8;

	unsigned isTriggered :   	1; // used for triggerOnce objects
	unsigned triggerPush :   	1;
	unsigned triggerImpact : 	1;
	unsigned triggerRsvd1 :   	1;
	unsigned triggerRsvd2 : 	1;

	unsigned decoupled :		1;
	unsigned panAlways :		1;
	unsigned restState :		1; // state to return to on callback
	unsigned waitTime : 		8; // delay before callback

	unsigned pad1 : 32;
	unsigned pad2 : 32;
};

struct XSECTOR4
{
	signed reference :			16;
	unsigned state : 			1;

	// trigger data
	unsigned busy : 			17;
	unsigned type :				10;
	signed data :				16;
	unsigned txID :				10;
	unsigned rxID :				10;
	unsigned command : 			3;
	unsigned triggerOn : 		1;
	unsigned triggerOff : 		1;
	unsigned triggerOnce :		1;
	unsigned restState :		1;
	unsigned key :				3;

	// lighting data
	signed amplitude :			8;
	unsigned freq :				8;
	unsigned wave :				4;
	unsigned shadeAlways : 		1;
	unsigned phase :			8;
	unsigned shadeFloor :		1;
	unsigned shadeCeiling : 	1;
	unsigned shadeWalls :		1;
	signed shade :				8;

	// panning data
	unsigned panFloor :			1;
	unsigned panCeiling :		1;
	unsigned panDrag :			1;
	unsigned waitTime : 		8; // delay before callback
	unsigned panAlways :		1;

	unsigned busyTime :			8; // time to reach next state

	// wind/water stuff
	unsigned underwater :		1;
	unsigned depth :			2;
	unsigned panVel :      		8;
	unsigned panAngle :			11;

	unsigned wind :		    	1;

	unsigned isTriggered :		1;
	unsigned triggerPush :		1;
	unsigned triggerImpact :	1;
	unsigned triggerEnter :		1;
	unsigned triggerExit :		1;
	unsigned decoupled :		1;

	unsigned triggerWPush :		1;

	signed offCeilZ :			32;
	signed onCeilZ :			32;
	signed offFloorZ :			32;
	signed onFloorZ :			32;

	unsigned pad1 : 32;
	unsigned pad2 : 32;
};


// stuff to get tile sizes from art
static void CalcPicsiz( int nTile, int x, int y )
{
	int i;
	uchar n = 0;

	for (i = 2; i <= x; i <<= 1)
		n += 0x01;

	for (i = 2; i <= y; i <<= 1)
		n += 0x10;

	picsiz[nTile] = n;
}


void artInit( void )
{
	char filename[20];
	int i;
	int nTiles;
	int nFile, hFile;
	int tileStart, tileEnd;
	long artversion;
	long numtiles;

	memset(tilesizx, 0, sizeof(tilesizx));
	memset(tilesizy, 0, sizeof(tilesizy));
 	memset(picanm, 0, sizeof(picanm));

	nFile = 0;
	while (1)
	{
		sprintf(filename, "TILES%03i.ART", nFile);
		hFile = open(filename, O_BINARY | O_RDWR);

		if ( hFile == -1 )
			break;

		read(hFile, &artversion, sizeof(artversion));
		if (artversion != 1)
			ThrowError("Bad art file", ES_ERROR);

		read(hFile, &numtiles, sizeof(numtiles));
		read(hFile, &tileStart, sizeof(tileStart));
		read(hFile, &tileEnd, sizeof(tileEnd));

		nTiles = tileEnd - tileStart + 1;
		read(hFile, &tilesizx[tileStart], nTiles * sizeof(tilesizx[0]));
		read(hFile, &tilesizy[tileStart], nTiles * sizeof(tilesizy[0]));
		read(hFile, &picanm[tileStart], nTiles * sizeof(picanm[0]));

		nFile++;
	}

	for (i = 0; i < kMaxTiles; i++)
	{
		// setup the tile size log 2 array for internal engine use
		CalcPicsiz(i, tilesizx[i], tilesizy[i]);
	}
}


void ShowUsage(void)
{
	printf("Usage:\n");
	printf("  CONVDB4 map1 map2 (wild cards ok)\n");
	printf("\nTECHNICAL INFO:\n  sizeof(XSPRITE)=%d\n  sizeof(XSECTOR)=%d\n  sizeof(XWALL)=%d",
		sizeof(XSPRITE), sizeof(XSECTOR), sizeof(XWALL));
	exit(0);
}

void QuitMessage(char * fmt, ...)
{
	char msg[80];
	va_list argptr;
	va_start( argptr, fmt );
	vsprintf( msg, fmt, argptr );
	va_end(argptr);
	printf(msg);
	exit(1);
}


void ProcessFile(char *filespec)
{
	int hFile;
	int i;
	HEADER4 header4;
	HEADER5 header5;
	INFO4 info4;
	INFO5 info5;
	char filename[_MAX_PATH], bakfilename[_MAX_PATH];
	short numsprites;
	int length;

	BYTE *buffer;

	strcpy(filename, filespec);
	ChangeExtension(filename, ".MAP");
	printf("%s ", filename);

	hFile = open(filename, O_RDONLY | O_BINARY);
	if ( hFile == -1 )
		ThrowError("unable to open MAP file", ES_ERROR);

	length = filelength(hFile);
	buffer = (BYTE *)malloc(length);
	if (read(hFile, buffer, length) == length)
	{
		close(hFile);

		IOBuffer iob(buffer, length);

		iob.Read(&header4, sizeof(HEADER4));

		// check signature and major version number
		if ( (memcmp(header4.signature, kBloodMapSig, sizeof(header4.signature)) != 0)
			|| ((header4.version >> 8) != (kBloodMapOldVersion >> 8)) )
		{
			printf("not expected format, skipping\n");
			return;
		}

		iob.Read(&info4, sizeof(INFO4));
		numsectors = info4.numsectors;
		numwalls = info4.numwalls;
		numsprites = info4.numsprites;

		iob.Skip(info4.lenName + 1);
		iob.Skip(info4.lenAuthor + 1);

		pskybits = info4.pskybits;
		gSkyCount = 1 << pskybits;
		iob.Read(pskyoff, gSkyCount * sizeof(pskyoff[0]));

		for (i = 0; i < numsectors; i++)
		{
			iob.Read(&sector[i], sizeof(SECTOR));
			iob.Skip(sizeof(BYTE));	// old surface data
			iob.Skip(sizeof(BYTE));	// old surface data

			if (sector[i].extra > 0)
			{
				int j = sector[i].extra;

				XSECTOR4 xsector4;
				iob.Read(&xsector4, sizeof(XSECTOR4));
				memset(&xsector[j], 0, sizeof(XSECTOR));

				xsector[j].reference 		= xsector4.reference;
				xsector[j].state 			= xsector4.state;

				// trigger data
				xsector[j].busy				= 0;
				xsector[j].data				= xsector4.data;
				xsector[j].txID				= xsector4.txID;
				xsector[j].rxID				= xsector4.rxID;
				xsector[j].command			= xsector4.command;
				xsector[j].triggerOn		= xsector4.triggerOn;
				xsector[j].triggerOff		= xsector4.triggerOff;
				xsector[j].busyTime			= xsector4.busyTime;
				xsector[j].waitTime			= xsector4.waitTime;
				xsector[j].restState		= xsector4.restState;

				// lighting data
				xsector[j].amplitude		= xsector4.amplitude;
				xsector[j].freq				= xsector4.freq;
				xsector[j].wave				= xsector4.wave;
				xsector[j].shadeAlways		= xsector4.shadeAlways;
				xsector[j].phase			= xsector4.phase;
				xsector[j].shadeFloor		= xsector4.shadeFloor;
				xsector[j].shadeCeiling		= xsector4.shadeCeiling;
				xsector[j].shadeWalls		= xsector4.shadeWalls;
				xsector[j].shade			= xsector4.shade;

				// panning data
				xsector[j].panFloor			= xsector4.panFloor;
				xsector[j].panCeiling		= xsector4.panCeiling;
				xsector[j].panDrag			= xsector4.panDrag;
				xsector[j].panAlways		= xsector4.panAlways;

				// wind/water stuff
				xsector[j].underwater		= xsector4.underwater;
				xsector[j].depth			= xsector4.depth;
				xsector[j].panVel			= xsector4.panVel;
				xsector[j].panAngle			= xsector4.panAngle;
				xsector[j].wind				= xsector4.wind;

				// physical triggers
				xsector[j].decoupled		= xsector4.decoupled;
				xsector[j].triggerOnce		= xsector4.triggerOnce;
				xsector[j].isTriggered		= 0;
				xsector[j].key				= xsector4.key;
				xsector[j].triggerPush		= xsector4.triggerPush;
				xsector[j].triggerImpact 	= xsector4.triggerImpact;
				xsector[j].triggerExplode	= 0;
				xsector[j].triggerEnter		= xsector4.triggerEnter;
				xsector[j].triggerExit		= xsector4.triggerExit;
				xsector[j].triggerWPush		= xsector4.triggerWPush;
				xsector[j].triggerReserved1	= 0;
				xsector[j].triggerReserved2	= 0;

				// movement data
				xsector[j].offCeilZ			= xsector4.offCeilZ;
				xsector[j].onCeilZ			= xsector4.onCeilZ;
				xsector[j].offFloorZ		= xsector4.offFloorZ;
				xsector[j].onFloorZ     	= xsector4.onFloorZ;
				xsector[j].marker0			= sector[i].type;	// former lotag
				xsector[j].marker1			= sector[i].hitag;

				sector[i].type				= (ushort)xsector4.type;
			}
		}

		for (i = 0; i < numwalls; i++)
		{
			iob.Read(&wall[i], sizeof(WALL));
			iob.Skip(sizeof(BYTE));	// old surface data

			if (wall[i].extra > 0)
			{
				int j = wall[i].extra;

				XWALL4 xwall4;
				iob.Read(&xwall4, sizeof(XWALL4));
				memset(&xwall[j], 0, sizeof(XWALL));

				xwall[j].reference 			= xwall4.reference;
				xwall[j].state 				= xwall4.state;

				// trigger data
				xwall[j].busy 				= xwall4.busy;
				xwall[j].data 				= xwall4.data;
				xwall[j].txID 				= xwall4.txID;
				xwall[j].rxID 				= xwall4.rxID;
				xwall[j].command 			= xwall4.command;
				xwall[j].triggerOn 			= xwall4.triggerOn;
				xwall[j].triggerOff 		= xwall4.triggerOff;
				xwall[j].busyTime 			= 0;
				xwall[j].waitTime 			= xwall4.waitTime;
				xwall[j].restState 			= xwall4.restState;

				xwall[j].map 				= xwall4.map;

				// panning data
				xwall[j].panAlways 			= xwall4.panAlways;
				xwall[j].panXVel 			= xwall4.panXVel;
				xwall[j].panYVel 			= xwall4.panYVel;

				// physical triggers
				xwall[j].decoupled 			= xwall4.decoupled;
				xwall[j].triggerOnce 		= xwall4.triggerOnce;
				xwall[j].isTriggered 		= xwall4.isTriggered;
				xwall[j].key 				= xwall4.key;
				xwall[j].triggerPush 		= xwall4.triggerPush;
				xwall[j].triggerImpact 		= xwall4.triggerImpact;
				xwall[j].triggerExplode		= 0;

				wall[i].type				= (ushort)xwall4.type;
			}
		}

		for (i = 0; i < numsprites; i++)
		{
			iob.Read(&sprite[i], sizeof(SPRITE));
			iob.Skip(sizeof(BYTE));	// old surface data

			if (sprite[i].extra > 0)
			{
				int j = sprite[i].extra;

				XSPRITE4 xsprite4;
				iob.Read(&xsprite4, sizeof(XSPRITE4));
				memset(&xsprite[j], 0, sizeof(XSPRITE));

				xsprite[j].reference 		= xsprite4.reference;
				xsprite[j].state 			= xsprite4.state;

				// trigger data
				xsprite[j].busy				= 0;
				xsprite[j].txID 			= xsprite4.txID;
				xsprite[j].rxID				= xsprite4.rxID;
				xsprite[j].command			= xsprite4.command;
				xsprite[j].triggerOn		= xsprite4.triggerOn;
				xsprite[j].triggerOff		= xsprite4.triggerOff;
				xsprite[j].restState		= xsprite4.restState;
				xsprite[j].busyTime 		= 0;
				xsprite[j].waitTime			= xsprite4.waitTime;

				xsprite[j].difficulty		= xsprite4.difficulty;
				xsprite[j].map				= xsprite4.map;
				xsprite[j].soundKit			= xsprite4.soundKit;

				// physical triggers
				xsprite[j].decoupled		= xsprite4.decoupled;
				xsprite[j].triggerOnce		= xsprite4.triggerOnce;
				xsprite[j].isTriggered		= 0;
				xsprite[j].key				= xsprite4.key;
				xsprite[j].triggerPush		= xsprite4.triggerPush;
				xsprite[j].triggerImpact	= xsprite4.triggerImpact;
				xsprite[j].triggerExplode	= 0;
				xsprite[j].triggerPickup	= xsprite4.triggerPickup;
				xsprite[j].triggerTouch		= xsprite4.triggerTouch;
				xsprite[j].triggerSight		= xsprite4.triggerSight;
				xsprite[j].triggerProximity	= xsprite4.triggerProximity;

				xsprite[j].data1 			= xsprite4.data;
				xsprite[j].data2 			= sprite[i].type;	// former lotag
				xsprite[j].data3 			= sprite[i].mass;	// format hitag

				// respawn flags
				xsprite[j].respawn			= xsprite4.respawn;
				xsprite[j].respawnTime		= xsprite4.respawnTime;
				xsprite[j].launchMode		= xsprite4.launchMode;

				// this stuff needed for dudes (probably initialized dynamically)
				xsprite[j].moveState		= 0;
				xsprite[j].aiState			= 0;
				xsprite[j].health			= 0;
				xsprite[j].dudeDeaf			= 0;
				xsprite[j].dudeAmbush		= 0;
				xsprite[j].dudeFlag3		= 0;
				xsprite[j].dudeFlag4		= 0;
				xsprite[j].target			= 0;
				xsprite[j].targetX			= 0;
				xsprite[j].targetY			= 0;
				xsprite[j].targetZ			= 0;
				xsprite[j].avel				= 0;
				xsprite[j].weaponTimer		= 0;

				sprite[i].type				= (ushort)xsprite4.type;

				// remove the xsprite if not necessary
				if (sprite[i].type >= kWeaponItemBase && sprite[i].type < kDudeBase)
					sprite[i].extra = -1;
			}

			// offset old marker type
			if ( sprite[i].statnum == kStatMarker )
				sprite[i].type += kMarkerOff;

			// fix relocated item enums
			if ( sprite[i].type >= kAmmoMax && sprite[i].type <= kItemMax )
				sprite[i].type += kItemBase - kAmmoMax;

			// make sure we use origin alignment
			if ( !(sprite[i].cstat & kSpriteOriginAlign) )
			{
				sprite[i].cstat |= kSpriteOriginAlign;

				// adjust z position for alignment change
				if ( (sprite[i].cstat & kSpriteRMask) != kSpriteFloor )
				{
					int nTile = sprite[i].picnum;
					sprite[i].z -= (tilesizy[nTile] - tilesizy[nTile] / 2 - picanm[nTile].ycenter) * sprite[i].yrepeat << 2;
				}
			}
		}
	}
	else
		ThrowError("Error reading map file", ES_ERROR);

 	free(buffer);

	// add up all the elements in the file to determine the size of the buffer
	length = 0;
	length += sizeof(HEADER5);
	length += sizeof(INFO5);
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

	length += numsprites * sizeof(SPRITE);
	for (i = 0; i < numsprites; i++)
	{
		if (sprite[i].extra > 0)
			length += sizeof(XSPRITE);
	}
	length += 4;	// CRC

	buffer = (char *)malloc(length);
	IOBuffer iob(buffer, length);

	memcpy(header5.signature, kBloodMapSig, sizeof(header5.signature));
	header5.version = kBloodMapVersion;

	iob.Write(&header5, sizeof(header5));

	info5.x = info4.x;
	info5.y = info4.y;
	info5.z = info4.z;
	info5.angle = info4.angle;
	info5.sector = info4.sector;
	info5.pskybits = info4.pskybits;
	info5.visibility = 	info4.visibility;
	info5.songId = info4.songId;
	info5.parallax = info4.parallax;
	info5.mapRevisions = info4.mapRevisions;
	info5.numsectors = numsectors;
	info5.numwalls = numwalls;
	info5.numsprites = numsprites;

	iob.Write(&info5, sizeof(INFO5));
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

	for (i = 0; i < numsprites; i++)
	{
		iob.Write(&sprite[i], sizeof(SPRITE));
		if (sprite[i].extra > 0)
			iob.Write(&xsprite[sprite[i].extra], sizeof(XSPRITE));
	}

	gMapCRC = CRC32(buffer, length - sizeof(gMapCRC));
	iob.Write(&gMapCRC, sizeof(gMapCRC));

	// backup the map file
	strcpy(bakfilename, filename);
	ChangeExtension(filename, "MAP");
	ChangeExtension(bakfilename, "MA4");
	rename(filename, bakfilename);

	ChangeExtension(filename, "MAP");
	printf("=> %s ", filename);
	hFile = open(filename, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, S_IWUSR);
	if ( hFile == -1 )
		ThrowError("Error opening MAP file", ES_ERROR);

	if (write(hFile, buffer, length) != length)
		ThrowError("Error writing MAP file", ES_ERROR);

	close(hFile);
	free(buffer);

	printf("OK.\n");
}


void InsertFilename( char *fname )
{
	FNODE *n = (FNODE *)malloc(sizeof(FNODE) + strlen(fname));
	strcpy(n->name, fname);

	// insert the node at the tail, so it stays in order
	n->next = tail->next;
	tail->next = n;
	tail = n;
}


void ProcessArgument(char *s)
{
	char filespec[_MAX_PATH];
	char buffer[_MAX_PATH2];
	char path[_MAX_PATH];

	strcpy(filespec, s);
	ChangeExtension(filespec, ".MAP");

	char *drive, *dir;

	// separate the path from the filespec
	_splitpath2(s, buffer, &drive, &dir, NULL, NULL);
	_makepath(path, drive, dir, NULL, NULL);

	struct find_t fileinfo;

	unsigned r = _dos_findfirst(s, _A_NORMAL, &fileinfo);
	if (r != 0)
		printf("%s not found\n", s);

	while ( r == 0 )
	{
		strcpy(filespec, path);
		strcat(filespec, fileinfo.name);
		InsertFilename(filespec);

		r = _dos_findnext( &fileinfo );
	}

	_dos_findclose(&fileinfo);
}


/***********************************************************************
 * Process command line arguments
 **********************************************************************/
void ParseOptions( int argc, char *argv[])
{
	int c;
	while ( (c = GetOptions(argc, argv, "")) != GO_EOF ) {
		switch (c) {
			case GO_INVALID:
				QuitMessage("Invalid argument: %s", OptArgument);

			case GO_FULL:
				ProcessArgument(OptArgument);
				break;
		}
	}
}


void main(int argc, char *argv[])
{
	printf("Blood Map 5 Converter    Copyright (c) 1995 Q Studios Corporation\n");

	if (argc < 2) ShowUsage();

	artInit();	// get tile size info

	ParseOptions(argc, argv);

	// process the file list
	for (FNODE *n = head.next; n != &head; n = n->next)
		ProcessFile(n->name);
}



