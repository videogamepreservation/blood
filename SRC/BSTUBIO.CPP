#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>

#include "db.h"
#include "engine.h"
#include "error.h"

#include <memcheck.h>

/***********************************************************************
 * ImportBuildMap()
 *
 **********************************************************************/
int ImportBuildMap(
	char *filename,
	long *daposx,
	long *daposy,
	long *daposz,
	short *daang,
	short *dacursectnum )
{
	int fil;
	short int i, numsprites;
	long mapversion;

	if ((fil = open(filename,O_BINARY|O_RDWR,S_IREAD)) == -1)
	{
		mapversion = 6L;
		return -1;
	}

	read(fil,&mapversion,4);
	if (mapversion != 6L)
		return -1;

	*gMapName = '\0';
	*gMapAuthor = '\0';
	gSkyCount = 1;
	pskyoff[0] = 0;

	dbInit();	// calls initspritelists() too!

	memset(show2dsprite, 0, sizeof(show2dsprite));
	memset(show2dwall, 0, sizeof(show2dwall));

	read(fil,daposx,4);
	read(fil,daposy,4);
	read(fil,daposz,4);
	read(fil,daang,2);
	read(fil,dacursectnum,2);

	read(fil,&numsectors,2);
	read(fil,sector,sizeof(SECTOR)*numsectors);

	read(fil,&numwalls,2);
	read(fil,wall,sizeof(WALL)*numwalls);

	read(fil,&numsprites,2);
	read(fil,sprite,sizeof(SPRITE)*numsprites);

	for( i=0; i<numsprites; i++)
		insertsprite(sprite[i].sectnum,sprite[i].statnum);

		//Must be after loading sectors, etc!
	updatesector(*daposx,*daposy,dacursectnum);

	close(fil);

	highlightcnt = -1;
	highlightsectorcnt = -1;

	precache();
	printmessage16("Map imported successfully.");

	updatenumsprites();
	startposx = posx;      //this is same
	startposy = posy;
	startposz = posz;
	startang = ang;
	startsectnum = cursectnum;

	return 0;
}


/***********************************************************************
 * ExportBuildMap()
 *
 **********************************************************************/
int ExportBuildMap(
	char *filename,
	long *daposx,
	long *daposy,
	long *daposz,
	short *daang,
	short *dacursectnum )
{
	int fil;
	short int i, j, numsprites;
	long mapversion = 6L;

	if ((fil = open(filename,O_BINARY|O_TRUNC|O_CREAT|O_WRONLY,S_IWRITE)) == -1)
		return -1;

	write(fil,&mapversion,4);
	write(fil,daposx,4);
	write(fil,daposy,4);
	write(fil,daposz,4);
	write(fil,daang,2);
	write(fil,dacursectnum,2);

	write(fil,&numsectors,2);
	write(fil,sector,sizeof(SECTOR)*numsectors);

	write(fil,&numwalls,2);
	write(fil,wall,sizeof(WALL)*numwalls);

	numsprites = 0;
	for(j=0;j<kMaxStatus;j++)
	{
		i = headspritestat[j];
		while (i != -1)
		{
			numsprites++;
			i = nextspritestat[i];
		}
	}
	write(fil,&numsprites,2);

	for(j=0;j<kMaxStatus;j++)
	{
		i = headspritestat[j];
		while (i != -1)
		{
			write(fil,&sprite[i],sizeof(SPRITE));
			i = nextspritestat[i];
		}
	}

	close(fil);
	return 0;
}

#if 0
/***************************************************************************
	KEN'S TAG DEFINITIONS:      (Please define your own tags for your games)

 sector[?].lotag = 0   Normal sector
 sector[?].lotag = 1   If you are on a sector with this tag, then all sectors
								  with same hi tag as this are operated.  Once.
 sector[?].lotag = 2   Same as sector[?].tag = 1 but this is retriggable.
 sector[?].lotag = 3   A really stupid sector that really does nothing now.
 sector[?].lotag = 4   A sector where you are put closer to the floor
								  (such as the slime in DOOM1.DAT)
 sector[?].lotag = 5   A really stupid sector that really does nothing now.
 sector[?].lotag = 6   A normal door - instead of pressing D, you tag the
								  sector with a 6.  The reason I make you edit doors
								  this way is so that can program the doors
								  yourself.
 sector[?].lotag = 7   A door the goes down to open.
 sector[?].lotag = 8   A door that opens horizontally in the middle.
 sector[?].lotag = 9   A sliding door that opens vertically in the middle.
								  -Example of the advantages of not using BSP tree.
 sector[?].lotag = 10  A warping sector with floor and walls that shade.
 sector[?].lotag = 11  A sector with all walls that do X-panning.
 sector[?].lotag = 12  A sector with walls using the dragging function.
 sector[?].lotag = 13  A sector with some swinging doors in it.
 sector[?].lotag = 14  A revolving door sector.
 sector[?].lotag = 15  A subway track.
 sector[?].lotag = 16  A true double-sliding door.
 sector[?].lotag = 17  A true double-sliding door for subways only.

	wall[?].lotag = 0   Normal wall
	wall[?].lotag = 1   Y-panning wall
	wall[?].lotag = 2   Switch - If you flip it, then all sectors with same hi
								  tag as this are operated.
	wall[?].lotag = 3   Marked wall to detemine starting dir. (sector tag 12)
	wall[?].lotag = 4   Mark on the shorter wall closest to the pivot point
								  of a swinging door. (sector tag 13)
	wall[?].lotag = 5   Mark where a subway should stop. (sector tag 15)
	wall[?].lotag = 6   Mark for true double-sliding doors (sector tag 16)
	wall[?].lotag = 7   Water fountain

 sprite[?].lotag = 0   Normal sprite
 sprite[?].lotag = 1   If you press space bar on an AL, and the AL is tagged
								  with a 1, he will turn evil.
 sprite[?].lotag = 2   When this sprite is operated, a bomb is shot at its
								  position.
 sprite[?].lotag = 3   Rotating sprite.
 sprite[?].lotag = 4   Sprite switch.
 sprite[?].lotag = 5   Basketball hoop score.
**************************************************************************/

/***********************************************************************
 * ConvertKenTriggers()
 *
 * Called in loadboard and saveboard. Converts Ken's triggers
 * to XSECTOR data, leaving the sector lotag and hitag intact.
 **********************************************************************/

void ConvertKenTriggers( void )
{
	int nSector, nXSector;

	for ( nSector = 0; nSector < numsectors; nSector++ )
	{
		switch (sector[nSector].lotag)
		{
			case 1:   // kSectorTriggerOnEnterOnce
			case 2:   // kSectorTriggerOnEnter
			case 4:   // kSectorWadeWater
			case 6:   // kSectorRaiseDoor
			case 7:   // kSectorLowerDoor
			case 8:   // kSectorHorizontalDoors
			case 9:   // kSectorVerticalDoors
			case 10:  // kSectorTeleport
			case 11:  // kSectorXPanWalls
			case 12:  // kSectorDragWalls
			case 13:  // kSectorContainsSwingingDoors
			case 14:  // kSectorRevolveWalls
			case 16:  // kSectorPeggedVerticalDoors
			case 100: // kSectorMovesUp
			case 101: // kSectorMovesDown
				break;

			default:
				break;
		}

		// get or create an extra structure
		if ( sector[nSector].extra > 0 ) {
			nXSector = sector[nSector].extra;
			dassert(nXSector < kMaxXSectors);
		} else {
			nXSector = GetXSector(nSector);
			dassert(nXSector < kMaxXSectors);
		}
		// fill the extra here...
	}
}
#endif


