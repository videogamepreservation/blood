#ifndef __ENGINE_H
#define __ENGINE_H

#include <stddef.h>
#include "typedefs.h"

#define kMaxSectors 1024
#define kMaxWalls 8192
#define kMaxSprites 4096
#define kMaxTiles 4096
#define kMaxStatus 1024
#define kMaxViewSprites 1024

#define MAXXDIM 1600 // 640
#define MAXYDIM 1200 // 480
#define MAXPALOOKUPS 256
#define kMaxSkyTiles 256

// Hit definitions
#define kHitTypeMask	0xE000
#define kHitIndexMask	0x1FFF
#define kHitFloor		0x4000
#define kHitSector		0x4000
#define kHitCeiling		0x6000
#define kHitWall		0x8000
#define kHitSprite		0xC000

// SearchStat definitions
#define SS_CUSTOM		-2
#define SS_ALL			-1
#define SS_WALL			0
#define SS_CEILING		1
#define SS_FLOOR		2
#define SS_SPRITE		3
#define SS_MASKED		4
#define SS_FLATSPRITE	5
#define SS_SECTOR		6

// Shade definitions
#define kMinShade	127
#define kMaxShade	-128

/***********************************************************************
 * Structures and typedefs
 **********************************************************************/
struct SECTOR
{
	short wallptr;
	short wallnum;
	long ceilingz;
	long floorz;
	ushort ceilingstat;
	ushort floorstat;
	sshort ceilingpicnum;
	sshort ceilingslope;
	schar ceilingshade;
	uchar ceilingpal;
	uchar ceilingxpanning, ceilingypanning;
	sshort floorpicnum;
	sshort floorslope;
	schar floorshade;
	uchar floorpal;
	uchar floorxpanning, floorypanning;
	uchar visibility;
	uchar filler;
	ushort type;
	short hitag;
	short extra;
};
//cstat,	bit 0:	1 = parallaxed, 0 = not			"P"
//          bit 1:	1 = sloped, 0 = not
//          bit 2:	1 = swap texture x&y, 0 = not	"F"
//          bit 3:	1 = texture smoosh, 0 = not		"E"
//          bit 4:	1 = texture x-flip				"F"
//          bit 5:	1 = texture y-flip				"F"
//          bit 6:	1 = Rel-aligned to 1st wall 	"R"
//          bit 15:	1 = Force sprite to floor shade	"L"	(floorstat only: primarily used in shaded/parallaxed areas)
enum {
	kSectorParallax		= 0x01,
	kSectorSloped		= 0x02,
	kSectorSwapXY   	= 0x04,
	kSectorExpand   	= 0x08,
	kSectorFlipX    	= 0x10,
	kSectorFlipY    	= 0x20,
	kSectorFlipMask		= 0x34,
	kSectorRelAlign 	= 0x40,
	kSectorFloorShade	= 0x8000
};

struct WALL
{
	long x, y;
	short point2;			// right vertex of wall
	short nextwall;			// wall on other side (-1 if single sided)
	short nextsector;      // sector on other side (-1 if single sided)
	ushort cstat;
	short picnum, overpicnum;
	schar shade;
	uchar pal, xrepeat, yrepeat, xpanning, ypanning;
	ushort type;
	short hitag, extra;
};
/* wall cstat bits
	bit 0: 1 = Blocking wall (use with clipmove, egetzrange)    "B"
	bit 1: 1 = bottoms of walls swapped, 0 = not       			"2"
	bit 2: 1 = align picture on bottom (for doors), 0 = top     "O"
	bit 3: 1 = x-flipped, 0 = normal                            "F"
	bit 4: 1 = masking wall, 0 = not                            "M"
	bit 5: 1 = 1-way wall, 0 = not                              "1"
	bit 6: 1 = Blocking wall (use with hitscan)                 "H"
	bit 7: 1 = Transluscent, 0 = not                            "T"
	bit 8: 1 = y-flipped, 0 = normal                            "F"
	bit 9: 1 =  1 = Reverse translucent table)
	bit 14-15:	00 = normal
				01 = move with vector
				10 = move opposite vector
				11 = undefined
*/
enum {
	kWallBlocking    	= 0x0001,
	kWallBottomSwap  	= 0x0002,
	kWallBottomOrg   	= 0x0004,
	kWallOutsideOrg   	= 0x0004,
	kWallFlipX       	= 0x0008,
	kWallMasked      	= 0x0010,
	kWallOneWay      	= 0x0020,
	kWallHitscan        = 0x0040,
	kWallTranslucent	= 0x0080,
	kWallFlipY			= 0x0100,
	kWallFlipMask		= 0x0108,
	kWallTranslucentR	= 0x0200,
	kWallMapSecret		= 0x0400,
	kWallMapNever		= 0x0800,
	kWallMapAlways		= 0x1000,
	kWallMoveMask		= 0xC000,
	kWallMoveNone		= 0x0000,
	kWallMoveForward	= 0x4000,
	kWallMoveBackward	= 0x8000,
};

/*******************************************************************************
Texture alignment notes:

For single sided walls, the alignment bit (bit 2) specifies whether the texture
is pegged at the top (clear) or the bottom (set). For two sided walls, the bit
determines whether the texture is pegged at the floor and ceiling of the inside
sector (clear), or the ceiling of the outside/facing sector (set).
*******************************************************************************/

struct SPRITE
{
	long x, y, z;
	ushort cstat;
	sshort picnum;
	schar shade;
	uchar pal, clipdist;
	uchar filler;
	uchar xrepeat, yrepeat;
	schar xoffset, yoffset;
	short sectnum, statnum;
	short ang;
	short owner;
	short xvel, yvel, zvel;
	short type;
	ushort flags;
	short extra;
};
/* SPRITE cstat bits
cstat, bit 0: 1 = Blocking sprite (use with clipmove, getzrange)    "B"
       bit 1: 1 = 50/50 transluscence, 0 = normal                   "T"
       bit 2: 1 = x-flipped, 0 = normal                             "F"
       bit 3: 1 = y-flipped, 0 = normal                             "F"
       bits 4-5: 00 = FACE sprite (default)                         "R"
                 01 = WALL sprite (like masked walls)
                 10 = FLOOR sprite (parallel to ceilings&floors)
                 11 = SPIN sprite (face sprite that can spin 2draw style - not done yet)
       bit 6: 1 = 1-sided sprite, 0 = normal                        "1"
       bit 7: 1 = Real centered centering, 0 = foot center          "C"
       bit 8: 1 = Blocking sprite (use with hitscan)                "H"
       bit 9: 1 = Reverse translucence
       bit 10: reserved
       bit 11: reserved
       bit 12: reserved
       bit 13: reserved
       bit 14: reserved
       bit 15: Force invisible sprite
*/
enum {
	kSpriteBlocking    	= 0x0001,
	kSpriteTranslucent 	= 0x0002,
	kSpriteFlipX       	= 0x0004,
	kSpriteFlipY       	= 0x0008,

	kSpriteFace        	= 0x0000,
	kSpriteWall        	= 0x0010,
	kSpriteFloor       	= 0x0020,
	kSpriteSpin        	= 0x0030,
	kSpriteRMask		= 0x0030,

	kSpriteOneSided    	= 0x0040,
	kSpriteOriginAlign	= 0x0080,
	kSpriteHitscan      = 0x0100,
	kSpriteTranslucentR	= 0x0200,

	kSpriteMapSecret	= 0x0400,
	kSpriteMapNever		= 0x0800,
	kSpriteMapAlways	= 0x1000,

	kSpriteMoveMask		= 0x6000,
	kSpriteMoveNone		= 0x0000,
	kSpriteMoveForward	= 0x2000,
	kSpriteMoveFloor	= 0x2000,
	kSpriteMoveReverse	= 0x4000,
	kSpriteMoveCeiling	= 0x4000,

	kSpriteInvisible    = 0x8000,
};


// SPRITE flag attributes
enum {
	kAttrMove			= 0x0001,	// is affected by movement physics
	kAttrGravity		= 0x0002,	// is affected by gravity
	kAttrFalling		= 0x0004,	// in z motion

	kAttrSmoke			= 0x0100,	// receives tsprite smoke/steam
	kAttrBloodSpray		= 0x0200,	// receives tsprite blood spray
};


#pragma pack(1);

struct RGB6 {
	uchar r,g,b;
};

#pragma pack;


#ifdef __cplusplus
extern "C" {
#endif


/***********************************************************************
 * Engine global variables
 **********************************************************************/

//static int artfil;
/* DOS handle for currently open art file. */

//static char artfilename[];
/* Name of art file, i.e., "TILES000.ART" */

//static int artfilnum;
/* Number of the currently open art file. */

extern long artsize;
/* The total size in bytes of all the tile pieces.  Ken's engine will try to
allocate a cache this large. */

extern long artversion;
/* The current art version is now 1.  If artversion is not 1 then
either it's the wrong art version or something is wrong.  */

extern long asm1, asm2, asm3, asm4;
/* These are temporary holding registers for the interface from Ken's C code to
his assembly code. I guess he isn't using Watcom's parameter passing scheme. */

extern char automapping;
/* Set to 1 to automatically mark seen walls and sprites to appear in the map
*/

extern char britable[16][64];
/* Ken's gamma table */

extern short bunchfirst[];
/* Used internally by drawrooms().  Used as indexes into thewall[] */

extern short bunchlast[];
/* Used internally by drawrooms() */

extern short capturecount;
/* Increments each time a screen capture is done.  Use to sequentially number
capture files. */

extern int chainnumpages, chainsiz;
extern uchar *chainplace;

//extern short cliplist[];
/* used by getcliplines() */

// static short clipnum;
/* used by getcliplines() */

// static short clipobjectval[];
/* used by getcliplines() */

// static short clipsectnum;
/* used by getcliplines() */

// static short clipsectorlist[];
/* used by getcliplines() */

// static long clipx1[], clipx2[], clipy1[], clipy2[];
/* used by getcliplines() */

extern long cosglobalang;
/* temporary holder for a cos value */

extern char curbrightness;
/* The current brightness level (0 - 4) */

extern char curpag;
/* used for VESA mode */

extern long dahorizbak;
/* copy of horiz value passed to drawrooms() saved for internal use */

extern short dmost[MAXXDIM];
/* at the beginning of each drawing cycle, startdmost[] gets copied to dmost[] */

extern long dplc[];

extern long dwall[];

extern short editstatus;
/* Set by build.  Alters behavior of certain engine features */

extern BYTE *frameplace;
/* location of the virtual frame buffer for page 0 */

extern short globalang;
extern long globalbufplc;
extern short globalcursectnum;
extern long globalhoriz;
extern short globalorientation;
extern long globalpal;
extern long globalpalwritten;
extern short globalpicnum;
extern long globalposx;
extern long globalposy;
extern long globalposz;
extern char globalshade;
extern short globalshiftval;
extern long globalx1;
extern long globalx2;
extern long globalxpanning;
extern char globalxshift;
extern long globaly1;
extern long globaly2;
extern long globalypanning;
extern long globalyscale;
extern char globalyshift;
extern long globalzd;
extern long globalzx;

extern BYTE gotpic[kMaxTiles >> 3];
/* A bitmapped array of flags for each tile. The engine will set the respective
gotpic bit for each picnum drawn to the screen, including ceilings, floors,
walls, sprites, masked walls - even overwritesprites, etc.   This array is
mainly for making the game only calculate special bitmapped effects when that
certain picnum is on the screen. */

extern BYTE gotsector[kMaxSectors >> 3];
/* It works a lot like the gotpic array, but this array determines which
sectors were considered during the drawrooms function.  Note that gotsector,
unlike gotpic IS cleared to 0 during every call to drawrooms. */

extern short headspritesect[kMaxSectors+1];
/* list starting with headspritesect[kMaxSectors] is unused sprites. */

extern short headspritestat[kMaxStatus+1];
/* list starting with headspritestat[kMaxSectors] is unused sprites. */

extern short highlightcnt;
/* number of points to be highlighted in 2d mode */

extern hitcnt;
/* not used. size unknown */

// static short hitwalls[];


extern long lastx[];
/* used internally to sychronize floor and ceiling drawing with walls */

extern short linehighlight;
/* wall to be highlighted in 2d mode */

//static long localtileend;
/* Localtileend is the tile number of the last tile in this art file. */

//static long localtilestart;
/* tile number of the first tile in this art file */

extern long lplc[];

extern long lwall[];

extern long mapversion;
/* version of currently loaded board.  Should be 5 */

extern short maskwall[];
/* list of walls to be drawn in drawmasks().  Value is used as index to
thewall[] and thesector[] */

extern short maskwallcnt;
/* number of walls to be drawn in drawmasks() */

extern short nextspritesect[kMaxSprites];

extern short nextspritestat[kMaxSprites];

extern short numbunches;

extern long numframes;
/* The number of times the draw3dscreen function was called since the engine
was initialized.  This helps to determine frame rate.  (Frame rate = numframes
* 120 / totalclock.) */

extern short numhits;

extern short numpalookups;
/* number of levels used for depth cueing.  Normally 32, but determined by
palette.dat */

extern long numpages;

extern short numscans;

extern short numsectors;
/* the total number of existing sectors.  Modified every time you call the
loadboard function. */

//static long numtilefiles;

extern long numtiles;
/* the number of tiles found TILES.DAT. */

extern short numwalls;
/* the total number of existing walls.  Modified every time you call the
loadboard function. */

extern short p2[];
/* used internally for wall drawing.  Creates a linked list of references to
thewall[] */

extern long page;
/* number of the page being rendered */

extern long pageoffset;
/* offset of the active page (in pixels) */

extern RGB6 palette[256];

extern char paletteloaded;
/* Set to 1 after the palette has been loaded. */

extern uchar *palookup[MAXPALOOKUPS];
/* The palookup array is an array of pointers that point to the first byte of
each 8K palette lookup table.  All 256 pointers are initialized to NULL by
initengine() except for palookup[0] which is the default 8K palette.  This will
allow you to modify the palette lookup table directly for non-snowy fading
effects, etc.  Each palette lookup table has 32 shades.  Each shade has 256
bytes.  Shade 0 is closest (actual palette brightness) and shade 31 is farthest
(dark usually).  (256*32 = 8192 or 8K) */

extern long parallaxscale;
/* Controls the ratio at which the parallaxing skies scroll in relation to the
horizon.  Default is 65536.  With lower values, you don't need as much artwork
and can look higher. */

extern char parallaxtype;
/* 0 = Totally flat parallaxing sky.  1 = X-only stretching parallaxing sky
(This is what DOOM uses).  2 = X and Y stretching parallaxing sky. */

extern long parallaxyoffs;
/* It defaults to 0.  If you set it to 100, then all parallaxing skies will be
properly moved 100 pixels higher. */

extern short pskyoff[kMaxSkyTiles];
extern short pskybits;
/* pskyoff[kMaxSkyTiles] is an array of OFFSETS of each tile from the
picnum of the parallaxing sky. pskybits is NOT the actual number of tiles,
but the log (base 2) of the number of tiles.  Look at this table:
 For 1 tile, pskybits = 0
 For 2 tiles, pskybits = 1
 For 4 tiles, pskybits = 2
 For 8 tiles, pskybits = 3
 For 16 tiles, pskybits = 4, etc.

Most teams have a 1024 wide parallaxing sky that wraps all the way around.
Don't worry - this is the default now.  When initengine is called, the
variables default to this:  pskyoff[0] = 0; pskybits = 0;

You may have used a 512 wide parallaxing sky (like in my game) that repeated
every 180 degrees.  To make this work with the new version, set these
variables like this right after initengine is called:
 pskyoff[0] = 0;
 pskyoff[1] = 0;
 pskybits = 1;

Note that both pskyoff variables are 0 here.  This will make the parallaxing
sky repeat. With the new tiling, you can save memory by making small chuck
sizes, such as 64 or 128, and repeating certain sections.
*/

extern char pcxheader[];
/* Ken's doesn't use a structure for the pcx header */

extern uchar *pic;
/* Pointer to a large buffer used for the tile cache. Wall offsets are pointers
to locations within this block.  */

extern struct PICANM {
	unsigned frames : 5;	// number of frames - 1
	unsigned update : 1;	// this came from upper bit of frames
	unsigned type : 2;		// 0 = none, 1 = Oscil, 2 = Frwd, 3 = Bkwd
	signed xcenter : 8;
	signed ycenter : 8;
  	unsigned speed : 4;		// (clock >> speed) determines rate
	unsigned view : 3;
	unsigned registered : 1;
} picanm[kMaxTiles];

/* Upper 4 bits: y size shift; lower 4 bits: x size shift */
extern uchar picsiz[kMaxTiles];

extern short pointhighlight;
/* point to be highlighted in 2d mode */

extern posfil;
/* not used */

extern short prevspritesect[kMaxSprites];

extern short prevspritestat[kMaxSprites];

extern long qsetmode;
/* number of scanlines for current mode (480, 350, 200).  Can be use to
determine current display mode */

extern short radarang[];

extern short radarang2[];
/* table of angles used for sweeping across the screen */

extern long randomseed;

extern long rx1[];
/* Used for wall drawing */

extern long rx2[];
/* Used for wall drawing */

extern long ry1[];
/* Used for wall drawing */

extern long ry2[];
/* Used for wall drawing */

//static BYTE *screen;
/* same as frameplace */

extern short searchit;

extern short searchsector;

extern short searchstat;
/* Indicates the type of object being pointed to in 3d mode in the editor.
When searchstat == 0, it's wall searchwall; searchstat == 1, it's ceiling of
searchsector; searchstat == 2, it's floor of searchsector; searchstat == 3,
it's sprite in searchwall; searchstat == 4, it's masked wall in searchwall. */

extern short searchwall;

extern long searchx;

extern long searchy;

extern SECTOR sector[kMaxSectors];

// static short sectorborder[];

// static short sectorbordercnt;

extern BYTE show2dsprite[kMaxSprites>>3];
/* These variables are for auto-mapping with the draw2dscreen function.  When
you load a new board, these bits are all set to 0 - since you haven't mapped
out anything yet.  Note that these arrays are bit-mapped.  If you want
draw2dscreen() to show sprite #54 then you say:
	spritenum = 54;
	show2dsprite[spritenum>>3] |= (1<<(spritenum&7));
And if you want draw2dscreen() to not show sprite #54 then you say:
 	spritenum = 54;
	show2dsprite[spritenum>>3] &= ~(1<<(spritenum&7));
*/

extern BYTE show2dwall[kMaxWalls>>3];
extern BYTE show2dsector[kMaxSectors>>3];

extern char showinvisibility;
/* If you set this to 1, then all sprites with the invisible bit set will be
shown.  This is useful for editing invisiblie sprites. */

extern long singlobalang;

extern short sintable[2048];
/* sin table in 2:14 format
If you plan to use sintable, 2 identities you may want to keep in mind are:
	sintable[ang&2047]       = sin(ang * (3.141592/1024)) * 16383
	sintable[(ang+512)&2047] = cos(ang * (3.141592/1024)) * 16383
 */

extern char smalltextfont[1024];

extern short smost[];

extern short smostcnt;

extern short smoststart[];

extern short smostwall[];

extern short smostwallcnt;

extern SPRITE sprite[kMaxSprites];

extern long spritesortcnt;

extern long sqrtable[2048];

extern short startang;
/* not used */

extern short startdmost[MAXXDIM];
/* array of the lowest y-coordinates on each column that the engine is allowed
to write to.  You need to set it only once. */

extern long startposx, startposy, startposz;
/* not used */

extern short startsectnum;
/* not used */

extern short startumost[MAXXDIM];
/* array of the highest y-coordinates on each column that the engine is allowed
to write to.  You need to set it only once. */

extern long stereofps;
extern char stereopage;
extern long stereopixelwidth;
extern long stereowidth;

extern long swall[];

extern long swplc[];

extern char tablesloaded;
/* set to 1 when tables have been loaded.  Duh. */

extern long takernum;
/* Reflects the current activity of the engine; tracked for time statistics in
timetakercount[] */

extern short tantable[2048];
/* not used */

extern char textfont[1024];

extern short thesector[];
/* list of sector ids for masked walls to be drawn in drawmasks() */

extern short thewall[];
/* list of wall ids for masked walls to be drawn in drawmasks() */

extern char tilefilenum[];
/* list of art file numbers which contain each tile */

extern long tilefileoffs[];
/* offset of the tile within the art file. */

extern short tilesizx[kMaxTiles];
/* simply the x-dimension of the tile number. */

extern short tilesizy[kMaxTiles];
/* simply the y-dimension of the tile number. */

extern char timenames[][20];
/* text strings for each of the time statistic engine activities */

extern long timetakercount[];
/* Used to track engine statistics.  Each array element contains the total
ticks for each activity. */

extern volatile long totalclock;
/* TOTALCLOCK - When the engine is initialized, TOTALCLOCK is set to zero.  It
is up to the game timer code to update totalclock.  The timing value in
totalclock is used to synchronize tile animations. */

extern long totalclocklock;
/* Gets set to totalclock at the beginning of drawrooms(). */

extern uchar *transluc;
/* table (256 * 256) for combining translucent colors */

extern char tsengdriver;
/* not used */

extern SPRITE tsprite[kMaxSprites];

extern short umost[MAXXDIM];
/* at the beginning of each drawing cycle, startumost[] gets copied to umost[] */

extern long uplc[];

extern long uwall[];

extern char vesapageshift;
/* used for page flipping in VESA mode */

extern char vgapal16[48];
/* REG 16 color palette used for writing 16 color screen captures to PCX files */

extern long viewoffset;
/* Linear offset of the upper left of the 3d view window */

extern long visibility;
/* for special effects such as a gun shooting.  A value from 5 to around 15.
13 is normal, the lower the number, the darker.  */

extern long parallaxvisibility;
/* visibility value for parallaxed skies */

extern WALL wall[kMaxWalls];
/* This is the list of all the walls. */

extern BYTE *waloff[kMaxTiles];
/* pointers to the raw bitmap data for each tile. */

extern long windowx1, windowx2, windowy1, windowy2;
/* Window coordinates passed to setview(). */

extern long xb1[];
/* Used internally for wall drawing */

extern long xb2[];
/* Used internally for wall drawing */

extern long xdimen;
/* Width of the 3d view window */

extern long xdimenoffs;
/* x offset of the 3d view window.  Not used. */

extern long xdimenscale;
/* (xdimen << 16) / 320 */

extern long xdimscale;
/* (320 << 16) / xdimen */

extern long yb1[];
/* Used internally for wall drawing */

extern long yb2[];
/* Used internally for wall drawing */

//extern long ybot;

extern long ydim16;
/* clip y max for 16 color drawing.  Also determines whether drawing takes
place on the 2d map or on the status bar. */

extern long ydimen;
/* Height of the 3d view window */

//extern long ytop;

extern char vidoption;
/* Vidoption is simply the first parameter you pass to initengine.  Xdim and
Ydim are the screen sizes you pass to initengine, such as 320*200 or 640*480.
Ylookup is a lookup table that works like this:  If (vidoption == 0) ylookup[i]
= ((i*xdim)>>2); if (vidoption != 0) ylookup[i] = i*xdim; There is 1 exception:
If you are using a chained mode which can only fit only 1 viewing page, then
the engine actually does a screen-buffer mode and blits to the chained screen
so for this case, ylookup[i] = i*xdim. */

extern long xdim, ydim, ylookup[MAXYDIM+1];

extern long yxaspect, xyaspect;


/***********************************************************************
 * Engine functions
 **********************************************************************/

int addboxlinum( short nSector, short nWall );
/* Called internally by scansector(). */

void alignceilslope(short sectnum, long x, long y, long z);
void alignflorslope(short sectnum, long x, long y, long z);
/*These functions will align a slope to a given (x, y, z) point.  It will make
the slope pass through the point.  The function will do nothing if the point is
collinear to the first wall of the sector. */

void allocache(long *bufptr, long bufsiz, char *lockptr);
/* Called internally by loadtile() */

ulong allocatepermanenttile( short tilenume, long xsiz, long ysiz );
/* This function allocates a place on the cache as permanent.  Right now, I
reset the cache every time you call this function so I would recommend calling
this function right after loadpics.  Returns a pointer to the tile bitmap or 0
on failure. */

short animateoffs( short nTile, ushort nInfo );
/* Returns the current frame for the animating tile based on totalclocklock.
nInfo is used to identify what type of object is being animated.
nInfo is nSector, nWall | 0x4000, nSprite | 0x8000, or 0xC000 (just ignore-it's
for rotatesprite) */

int bunchfront( short, short );
/* Used internally by drawrooms() */

long calcksqrt( long );
/* Used internally to create sqrtable[] */

void calcwalls( short );
/* Used internally by drawrooms() */

BOOL cansee( long x1, long y1, long z1, short sectnum1, long x2, long y2, long z2, short sectnum2 );
/* This function determines whether or not two 3D points can "see" each other
or not.  All you do is pass it the coordinates of a 3D line defined by two 3D
points (with their respective sectors)  The function will return a 1 if the
points can see each other or a 0 if there is something blocking the two points
from seeing each other.  This is how I determine whether a monster can see you
or not. Try playing DOOM1.DAT to fully enjoy this great function! */

void ceilscan( long, long, short );
/* Used internally by calcwalls() */

int changespritesect(short spritenum, short newsectnum);
/* Changes the sector of sprite (spritenum) to the newsector (newsectnum).
This function may become internal to the engine in the movesprite function.
But this function is necessary since all the sectors have their own
doubly-linked lists of sprites. Returns -1 if sector doesn't change. */

int changespritestat(short spritenum, short newstatnum);
/* Changes the status of sprite (spritenum) to status newstatus).  Newstatus
can be any number from 0 to kMaxStatus-1.  You can use this function to put a
monster on a list of active sprites when it first sees you. Returns -1 is
statnum doesn't change.  */

void clear2dscreen( void );
/* Clears the drawing page in 16 color mode */

extern void clearview(int nColor);
/* clears the view to the specified color */

unsigned clipinsidebox(long x, long y, short wallnum, long walldist);
/* Clipinsidebox is used for clipping to determine whether a player or sprite
is too close to a wall.  X and y are the position of the sprite or player.
Wallnum is the wall to test, and walldist is the fatness of the sprite or
player (same as clipmove).  It returns a 1 if the sprite or player's clipping
square intersects the wall or 0 if not.  */

unsigned clipmove( long *x, long *y, long *z, short *sectnum, long xvect, long yvect,
	long walldist, long ceildist, long flordist, char cliptype );
/* Moves any object (x, y, z) in any direction at any velocity and will make
sure the object will stay a certain distance from walls (walldist).  Pass the
pointers of the starting position (x, y, z, and sectnum.)  These values will be
modified accordingly.  Pass the direction and velocity by using a vector xvect,
yvect).  Walldist tells how close the object can get to a wall.  If you
increase walldist for a certain object, the object might leak through a wall,
so don't do that!  If cliptype is 0, then the clipping is normal (Use 0 to clip
you and monsters).  If the cliptype is 1, then the object is clipped to the
same things that hitscan is clipped to (use 1 for all bullets).

Clipmove can either return 0 (touched nothing), 0x8000 + wallnum (wall first
touched), 0xC000 + spritenum (sprite first touched).

Clipmove uses getceilzofslope and getflorzofslope when checking for overlapping. */

void completemirror( void );
/* called after preparemirror, drawrooms, viewProcessSprites and drawmasks in mirrorDraw() */

void copytilepiece( long tilenume1, long sourcex1, long sourcey1,
	long xsiz, long ysiz, long tilenume2, long destx1, long desty1 );
/* This function simply copies any section of a source tile to any part of a
destination tile.  It will automatically skip transparent pixels.  It will
wrap-around in the source but not the destination.  If for some reason the
destination tile gets removed from the cache, the destination tile will be
reset to original form.  Use allocatepermanenttile() to prevent the tile from
being purged.  */

void deletesprite( short nSprite );
/* Deletes the sprite. */

void deletespritesect( short nSprite );
/* Remove the sprite from the sector linked list.  Called by deletesprite() */

void deletespritestat( short nSprite );
/* Remove the sprite from the status linked list.  Called by deletesprite() */

void dragpoint( short wallnum, long newx, long newy );
/* This function will drag a point in the exact same way a point is dragged in
2D EDIT MODE using the left mouse button.  Simply pass it which wall to drag
and then pass the new x and y coordinates for that point.  Please use this
function because if you try to drag points yourself, I can guarantee that it
won't work as well as mine and you will get confused.  Note:  Every wall of
course has 2 points.  When you pass a wall number to this function, you are
actually passing 1 point, the left side of the wall given that you are in the
sector of that wall)  Got it? */

void draw2dgrid( long x, long y, short ang, long zoom, short grid );
/* Draws the grid on the 2d screen.  Used internally by draw2dscreen() */

int drawoverheadmap(long cposx, long cposy, long czoom, short cang);
int drawmapview(long cposx, long cposy, long czoom, short cang);

void draw2dscreen( long x, long y, short ang, long zoom, short grid );
/* Draws the 2d screen - this function is a direct replacement for the
drawrooms() and drawmasks() functions.  Be sure to call either qsetmode640350()
or qsetmode640480() first.  When switching back to 3d mode, be sure to call
setgamemode().  IMPORTANT NOTES:  1.  The overwritesprite function should
only be called in 3D mode.  If you do this in 2D mode, junk will be written to
the 2D screen and a crash is possible.  2.  When you switch back to 3D mode,
you should call the permanentwritesprite functions to draw the status bar, or
whatever else you have to draw.  3.  You must call the nextpage() function in
both 2D and 3D modes. */

void drawalls( short );
/* Used internally by drawrooms() */

void drawline16(long x1, long y1, long x2, long y2, char nColor);
void drawline256(long x1, long y1, long x2, long y2, char nColor);

void drawmasks( void );
/* This function draws all the sprites and masked walls to the current drawing
page which is not yet shown.  The reason I have the drawing split up into these
2 routines is so you can animate just the sprites that are about to be drawn
instead of having to animate all the sprites on the whole board.  Drawrooms()
prepares these variables:  spritex[], spritey[], spritepicnum[], thesprite[],
and spritesortcnt.  Spritesortcnt is the number of sprites about to be drawn to
the page.  To change the sprite's picnum, simply modify the spritepicnum array
If you want to change other parts of the sprite structure, then you can use the
thesprite array to get an index to the actual sprite number. */

void drawmaskwall( short index );
/* Draws the masked wall referenced in maskwall[index]. */

void drawrooms( long posx, long posy, long posz, short ang, long horiz,
	short cursectnum );
/* This function draws the 3D screen to the current drawing page, which is not
yet shown.  This way, you can overwrite some things over the 3D screen such as
a gun.  Be sure to call the drawmasks() function soon after you call the
drawrooms() function.  To view the screen, use the nextpage() function.  The
nextpage() function should always be called sometime after each draw3dscreen()
function. */

void drawsprite( long x, long y, short nTile, short nSprite, char pal);
/* Draw the specified sprite to the 3d screen.  Called internally by
drawmasks(). */

void faketimerhandler( void );
/* Called at various places in engine.  Must be supplied externally. */

void florscan( long xb1, long xb2, short );
/* Called internally by calcwalls() */

short getangle( long xvect, long yvect );
/* returns (short)angle; Use this function call to determine the angle between
two points.  For example, if you want a monster to shoot a bullet towards you,
you would get the bullet's angle this way:  sprite[bullet].ang = getangle(posx
- sprite[monst].x, posy - sprite[monst].y); */

long getceilzofslope(short sectnum, long x, long y);
long getflorzofslope(short sectnum, long x, long y);
void getzsofslope(short sectnum, long x, long y, long *ceilz, long *florz);
/* Returns the z coordinate of the ceiling/floor at that x, y location.  If the
sector doesn't have a ceiling/floor slope then it immediately returns the
sector[].floorz or sector[].ceilingz so it's not that slow.  You may want to
check for slopes yourself ceilingstat&2/floorstat&2 if you think the overhead
of calling these functions are too slow. */

void getcliplines( long x, long y, long z, long newX, long newY, short sectnum,
	long walldist, long ceildist, long floordist, char cliptype);
/* generates a list of clip areas for clipmove */

void getmousevalues( short *xMickeys, short *yMickeys, short *buttons );
/* Retrieves mouse info using int 33h, 0Bh and int 33h, 05h */

int getpalookup(long nVis, long nShade);

unsigned getpixel16( long offset );
/* retrieves pixel color addressed by linear pixel number.  Weird.  Used by
screencapture(). */

void getzrange( long x, long y, long z, short sectnum,
	long *ceilZ, long *ceilHit, long *floorZ, long *floorHit,
	long walldist, char cliptype );
/* Determines the vertical range for an area.  Use this in conjunction with
clipmove.  This function will keep the player from falling off cliffs when
you're too close to the edge.  This function finds the highest and lowest z
coordinates that your clipping BOX can get to.  It must search for all sectors
(and sprites) that go into your clipping box.  This method is better than using
sector[].ceilingz and sector[].floorz because this searches the whole clipping
box for objects, not just 1 point.  Pass x, y, z, sector normally.  Walldist
can be 128.  Cliptype can be 0, 1, or 2. (just like movesprite and clipmove)
This function returns the z extents in ceilZ and floorZ. It will return the
object hit in ceilHit and floorHit.  CeilHit and floorHit will also be either:
0x4000 | sector (sector first touched) or 0xC000 | spritenum (sprite first
touched) */


void grouvline( long, long, long, short );
/* used internally by calcwalls() */

void hitscan( long xstart, long ystart, long zstart, short startsectnum,
	long vectorx, long vectory, long vectorz, short *hitsect,
	short *hitwall, short *hitsprite, long *hitx, long *hity, long *hitz );
/* Pass the starting 3D position:  (xstart, ystart, zstart, startsectnum) Then
pass the 3D angle to shoot (defined as a 3D vector):  (vectorx, vectory,
vectorz) Then set up the return values for the object hit:  (hitsect, hitwall,
hitsprite) and the exact 3D point where the ray hits:  (hitx, hity, hitz)

How to determine what was hit:

	Hitsect is always equal to the sector that was hit (always >= 0).

	If the ray hits a sprite then:
		hitsect = thesectornumber
		hitsprite = thespritenumber
		hitwall = -1

	If the ray hits a wall then:
		hitsect = thesectornumber
		hitsprite = -1
		hitwall = thewallnumber

	If the ray hits the ceiling of a sector then:
		hitsect = thesectornumber
		hitsprite = -1
		hitwall = -1
		vectorz < 0 (If vectorz < 0 then you're shooting upward which means
		that you couldn't have hit a floor)

	If the ray hits the floor of a sector then:
		hitsect = thesectornumber
		hitsprite = -1
		hitwall = -1
		vectorz > 0 (If vectorz > 0 then you're shooting downward which means
		that you couldn't have hit a ceiling)
*/

void hline( long, long, long, short );
/* used internally by ceilscan() and floorscan() */

void initcache(long cachestart, long cachesize);
/* clears all but permanent tiles from the cache */

void initengine( int vidoption, int xdim, int ydim );
/* Sets up interrupt vectors for keyboard, and initializes many variables for
the BUILD engine.  You should call this once before any other functions of the
BUILD engine are used.  vidoption can be anywhere from 0-6 xdim,ydim can be any
mode x resolution if vidoption = 0 xdim,ydim can be any vesa resolution if
vidoption = 1 xdim,ydim must be 320*200 for any other mode. (see graphics mode
selection in my setup program) */

void initkeys( void );

unsigned initmouse();

void initspritelists( void );
/* Called internally by loadboard().  Creates the linked sector and status
lists based on the sprite sectnum and statnum fields. */

short insertsprite(short sectnum, short statnum);
/* Returns spritenum.  Whenever you insert a sprite, you must pass it
the sector number, and a status number (statnum).  The status number can be any
number from 0 to kMaxStatus-1.  Insertsprite works like a memory allocation
function and returns the sprite number.  */

short insertspritesect( short sectnum );
/* Called internally by insertsprite().  Adds a sprite to the sector linked
list.  Returns spritenum. */

short insertspritestat( short statnum );
/* Called internally by insertsprite().  Adds a sprite to the status linked
list.  Returns spritenum. */

unsigned inside( long x, long y, short sectnum );
/* Tests to see whether the overhead point (x, y) is inside sector (sectnum)
Returns either 0 or 1, where 1 means it is inside, and 0 means it is not. */

void keepaway( long *, long *, long, short );
/* Called internally by clipmove(). */

void __interrupt __far keyhandler( void );
/* Ken's keyboard interrupt routine.  */

void * kmalloc( size_t size );
void kfree( void * ptr );
/* Ken's replaceable memory handling functions, that just call malloc and free. */

unsigned krand( void );
/* This simply returns a random number.  You can easily set the random seed by
externing the randomseed variable as a long.  This is useful for keeping the
random seed the same on multiple computers when playing multi-player mode.
Note: this random number routine is extremely simplistic and can very easily
generate patterns.  Use rand() instead. */

ushort ksqrt( long num );
/* returns square root.  A square root function optimized for integers.  Use
this function only if you want to.  */

short lastwall( short nWall );
/* Returns the number of the wall whose .point2 equals nWall.  Returns nWall if
no previous wall is found. */

BOOL lintersect( long x, long y, long z, long dx, long dy, long dz, long xc,
	long yc, long xd, long yd, long *xint, long *yint, long *zint);
/* Determines if and where and if the line segment specified by origin (x,y,z)
and magnitude (dx,dy,dz) intersects with line segment CD.  If the lines
intersect, returns TRUE and sets x, y, and z to the point of intersection. */

int loadboard( char *filename, long *posx, long *posy, long *posz, short *ang,
	short *cursectnum );
/* Loads the given board file into memory for the BUILD engine.  Returns -1 if
file not found.  If no extension is given, .MAP will be appended to the
filename. */

void loadpalette( void );
/* Loads "PALETTE.DAT".  Gets palette, color lookup tables, and transparency
table. */

int loadpics( char *filename );
/* Loads the given artwork file into memory for the BUILD engine.  Returns -1
if file not found.  If no extension is given, .ART will be appended to the
filename. */

void loadredbluepalette( void );
/* Loads special red/blue palette for stereo vision */

void loadtables( void );
/* Reads "TABLES.DAT" and "SETUP.DAT".  Builds sqrtable[], loads sintable[],
tantable[], radarang[], * textfont, smalltextfont, and setup options. */

void loadtile( short nTile );
/* Loads the specified tile into the cache.  Called internally when
waloff[nTile] == NULL */

void makepalookup( long palnum, char *remapbuf, char red, char green, char blue, char stat );
/* This function allows different shirt colors for sprites.  First prepare
remapbuf, which is a 256 byte buffer of chars which the colors to remap.
Palnum can be anywhere from 1-15.  Since 0 is where the normal palette is
stored, it is a bad idea to call this function with palnum = 0.  The last 3
parameters are the color that the palette fades to as you get further away.
This color is normally black 0,0,0).  White would be (63,63,63).  This allows
you to do fog and other effects.
if ((stat&1) == 0) then makepalookup will allocate & deallocate the memory block
for use but will not waste the time creating a palookup table (assuming you will
create one yourself)
if ((dastat&1) != 0) then makepalookup will allocate & deallocate the memory
block AND create a palookup table using the rgb values you pass.*/

void makeradarang( long width );
/* Builds the radarang2[] array */

void maskwallscan( long xb1, long xb2, long *uwall, long *dwall,
	long *swall, long *lwall );
/* Draws the masked wall.  Called by drawmaskwall() */

unsigned movesprite(short spritenum, long dx, long dy, long dz,
	long ceildist, long flordist, char cliptype, long numtics);
/* This function moves the sprite given by spritenum by the 3 increments, dx,
dy, and dz.  Walldist tells how close the object can get to a wall.  I use 128L
as my default.  If you increase walldist all of a sudden for a certain object,
the object might leak through a wall, so don't do that!  If cliptype is 0, then
the clipping is normal (Use 0 to clip you and monsters).  If the cliptype is 1,
then the object is clipped to the same things that hitscan is clipped to (use 1
for all bullets).  Movesprite can either return 0 (touched nothing),
kHitFloor | nSector (floor hit), kHitCeiling | nSector (ceiling hit),
kHitWall | nWall (wall hit), or kHitSprite | nSprite (sprite hit).
*/

void neartag(
	long x, long y, long z, short sectnum, // Starting position
	short ang,  			//Starting angle
	short *neartagsector,	//Returns near sector if sector[].tag != 0
	short *neartagwall,     //Returns near wall if wall[].tag != 0
	short *neartagsprite,   //Returns near sprite if sprite[].tag != 0
	long *neartaghitdist,   //Returns actual distance to object (scale: 1024=largest grid size)
	long tagrange,			//Choose maximum distance to scan (scale: 1024=largest grid size)
	char tagsearch );		//1=lotag, 2=hitag, 3=lotag and hitag
/* Neartag works sort of like hitscan, but is optimized to scan only close
objects and scan only objects with tags != 0.  Neartag is perfect for the first
line of your space bar code.  It will tell you what door you want to open or
what switch you want to flip. */

void nextpage( void );
/* After a screen is prepared, use this function to view the screen. */

int nextsectorneighborz( short sectnum, long thez, short topbottom, short direction );
/* This function searches z-coordinates of neighboring sectors to find the
closest (next) ceiling starting at the given z-coordinate (thez).  For example,
if you want to find the goal z-coordinate when opening a door, you might want
the door to stop at the next closest neighboring ceiling z-coordinate.  You can
get the z-coordinate this way:

	newz = sector[nextsectorneighborz(sectnum,startz,-1,-1)].ceilingz

	topbottom =	-1:	search ceilings
				1:	search floors
	direction = -1:	search upwards
   				1:	search downwards
*/

enum {
	kOrientNormal		= 0,
	kOrientMiddle		= 0x01,
	kOrientScale		= 0x02,
	kOrientTranslucent	= 0x04,
	kOrientXFlip		= 0x08,
	kOrientYFlip		= 0x10,
};

//void overwritesprite(long x, long y, short nTile, signed char shade,
//	char orientation, char nPal );
/* Use this function to draw any sprites that must be drawn to the screen for
every single frame, such as a gun or a menu system.

	Bit 0 (1) of orientation = 0: (x, y) is top-left corner
	Bit 0 (1) of orientation = 1: (x, y) is middle
	Bit 1 (2) of orientation = 0: no relation to viewing window
	Bit 1 (2) of orientation = 1: scale and clip to viewing window
	Bit 2 (4) of orientation = 0: normal
	Bit 2 (4) of orientation = 1: 50/50 transluscent!
	Bit 3 (8) of orientation = 0: normal
	Bit 3 (8) of orientation = 1: x-flipped
	Bit 4 (16) of orientation = 0: normal
	Bit 4 (16) of orientation = 1: y-flipped

If it works at full screen, simply set bit 1 of orientation to 1, and it should
automatically scale properly!

Use this function to write sprites over the 3d view.  For example, you can make
a menu system with this function.  Be sure that you call this function for
every single frame after the 3d view is drawn or else it will be flashed on for
only 1 frame.  If you want x and y to be the top left corner, set the
orientation to 0.  If you want x and y to be the middle of the sprite, set the
orientation to 1.  The reason I included the orienation = 1 option is so that
if you want a sprite centered and the size of the tile changes, you don't need
to recompile and guess where the new top left corner is.  This function will
clip the sprite to the startumost and startdmost arrays.  nPal refers to a
palette lookup list (normally 0).  */

void plotpixel(long x, long y, uchar hue);

uchar getpixel(long x, long y);

void precache( void );
/* Load all tiles used in the current map into the cache */

void preparemirror( long x, long y, long z, short ang, long horiz,
	short wallnum, short sectnum, long *tx, long *ty, short *tang );
/* called before drawrooms, viewProcessSprites, drawmasks and completemirror in mirrorDraw() */

void printext16(int x, int y, short nForeColor, short nBackColor, char *text, char nFont);
/* color -1 is transparent, nFont 0=8*8, 1=3*5 */

void printext256(int x, int y, short nForeColor, short nBackColor, char *text, char nFont);
/* color -1 is transparent, nFont 0=8*8, 1=3*5 */

void printext( long x, long y, char buffer[42], short tilenum, char invisiblecol );
/* Use this function to print text anywhere on the screen from a font that you
can create in EDITART.  Please see my example font in TILES.ART to see how I
lay out the user-defined font.  X ranges from 0-319. Y ranges from 0-199.  The
buffer is the string to print.  Tilenum specifies which font to use.
Invisiblecol tells printext what color to draw the transparent pixels.  If
invisiblecol is 255 then the transpararent pixels are still transparent. */

void printmessage16(char *);
void printmessage256(char *);
/* quick way to display message in the top right of status bar in 2d mode */

void __interrupt __far printscreeninterrupt( void );
/* Gets called when the PrScr key is pressed,  Currently passes it to INT 5 */

void qsetmode640350( void );
/* Set to the 2D map mode #1 (640*350*16) */

void qsetmode640480( void );
/* Set to the 2D map mode #2 (640*480*16) */

int raytrace( long, long, long, long, long *, long * );
/* Used internally by clipmove(). */

void resettiming( void );
/* Resets timing, such as setting totalclock = 0.  Also resets other timers.
This is for use with the showengineinfo */

int rintersect( int x, int y, int z, int dx, int dy, int dz, int xc,
	int yc, int xd, int yd, int *xint, int *yint, int *zint);
/* Determines if and where and if the ray specified by origin (x,y,z) and
magnitude (dx,dy,dz) intersects with line segment CD.  If the lines intersect,
returns 1 and sets x, y, and z to the point of intersection. */

void rotatepoint( long xpivot, long ypivot, long x, long y, short dAng,
	long *newX, long *newY );
/* Rotatepoint will rotate point(x,y) around point(xpivot,ypivot) by the
dAng value.  The resultant point will be s */

enum {
	kRotateNormal		= 0,
	kRotateTranslucent	= 0x01,
	kRotateScale		= 0x02,
	kRotateYFlip		= 0x04,
	kRotateUnclipped	= 0x08,
	kRotateStatus		= 0x0A,
	kRotateCorner		= 0x10,
	kRotateTranslucentR	= 0x20,
	kRotateNoMask		= 0x40,
	kRotateAllPages		= 0x80,
};

void rotatesprite(long sx, long sy, long zoom, short ang,
	short picnum, schar shade, char pal, char flags,
	long cx1, long cy1, long cx2, long cy2);
/*
	(sx, sy) is the center of the sprite to draw defined as
		 screen coordinates shifted up by 16.
	(zoom) is the zoom in 16:16 format. Normal zoom is 65536.
	(ang) is the angle (0 is straight up)
	(picnum) is the tile number
	(shade) is 0 normally but can be any standard shade
	(pal) is the palookup number from 0-255.
	flags & 0x01:	Translucence
	flags & 0x02:	Scale to viewing window
	flags & 0x04:	Y flip
	flags & 0x08:	Don't clip to startumost/startdmost.
	flags & 0x10:	Position by top left corner instead of origin
	flags & 0x20:	Translucence using reverse translucency values
	flags & 0x40:	Don't check for mask color (faster)
	flags & 0x80:	Write sprite successively to all pages

	Auto-scale mode (kRotateScale) will automatically scale from 320*200
	resolution coordinates to the clipping window passed (cx1, cy1, cx2, cy2).
	In auto-scale mode, don't pre-scale the (sx, sy) coordinates.  Simply pass
	(sx, sy) as if the resolution was 320*200 even though it may be different.
	This means that you shouldn't use xdim or ydim to get (sx, sy).

	(cx1, cy1, cx2, cy2) - The clipping window.  These coordinates are never
	scaled, not even in auto-scale mode.  Usually you should pass them as
	(windowx1,windowy1,windowx2,windowy2) for things scaled to the viewing
	window or (0L,0L,xdim-1L,ydim-1L) for things scaled to full screen.
	Probably the only time you wouldn't follow this rule is if you program a
	non-scaled tiled background function.

	Note:  As a special case, if both (flags & 2) && (flags & 8) then
	rotatesprite will scale to the full screen (0,0,xdim-1,ydim-1) rather than
	setview's viewing window.  (windowx1,windowy1,etc.)  This case is useful
	for status bars, etc.
*/

int saveboard( char *filename, long *posx, long *posy, long *posz, short *ang,
	short *cursectnum );
/* Saves the given board from memory inro the specified filename.  Returns -1
if unable to save.  If no extension is given, .MAP will be appended to the
filename. */

void scansector( short nSector );
/* Used internally by drawrooms() and drawalls(). */

void screencapture(char *filename);
/* Capture the screen and save it as a .BMP file.  I don't know why my .BMP
format isn't compatible with other programs. */

short sectorofwall(short nWall);
/* Returns the sector index for a given wall. */

void setaspect( long xRange, int aspectRatio );
/* Parameter aspectRatio is a 16:16 fraction that is pixWidth/pixHeight.  Note
you will need to pass in the reciprical fraction if you use 4/3 * ydim/xdim.
For a standard 90ø 320*200 screen, xRange and aspectRatio are 65536.  For
square aspect ratio at 320*400, set aspectRatio to 131072.  Since xRange is
actually zoom, you must modify the aspect ratio inversely if you only want to
change the viewing angle.  */

void setbrightness(char brightness, char *pal);
/* Simply call this function where brightness ranges from 0 to 4.  Brightness
defaults to 0.  Levels 1-4 are all brighter than 0.  If you switch between 2D &
3D modes, the engine will remember the current brightness level.  */

void setgamemode( void );
/* This function sets the video mode to 320*200*256color graphics.  Since BUILD
supports several different modes including mode x, mode 13h, and other special
modes, I don't expect you to write any graphics output functions.  Soon I have
all the necessary functions)  If for some reason, you use your own graphics
mode, you must call this function again before using the BUILD drawing
functions. */

void setsprite(short spritenum, long newx, long newy, long newz);
/* This function simply sets the sprite's position to a specified coordinate
newx, newy, newz) without any checking to see whether the position is
valid or not.  You could directly modify the sprite[].x, sprite[].y, and
sprite[].z values, but if you use my function, the sprite is guaranteed to
be in the right sector. */

void setverts( long, long, long, long, long, long );
/* Used internally by stereoinit() */

void setvesamode( long mode );
/* Initializes hires VESA 3d mode */

void setview( long x1, long y1, long x2, long y2 );
/* Change the size of the 3d view window */

void setviewtotile( short tile, long x, long y );
void setviewback( void );
/*  */

void showengineinfo( void );
/* Use this function after setting to text mode to view some statistics about
the engine, such as frame rate.  */

int spritewallfront( short nSprite, short nWall );
/* determines z order of sprites and masked walls.  Used internally by
drawmasks(). */

void stereoblit( int page, int screen );
/* flips pages in stereo view mode */

void stereoinit( void );
/* initializes stereo view mode */

void transmaskvline( long );
/* Used internally by transmaskwallscan(). */

void transmaskwallscan( long xb1, long xb2 );
/* Called by drawmaskwall() for walls that are translucent. */

void uninitcache( int );

void uninitengine( void );
/* Restores interrupt vectors for keyboard and timer, and frees buffers.  You
should call this once at the end of the program before quitting to dos. */

void uninitkeys( void );

void updatesector( long x, long y, short *sectnum );
/* This function updates the sector number according to the x and y values
passed to it.  Be careful when you use this function with sprites because
remember that the sprite's sector number should not be modified directly.  If
you want to update a sprite's sector, I recomment using setsprite(). */

int wallfront( short nWall1, short nWall2);
/* determines wall z order */

int wallmost( long *, long, long );
/* used internally in calcwalls() */

void wallscan( long xb1, long xb2, long *uwall, long *dwall, long *swplc, long *lplc );
/* Draws walls.  Called internally by calcwalls(). */

void __interrupt __far zerohandler( void );
/* Ken's spiffy divide by zero handler.  Obsolete. */

/***********************************************************************
 * Assembly helper functions
 **********************************************************************/
void fixtransluscence( void * );
/* Fixup self modifying assembly to point to the current translucence table */

void setpalookupaddress( void *);
/* Fixup self modifying assembly to point to the current palookup table */

void setupvlineasm(int shift);
#pragma aux setupvlineasm parm [eax];

void setupmvlineasm(int shift);
#pragma aux setupmvlineasm parm [eax];

extern long vplce[4];	// this is the v coordinate
extern long vince[4];	// this is the dv coordinate
extern BYTE *palookupoffse[4];	// this is the pointer to the 256 byte subtable
extern BYTE *bufplce[4];	// this is the pointer to the texture

long vlineasm1(long dv, BYTE *pal, long count, long v, BYTE *source, BYTE *dest);
#pragma aux vlineasm1 parm [eax][ebx][ecx][edx][esi][edi];

long mvlineasm1(long vinc, BYTE *pal, long, long vplc, BYTE *source, BYTE *dest);
#pragma aux mvlineasm1 parm [eax][ebx][ecx][edx][esi][edi];

long vlineasm4(long count, BYTE *dest);
#pragma aux vlineasm4 parm [ecx][edi] modify [eax ebx ecx edx esi edi];

long mvlineasm4(long count, BYTE *dest);
#pragma aux mvlineasm4 parm [ecx][edi] modify [eax ebx ecx edx esi edi];

#ifdef __cplusplus
}
#endif

#endif /* __ENGINE_H */


















