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

#include <memcheck.h>

/***********************************************************************
 * Structures and typedefs
 **********************************************************************/
struct XSPRITE4
{
	signed reference :			16;
	unsigned type :				10;

	// trigger data
	unsigned state :			1;
	unsigned busy :				16;
	unsigned txID :				10;
	unsigned rxID :				10;
	unsigned command : 			3;
	unsigned triggerOn : 		1;
	unsigned triggerOff : 		1;
	unsigned key :				3;
	signed data :				16;

	unsigned restState :		1; // state to return to on callback
	unsigned waitTime : 		8;

	unsigned triggerPush :      1; // gates?
	unsigned triggerImpact :    1; // exploding things, etc.
	unsigned triggerPickup :    1; // secrets
	unsigned triggerTouch :     1; // sawblades, spikes, zaps?
	unsigned triggerSight :     1; // dunno, yet.
	unsigned triggerProximity : 1; // proximity bombs
	unsigned decoupled :		1;
	unsigned triggerOnce :		1;
	unsigned isTriggered :      1; // used for triggerOnce objects

	unsigned difficulty :		2;
	unsigned detail :			2;
	unsigned view :				4;
	unsigned soundKit :			8;

	unsigned map : 				2;
	unsigned permanent :		1; // 0=not permanent, 1=permanent (respawn ignored)
	unsigned respawn :			2; // 0=never, 1=always, 2=optional never, 3=optional always
	unsigned respawnTime :		8; // 0=instant, >0=time in tenths of a second,
	unsigned launchMode :		2; // 0=all, 1=bloodbath, 2=ally, 3=ally&bloodbath,
};

struct XWALL4
{
	signed reference :			16;
	unsigned type :				10;

	// trigger data
	unsigned state : 			1;
	unsigned busy : 			16;
	unsigned txID :				10;
	unsigned rxID :				10;
	unsigned command : 			3;
	unsigned triggerOn : 		1;
	unsigned triggerOff : 		1;
	unsigned key :				3;
	signed data :				16;

	unsigned waitTime : 		8; // delay before callback
	unsigned restState :		1; // state to return to on callback

	unsigned triggerPush :   	1;
	unsigned triggerImpact : 	1;
	unsigned triggerRsvd1 :   	1;
	unsigned triggerRsvd2 : 	1;
	unsigned decoupled :		1;
	unsigned triggerOnce :		1;
	unsigned isTriggered :   	1; // used for triggerOnce objects

	// panning data
	signed panXVel :			8;
	signed panYVel :			8;
	unsigned panAlways :		1;

	unsigned map :				2;
};

struct XSECTOR4
{
	signed reference :			16;
	unsigned type :				10;

	// trigger data
	unsigned state : 			1;
	unsigned busy : 			16;
	unsigned txID :				10;
	unsigned rxID :				10;
	unsigned command : 			3;
	unsigned triggerOn : 		1;
	unsigned triggerOff : 		1;
	unsigned key :				3;
	signed data :				16;

	unsigned busyTime :			8; // time to reach next state
	unsigned waitTime : 		8; // delay before callback
	unsigned restState :		1; // state to return to on callback

	unsigned triggerPush :		1;
	unsigned triggerImpact :	1;
	unsigned triggerEnter :		1;
	unsigned triggerExit :		1;
	unsigned triggerWPush :		1;
	unsigned decoupled :		1;
	unsigned triggerOnce :		1;
	unsigned isTriggered :		1;

	// lighting data
	signed shade :				8;
	signed amplitude :			8;
	unsigned freq :				8;
	unsigned phase :			8;
	unsigned wave :				4;
	unsigned shadeFloor :		1;
	unsigned shadeCeiling : 	1;
	unsigned shadeWalls :		1;
	unsigned shadeAlways : 		1;

	// panning data
	unsigned panFloor :			1;
	unsigned panCeiling :		1;
	unsigned Drag :				1;
	unsigned panAlways :		1;

	// wind/water stuff
	unsigned depth :			2;
	unsigned panVel :      		8;
	unsigned panAngle :			11;
	unsigned underwater :		1;
	unsigned wind :		    	1;

	signed ceilingOffZ :		32;
	signed ceilingOnZ :			32;
	signed floorOffZ :			32;
	signed floorOnZ :			32;
};


#define kBloodMapSig		"BLM\x1A"
#define kBloodMapVersion	0x0400

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
XSECTOR4 	xsector4[kMaxXSectors];
XWALL4 		xwall4[kMaxXWalls];
XSPRITE4 	xsprite4[kMaxXSprites];

uchar		spriteSurf[kMaxSprites];
uchar 		wallSurf[kMaxWalls];
uchar 		floorSurf[kMaxSectors];
uchar 		ceilingSurf[kMaxSectors];
short 		pskyoff[kMaxSkyTiles];

short pskybits;
char  gMapAuthor[64] = "";
char  gMapName[64] = "";
ulong gMapCRC;
int   gSkyCount;
short numsectors, numwalls;

struct IOBuffer
{
	int remain;
	char *p;
	IOBuffer( char *buffer, int l ) : p(buffer), remain(l) {};
	void Read(void *, int, int);
	void Write(void *, int, int);
};

void IOBuffer::Read(void *s, int size, int nError)
{
	if (size <= remain)
	{
		memcpy(s, p, size);
		remain -= size;
		p += size;
	}
	else
		ThrowError("Read buffer overflow", ES_ERROR);
}

void IOBuffer::Write(void *s, int size, int nError)
{
	if (size <= remain)
	{
		memcpy(p, s, size);
		remain -= size;
		p += size;
	}
	else
		ThrowError("Write buffer overflow", ES_ERROR);
}

struct HEADER4
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


void ShowUsage(void)
{
	printf("Usage:\n");
	printf("  MAPINFO map1 map2 (wild cards ok)\n");
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
	HEADER3 header3;
	HEADER4 header4;
	INFO3 info3;
	INFO4 info4;
	char filename[_MAX_PATH], bakfilename[_MAX_PATH];
	short nSector, numsprites;
	int length;

	char *buffer;

	strcpy(filename, filespec);
	printf("%s ", filename);

	hFile = open(filename, O_RDONLY | O_BINARY);
	if ( hFile == -1 )
		ThrowError( "unable to open MAP file", ES_ERROR);

	length = filelength(hFile);
	buffer = (char *)malloc(length);
	if (read(hFile, buffer, length) == length)
	{
		close(hFile);

		IOBuffer iob(buffer, length);

		iob.Read(&header3, sizeof(HEADER3), 1);

		// check signature and major version number
		if ( strncmp(header3.signature, kBloodMapSig, sizeof(header3.signature))
			|| header3.version & 0xFF00 != kBloodMapOldVersion & 0xFF00 )
		{
			printf("not expected format, skipping\n");
			return;
		}

		iob.Read(&info3, sizeof(INFO3), 2);
		numsectors = info3.numsectors;
		numwalls = info3.numwalls;
		numsprites = info3.numsprites;

		iob.Read(gMapName, info3.lenName + 1, 3);
		iob.Read(gMapAuthor, info3.lenAuthor + 1, 4);

		pskybits = info3.pskybits;
		gSkyCount = 1 << pskybits;
		iob.Read(pskyoff, gSkyCount * sizeof(pskyoff[0]), 5);

		for (i = 0; i < numsectors; i++)
		{
			iob.Read(&sector[i], sizeof(SECTOR), 6);
			iob.Read(&ceilingSurf[i], sizeof(ceilingSurf[i]), 7);
			iob.Read(&floorSurf[i], sizeof(floorSurf[i]), 8);

			// expand sector shade range
			sector[i].ceilingshade = (schar)ClipRange(sector[i].ceilingshade << 1, -128, 127);
			sector[i].floorshade = (schar)ClipRange(sector[i].floorshade << 1, -128, 127);

			if (sector[i].extra > 0)
			{
				XSECTOR3 xsector3;
				iob.Read(&xsector3, sizeof(XSECTOR3), 9);

				memset(&xsector4[i], 0, sizeof(XSECTOR));
				xsector4[i].reference		= xsector3.reference;
				xsector4[i].state			= xsector3.state;
				xsector4[i].busy			= 0;
				xsector4[i].type			= xsector3.type;
				xsector4[i].data			= xsector3.data;
				xsector4[i].txID			= xsector3.txID;
				xsector4[i].rxID			= xsector3.rxID;
				xsector4[i].command			= xsector3.command;
				xsector4[i].triggerOn		= xsector3.triggerOn;
				xsector4[i].triggerOff		= xsector3.triggerOff;
				xsector4[i].triggerOnce		= xsector3.triggerOnce;
				xsector4[i].restState		= xsector3.restState;
				xsector4[i].key				= xsector3.key;
				xsector4[i].amplitude		= ClipRange(xsector3.amplitude << 1, -128, 127);
				xsector4[i].freq			= xsector3.freq;
				xsector4[i].wave			= xsector3.wave;
				xsector4[i].shadeAlways		= xsector3.shadeAlways;
				xsector4[i].phase			= xsector3.phase;
				xsector4[i].shadeFloor		= xsector3.shadeFloor;
				xsector4[i].shadeCeiling	= xsector3.shadeCeiling;
				xsector4[i].shadeWalls		= xsector3.shadeWalls;
				xsector4[i].shade			= xsector3.shade;
				xsector4[i].panFloor		= xsector3.panFloor;
				xsector4[i].panCeiling		= xsector3.panCeiling;
				xsector4[i].panDrag			= xsector3.panDrag;
				xsector4[i].waitTime		= xsector3.waitTime;
				xsector4[i].panAlways		= xsector3.panAlways;
				xsector4[i].busyTime		= xsector3.busyTime;
				xsector4[i].underwater		= xsector3.underwater;
				xsector4[i].depth			= xsector3.depth;
				xsector4[i].panVel			= xsector3.panVel;
				xsector4[i].panAngle		= xsector3.panAngle;
				xsector4[i].wind			= xsector3.wind ? 1 : 0;
				xsector4[i].isTriggered		= 0;
				xsector4[i].triggerPush		= xsector3.triggerPush;
				xsector4[i].triggerImpact	= xsector3.triggerImpact;
				xsector4[i].triggerEnter	= xsector3.triggerEnter;
				xsector4[i].triggerExit		= xsector3.triggerExit;
				xsector4[i].triggerWPush	= xsector3.triggerWPush;
				xsector4[i].decoupled		= xsector3.decoupled;

				// zero all onZ offZ values
				xsector4[i].ceilingOffZ	= 0;
				xsector4[i].ceilingOnZ   = 0;
				xsector4[i].floorOffZ	= 0;
				xsector4[i].floorOnZ     = 0;

				switch( xsector4[i].type )
				{
					case kSectorTopDoor:
						xsector4[i].ceilingOffZ = sector[nSector].floorz;
						xsector4[i].ceilingOnZ = sector[nSector].ceilingz;

						// open or close door as necessary for initial state
						if ( xsector4[i].state == 0 )
							sector[nSector].ceilingz = xsector4[i].ceilingOffZ;
						break;

					case kSectorBottomDoor:
						xsector4[i].ceilingOffZ = sector[nSector].ceilingz;
						xsector4[i].ceilingOnZ = sector[nSector].floorz;

						// open or close door as necessary for initial state
						if ( xsector4[i].state == 0 )
							sector[nSector].floorz = xsector4[i].ceilingOffZ;
						break;

					case kSectorHSplitDoor:
						xsector4[i].ceilingOffZ = (sector[nSector].ceilingz + sector[nSector].floorz) / 2;
						xsector4[i].ceilingOnZ = sector[nSector].floorz;
						xsector4[i].ceilingOnZ = sector[nSector].ceilingz;

						// open or close door as necessary for initial state
						if ( xsector4[i].state == 0 )
						{
							sector[nSector].floorz   = xsector4[i].ceilingOffZ;
							sector[nSector].ceilingz = xsector4[i].ceilingOffZ;
						}
						break;

					default:
						break;
				}
			}
		}

		for (i = 0; i < numwalls; i++)
		{
			iob.Read(&wall[i], sizeof(WALL), 10);
			iob.Read(&wallSurf[i], sizeof(wallSurf[i]), 11);

			// expand wall shade range
			wall[i].shade = (schar)ClipRange(wall[i].shade << 1, -128, 127);

			if (wall[i].extra > 0)
			{
				XWALL3 xwall3;
				iob.Read(&xwall3, sizeof(XWALL3), 12);

				memset(&xwall4[i], 0, sizeof(XWALL));
				xwall4[i].reference =		xwall3.reference;
				xwall4[i].type =            xwall3.type;
				xwall4[i].state =           xwall3.state;
				xwall4[i].busy =            xwall3.busy;
				xwall4[i].txID =            xwall3.txID;
				xwall4[i].rxID =            xwall3.rxID;
				xwall4[i].command =         xwall3.command;
				xwall4[i].triggerOn =       xwall3.triggerOn;
				xwall4[i].triggerOff =      xwall3.triggerOff;
				xwall4[i].key =             xwall3.key;
				xwall4[i].data =            xwall3.data;
				xwall4[i].waitTime =        xwall3.waitTime;
				xwall4[i].restState =       xwall3.restState;
				xwall4[i].triggerPush =     xwall3.triggerPush;
				xwall4[i].triggerImpact =   xwall3.triggerImpact;
				xwall4[i].triggerRsvd1 =    xwall3.triggerRsvd1;
				xwall4[i].triggerRsvd2 =    xwall3.triggerRsvd2;
				xwall4[i].decoupled =       xwall3.decoupled;
				xwall4[i].triggerOnce =     xwall3.triggerOnce;
				xwall4[i].isTriggered =     xwall3.isTriggered;
				xwall4[i].panXVel =         xwall3.panXVel;
				xwall4[i].panYVel =         xwall3.panYVel;
				xwall4[i].panAlways =       xwall3.panAlways;
				xwall4[i].map =             xwall3.map;
			}
		}

		for (i = 0; i < numsprites; i++)
		{
			iob.Read(&sprite[i], sizeof(SPRITE), 13);
			iob.Read(&spriteSurf[i], sizeof(spriteSurf[i]), 14);

			// expand sprite shade range
			if ( sprite[i].statnum < kMaxStatus )
				sprite[i].shade = (schar)ClipRange(sprite[i].shade << 1, -128, 127);

			if (sprite[i].extra > 0)
			{
				XSPRITE3 xsprite3;
				iob.Read(&xsprite3, sizeof(XSPRITE3), 15);

				memset(&xsprite4[i], 0, sizeof(XSPRITE));
				xsprite4[i].reference 	= xsprite3.reference;
				xsprite4[i].state 		= xsprite3.state;
				xsprite4[i].type 		= xsprite3.type;
				xsprite4[i].txID 		= xsprite3.txID;
				xsprite4[i].rxID 		= xsprite3.rxID;
				xsprite4[i].command 	= xsprite3.command;
				xsprite4[i].triggerOn 	= xsprite3.triggerOn;
				xsprite4[i].triggerOff 	= xsprite3.triggerOff;
				xsprite4[i].triggerOnce	= xsprite3.triggerOnce;
				xsprite4[i].restState	= xsprite3.restState;
				xsprite4[i].key 		= xsprite3.key;
				xsprite4[i].difficulty 	= xsprite3.difficulty;
				xsprite4[i].detail 		= xsprite3.detail;
				xsprite4[i].map 		= xsprite3.map;
				xsprite4[i].view 		= xsprite3.view;
				xsprite4[i].soundKit 	= xsprite3.soundKit;
				xsprite4[i].data 		= xsprite3.data;

				xsprite4[i].isTriggered			= 0;
				xsprite4[i].triggerPush			= xsprite3.isTriggered;
				xsprite4[i].triggerImpact		= xsprite3.triggerImpact;
				xsprite4[i].triggerPickup		= xsprite3.triggerPickup;
				xsprite4[i].triggerTouch		= xsprite3.triggerTouch;
				xsprite4[i].triggerSight		= xsprite3.triggerSight;
				xsprite4[i].triggerProximity	= xsprite3.triggerProximity;
				xsprite4[i].decoupled			= xsprite3.decoupled;
				xsprite4[i].waitTime			= xsprite3.waitTime;

				xsprite4[i].permanent	= 0;
				xsprite4[i].respawn		= 0;
				xsprite4[i].respawnTime	= 0;
				xsprite4[i].launchMode	= 0;
			}
		}
	}
	else
		ThrowError("Error reading map file", ES_ERROR);

 	free(buffer);

	// add up all the elements in the file to determine the size of the buffer
	length = 0;
	length += sizeof(HEADER4);
	length += sizeof(INFO4);
	length += strlen(gMapName) + 1;
	length += strlen(gMapAuthor) + 1;
	length += gSkyCount * sizeof(pskyoff[0]);

	length += numsectors * sizeof(SECTOR);
	length += numsectors * sizeof(ceilingSurf[0]);
	length += numsectors * sizeof(floorSurf[0]);
	for (i = 0; i < numsectors; i++)
	{
		if (sector[i].extra > 0)
			length += sizeof(XSECTOR4);
	}

	length += numwalls * sizeof(WALL);
	length += numwalls * sizeof(wallSurf[0]);
	for (i = 0; i < numwalls; i++)
	{
		if (wall[i].extra > 0)
			length += sizeof(XWALL4);
	}

	length += numsprites * sizeof(SPRITE);
	length += numsprites * sizeof(spriteSurf[0]);
	for (i = 0; i < numsprites; i++)
	{
		if (sprite[i].extra > 0)
			length += sizeof(XSPRITE4);
	}
	length += 4;	// CRC

	buffer = (char *)malloc(length);
	IOBuffer iob(buffer, length);

	memcpy(header4.signature, kBloodMapSig, sizeof(header4.signature));
	header4.version = kBloodMapVersion;

	iob.Write(&header4, sizeof(header4), 1);

	info4.x = info3.x;
	info4.y = info3.y;
	info4.z = info3.z;
	info4.angle = info3.angle;
	info4.sector = info3.sector;
	info4.pskybits = info3.pskybits;
	info4.visibility = 	1 << (22 - info3.visibility);
	info4.songId = info3.songId;
	info4.parallax = info3.parallax;
	info4.mapRevisions = info3.mapRevisions;
	info4.lenName = (char)strlen(gMapName);
	info4.lenAuthor = (char)strlen(gMapAuthor);
	info4.numsectors = numsectors;
	info4.numwalls = numwalls;
	info4.numsprites = numsprites;

	iob.Write(&info4, sizeof(INFO3), 2);
	iob.Write(gMapName, info4.lenName + 1, 3);
	iob.Write(gMapAuthor, info4.lenAuthor + 1, 4);
	iob.Write(pskyoff, gSkyCount * sizeof(pskyoff[0]), 5);

	for (i = 0; i < numsectors; i++)
	{
		iob.Write(&sector[i], sizeof(SECTOR), 6);
		iob.Write(&ceilingSurf[i], sizeof(ceilingSurf[i]), 7);
		iob.Write(&floorSurf[i], sizeof(floorSurf[i]), 8);

		if (sector[i].extra > 0)
			iob.Write(&xsector4[sector[i].extra], sizeof(XSECTOR), 9);
	}

	for (i = 0; i < numwalls; i++)
	{
		iob.Write(&wall[i], sizeof(WALL), 10);
		iob.Write(&wallSurf[i], sizeof(wallSurf[i]), 11);

		if (wall[i].extra > 0)
			iob.Write(&xwall4[wall[i].extra], sizeof(XWALL), 12);
	}

	for (i = 0; i < numsprites; i++)
	{
		iob.Write(&sprite[i], sizeof(SPRITE), 13);
		iob.Write(&spriteSurf[i], sizeof(spriteSurf[i]), 14);

		if (sprite[i].extra > 0)
			iob.Write(&xsprite4[sprite[i].extra], sizeof(XSPRITE), 15);
	}

	gMapCRC = CRC32(buffer, length - sizeof(gMapCRC));
	iob.Write(&gMapCRC, sizeof(gMapCRC), 16);

	// backup the map file
	strcpy(bakfilename, filename);
	ChangeExtension(filename, "MAP");
	ChangeExtension(bakfilename, "~AK");
	rename(filename, bakfilename);
	ChangeExtension(filename, "MPX");
	ChangeExtension(bakfilename, "~PX");
	rename(filename, bakfilename);

	ChangeExtension(filename, "MAP");
	printf("=> %s ", filename);
	hFile = open(filename, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, S_IWUSR);
	if ( hFile == -1 )
		ThrowError("Error opening MAP file", ES_ERROR);

	if (write(hFile, buffer, length) != length)
		ThrowError("Error write MAP file", ES_ERROR);

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
	printf("Blood Map 3 Converter    Copyright (c) 1995 Q Studios Corporation\n");

	if (argc < 2) ShowUsage();

	ParseOptions(argc, argv);

	// process the file list
	for (FNODE *n = head.next; n != &head; n = n->next)
		ProcessFile(n->name);
}



