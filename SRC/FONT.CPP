#include <stdio.h>
#include <string.h>
#include <io.h>

#include "debug4g.h"
#include "error.h"
#include "font.h"
#include "names.h"

#include "misc.h" // remove before release, used for FileSave

FONTDATA testFont = {
	{ "FNT", kFontVersion },
	"7777777",
	 2,
	 5,
	10,
	kFontTech,
	{	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
		15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
		31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46,
		47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62,
		63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78,
		79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, -1
	},
	{	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	}
};

Font::Font(void)
{
	hFont = NULL;
	pFont = NULL;
}

//Font::~Font()
//{
//	hFont = NULL;
//	pFont = NULL;
//}

void Font::Unload( void )
{
	if ((hFont == NULL) && (pFont != NULL))
	{
		// forces deallocation in the event the font was created
		// internally and not loaded as a resource via Lookup()
		// this code should go away once all fonts are resources
		dprintf("Unloading font %s\n",pFont->name);
		Resource::Free( pFont );
		pFont = NULL;
	}
}

void Font::Load( char *zName )
{
	dassert( zName != NULL );

	hFont = gSysRes.Lookup( zName, ".fnt" );

#if 0
	if (hFont != NULL) {
		pFont = (FONTDATA *)gSysRes.Lock( hFont );

		dprintf("Font: %s  Version: %d\n", pFont->name, pFont->header.version);
		dprintf("charSpace: %d  lineSpace: %d  wordSpace: %d\n",
			pFont->charSpace, pFont->lineSpace, pFont->wordSpace);
 		dprintf("firstTile: %d\n", pFont->firstTile);

		gSysRes.Unlock( hFont );
		pFont = NULL;
	}
#endif

	if (hFont == NULL) {
		pFont = (FONTDATA *)Resource::Alloc( sizeof(FONTDATA) );
		if (pFont == NULL)
			ThrowError("Error allocating FNT resource", ES_ERROR );

		*pFont = testFont; // memcpy( pFont, testFont, sizeof(FONTDATA) );

		// manually generate a width table until we have font tools
		for (int i = 0; i < SCHAR_MAX; i++ )
		{
			if (pFont->tileOffset[i] == -1)
				pFont->tileWidth[i] = pFont->wordSpace;
			else
				pFont->tileWidth[i] = tilesizx[ pFont->firstTile + pFont->tileOffset[i] ];
		}

		// create and null terminate the internal font name
		strncpy(pFont->name, zName, kFontNameSize);
		*(pFont->name + kFontNameSize) = '\0';

		// save the file until we have font tools
		char zFilename[_MAX_PATH];
		strcpy(zFilename,zName);
		strcat(zFilename,".fnt");
		FileSave(zFilename, pFont, sizeof(FONTDATA) );

		//dprintf("Error creating resource handle for %s.fnt\n", zName );
		//ThrowError("Error reading FNT resource", ES_ERROR );
	}
}

int Font::DrawText( int nDestTile, unsigned justFlags, int x, int y, char *zText )
{
	char *p;
	short nWidth;

	if (hFont != NULL)
		pFont = (FONTDATA *)gSysRes.Lock( hFont );

	for (p = zText, nWidth = 0; *p != '\0'; p++)
	{
		nWidth += pFont->tileWidth[ (*p) & 0x7F ] + pFont->charSpace;
	}
	nWidth -= pFont->charSpace;

	switch (justFlags)
	{
//		case kFontJustLeft:
//			break;
		case kFontJustRight:
			x -= nWidth;
			break;
		case kFontJustCenter:
			x -= nWidth / 2;
			break;
	}

	for (p = zText; *p != '\0'; p++)
	{
		int cIndex = (*p) & 0x7F;
		int nTile = pFont->firstTile + pFont->tileOffset[ cIndex ];

		// handle drawing to the screen or an allocated tile
		if (nDestTile == -1) {
			if (pFont->tileOffset[ cIndex ] == -1) {
				x += pFont->wordSpace;
			} else {
//				overwritesprite(x, y, (short)nTile, 0, 0, 0);
				x += pFont->tileWidth[ cIndex ] + pFont->charSpace;
			}
		} else {
			if (pFont->tileOffset[ cIndex ] == -1) {
				x += pFont->wordSpace;
			} else {
				(void)y;
//				copytilepiece(nTile, 0, 0, tilesizx[nTile], tilesizy[nTile],
//					nDestTile, x, y);
				x += pFont->tileWidth[ cIndex ] + pFont->charSpace;
			}
		}
	}

	if (hFont != NULL) {
		gSysRes.Unlock( hFont );
		pFont = NULL;
	}

	return 0;
}

int Font::Print( int x, int y, char *zFormat, ... )
{
	int     len;
	va_list argptr;
	char    buf[ kFontMaxText + 1 ];

	// print variable argument string into buf for output
	va_start( argptr, zFormat );
	len = vsprintf( buf, zFormat, argptr );
	va_end( argptr );

	if (len >= kFontMaxText) {  // buf overwritten? return error.
		ThrowError("Message length exceeded buffer length!", ES_ERROR );
	}
	else if (len >= 0) {   // no vsprintf error? print buf.
		DrawText( -1, kFontJustLeft, x, y, buf );
	}

	return len;
}

int Font::Center( int x, int y, char *zFormat, ... )
{
	int     len;
	va_list argptr;
	char    buf[ kFontMaxText + 1 ];

	// print variable argument string into buf for output
	va_start( argptr, zFormat );
	len = vsprintf( buf, zFormat, argptr );
	va_end( argptr );

	if (len >= kFontMaxText) {  // buf overwritten? return error.
		ThrowError("Message length exceeded buffer length!", ES_ERROR );
	}
	else if (len >= 0) {   // no vsprintf error? print buf.
		DrawText( -1, kFontJustCenter, x, y, buf );
	}

	return len;
}

int Font::Draw( int nTile, int x, int y, char *zFormat, ... )
{
	int     len;
	va_list argptr;
	char    buf[ kFontMaxText + 1 ];

	// print variable argument string into buf for output
	va_start( argptr, zFormat );
	len = vsprintf( buf, zFormat, argptr );
	va_end( argptr );

	if (len >= kFontMaxText) {  // buf overwritten? return error.
		ThrowError("Message length exceeded buffer length!", ES_ERROR );
	}
	else if (len >= 0) {   // no vsprintf error? print buf.
		DrawText( nTile, kFontJustLeft, x, y, buf );
	}

	return len;
}
