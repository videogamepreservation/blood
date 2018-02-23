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

#define kBloodMapSig		"BLM\x1A"
#define kBloodMapVersion	0x0400
#define kBloodMapOldVersion	0x0301

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
		ThrowError("IOB", nError, "Read buffer overflow", ES_ERROR);
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
		ThrowError("IOB", nError, "Write buffer overflow", ES_ERROR);
}

struct HEADER3
{
	char signature[4];
	short version;
};

struct HEADER4
{
	char signature[4];
	short version;
};

// version 3.x header
struct INFO3
{
	long	x, y, z;
	ushort	angle;
	ushort	sector;
	ushort 	pskybits;
	uchar 	visibility;
	int		songId;
	uchar	parallax;
	int 	mapRevisions;
	uchar 	lenName;
	uchar 	lenAuthor;
	ushort	numsectors;
	ushort	numwalls;
	ushort	numsprites;
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

/***********************************************************************
 * Old version structures and typedefs
 **********************************************************************/
struct XSPRITE3
{
	signed reference :			16;
	unsigned state :			1;

	// trigger data
	unsigned busy :				16;
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
	unsigned view :				4;
	unsigned soundKit :			8;

	unsigned isTriggered :      1; // used for triggerOnce objects
	unsigned triggerPush :      1; // gates?
	unsigned triggerImpact :    1; // exploding things, etc.
	unsigned triggerPickup :    1; // secrets
	unsigned triggerTouch :     1; // sawblades, spikes, zaps?
	unsigned triggerSight :     1; // dunno, yet.
	unsigned triggerProximity : 1; // proximity bombs

	unsigned decoupled :		1;
	unsigned waitTime : 		8;
};

struct XWALL3
{
	signed reference :			16;
	unsigned state : 			1;
	unsigned map :				2;

	// trigger data
	unsigned busy : 			16;
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
	unsigned waitTime :			8;
};

struct XSECTOR3
{
	signed reference :			16;
	unsigned state : 			1;

	// trigger data
	unsigned busy : 			16;
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

	// lighting data
	signed amplitude :			8;
	unsigned freq :				8;
	unsigned wave :				4;
	unsigned shadeAlways : 		1;
	unsigned unused2 :			3;	// !!! USE ME !!!
	unsigned phase :			8;
	unsigned shadeFloor :		1;
	unsigned shadeCeiling : 	1;
	unsigned shadeWalls :		1;
	signed shade :				8;

	// panning data
	unsigned panFloor :			1;
	unsigned panCeiling :		1;
	unsigned panDrag :			1;
	unsigned waitTime : 		8;
	unsigned panAlways :		1;
	unsigned unused3 :			3;	// !!! USE ME !!!

	// speed for moving sectors
	unsigned busyTime :			8;

	// water stuff
	unsigned underwater :		1;
	unsigned depth :			2;
	unsigned panVel :      		8;
	unsigned panAngle :			12; // change to 11 bits in CONVDB4!!!

	unsigned wind :		    	3;

	unsigned isTriggered :		1; // used for triggerOnce objects
	unsigned triggerPush :		1;
	unsigned triggerImpact :	1;
	unsigned triggerEnter :		1;
	unsigned triggerExit :		1;
	unsigned decoupled :		1;

	unsigned triggerWPush :		1;	// push from outside wall (convenience bit)
};


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
	HEADER3 header3;
	HEADER4 header4;
	INFO3 info3;
	INFO4 info4;
	char filename[_MAX_PATH], bakfilename[_MAX_PATH];
	short numsprites;
	int length;

	char *buffer;

	strcpy(filename, filespec);
	printf("%s ", filename);

	hFile = open(filename, O_RDONLY | O_BINARY);
	if ( hFile == -1 )
		ThrowError( "MAP", 0, "unable to open MAP file", ES_ERROR);

	length = filelength(hFile);
	buffer = (char *)malloc(length);
	if (read(hFile, buffer, length) == length)
	{
		close(hFile);

		IOBuffer iob(buffer, length);

		iob.Read(&header3, sizeof(HEADER3), 1);

		// check signature and major version number
		if ( (memcmp(header3.signature, kBloodMapSig, 4) != 0)
			|| ((header3.version >> 8) != (kBloodMapOldVersion >> 8)) )
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

//		printf("Converting %i sectors...\n",numsectors);
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
				int j = sector[i].extra;

				XSECTOR3 xsector3;
				iob.Read(&xsector3, sizeof(XSECTOR3), 9);

				memset(&xsector[j], 0, sizeof(XSECTOR));
				xsector[j].reference		= xsector3.reference;
				xsector[j].state			= xsector3.state;
				xsector[j].busy			= 0;
				xsector[j].type			= xsector3.type;
				xsector[j].data			= xsector3.data;
				xsector[j].txID			= xsector3.txID;
				xsector[j].rxID			= xsector3.rxID;
				xsector[j].command			= xsector3.command;
				xsector[j].triggerOn		= xsector3.triggerOn;
				xsector[j].triggerOff		= xsector3.triggerOff;
				xsector[j].triggerOnce		= xsector3.triggerOnce;
				xsector[j].restState		= xsector3.restState;
				xsector[j].key				= xsector3.key;
				xsector[j].amplitude		= ClipRange(xsector3.amplitude << 1, -128, 127);
				xsector[j].freq			= xsector3.freq;
				xsector[j].wave			= xsector3.wave;
				xsector[j].shadeAlways		= xsector3.shadeAlways;
				xsector[j].phase			= xsector3.phase;
				xsector[j].shadeFloor		= xsector3.shadeFloor;
				xsector[j].shadeCeiling	= xsector3.shadeCeiling;
				xsector[j].shadeWalls		= xsector3.shadeWalls;
				xsector[j].shade			= xsector3.shade;
				xsector[j].panFloor		= xsector3.panFloor;
				xsector[j].panCeiling		= xsector3.panCeiling;
				xsector[j].panDrag			= xsector3.panDrag;
				xsector[j].waitTime		= xsector3.waitTime;
				xsector[j].panAlways		= xsector3.panAlways;
				xsector[j].busyTime		= xsector3.busyTime;
				xsector[j].underwater		= xsector3.underwater;
				xsector[j].depth			= xsector3.depth;
				xsector[j].panVel			= xsector3.panVel;
				xsector[j].panAngle		= xsector3.panAngle;
				xsector[j].wind			= xsector3.wind ? 1 : 0;
				xsector[j].isTriggered		= 0;
				xsector[j].triggerPush		= xsector3.triggerPush;
				xsector[j].triggerImpact	= xsector3.triggerImpact;
				xsector[j].triggerEnter	= xsector3.triggerEnter;
				xsector[j].triggerExit		= xsector3.triggerExit;
				xsector[j].triggerWPush	= xsector3.triggerWPush;
				xsector[j].decoupled		= xsector3.decoupled;

				// default all onZ/offZ values
				xsector[j].offCeilZ		= sector[i].ceilingz;
				xsector[j].onCeilZ		= sector[i].ceilingz;
				xsector[j].offFloorZ	= sector[i].floorz;
				xsector[j].onFloorZ     = sector[i].floorz;

				switch( xsector[j].type )
				{
					case kSectorTopDoor:
						xsector[j].offCeilZ = sector[i].floorz;
						xsector[j].type = kSectorZMotion;
						break;

					case kSectorBottomDoor:
						xsector[j].offFloorZ = sector[i].ceilingz;
						xsector[j].type = kSectorZMotion;
						break;

					case kSectorHSplitDoor:
						xsector[j].offCeilZ  = (sector[i].ceilingz + sector[i].floorz) / 2;
						xsector[j].offFloorZ = (sector[i].ceilingz + sector[i].floorz) / 2;
 						xsector[j].type = kSectorZMotion;
						break;
					default:
						break;
				}
			}
		}

//		printf("Converting %i walls...\n",numwalls);
		for (i = 0; i < numwalls; i++)
		{
			iob.Read(&wall[i], sizeof(WALL), 10);
			iob.Read(&wallSurf[i], sizeof(wallSurf[i]), 11);

			// expand wall shade range
			wall[i].shade = (schar)ClipRange(wall[i].shade << 1, -128, 127);

			if (wall[i].extra > 0)
			{
				int j = wall[i].extra;

				XWALL3 xwall3;
				iob.Read(&xwall3, sizeof(XWALL3), 12);

				memset(&xwall[j], 0, sizeof(XWALL));
				xwall[j].reference =		xwall3.reference;
				xwall[j].type =            xwall3.type;
				xwall[j].state =           xwall3.state;
				xwall[j].busy =            xwall3.busy;
				xwall[j].txID =            xwall3.txID;
				xwall[j].rxID =            xwall3.rxID;
				xwall[j].command =         xwall3.command;
				xwall[j].triggerOn =       xwall3.triggerOn;
				xwall[j].triggerOff =      xwall3.triggerOff;
				xwall[j].key =             xwall3.key;
				xwall[j].data =            xwall3.data;
				xwall[j].waitTime =        xwall3.waitTime;
				xwall[j].restState =       xwall3.restState;
				xwall[j].triggerPush =     xwall3.triggerPush;
				xwall[j].triggerImpact =   xwall3.triggerImpact;
				xwall[j].triggerRsvd1 =    xwall3.triggerRsvd1;
				xwall[j].triggerRsvd2 =    xwall3.triggerRsvd2;
				xwall[j].decoupled =       xwall3.decoupled;
				xwall[j].triggerOnce =     xwall3.triggerOnce;
				xwall[j].isTriggered =     xwall3.isTriggered;
				xwall[j].panXVel =         xwall3.panXVel;
				xwall[j].panYVel =         xwall3.panYVel;
				xwall[j].panAlways =       xwall3.panAlways;
				xwall[j].map =             xwall3.map;
			}
		}

//		printf("Converting %i sprites...\n",numsprites);
		for (i = 0; i < numsprites; i++)
		{
			iob.Read(&sprite[i], sizeof(SPRITE), 13);
			iob.Read(&spriteSurf[i], sizeof(spriteSurf[i]), 14);

			// expand sprite shade range
			if ( sprite[i].statnum < kMaxStatus )
				sprite[i].shade = (schar)ClipRange(sprite[i].shade << 1, -128, 127);

			if (sprite[i].extra > 0)
			{
				int j = sprite[i].extra;

				XSPRITE3 xsprite3;
				iob.Read(&xsprite3, sizeof(XSPRITE3), 15);

				memset(&xsprite[j], 0, sizeof(XSPRITE));
				xsprite[j].reference 	= xsprite3.reference;
				xsprite[j].state 		= xsprite3.state;
				xsprite[j].type 		= xsprite3.type;
				xsprite[j].txID 		= xsprite3.txID;
				xsprite[j].rxID 		= xsprite3.rxID;
				xsprite[j].command 	= xsprite3.command;
				xsprite[j].triggerOn 	= xsprite3.triggerOn;
				xsprite[j].triggerOff 	= xsprite3.triggerOff;
				xsprite[j].triggerOnce	= xsprite3.triggerOnce;
				xsprite[j].restState	= xsprite3.restState;
				xsprite[j].key 		= xsprite3.key;
				xsprite[j].difficulty 	= xsprite3.difficulty;
				xsprite[j].detail 		= xsprite3.detail;
				xsprite[j].map 		= xsprite3.map;
				xsprite[j].view 		= xsprite3.view;
				xsprite[j].soundKit 	= xsprite3.soundKit;
				xsprite[j].data 		= xsprite3.data;

				xsprite[j].isTriggered			= 0;
				xsprite[j].triggerPush			= xsprite3.triggerPush;
				xsprite[j].triggerImpact		= xsprite3.triggerImpact;
				xsprite[j].triggerPickup		= xsprite3.triggerPickup;
				xsprite[j].triggerTouch		= xsprite3.triggerTouch;
				xsprite[j].triggerSight		= xsprite3.triggerSight;
				xsprite[j].triggerProximity	= xsprite3.triggerProximity;
				xsprite[j].decoupled			= xsprite3.decoupled;
				xsprite[j].waitTime			= xsprite3.waitTime;

				xsprite[j].permanent	= 0;
				xsprite[j].respawn		= 0;
				xsprite[j].respawnTime	= 0;
				xsprite[j].launchMode	= 0;
			}
		}
	}
	else
		ThrowError( "DB", 16, "Error reading map file", ES_ERROR);

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
			length += sizeof(XSECTOR);
	}

	length += numwalls * sizeof(WALL);
	length += numwalls * sizeof(wallSurf[0]);
	for (i = 0; i < numwalls; i++)
	{
		if (wall[i].extra > 0)
			length += sizeof(XWALL);
	}

	length += numsprites * sizeof(SPRITE);
	length += numsprites * sizeof(spriteSurf[0]);
	for (i = 0; i < numsprites; i++)
	{
		if (sprite[i].extra > 0)
			length += sizeof(XSPRITE);
	}
	length += 4;	// CRC

	buffer = (char *)malloc(length);
	IOBuffer iob(buffer, length);

	memcpy(header4.signature, kBloodMapSig, 4);
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

	iob.Write(&info4, sizeof(INFO4), 2);
	iob.Write(gMapName, info4.lenName + 1, 3);
	iob.Write(gMapAuthor, info4.lenAuthor + 1, 4);
	iob.Write(pskyoff, gSkyCount * sizeof(pskyoff[0]), 5);

	for (i = 0; i < numsectors; i++)
	{
		iob.Write(&sector[i], sizeof(SECTOR), 6);
		iob.Write(&ceilingSurf[i], sizeof(ceilingSurf[i]), 7);
		iob.Write(&floorSurf[i], sizeof(floorSurf[i]), 8);

		if (sector[i].extra > 0)
			iob.Write(&xsector[sector[i].extra], sizeof(XSECTOR), 9);
	}

	for (i = 0; i < numwalls; i++)
	{
		iob.Write(&wall[i], sizeof(WALL), 10);
		iob.Write(&wallSurf[i], sizeof(wallSurf[i]), 11);

		if (wall[i].extra > 0)
			iob.Write(&xwall[wall[i].extra], sizeof(XWALL), 12);
	}

	for (i = 0; i < numsprites; i++)
	{
		iob.Write(&sprite[i], sizeof(SPRITE), 13);
		iob.Write(&spriteSurf[i], sizeof(spriteSurf[i]), 14);

		if (sprite[i].extra > 0)
			iob.Write(&xsprite[sprite[i].extra], sizeof(XSPRITE), 15);
	}

	gMapCRC = CRC32(buffer, length - sizeof(gMapCRC));
	iob.Write(&gMapCRC, sizeof(gMapCRC), 16);

	// backup the map file
	strcpy(bakfilename, filename);
	ChangeExtension(filename, "MAP");
	ChangeExtension(bakfilename, "MA3");
//	printf("Creating backup file %s",bakfilename);
	rename(filename, bakfilename);

//	printf("Saving converted file %s",filename);
	ChangeExtension(filename, "MAP");
	printf("=> %s ", filename);
	hFile = open(filename, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, S_IWUSR);
	if ( hFile == -1 )
		ThrowError( "DB", 0, "Error opening MAP file", ES_ERROR);

	if (write(hFile, buffer, length) != length)
		ThrowError( "DB", 1, "Error write MAP file", ES_ERROR);

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
	printf("Blood Map 4 Converter    Copyright (c) 1995 Q Studios Corporation\n");

	if (argc < 2) ShowUsage();

	ParseOptions(argc, argv);

	// process the file list
	for (FNODE *n = head.next; n != &head; n = n->next)
		ProcessFile(n->name);
}



