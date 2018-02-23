#ifndef __VIEW_H
#define __VIEW_H

#include "typedefs.h"
#include "engine.h"

struct QFONT;

/***********************************************************************
 * Constants
 **********************************************************************/
enum {
	kView2D     = 2,
	kView3D     = 3,
	kView2DIcon = 4,
};	// used with gViewMode

enum FONT_ID
{
	kFontMessage	= 0,
	kFontMenu,
};

enum
{
	TA_LEFT	= 0,
	TA_CENTER,
	TA_RIGHT,
};


enum VIEWPOS {
	kViewPosCenter = 0,
	kViewPosBack,
	kViewPosLeftBack,
	kViewPosLeft,
	kViewPosLeftFront,
	kViewPosFront,
	kViewPosRightFront,
	kViewPosRight,
	kViewPosRightBack,
};	// used with gViewPos

enum {
	// rotatesprite compatible flags
	kDrawNormal			= 0,
	kDrawTranslucent	= 0x0001,
	kDrawScale			= 0x0002,
	kDrawYFlip			= 0x0004,
	kDrawUnclipped		= 0x0008,
	kDrawStatus			= 0x000A,
	kDrawCorner			= 0x0010,
	kDrawTranslucentR	= 0x0020,
	kDrawNonMasked		= 0x0040,
	kDrawMultiPage		= 0x0080,

	// viewDrawSprite specific flags
	kDrawXFlip			= 0x0100,
};


/***********************************************************************
 * Variables
 **********************************************************************/
extern int gViewIndex;
extern int gViewSize;
extern int gViewMode;
extern VIEWPOS gViewPos;
extern int gViewPages;
extern int gZoom;

extern int gInterpolate;

extern int gViewX0, gViewY0, gViewX1, gViewY1;
extern int gViewXCenter, gViewYCenter;

extern int gShowFrameRate;
extern int gShowFrags;
extern long gScreenTilt;

extern int deliriumTilt;
extern int deliriumTurn;
extern int deliriumPitch;

/***********************************************************************
 * Functions
 **********************************************************************/
extern void viewInit(void);
extern void viewResizeView(int change);
extern void viewSetMessage( char *s );
extern void viewDisplayMessage( void );
extern void viewDrawChar( QFONT *pFont, BYTE c, int x, int y, BYTE *pPalookup );
extern void viewDrawText( FONT_ID nFontId, char *s, int x, int y, int shade, int nPLU,
	int nAlign = TA_LEFT );
extern void viewDrawScreen( void );
extern void viewDrawInterface( void );
extern void viewBackupPlayerLoc( int nPlayer );
extern void viewBackupSpriteLoc( int nSprite, SPRITE *pSprite );
extern void viewBackupAllSpriteLoc( void );
extern void viewProcessSprites( int x, int y, int z );
extern void viewCalcPosition( SPRITE *pSprite, long *px, long *py, long *pz, short *pAngle, short *pSector );
extern void viewSetDetail( int nDetail );

extern void viewDrawSprite( long sx, long sy,
	long nZoom, short nAngle, short nTile, schar nShade, char nPLU, ushort nFlags,
	long wx1, long wy1, long wx2, long wy2 );

#endif

