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
#define kBloodMapVersion	0x0300

struct FNODE
{
	FNODE *next;
	char name[1];
};

FNODE head = { &head, "" };
FNODE *tail = &head;

SECTOR 		sector[kMaxSectors];
XSECTOR 	xsector[kMaxXSectors];
WALL 		wall[kMaxWalls];
XWALL 		xwall[kMaxXWalls];
SPRITE 		sprite[kMaxSprites];
XSPRITE 	xsprite[kMaxXSprites];
uchar		spriteSurf[kMaxSprites];
uchar 		wallSurf[kMaxWalls];
uchar 		floorSurf[kMaxSectors];
uchar 		ceilingSurf[kMaxSectors];
short 		pskyoff[kMaxSkyTiles];

short pskybits;
char gMapAuthor[64] = "";
char gMapName[64] = "";
ulong gMapCRC;
int gSkyCount;
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


struct OLDHEADER
{
	long version;
	long x, y, z;
	short angle;
	short sectnum;
};

struct MPXHEADER
{
	char signature[4];
	short version;
	short dummy;
};

struct HEADER
{
	char signature[4];
	short version;
};

// version 2.x header
struct INFO2
{
	short xspriteSize;
	short xwallSize;
	short xsectorSize;
	char visibility;
	short xspriteCount;
	short xwallCount;
	short xsectorCount;
	short pskybits;
	char lenAuthor;
	char lenName;
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

struct XSPRITE2
{
	signed reference :		16;
	unsigned rxID :			10;
	unsigned difficulty :	2;
	unsigned detail :		2;
	unsigned map : 			2;
	unsigned txID :			10;
	unsigned key :			3;
	unsigned command : 		3;
	unsigned type :			10;
	unsigned state: 		1;
	unsigned triggerOn : 	1;
	unsigned triggerOff : 	1;
	unsigned triggerOnce :	1;
	unsigned view :			2;	// changed to : 3
	unsigned soundKit :		8;
	unsigned data :			16;
};

struct XWALL2
{
	signed reference :		16;
	unsigned rxID :			10;
	unsigned surfType :		6;
	unsigned txID :			10;
    unsigned key :			3;
	unsigned command : 		3;
	unsigned type :			10;
	unsigned map : 			2;
	unsigned state: 		1;
	unsigned triggerOn : 	1;
	unsigned triggerOff : 	1;
	unsigned data :			16;
};

struct XSECTOR2
{
	signed reference :		16;
	unsigned rxID :			10;
	unsigned command : 		3;
	unsigned triggerOn : 	1;
	unsigned triggerOff : 	1;
	unsigned triggerOnce :	1;
	signed ceilingShade0 :	8;	// deleted
	signed floorShade0 :	8;	// deleted
	signed amplitude :		8;
	unsigned freq :			8;
	unsigned wave :			8;
	unsigned phase :		8;
	unsigned state: 		1;
	unsigned shadeFloor :	1;
	unsigned shadeCeiling : 1;
	unsigned shadeWalls :	1;
	unsigned panFloor :		1;
	unsigned panCeiling :	1;
	unsigned panDrag :		1;
	unsigned panWalls :		1;
	unsigned panAngle :		12;
	signed panSpeed :		8;
	signed shade :			8;
	unsigned surfType :		6;	// deleted
	unsigned ceilSurf :		6;
	unsigned floorSurf :	6;
	unsigned txID :			10;
	unsigned key :			3;
	unsigned type :			10;
	unsigned underwater :	1;
	unsigned data :			16;


};


void ShowUsage(void)
{
	printf("Usage:\n");
	printf("  CONVDB3 map1 map2 (wild cards ok)\n");
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
	OLDHEADER oldHeader;
	MPXHEADER mpxheader;
	HEADER header;
	INFO2 info2;
	INFO3 info3;
	char filename[_MAX_PATH], bakfilename[_MAX_PATH];
	int x, y, z;
	short angle, nSector, numsprites;
	int length;

	char *buffer;

	strcpy(filename, filespec);
	printf("%s ", filename);

	hFile = open(filename, O_RDONLY | O_BINARY);
	if ( hFile == -1 )
		ThrowError( "MPX", 0, "unable to open MAP file", ES_ERROR);

	length = filelength(hFile);
	buffer = (char *)malloc(length);
	if (read(hFile, buffer, length) == length)
	{
		close(hFile);

		IOBuffer iob(buffer, length);

		iob.Read(&oldHeader, sizeof(OLDHEADER), 1);

		if (oldHeader.version != 6)
		{
			printf("not old format, skipping\n");
			return;
		}

		x = oldHeader.x;
		y = oldHeader.y;
		z = oldHeader.z;
		angle = oldHeader.angle;
		nSector = oldHeader.sectnum;

		iob.Read(&numsectors, sizeof(numsectors), 2);
		iob.Read(&sector[0], sizeof(SECTOR) * numsectors, 3);
		iob.Read(&numwalls, sizeof(numwalls), 4);
		iob.Read(&wall[0], sizeof(WALL) * numwalls, 5);
		iob.Read(&numsprites, sizeof(numsprites), 6);
		iob.Read(&sprite[0], sizeof(SPRITE) * numsprites, 7);
	}
	else
		ThrowError( "DB", 8, "Error reading map file", ES_ERROR);

 	free(buffer);

	ChangeExtension(filename, "MPX");

	printf("%s ", filename);
	hFile = open(filename, O_RDONLY | O_BINARY);
	if ( hFile == -1 )
		ThrowError( "MPX", 9, "unable to open MPX file", ES_ERROR);

	length = filelength(hFile);
	buffer = (char *)malloc(length);
	if (read(hFile, buffer, length) == length)
	{
		close(hFile);

		IOBuffer iob(buffer, length);

		iob.Read(&mpxheader, sizeof(MPXHEADER), 10);

		if (memcmp(mpxheader.signature, "MPX\x1A", 4) != 0)
		{
			close(hFile);
			ThrowError( "MPX", 11, "MPX file corrupted", ES_ERROR);
		}

		if ( (mpxheader.version >> 8) != 2)
			ThrowError("MPX", 12, "Incorrect MPX version encounted.", ES_ERROR);

		iob.Read(&info2, sizeof(INFO2), 13);

		for (i = 0; i < info2.xspriteCount; i++)
		{
			XSPRITE2 xsprite2;
			iob.Read(&xsprite2, sizeof(XSPRITE2), 14);

			memset(&xsprite[i], 0, sizeof(XSPRITE));
			xsprite[i].reference 	= xsprite2.reference;
			xsprite[i].rxID 		= xsprite2.rxID;
			xsprite[i].difficulty 	= xsprite2.difficulty;
			xsprite[i].detail 		= xsprite2.detail;
			xsprite[i].map 			= xsprite2.map;
			xsprite[i].txID 		= xsprite2.txID;
			xsprite[i].key 			= xsprite2.key;
			xsprite[i].command 		= xsprite2.command;
			xsprite[i].type 		= xsprite2.type;
			xsprite[i].state 		= xsprite2.state;
			xsprite[i].triggerOn 	= xsprite2.triggerOn;
			xsprite[i].triggerOff 	= xsprite2.triggerOff;
			xsprite[i].triggerOnce 	= xsprite2.triggerOnce;
			xsprite[i].view 		= xsprite2.view;
			xsprite[i].soundKit 	= xsprite2.soundKit;
			xsprite[i].data 		= xsprite2.data;
		}

		for (i = 0; i < info2.xwallCount; i++)
		{
			XWALL2 xwall2;
			iob.Read(&xwall2, sizeof(XWALL2), 15);

			memset(&xwall[i], 0, sizeof(XWALL));
			xwall[i].reference 		= xwall2.reference;
			xwall[i].rxID 			= xwall2.rxID;
			xwall[i].txID 			= xwall2.txID;
		    xwall[i].key 			= xwall2.key;
			xwall[i].command 		= xwall2.command;
			xwall[i].type 			= xwall2.type;
			xwall[i].map 			= xwall2.map;
			xwall[i].state 			= xwall2.state;
			xwall[i].triggerOn 		= xwall2.triggerOn;
			xwall[i].triggerOff 	= xwall2.triggerOff;
			xwall[i].data 			= xwall2.data;
		}

		for (i = 0; i < info2.xsectorCount; i++)
		{
			XSECTOR2 xsector2;
			iob.Read(&xsector2, sizeof(XSECTOR2), 16);

			memset(&xsector[i], 0, sizeof(XSECTOR));
			xsector[i].reference	= xsector2.reference;
			xsector[i].rxID			= xsector2.rxID;
			xsector[i].command		= xsector2.command;
			xsector[i].triggerOn	= xsector2.triggerOn;
			xsector[i].triggerOff	= xsector2.triggerOff;
			xsector[i].triggerOnce	= xsector2.triggerOnce;
			xsector[i].amplitude 	= xsector2.amplitude ;
			xsector[i].freq		= xsector2.freq;
			xsector[i].wave			= xsector2.wave;
			xsector[i].phase 		= xsector2.phase;
			xsector[i].state		= xsector2.state;
			xsector[i].shadeFloor	= xsector2.shadeFloor;
			xsector[i].shadeCeiling	= xsector2.shadeCeiling;
			xsector[i].shadeWalls	= xsector2.shadeWalls;
			xsector[i].panFloor		= xsector2.panFloor;
			xsector[i].panCeiling	= xsector2.panCeiling;
			xsector[i].panDrag		= xsector2.panDrag;
			xsector[i].panAngle		= xsector2.panAngle;
			xsector[i].panVel		= xsector2.panSpeed;
			xsector[i].shade 		= xsector2.shade ;
			xsector[i].txID			= xsector2.txID;
			xsector[i].key			= xsector2.key;
			xsector[i].type			= xsector2.type;
			xsector[i].underwater	= xsector2.underwater;
			xsector[i].data			= xsector2.data;
		}

		pskybits = info2.pskybits;
		gSkyCount = 1 << pskybits;
		iob.Read(pskyoff, gSkyCount * sizeof(pskyoff[0]), 17);
		iob.Read(gMapAuthor, info2.lenAuthor + 1, 18);
		iob.Read(gMapName, info2.lenName + 1, 19);
	}
	else
		ThrowError( "DB", 20, "Error reading MPX file", ES_ERROR);

 	free(buffer);

	// remove sectors with a state of OFF
	for (i = 0; i < numsectors; i++)
	{
		if (sector[i].extra != -1)
		{
			if (!xsector[sector[i].extra].state)
				sector[i].extra = -1;
		}
	}

	// modify new hitscan bit for face,wall,floor sprites
	for (i = 0; i < numsprites; i++)
	{
		// skip invalid sprites
		if (sprite[i].statnum >= kMaxStatus)
			continue;

		// process valid sprites
		if (sprite[i].cstat & kSpriteBlocking) {
			if ((sprite[i].cstat & kSpriteRMask) == 0) {
				// set hitscan for all blocking face sprites
				sprite[i].cstat |= kSpriteHitscan;
			} else {
				// clear hitscan for all blocking wall and floor sprites
				// game should re-set these if they are switches
				sprite[i].cstat &= ~kSpriteHitscan;
			}
		}
	}

	// add up all the elements in the file to determine the size of the buffer
	length = 0;
	length += sizeof(HEADER);
	length += sizeof(INFO3);
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
	for (i = 0; i < numsprites; i++)
	{
		if (sprite[i].extra > 0)
			length += sizeof(XSPRITE);
	}
	length += numsprites * sizeof(SPRITE);
	length += numsprites * sizeof(spriteSurf[0]);
	length += 4;	// CRC

	buffer = (char *)malloc(length);
	IOBuffer iob(buffer, length);

	memcpy(header.signature, kBloodMapSig, 4);
	header.version = kBloodMapVersion;

	iob.Write(&header, sizeof(header), 1);

	info3.x = oldHeader.x;
	info3.y = oldHeader.y;
	info3.z = oldHeader.z;
	info3.angle = oldHeader.angle;
	info3.sector = oldHeader.sectnum;
	info3.pskybits = pskybits;
	info3.visibility = info2.visibility;
	info3.songId = 0;
	info3.parallax = 0;
	info3.mapRevisions = 0;
	info3.lenName = (char)strlen(gMapName);
	info3.lenAuthor = (char)strlen(gMapAuthor);
	info3.numsectors = numsectors;
	info3.numwalls = numwalls;
	info3.numsprites = (short)numsprites;

	iob.Write(&info3, sizeof(INFO3), 2);
	iob.Write(gMapName, info3.lenName + 1, 3);
	iob.Write(gMapAuthor, info3.lenAuthor + 1, 4);
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
	ChangeExtension(bakfilename, "~AK");
	rename(filename, bakfilename);
	ChangeExtension(filename, "MPX");
	ChangeExtension(bakfilename, "~PX");
	rename(filename, bakfilename);

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
	printf("Blood Map 3 Converter    Copyright (c) 1995 Q Studios Corporation\n");

	if (argc < 2) ShowUsage();

	ParseOptions(argc, argv);

	// process the file list
	for (FNODE *n = head.next; n != &head; n = n->next)
		ProcessFile(n->name);
}



