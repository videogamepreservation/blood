#ifndef __BUILD_H
#define __BUILD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine.h"

/***********************************************************************
 * Build global variables
 **********************************************************************/

extern short ang;

extern long vel, svel, angvel;

extern short asksave;		// set to 1 to indicate changes have been made

//static char boardfilename[13];

extern char buildkeys[];

//static short brightness;

extern short cursectnum;

//static long fillist[640];

//static int gettilezoom;

//static short grid;

//static short gridlock;

extern short highlight[kMaxWalls];

extern short highlightsector[kMaxSectors];

extern short highlightsectorcnt;

extern long horiz;

//static long hvel;

//static short localartfreq[kMaxTiles];

//static short localartlookup[kMaxTiles];

//static short localartlookupnum;

//static long lockclock, long lockspeed;

//static char menuname[MAXMENUFILES][17], curpath[80], menupath[80];
//static long menunamecnt, menuhighlight;

extern char names[kMaxTiles][17];

//static long numsprites;

//static short oldmousebstatus;

extern long posx, posy, posz;

//static char pskysearch[kMaxSectors];

//static long repeatcountx, repeatcounty;

//static char scantoasc[128];

//static char scantoascwithshift[128];

//static short showtags;

//static char somethingintab = 255;

//static long synctics = 0, lockclock = 0;

extern short temppicnum, tempcstat, templotag, temphitag, tempextra;
extern char tempshade, temppal, tempxrepeat, tempyrepeat;
extern char somethingintab;

extern char totalclockinreal;	// unused

extern long whitecol;

extern long zlock, zmode, kensplayerheight;
extern short defaultspritecstat;

//static long zoom;

/***********************************************************************
 * Build function prototypes
 **********************************************************************/

void adjustmark( long, long, long );

char changechar( char dachar, long dadir, char smooshyalign, char boundcheck );

int checkautoinsert( long, long, long );

void checksectorpointer( short nWall, short nSector );

void clearmidstatbar16( void );	// Clear middle of status bar

int clockdir( long );

void copysector( short nSector1, short nSector2, short nWall, char);

void deletepoint( short nWall );

void deletesector( short nSector );

void drawtilescreen( long, long );

void editinput( void );

void fillsector( short nSector, char );

void fixrepeats( short wall );

void fixspritesectors( void );

void flipwalls( short, short );

int getcard( void );

void getfilenames( char * );

int getlinehighlight( int x, int y );

short getnumber16(char namestart[80], short num, long maxnumber);

short getnumber256(char namestart[80], short num, long maxnumber);

void getpoint( int x, int y, int *a, int *b );
/* converts a screen x,y into a map x,y in 2D mode */

int getpointhighlight( int x, int y );
/* returns closest wall vertice or sprite | 0x4000 to the specified point */

long gettile( long );

void initmenupaths( char * );

void inittimer( void );

void insertpoint( short nWall, long x, long y );

void keytimerstuff( void );

void loadnames( void );

int loopinside( long x, long y, short nWall );

short loopnumofsector( short, short, short nWall );

int menuselect( void );

void movewalls( short nWall, int );

int numloopsofsector( short nSector );

void overheadeditor( void );

void printcoords16( long x, long y, short ang );

void showmouse( void );

void showsectordata( short nSector );

void showspritedata( short nSprite );

void showwalldata( short nWall );

void sortfilenames( void );

void uninittimer( void );

void updatenumsprites( void );

int whitelinescan( short nWall );

#ifdef __cplusplus
}
#endif

#endif
