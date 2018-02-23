#ifndef __FONT_H
#define __FONT_H

#include <limits.h>
#include <stdarg.h>
#include "globals.h"

#define kFontNameSize  7
#define kFontMaxTiles  (SCHAR_MAX + 1)
#define kFontMaxText   128
#define kFontNoTile    -1
#define kFontVersion   0x0100

#define kFontSig		"FNT\x1A"

enum {
	kFontJustLeft   = 0x01,
	kFontJustRight  = 0x02,
	kFontJustCenter = 0x03,
};

struct FONTHEADER
{
	char signature[ 4 ];
	short version;
};

struct FONTDATA {
	FONTHEADER header;
	char  name[ kFontNameSize + 1 ];
	uchar charSpace;
	uchar lineSpace;
	uchar wordSpace;
	short firstTile;
	schar tileOffset[ kFontMaxTiles ];
	short tileWidth[ kFontMaxTiles ];
};

class Font {
	RESHANDLE  hFont;
	FONTDATA  *pFont;

private:
	// if nTile == -1, text is drawn to the screen
	// justFlags enumerated in FONT.H
	int DrawText( int nDestTile, unsigned justFlags, int x, int y, char *zText );

public:
	Font(void);
//	~Font();
	void Load( char *zName );
	void Unload( void );
	int  Print( int x, int y, char *zFormat, ... );
	int  Center( int x, int y, char *zFormat, ... );
	int  Draw( int nTile, int x, int y, char *zFormat, ... );
};

#endif /* __FONT_H */
