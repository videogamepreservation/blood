#ifndef __SCREEN_H
#define __SCREEN_H

#include "typedefs.h"


/***********************************************************************
 * Constants
 **********************************************************************/
enum {
	kPLUNormal 		= 0,
	kPLURed			= 2,
	kPLUCultist2	= 3,
	kPLUSpider3		= 4,
	kPLUGray		= 5,
	kPLUGrayish		= 6,
	kPLUSpider1		= 7,
	kPLUSpider2		= 8,
	kPLUFlame		= 9,
	kPLUCold		= 10,
	kPLUPlayer1		= 11,		// also kPLUPlayer5
	kPLUPlayer2,				// also kPLUPlayer6
	kPLUPlayer3,				// also kPLUPlayer7
	kPLUPlayer4,				// also kPLUPlayer8
	kMaxPLU,
};

enum {
	kPalNormal		= 0,
	kPalWater,
	kPalBeast,
	kMaxPalettes,
};

#define kPalLookups 64

extern int gGammaLevels;
extern BOOL gFogMode;
extern BYTE gStdColor[32];


/***********************************************************************
 * Functions
 **********************************************************************/
BYTE scrFindClosestColor( int r, int g, int b );
void scrCreateStdColors( void );
void scrLoadPLUs( void );
void scrLoadPalette( void );
void scrSetMessage( char * );
void scrDisplayMessage( int nColor );
void scrSetPalette(int nPal);
void scrSetGamma( int nGamma );
void scrDacRelEffect(int r, int g, int b);
void scrDacAbsEffect(int r, int g, int b);
void scrSetupFade( BYTE r, BYTE g, BYTE b );
void scrSetupUnfade( void );
void scrFadeAmount( int nFrac );
void scrSetDac( ulong nTicks );
void scrInit(void);
void scrSetGameMode( void );
void scrNextPage( void );

#endif


