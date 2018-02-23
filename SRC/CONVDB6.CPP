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
#include "trig.h"

#include <memcheck.h>

#define kBloodMapSig		"BLM\x1A"
#define kBloodMapVersion	0x0600
#define kBloodMapOldVersion	0x0503

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

ulong gMapCRC;
int   gSkyCount;
short numsectors, numwalls;

struct HEADER5
{
	char signature[4];
	short version;
};

struct HEADER6
{
	char signature[4];
	short version;
};

// version 5.x header
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

struct INFO6
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
struct SPRITE5
{
	long x;
	long y;
	long z;
	ushort cstat;
	schar shade;
	uchar pal;
	uchar clipdist;
	uchar xrepeat;
	uchar yrepeat;
	schar xoffset;
	schar yoffset;
	sshort picnum;
	sshort ang;
	sshort xvel;
	sshort yvel;
	sshort zvel;
	sshort owner;
	sshort sectnum;
	sshort statnum;
	ushort type;
	ushort mass;
	sshort extra;
};

struct WALL5
{
	long x;
	long y;
	ushort point2;
	short nextsector;
	short nextwall;
	short picnum;
	short overpicnum;
	signed char shade;
	char pal;
	short cstat;
	uchar xrepeat;
	uchar yrepeat;
	uchar xpanning;
	uchar ypanning;
	ushort type;
	short hitag;
	short extra;
};

struct SECTOR5
{
	unsigned short wallptr;
	unsigned short wallnum;
	short ceilingpicnum;
	short floorpicnum;
	short ceilingslope;
	short floorslope;
	long ceilingz;
	long floorz;
	schar ceilingshade;
	schar floorshade;
	uchar ceilingxpanning;
	uchar floorxpanning;
	uchar ceilingypanning;
	uchar floorypanning;
	uchar ceilingstat;
	uchar floorstat;
	uchar ceilingpal;
	uchar floorpal;
	uchar visibility;
	ushort type;
	short hitag;
	short extra;
};

struct XSPRITE5
{
	signed reference :			14;
	unsigned state :			1;

	// trigger data
	unsigned busy :				17;
	unsigned txID :				10;
	unsigned rxID :				10;
	unsigned command : 			8;
	unsigned triggerOn : 		1;
	unsigned triggerOff : 		1;
	unsigned busyTime :		   12; // time to reach next state
	unsigned waitTime : 	   12; // delay before callback
	unsigned restState :		1; // state to return to on callback

	unsigned difficulty :		2;
	unsigned map : 				2;
	unsigned soundKit :			8;

	// physical triggers
	unsigned decoupled :		1;
	unsigned triggerOnce :		1;
	unsigned isTriggered :      1; // used for triggerOnce objects
	unsigned key :				3;
	unsigned triggerPush :      1; // action key
	unsigned triggerImpact :    1; // vector hits
	unsigned triggerExplode :   1; // exploding things, etc.
	unsigned triggerPickup :    1; // secrets
	unsigned triggerTouch :     1; // sawblades, spikes, zaps?
	unsigned triggerSight :     1; // dunno, yet.
	unsigned triggerProximity : 1; // proximity bombs
	unsigned triggerReserved1 :	1;
	unsigned triggerReserved2 :	1;

	signed data1 :				16;	// combo value?
	signed data2 :				16;	// combo key?
	signed data3 :				16;	// combo max?

	unsigned respawn :			2; // 0=optional never, 1=optional always, 2=always, 3=never
	unsigned respawnTime :	   12; // 0=permanent, >0=time in tenths of a second,
	unsigned launchMode :		2; // 0=all, 1=bloodbath, 2=ally, 3=ally&bloodbath,

	// this stuff needed for dudes
	unsigned moveState :		8;	// same as player move states
	unsigned aiState :			8;
	signed health :				12;
	unsigned dudeDeaf :			1;
	unsigned dudeAmbush :		1;
	unsigned dudeFlag3 :		1;
	unsigned dudeFlag4 :		1;
	signed target :				16;
	signed targetX :			32;
	signed targetY :			32;
	signed targetZ :			32;
	signed avel :				16;
	unsigned weaponTimer :		16;
	unsigned stateTimer :		16;

	signed support :			16;		// will be used by physics system

	unsigned interruptable :	1;

	unsigned pad : 31;
};

struct XWALL5
{
	signed reference :			14;
	unsigned state : 			1;

	// trigger data
	unsigned busy : 			17;
	signed data :				16;
	unsigned txID :				10;
	unsigned rxID :				10;
	unsigned command : 			8;
	unsigned triggerOn : 		1;
	unsigned triggerOff : 		1;
	unsigned busyTime :		   12; // time to reach next state
	unsigned waitTime : 	   12; // delay before callback
	unsigned restState :		1; // state to return to on callback

	unsigned map :				2;

	// panning data
	unsigned panAlways :		1;
	signed panXVel :			8;
	signed panYVel :			8;

	// physical triggers
	unsigned decoupled :		1;
	unsigned triggerOnce :		1;
	unsigned isTriggered :   	1; // used for triggerOnce objects
	unsigned key :				3;
	unsigned triggerPush :   	1;
	unsigned triggerImpact : 	1;
	unsigned triggerExplode :   1; // exploding things, etc.
	unsigned triggerReserved1 :	1;
	unsigned triggerReserved2 :	1;

	unsigned xpanFrac :			8;
	unsigned ypanFrac :			8;

	unsigned interruptable :	1;

	unsigned pad1 : 15;
	unsigned pad2 : 32;
};

struct XSECTOR5
{
	signed reference :			14;
	unsigned state : 			1;

	// trigger data
	unsigned busy : 			17;
	signed data :				16;
	unsigned txID :				10;
	unsigned rxID :				10;
	unsigned command : 			8;
	unsigned triggerOn : 		1;
	unsigned triggerOff : 		1;
	unsigned busyTime :		   12; // time to reach next state
	unsigned waitTime : 	   12; // delay before callback
	unsigned restState :		1; // state to return to on callback

	// lighting data
	unsigned shadeAlways : 		1;
	signed amplitude :			8;
	unsigned freq :				8;
	unsigned wave :				4;
	unsigned phase :			8;
	unsigned shadeFloor :		1;
	unsigned shadeCeiling : 	1;
	unsigned shadeWalls :		1;
	signed shade :				8;

	// panning data
	unsigned panAlways :		1;
	unsigned panFloor :			1;
	unsigned panCeiling :		1;
	unsigned drag :				1;

	// wind/water stuff
	unsigned underwater :		1;
	unsigned depth :			2;
	unsigned panVel :      		8;
	unsigned panAngle :			11;
	unsigned wind :		    	1;

	// physical triggers
	unsigned decoupled :		1;
	unsigned triggerOnce :		1;
	unsigned isTriggered :		1;
	unsigned key :				3;
	unsigned triggerPush :		1;
	unsigned triggerImpact :	1;
	unsigned triggerExplode :   1; // exploding things, etc.
	unsigned triggerEnter :		1;
	unsigned triggerExit :		1;
	unsigned triggerWPush :		1;
	unsigned triggerReserved1 :	1;
	unsigned triggerReserved2 :	1;

	// movement data
	signed offCeilZ :			32;
	signed onCeilZ :			32;
	signed offFloorZ :			32;
	signed onFloorZ :			32;
	signed marker0 :			16;
	signed marker1 :			16;

//	unsigned crush :			1;

	unsigned ceilxpanFrac :		8;
	unsigned ceilypanFrac :		8;
	unsigned floorxpanFrac :	8;
	unsigned floorypanFrac :	8;

	unsigned interruptable :	1;

	unsigned pad1 : 31;
};


void ShowUsage(void)
{
	printf("Usage:\n");
	printf("  CONVDB6 map1 map2 (wild cards ok)\n");
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
	HEADER5 header5;
	HEADER6 header6;
	INFO5 info5;
	INFO6 info6;
	char filename[_MAX_PATH], bakfilename[_MAX_PATH];
	short numsprites;
	int length;

	BYTE *buffer;

	strcpy(filename, filespec);
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

		iob.Read(&header5, sizeof(HEADER5));

		// check signature and major version number
		if ( (memcmp(header5.signature, kBloodMapSig, sizeof(header5.signature)) != 0)
			|| ((header5.version >> 8) != (kBloodMapOldVersion >> 8)) )
		{
			printf("not expected format, skipping\n");
			return;
		}

		iob.Read(&info5, sizeof(INFO5));
		numsectors = info5.numsectors;
		numwalls = info5.numwalls;
		numsprites = info5.numsprites;

		gSkyCount = 1 << info5.pskybits;
		iob.Read(pskyoff, gSkyCount * sizeof(pskyoff[0]));

		for (i = 0; i < numsectors; i++)
		{
			SECTOR5 sector5;
			iob.Read(&sector5, sizeof(sector5));
			sector[i].wallptr			= sector5.wallptr;
			sector[i].wallnum			= sector5.wallnum;
			sector[i].ceilingz			= sector5.ceilingz;
			sector[i].floorz			= sector5.floorz;
			sector[i].ceilingstat		= sector5.ceilingstat;
			sector[i].floorstat			= sector5.floorstat;
			sector[i].ceilingpicnum		= sector5.ceilingpicnum;
			sector[i].ceilingslope		= (sshort)(sector5.ceilingslope << 5);
			sector[i].ceilingshade		= sector5.ceilingshade;
			sector[i].ceilingpal		= sector5.ceilingpal;
			sector[i].ceilingxpanning	= sector5.ceilingxpanning;
			sector[i].ceilingypanning	= sector5.ceilingypanning;
			sector[i].floorpicnum		= sector5.floorpicnum;
			sector[i].floorslope		= (sshort)(sector5.floorslope << 5);
			sector[i].floorshade		= sector5.floorshade;
			sector[i].floorpal			= sector5.floorpal;
			sector[i].floorxpanning		= sector5.floorxpanning;
			sector[i].floorypanning		= sector5.floorypanning;
			sector[i].visibility		= sector5.visibility;
			sector[i].filler			= 0;
			sector[i].type				= sector5.type;
			sector[i].hitag				= sector5.hitag;
			sector[i].extra				= sector5.extra;

			if ( sector[i].ceilingslope == 0 )
				sector[i].ceilingstat &= ~kSectorSloped;

			if ( sector[i].floorslope == 0 )
				sector[i].floorstat &= ~kSectorSloped;

			if (sector[i].extra > 0)
			{
				int j = sector[i].extra;

				XSECTOR5 xsector5;
				iob.Read(&xsector5, sizeof(XSECTOR5));
				memset(&xsector[j], 0, sizeof(XSECTOR));

				xsector[j].reference 		= xsector5.reference;
				xsector[j].state 			= xsector5.state;

				// trigger data
				xsector[j].busy				= 0;
				xsector[j].data				= xsector5.data;
				xsector[j].txID				= xsector5.txID;
				xsector[j].rxID				= xsector5.rxID;
				xsector[j].command			= xsector5.command;
				xsector[j].triggerOn		= xsector5.triggerOn;
				xsector[j].triggerOff		= xsector5.triggerOff;
				xsector[j].busyTime			= xsector5.busyTime;
				xsector[j].waitTime			= xsector5.waitTime;
				xsector[j].restState		= xsector5.restState;
				xsector[j].interruptable	= xsector5.interruptable;

				// lighting data
				xsector[j].amplitude		= xsector5.amplitude;
				xsector[j].freq				= xsector5.freq;
				xsector[j].phase			= xsector5.phase;
				xsector[j].wave				= xsector5.wave;
				xsector[j].shadeAlways		= xsector5.shadeAlways;
				xsector[j].shadeFloor		= xsector5.shadeFloor;
				xsector[j].shadeCeiling		= xsector5.shadeCeiling;
				xsector[j].shadeWalls		= xsector5.shadeWalls;
				xsector[j].shade			= xsector5.shade;

				// panning data
				xsector[j].panAlways		= xsector5.panAlways;
				xsector[j].panFloor			= xsector5.panFloor;
				xsector[j].panCeiling		= xsector5.panCeiling;
				xsector[j].drag				= xsector5.drag;

				// wind/water stuff
				xsector[j].underwater		= xsector5.underwater;
				xsector[j].depth			= xsector5.depth;
				xsector[j].panVel			= xsector5.panVel;
				xsector[j].panAngle			= xsector5.panAngle;
				xsector[j].wind				= xsector5.wind;

				// physical triggers
				xsector[j].decoupled		= xsector5.decoupled;
				xsector[j].triggerOnce		= xsector5.triggerOnce;
				xsector[j].isTriggered		= xsector5.isTriggered;
				xsector[j].key				= xsector5.key;
				xsector[j].triggerPush		= xsector5.triggerPush;
				xsector[j].triggerImpact 	= xsector5.triggerImpact;
				xsector[j].triggerExplode	= xsector5.triggerExplode;
				xsector[j].triggerEnter		= xsector5.triggerEnter;
				xsector[j].triggerExit		= xsector5.triggerExit;
				xsector[j].triggerWPush		= xsector5.triggerWPush;
				xsector[j].triggerReserved1	= xsector5.triggerReserved1;
				xsector[j].triggerReserved2	= xsector5.triggerReserved2;

				// movement data
				xsector[j].offCeilZ			= xsector5.offCeilZ;
				xsector[j].onCeilZ			= xsector5.onCeilZ;
				xsector[j].offFloorZ		= xsector5.offFloorZ;
				xsector[j].onFloorZ     	= xsector5.onFloorZ;
				xsector[j].marker0			= xsector5.marker0;
				xsector[j].marker1			= xsector5.marker1;
				xsector[j].crush			= 0;
			}
		}

		for (i = 0; i < numwalls; i++)
		{
			WALL5 wall5;
			iob.Read(&wall5, sizeof(wall5));
			wall[i].x					= wall5.x;
			wall[i].y					= wall5.y;
			wall[i].point2				= wall5.point2;
			wall[i].nextwall			= wall5.nextwall;
			wall[i].nextsector			= wall5.nextsector;
			wall[i].cstat				= wall5.cstat;
			wall[i].picnum				= wall5.picnum;
			wall[i].overpicnum			= wall5.overpicnum;
			wall[i].shade				= wall5.shade;
			wall[i].pal					= wall5.pal;
			wall[i].xrepeat				= wall5.xrepeat;
			wall[i].yrepeat				= wall5.yrepeat;
			wall[i].xpanning			= wall5.xpanning;
			wall[i].ypanning			= wall5.ypanning;
			wall[i].type				= wall5.type;
			wall[i].hitag				= wall5.hitag;
			wall[i].extra				= wall5.extra;

			if (wall[i].extra > 0)
			{
				int j = wall[i].extra;

				XWALL5 xwall5;
				iob.Read(&xwall5, sizeof(XWALL5));
				memset(&xwall[j], 0, sizeof(XWALL));

				xwall[j].reference 			= xwall5.reference;
				xwall[j].state 				= xwall5.state;

				// trigger data
				xwall[j].busy 				= xwall5.busy;
				xwall[j].data 				= xwall5.data;
				xwall[j].txID 				= xwall5.txID;
				xwall[j].rxID 				= xwall5.rxID;
				xwall[j].command 			= xwall5.command;
				xwall[j].triggerOn 			= xwall5.triggerOn;
				xwall[j].triggerOff 		= xwall5.triggerOff;
				xwall[j].busyTime 			= xwall5.busyTime;
				xwall[j].waitTime 			= xwall5.waitTime;
				xwall[j].restState 			= xwall5.restState;
				xwall[j].interruptable		= xwall5.interruptable;

				// panning data
				xwall[j].panAlways 			= xwall5.panAlways;
				xwall[j].panXVel 			= xwall5.panXVel;
				xwall[j].panYVel 			= xwall5.panYVel;

				// physical triggers
				xwall[j].decoupled 			= xwall5.decoupled;
				xwall[j].triggerOnce 		= xwall5.triggerOnce;
				xwall[j].isTriggered 		= xwall5.isTriggered;
				xwall[j].key 				= xwall5.key;
				xwall[j].triggerPush 		= xwall5.triggerPush;
				xwall[j].triggerImpact 		= xwall5.triggerImpact;
				xwall[j].triggerExplode		= xwall5.triggerExplode;
			}
		}

		for (i = 0; i < numsprites; i++)
		{
			SPRITE5 sprite5;
			iob.Read(&sprite5, sizeof(sprite5));
			sprite[i].x					= sprite5.x;
			sprite[i].y					= sprite5.y;
			sprite[i].z					= sprite5.z;
			sprite[i].cstat				= sprite5.cstat;
			sprite[i].picnum			= sprite5.picnum;
			sprite[i].shade				= sprite5.shade;
			sprite[i].pal				= sprite5.pal;
			sprite[i].clipdist			= sprite5.clipdist;
			sprite[i].filler			= 0;
			sprite[i].xrepeat			= sprite5.xrepeat;
			sprite[i].yrepeat			= sprite5.yrepeat;
			sprite[i].xoffset			= sprite5.xoffset;
			sprite[i].yoffset			= sprite5.yoffset;
			sprite[i].sectnum			= sprite5.sectnum;
			sprite[i].statnum			= sprite5.statnum;
			sprite[i].ang				= sprite5.ang;
			sprite[i].owner				= sprite5.owner;
			sprite[i].xvel				= sprite5.xvel;
			sprite[i].yvel				= sprite5.yvel;
			sprite[i].zvel				= sprite5.zvel;
			sprite[i].type				= sprite5.type;
			sprite[i].flags				= 0;	// sprite5.mass;
			sprite[i].extra				= sprite5.extra;

			if (sprite[i].extra > 0)
			{
				int j = sprite[i].extra;

				XSPRITE5 xsprite5;
				iob.Read(&xsprite5, sizeof(XSPRITE5));
				memset(&xsprite[j], 0, sizeof(XSPRITE));

				xsprite[j].reference 		= xsprite5.reference;
				xsprite[j].state 			= xsprite5.state;

				// trigger data
				xsprite[j].busy				= 0;
				xsprite[j].txID 			= xsprite5.txID;
				xsprite[j].rxID				= xsprite5.rxID;
				xsprite[j].command			= xsprite5.command;
				xsprite[j].triggerOn		= xsprite5.triggerOn;
				xsprite[j].triggerOff		= xsprite5.triggerOff;
				xsprite[j].busyTime 		= xsprite5.busyTime;
				xsprite[j].waitTime			= xsprite5.waitTime;
				xsprite[j].restState		= xsprite5.restState;
				xsprite[j].interruptable	= xsprite5.interruptable;

				xsprite[j].difficulty		= xsprite5.difficulty;
				xsprite[j].soundKit			= xsprite5.soundKit;

				// physical triggers
				xsprite[j].decoupled		= xsprite5.decoupled;
				xsprite[j].triggerOnce		= xsprite5.triggerOnce;
				xsprite[j].isTriggered		= 0;
				xsprite[j].key				= xsprite5.key;
				xsprite[j].triggerPush		= xsprite5.triggerPush;
				xsprite[j].triggerImpact	= xsprite5.triggerImpact;
				xsprite[j].triggerExplode	= xsprite5.triggerExplode;
				xsprite[j].triggerPickup	= xsprite5.triggerPickup;
				xsprite[j].triggerTouch		= xsprite5.triggerTouch;
				xsprite[j].triggerSight		= xsprite5.triggerSight;
				xsprite[j].triggerProximity	= xsprite5.triggerProximity;

				xsprite[j].data1 			= xsprite5.data1;
				xsprite[j].data2 			= xsprite5.data2;
				xsprite[j].data3 			= xsprite5.data3;

				xsprite[j].support 			= xsprite5.support;

				// respawn flags
				xsprite[j].respawn			= xsprite5.respawn;
				xsprite[j].respawnTime		= xsprite5.respawnTime;
				xsprite[j].launchMode		= xsprite5.launchMode;

				// this stuff needed for dudes (probably initialized dynamically)
				xsprite[j].moveState		= 0;
				xsprite[j].aiState			= 0;
				xsprite[j].health			= 0;
				xsprite[j].dudeDeaf			= 0;
				xsprite[j].dudeAmbush		= 0;
				xsprite[j].dudeGuard		= 0;
				xsprite[j].dudeFlag4		= 0;
				xsprite[j].target			= 0;
				xsprite[j].targetX			= 0;
				xsprite[j].targetY			= 0;
				xsprite[j].targetZ			= 0;
				xsprite[j].avel				= 0;
				xsprite[j].stateTimer		= 0;
			}
		}
	}
	else
		ThrowError("Error reading map file", ES_ERROR);

 	free(buffer);

	// convert slide types and angle markers for maps before 5.03
	if ( (header5.version & 0xFF) < 3 )
	{
		for ( i = 0; i < kMaxSectors; i++)
		{
			if ( sector[i].type == kSectorSlide )
			{
				sector[i].type = kSectorSlideMarked;
			}
		}

		for ( i = 0; i < kMaxSprites; i++)
			if ( sprite[i].statnum == kStatMarker )
			{
				if ( sprite[i].ang >= kAngle180 )
					sprite[i].ang -= kAngle360;
			}
	}

	// add up all the elements in the file to determine the size of the buffer
	length = 0;
	length += sizeof(HEADER6);
	length += sizeof(INFO6);
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

	buffer = (BYTE *)malloc(length);
	IOBuffer iob(buffer, length);

	memcpy(header6.signature, kBloodMapSig, sizeof(header6.signature));
	header6.version = kBloodMapVersion;

	iob.Write(&header6, sizeof(header6));

	info6.x = info5.x;
	info6.y = info5.y;
	info6.z = info5.z;
	info6.angle = info5.angle;
	info6.sector = info5.sector;
	info6.pskybits = info5.pskybits;
	info6.visibility = 	info5.visibility;
	info6.songId = info5.songId;
	info6.parallax = info5.parallax;
	info6.mapRevisions = info5.mapRevisions;
	info6.numsectors = numsectors;
	info6.numwalls = numwalls;
	info6.numsprites = numsprites;

	iob.Write(&info6, sizeof(info6));
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
	ChangeExtension(bakfilename, "MA5");
	rename(filename, bakfilename);

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
	AddExtension(filespec, ".MAP");

	char *drive, *dir;

	// separate the path from the filespec
	_splitpath2(filespec, buffer, &drive, &dir, NULL, NULL);
	_makepath(path, drive, dir, NULL, NULL);

	struct find_t fileinfo;

	unsigned r = _dos_findfirst(filespec, _A_NORMAL, &fileinfo);
	if (r != 0)
		printf("%s not found\n", filespec);

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
void ParseOptions( void )
{
	static SWITCH switches[] = { { NULL, 0, FALSE } };
	int r;
	while ( (r = GetOptions(switches)) != GO_EOF ) {
		switch (r)
		{
			case GO_INVALID:
				QuitMessage("Invalid argument: %s", OptArgument);

			case GO_FULL:
				ProcessArgument(OptArgument);
				break;
		}
	}
}


void main(int argc)
{
	printf("Blood Map 6 Converter    Copyright (c) 1995 Q Studios Corporation\n");

	if (argc < 2) ShowUsage();

	ParseOptions();

	// process the file list
	for (FNODE *n = head.next; n != &head; n = n->next)
		ProcessFile(n->name);
}



