#include <string.h>

#include "engine.h"
#include "multi.h"

#include "typedefs.h"
#include "misc.h"
#include "gameutil.h"
#include "trig.h"
#include "view.h"
#include "globals.h"
#include "textio.h"
#include "names.h"
#include "db.h"
#include "debug4g.h"
#include "resource.h"
#include "error.h"
#include "player.h"
#include "screen.h"
#include "options.h"
#include "tile.h"
#include "levels.h"
#include "actor.h"
#include "sectorfx.h"
#include "mirrors.h"
#include "fire.h"
#include "weapon.h"
#include "map2d.h"
#include "gfx.h"

#include <memcheck.h>

#define kStatBarHeight	25

/***********************************************************************
 * Variables
 **********************************************************************/
int gViewIndex = 0;
int gViewSize = 1;
int gViewMode = kView3D;
VIEWPOS gViewPos = kViewPosCenter;
int gZoom = 1024;
int gViewX0, gViewY0, gViewX1, gViewY1;
int gViewXCenter, gViewYCenter;
static long messageTime = 0;
static char message[256];

static int pcBackground = 0;

int gShowFrameRate = FALSE;
int gShowFrags = FALSE;

long gScreenTilt = 0;

int deliriumTilt = 0;
int deliriumTurn = 0;
int deliriumPitch = 0;

struct PLOCATION
{
	int x, y, z;
	int bobHeight, bobWidth;
	int swayHeight, swayWidth;
	int horiz;
	short ang;
} gPrevPlayerLoc[kMaxPlayers];

struct LOCATION
{
	int x, y, z;
	short ang;
} gPrevSpriteLoc[kMaxSprites];

int gInterpolate;

int gViewPosAngle[] =
{
	0,						// kViewPosCenter
	kAngle180,				// kViewPosBack
	kAngle180 - kAngle45,	// kViewPosLeftBack
	kAngle90,				// kViewPosLeft
	kAngle45,				// kViewPosLeftFront
	0,						// kViewPosFront
	-kAngle45,				// kViewPosRightFront
	-kAngle90,				// kViewPosRight
	-kAngle90 - kAngle45,	// kViewPosRightBack
};


void viewDrawChar( QFONT *pFont, BYTE c, int x, int y, BYTE *pPalookup )
{
	dassert(pFont != NULL);
	y += pFont->baseline;

	CHARINFO *pInfo = &pFont->info[c];
	int sizeX = pInfo->cols;
	int sizeY = pInfo->rows;
	int nSize = sizeX * sizeY;
	if ( nSize == 0 )
		return;

	Rect dest(x, y + pInfo->voffset, x + sizeX, y + sizeY + pInfo->voffset);
	Rect screen(0, 0, xdim, ydim);
	dest &= screen;

	if ( !dest )
		return;

	BYTE *pSource = &pFont->data[pInfo->offset];

	for (int i = 0; i < 4; i++)
	{
		palookupoffse[i] = pPalookup;
		vince[i] = 0x00010000;
	}

 	BYTE *bufplc = pSource;
	BYTE *p = frameplace + ylookup[dest.y0] + dest.x0;

	x = dest.x0;

	// dword align to video memory
	while (x < dest.x1 && (x & 3) )
	{
		mvlineasm1(0x00010000, pPalookup, sizeY - 1, 0, bufplc, p);
		bufplc += sizeY;
		p++;
		x++;
	}

	while ( x + 3 < dest.x1 )
	{
		for (i = 0; i < 4; i++)
		{
			bufplce[i] 	= bufplc;
			bufplc += sizeY;
			vplce[i] = 0;
		}
		mvlineasm4(sizeY, p);
		p += 4;
		x += 4;
	}

	while ( x < dest.x1 )
	{
		mvlineasm1(0x00010000, pPalookup, sizeY - 1, 0, bufplc, p);
		bufplc += sizeY;
		p++;
		x++;
	}
}


void viewDrawText( FONT_ID nFontId, char *string, int x, int y, int shade, int nPLU, int nAlign )
{
	char *s;
	dassert(string != NULL);

	RESHANDLE hFont = gSysRes.Lookup(nFontId, ".QFN");
	dassert(hFont != NULL);
	QFONT *pFont = (QFONT *)gSysRes.Lock(hFont);

	BYTE *pPalookup = palookup[nPLU] + (getpalookup(0, shade) << 8);
	setupmvlineasm(16);

	if ( nAlign != TA_LEFT )
	{
		int nWidth = -pFont->charSpace;
		for ( s = string; *s; s++ )
			nWidth += pFont->info[*s].hspace + pFont->charSpace;

		if ( nAlign == TA_CENTER )
			nWidth >>= 1;

		x -= nWidth;
	}

	for ( s = string; *s; s++ )
	{
		viewDrawChar(pFont, *s, x, y, pPalookup);
		x += pFont->info[*s].hspace + pFont->charSpace;
	}

	gSysRes.Unlock(hFont);
}




static void TileSprite( int nTile, int shade, int nPLU, int x0, int y0, int x1, int y1)
{
	long x, y, i;

	Rect clip(x0, y0, x1, y1);
	Rect screen(0, 0, xdim, ydim);
	clip &= screen;

	if ( !clip)
		return;

	dassert(nTile >= 0 && nTile < kMaxTiles);
	int sizeX = tilesizx[nTile];
	int sizeY = tilesizy[nTile];
	int nSize = sizeX * sizeY;
	if ( nSize == 0 )
		return;

	BYTE *palookupoffs = palookup[nPLU] + (getpalookup(0, shade) << 8);
	BYTE *pTile = tileLoadTile(nTile);
	BYTE *wrap = pTile + nSize;

	setupvlineasm(16);

	for (i = 0; i < 4; i++)
	{
		palookupoffse[i] = palookupoffs;
		vince[i] = 0x00010000;
	}

	int yNext;
	for (y = clip.y0; y < clip.y1; y = yNext)
	{
		yNext = ClipHigh(IncBy(y, sizeY), clip.y1);
		x = clip.x0;

		BYTE *bufplc = pTile + (x % sizeX) * sizeY + (y % sizeY);
		BYTE *p = frameplace + ylookup[y] + x;

		// dword align to video memory
		while (x < clip.x1 && (x & 3) )
		{
			vlineasm1(0x00010000, palookupoffs, yNext - y - 1, 0, bufplc, p);
			bufplc += sizeY;
			if (bufplc >= wrap) bufplc -= nSize;
			p++;
			x++;
		}

		while ( x + 3 < clip.x1 )
		{
			for (i = 0; i < 4; i++)
			{
				bufplce[i] 	= bufplc;
				bufplc += sizeY;
				if (bufplc >= wrap) bufplc -= nSize;
				vplce[i] = 0;
			}
			vlineasm4(yNext - y, p);
			p += 4;
			x += 4;
		}

		while ( x < clip.x1 )
		{
			vlineasm1(0x00010000, palookupoffs, yNext - y - 1, 0, bufplc, p);
			bufplc += sizeY;
			if (bufplc >= wrap) bufplc -= nSize;
			p++;
			x++;
		}
	}
}


static void ClipSprite( int x, int y, int nTile, int shade, int nPLU, int x0, int y0, int x1, int y1 )
{
	Rect clip(x0, y0, x1, y1);
	Rect screen(0, 0, xdim, ydim);
	clip &= screen;

	if ( !clip)
		return;

	dassert(nTile >= 0 && nTile < kMaxTiles);
	int sizeX = tilesizx[nTile];
	int sizeY = tilesizy[nTile];
	int nSize = sizeX * sizeY;
	if ( nSize == 0 )
		return;

	Rect dest(x, y, x + sizeX, y + sizeY);
	dest &= clip;

	if ( !dest )
		return;

	Rect source(dest);
	source.offset(-x, -y);

	BYTE *palookupoffs = palookup[nPLU] + (getpalookup(0, shade) << 8);
	BYTE *pTile = tileLoadTile(nTile);

	setupvlineasm(16);

	for (int i = 0; i < 4; i++)
	{
		palookupoffse[i] = palookupoffs;
		vince[i] = 0x00010000;
	}

	BYTE *bufplc = pTile + (source.x0 % sizeX) * sizeY + (source.y0 % sizeY);
	BYTE *p = frameplace + ylookup[dest.y0] + dest.x0;

	x = dest.x0;

	// dword align to video memory
	while (x < dest.x1 && (x & 3) )
	{
		vlineasm1(0x00010000, palookupoffs, source.rows() - 1, 0, bufplc, p);
		bufplc += sizeY;
		p++;
		x++;
	}

	while ( x + 3 < dest.x1 )
	{
		for (i = 0; i < 4; i++)
		{
			bufplce[i] 	= bufplc;
			bufplc += sizeY;
			vplce[i] = 0;
		}
		vlineasm4(source.rows(), p);
		p += 4;
		x += 4;
	}

	while ( x < dest.x1 )
	{
		vlineasm1(0x00010000, palookupoffs, source.rows() - 1, 0, bufplc, p);
		bufplc += sizeY;
		p++;
		x++;
	}
}


void InitStatusBar( void )
{
	tileLoadTile(kPicStatBar);
}


static void DrawStatSprite( int nTile, int x, int y, int nShade = 0, int nPLU = kPLUNormal )
{
	rotatesprite(x << 16, y << 16, 0x10000, 0, (short)nTile, (schar)nShade, (char)nPLU,
		kRotateStatus | kRotateNoMask, 0, 0, xdim-1, ydim-1);
}


static void DrawStatMaskedSprite( int nTile, int x, int y, int nShade = 0, int nPLU = kPLUNormal )
{
	rotatesprite(x << 16, y << 16, 0x10000, 0, (short)nTile, (schar)nShade, (char)nPLU,
		kRotateStatus, 0, 0, xdim-1, ydim-1);
}


static void DrawStatNumber( char *sFormat, int n, int nTile, int x, int y, int xSpace )
{
	char buffer[80];
	sprintf(buffer, sFormat, n);
	for (int i = 0; i < strlen(buffer); i++)
	{
		if ( buffer[i] != ' ' )
			DrawStatSprite(nTile + buffer[i] - '0', x + i * xSpace, y);
	}
}


static void TileHGauge( int nTile, int x, int y, int n, int total )
{
	int nGauge = n * tilesizx[nTile] / total;

	ClipSprite(x, y, nTile, 0, 0, 0, 0, x + nGauge, ydim);
}


static void UpdateStatusBar(void)
{
	SPRITE *pSprite = gView->sprite;
	XSPRITE *pXSprite = gView->xsprite;
	int i;

	if (gViewMode == kView2D || gViewSize > 0)		// status bar present?
	{
		DrawStatMaskedSprite(2200, 160, 178);	// status bar
		DrawStatMaskedSprite(2234, 296, 179);	// heart

		int nHealthTile = 2231;	// green
		if ( pXSprite->health < (80 << 4) )
			nHealthTile = 2232;	// yellow
		if ( pXSprite->health < (40 << 4) )
			nHealthTile = 2227;	// red

		// show health amount
		DrawStatNumber("%3d", pXSprite->health >> 4, 2250, 157, 182, 5);

		TileHGauge(nHealthTile, 174, 180, pXSprite->health, 100 << 4);	// health meter
		TileHGauge(2226, 221, 192, gView->bloodlust, 100 << 4);	// lust

		// draw keys
		for (i = 0; i < 6; i++)
		{
			if ( gView->hasKey[i+1] )
				DrawStatSprite(2220 + i, 156 + i * 10, 193);
			else
				DrawStatSprite(2220 + i, 156 + i * 10, 193, 40, kPLUGrayish);
		}

		if ( gView->weapon != 0 )
		{
			DrawStatMaskedSprite(2229, 38 + gView->weapon * 13, 185);	// highlight

			// show ammo count
			if ( gView->weaponAmmo != kAmmoNone )
				DrawStatNumber("%3d", gView->ammoCount[gView->weaponAmmo], 2190, 13, 180, 9);
		}

		// display level name
		if ( gViewMode == kView2D )
			gfxDrawText(0, 160, gStdColor[kColorWhite], gLevelDescription);
	}

	if (gCacheMiss)
	{
		DrawStatSprite(kPicDisk, xdim - 15, ydim - 15);
		gCacheMiss = ClipLow(gCacheMiss - kFrameTicks, 0);
	}
}


void viewInit(void)
{
	gViewXCenter = xdim / 2;
	gViewYCenter = ydim / 2;

	gViewSize 		= BloodINI.GetKeyInt("View", "Size", 1);

	tioPrint("Initializing status bar");
	InitStatusBar();
}


void viewResizeView(int size)
{
	//int x0, y0, x1, y1;

	gViewSize = ClipRange(size, 0, 8);

	// full screen mode
	if (gViewSize == 0)
	{
		gViewX0 = 0;
		gViewY0 = 0;
		gViewX1 = xdim - 1;
		gViewY1 = ydim - 1;

		setview(gViewX0, gViewY0, gViewX1, gViewY1);
		return;
	}

	gViewX0 = 0;
	gViewY0 = 0;
	gViewX1 = xdim - 1;
	gViewY1 = ydim - kStatBarHeight;

	gViewX0 += (gViewSize - 1) * xdim / 16;
	gViewX1 -= (gViewSize - 1) * xdim / 16;
	gViewY0 += (gViewSize - 1) * ydim / 16;
	gViewY1 -= (gViewSize - 1) * ydim / 16;

	setview(gViewX0, gViewY0, gViewX1, gViewY1);

	pcBackground = numpages;
}


#define kBackTile 230
void viewDrawInterface( void )
{
	int x0, y0, x1, y1;

	if (gViewMode == kView3D && gViewSize > 1)
	{
		if ( pcBackground )
		{
			x0 = gViewX0;
			y0 = gViewY0;
			x1 = gViewX1;
			y1 = gViewY1;

			TileSprite(kBackTile, 20, kPLUNormal, 0, 0, xdim, y0-3); 		// top
			TileSprite(kBackTile, 20, kPLUNormal, 0, y1+4, xdim, ydim);	// bottom
			TileSprite(kBackTile, 20, kPLUNormal, 0, y0-3, x0-3, y1+4);		// left
			TileSprite(kBackTile, 20, kPLUNormal, x1+4, y0-3, xdim, y1+4);	// right

			TileSprite(kBackTile, 40, kPLUNormal, x0-3, y0-3, x0, y1+1);		// left
			TileSprite(kBackTile, 40, kPLUNormal, x0, y0-3, x1+4, y0); 		// top
			TileSprite(kBackTile, 0, kPLUNormal, x1+1, y0, x1+4, y1+4);		// right
			TileSprite(kBackTile, 0, kPLUNormal, x0-3, y1+1, x1+1, y1+4);		// bottom

			pcBackground--;
		}
	}

	UpdateStatusBar();

	powerupDraw( gView );
}


#if 0
short viewInsertTSprite( short nSector, short nStatus )
{
	short nTSprite = -1;

	nTSprite = (short)spritesortcnt;
	SPRITE *pTSprite = &tsprite[nTSprite];
	memset(pTSprite, 0, sizeof(SPRITE));
	pTSprite->type = kNothing;
	pTSprite->sectnum = nSector;
	pTSprite->statnum = nStatus;
	pTSprite->cstat = kSpriteOriginAlign;
	pTSprite->xrepeat = 64;
	pTSprite->yrepeat = 64;
	pTSprite->owner = -1;
	pTSprite->extra = -1;
	spritesortcnt++;

	return nTSprite;
}
#endif

static SPRITE *viewInsertTSprite( short nSector, short nStatus, SPRITE *pSource = NULL );

static SPRITE *viewInsertTSprite( short nSector, short nStatus, SPRITE *pSource )
{
	short nTSprite = -1;

	nTSprite = (short)spritesortcnt;
	SPRITE *pTSprite = &tsprite[nTSprite];
	memset(pTSprite, 0, sizeof(SPRITE));
	pTSprite->type = (short)-spritesortcnt;			// negated temporary tsprite index - cleared after display
	pTSprite->sectnum = nSector;
	pTSprite->statnum = nStatus;
	pTSprite->cstat = kSpriteOriginAlign;
	pTSprite->xrepeat = 64;
	pTSprite->yrepeat = 64;
	pTSprite->owner = -1;
	pTSprite->extra = -1;
	spritesortcnt++;

	if ( pSource != NULL )
	{
		pTSprite->x = pSource->x;
		pTSprite->y = pSource->y;
		pTSprite->z = pSource->z;
		pTSprite->owner = pSource->owner;
		pTSprite->ang = pSource->ang;
	}

	return &tsprite[nTSprite];
}


enum VIEW_EFFECT {
	kViewEffectShadow = 0,
	kViewEffectFlareHalo,
	kViewEffectCeilGlow,
	kViewEffectFloorGlow,
	kViewEffectTorchHigh,
	kViewEffectTorchLow,
	kViewEffectSmokeHigh,
	kViewEffectSmokeLow,
	kViewEffectFlame,
	kViewEffectSpear,
	kViewEffectBloodSpray,
	kViewEffectPhase,

	kViewEffectMax,
};

int effectDetail[ kViewEffectMax ] =
{
	kDetailLevelMax,	// kViewEffectShadow
	kDetailLevelMax,	// kViewEffectFlareHalo
	kDetailLevelMax,	// kViewEffectCeilGlow,
	kDetailLevelMax,	// kViewEffectFloorGlow,
	kDetailLevelMin,	// kViewEffectTorchHigh
	kDetailLevelMin,	// kViewEffectTorchLow
	kDetailLevelMin,	// kViewEffectSmokeHigh
	kDetailLevelMin,	// kViewEffectSmokeLow
	kDetailLevelMin,	// kViewEffectFlame
	kDetailLevelMin,	// kViewEffectSpear
	kDetailLevel2,		// kViewEffectBloodSpray
};


static void viewAddEffect( int nTSprite, VIEW_EFFECT nViewEffect )
{
	dassert( nViewEffect >= 0 && nViewEffect < kViewEffectMax );
	SPRITE *pTSprite = &tsprite[nTSprite];

	if ( gDetail < effectDetail[nViewEffect] )	// skip effects at higher detail levels
		return;

	// can more tsprites be inserted?
	if ( spritesortcnt < kMaxViewSprites )
	{
		switch (nViewEffect)
		{
			case kViewEffectPhase:
			{
				int nAngle = pTSprite->ang;

				// remember: things like butcher knive sprites are already +kAngle90 because they are wall sprites
				if (pTSprite->cstat & kSpriteWall)
					 nAngle = (nAngle + kAngle90) & kAngleMask;
				else
					 nAngle = (nAngle + kAngle180) & kAngleMask;

				for (int i=0; i<5 && spritesortcnt < kMaxViewSprites; i++)
				{
					short nSector;
					SPRITE *pTEffect = viewInsertTSprite( pTSprite->sectnum, 0x7FFF );
					pTEffect->ang = pTSprite->ang;
					pTEffect->x = pTSprite->x + mulscale30( (i+1) << 7, Cos(nAngle));
					pTEffect->y = pTSprite->y + mulscale30( (i+1) << 7, Sin(nAngle));
					pTEffect->z = pTSprite->z;
			        FindSector(pTEffect->x, pTEffect->y, pTEffect->z, &nSector);
					pTEffect->sectnum = nSector;
					pTEffect->owner = pTSprite->owner;
					pTEffect->picnum = pTSprite->picnum;
					pTEffect->cstat = (uchar)(pTSprite->cstat & ~kSpriteBlocking);
					pTEffect->cstat |= kSpriteTranslucent;
					if (i < 2)
						pTEffect->cstat |= kSpriteTranslucentR;
					pTEffect->shade = (schar)ClipLow(pTSprite->shade - 16, -128);
					pTEffect->xrepeat = pTSprite->xrepeat;
					pTEffect->yrepeat = pTSprite->yrepeat;
					pTEffect->picnum = pTSprite->picnum;
				}
				break;
			}
			case kViewEffectFlame:
			{
				SPRITE *pTEffect = viewInsertTSprite( pTSprite->sectnum, 0x7FFF, pTSprite);
				pTEffect->z = pTSprite->z;
				pTEffect->cstat = (uchar)(pTSprite->cstat & ~kSpriteBlocking);
				pTEffect->shade = kMaxShade;
				pTEffect->xrepeat = pTEffect->yrepeat =
					(uchar)(tilesizx[pTSprite->picnum] * pTSprite->xrepeat / 64);
				pTEffect->picnum = kAnmFlame3;
				pTEffect->statnum = kStatDefault; // show up in front of burning objects
				break;
			}

			case kViewEffectSmokeHigh:
			{
				SPRITE *pTEffect = viewInsertTSprite( pTSprite->sectnum, 0x7FFF, pTSprite);
				int zTop, zBot;
				GetSpriteExtents(pTSprite, &zTop, &zBot);
				pTEffect->z = zTop;
				if ( IsDudeSprite(pTSprite->owner)  )
					pTEffect->picnum = kAnmSmoke1;
				else
					pTEffect->picnum = kAnmSmoke2;
				pTEffect->cstat = (short)(pTSprite->cstat | kSpriteTranslucent & ~kSpriteBlocking);
				pTEffect->shade = 8;
				pTEffect->xrepeat = pTSprite->xrepeat;
				pTEffect->yrepeat = pTSprite->yrepeat;
				break;
			}

			case kViewEffectSmokeLow:
			{
				SPRITE *pTEffect = viewInsertTSprite( pTSprite->sectnum, 0x7FFF, pTSprite);
				int zTop, zBot;
				GetSpriteExtents(pTSprite, &zTop, &zBot);
				pTEffect->z = zBot;
				if ( IsDudeSprite(pTSprite->owner)  )
					pTEffect->picnum = kAnmSmoke1;
				else
					pTEffect->picnum = kAnmSmoke2;
				pTEffect->cstat = (short)(pTSprite->cstat | kSpriteTranslucent & ~kSpriteBlocking);
				pTEffect->shade = 8;
				pTEffect->xrepeat = pTSprite->xrepeat;
				pTEffect->yrepeat = pTSprite->yrepeat;
				break;
			}

			case kViewEffectTorchHigh:
			{
				SPRITE *pTEffect = viewInsertTSprite( pTSprite->sectnum, 0x7FFF, pTSprite);
				int zTop, zBot;
				GetSpriteExtents(pTSprite, &zTop, &zBot);
				pTEffect->z = zTop;
				pTEffect->picnum = kAnmFlame2;
				pTEffect->cstat = (short)(pTSprite->cstat & ~kSpriteBlocking);
				pTEffect->shade = kMaxShade;
				pTEffect->xrepeat = pTEffect->yrepeat =
					(uchar)(tilesizx[pTSprite->picnum] * pTSprite->xrepeat / 32);
				break;
			}

			case kViewEffectTorchLow:
			{
				SPRITE *pTEffect = viewInsertTSprite( pTSprite->sectnum, 0x7FFF, pTSprite);
				int zTop, zBot;
				GetSpriteExtents(pTSprite, &zTop, &zBot);
				pTEffect->z = zBot;
				pTEffect->picnum = kAnmFlame2;
				pTEffect->cstat = (short)(pTSprite->cstat & ~kSpriteBlocking);
				pTEffect->shade = kMaxShade;
				pTEffect->xrepeat = pTEffect->yrepeat =
					(uchar)(tilesizx[pTSprite->picnum] * pTSprite->xrepeat / 32);
				break;
			}

			case kViewEffectShadow:
			{
				SPRITE *pTEffect = viewInsertTSprite( pTSprite->sectnum, 0x7FFF, pTSprite);
				// insert a shadow
				pTEffect->z = getflorzofslope(pTSprite->sectnum, pTEffect->x, pTEffect->y);

				pTEffect->cstat = (uchar)(pTSprite->cstat | kSpriteTranslucent);
				pTEffect->shade = kMinShade;
				pTEffect->xrepeat = pTSprite->xrepeat;
				pTEffect->yrepeat = (uchar)(pTSprite->yrepeat >> 2);
				pTEffect->picnum = pTSprite->picnum;
				int nTile = pTEffect->picnum;

				// position it so it's based on the floor
				pTEffect->z -= (tilesizy[nTile] - (tilesizy[nTile] / 2 + picanm[nTile].ycenter)) * (pTEffect->yrepeat << 2);
				break;
			}

			case kViewEffectFlareHalo:
			{
				SPRITE *pTEffect = viewInsertTSprite( pTSprite->sectnum, 0x7FFF, pTSprite);
				// insert a lens flare halo effect around flare missiles
				pTEffect->z = pTSprite->z;
				pTEffect->cstat = (uchar)(pTSprite->cstat | kSpriteTranslucent);
				pTEffect->shade = kMaxShade;
				pTEffect->pal = kPLURed;
				pTEffect->xrepeat = pTSprite->xrepeat;
				pTEffect->yrepeat = pTSprite->yrepeat;
				pTEffect->picnum = kAnmFlareHalo;
				break;
			}

			case kViewEffectCeilGlow:
			{
				SPRITE *pTEffect = viewInsertTSprite( pTSprite->sectnum, 0x7FFF, pTSprite);
				SECTOR *pSector = &sector[pTSprite->sectnum];
				int zDist = (pTSprite->z - pSector->ceilingz) >> 8;
				pTEffect->x = pTSprite->x;
				pTEffect->y = pTSprite->y;
				pTEffect->z = pSector->ceilingz;
				pTEffect->cstat = (uchar)(pTSprite->cstat | kSpriteTranslucent | kSpriteFloor | kSpriteOneSided | kSpriteFlipY);
				pTEffect->shade = (schar)(-64 + zDist);
				pTEffect->pal = kPLURed;
				pTEffect->xrepeat = 64;
				pTEffect->yrepeat = 64;
				pTEffect->picnum = kAnmGlowSpot1;
				pTEffect->ang = pTSprite->ang;
				pTEffect->owner = pTSprite->owner;	//(short)nTSprite;
				break;
			}

			case kViewEffectFloorGlow:
			{
				SPRITE *pTEffect = viewInsertTSprite( pTSprite->sectnum, 0x7FFF, pTSprite);
				SECTOR *pSector = &sector[pTSprite->sectnum];
				int zDist = (pSector->floorz - pTSprite->z) >> 8;
				pTEffect->x = pTSprite->x;
				pTEffect->y = pTSprite->y;
				pTEffect->z = pSector->floorz;
				pTEffect->cstat = (uchar)(pTSprite->cstat | kSpriteTranslucent | kSpriteFloor | kSpriteOneSided);
				pTEffect->shade = (schar)(-32 + zDist);
				pTEffect->pal = kPLURed;
				pTEffect->xrepeat = (uchar)zDist;
				pTEffect->yrepeat = (uchar)zDist;
				pTEffect->picnum = kAnmGlowSpot1;
				pTEffect->ang = pTSprite->ang;
				pTEffect->owner = pTSprite->owner;	//(short)nTSprite;
				break;
			}

			case kViewEffectSpear:
			{
				SPRITE *pTEffect = viewInsertTSprite( pTSprite->sectnum, 0x7FFF, pTSprite);
				pTEffect->z = pTSprite->z;
				pTEffect->cstat = (uchar)((pTSprite->cstat & ~kSpriteRMask) | kSpriteFloor);
				pTEffect->shade = pTSprite->shade;
				pTEffect->xrepeat = pTSprite->xrepeat;
				pTEffect->yrepeat = pTSprite->yrepeat;
				pTEffect->picnum = pTSprite->picnum;
				pTSprite->ang += kAngle180;
				pTSprite->ang &= kAngleMask;
				break;
			}

			case kViewEffectBloodSpray:
			{
				SPRITE *pTEffect = viewInsertTSprite( pTSprite->sectnum, 0x7FFF, pTSprite);
				pTEffect->z = pTSprite->z;
				pTEffect->cstat = (uchar)(pTSprite->cstat & ~kSpriteBlocking);
				if (gDetail > kDetailLevel2)
					pTEffect->cstat |= kSpriteTranslucent | kSpriteTranslucentR;
				pTEffect->shade = (schar)ClipLow((int)(pTSprite->shade - 32), -128);
				pTEffect->xrepeat = pTSprite->xrepeat;
				pTEffect->yrepeat = 64;
				pTEffect->picnum = kAnmBloodSpray;
				break;
			}
		}
	}
}


void viewProcessSprites( int nViewX, int nViewY, int nViewZ )
{
	int nTSprite, nOctant;
	short nXSprite;
	long dx, dy;

	// count down so we don't process shadows
	for (nTSprite = spritesortcnt - 1; nTSprite >= 0; nTSprite--)
	{
		SPRITE *pTSprite = &tsprite[nTSprite];
		XSPRITE *pTXSprite = NULL;
		nXSprite = pTSprite->extra;

		if ( nXSprite > 0 )
			pTXSprite = &xsprite[nXSprite];

		// KEN'S CODE TO SHADE SPRITES
		// Add relative shading code for textures, and possibly sector cstat bit to
		// indicate where shading comes from (i.e. shadows==floorshade, sky==ceilingshade)
		// ALSO: This code does not affect sprite effects added to the tsprite list

		pTSprite->shade = (schar)ClipRange( pTSprite->shade + 6, -128, 127 );
		if ( (sector[pTSprite->sectnum].ceilingstat & 1)
		&& !(sector[pTSprite->sectnum].floorstat & kSectorFloorShade) )
			pTSprite->shade = (schar)ClipRange( pTSprite->shade + sector[pTSprite->sectnum].ceilingshade, -128, 127 );
		else
			pTSprite->shade = (schar)ClipRange( pTSprite->shade + sector[pTSprite->sectnum].floorshade, -128, 127 );

		int nTile = pTSprite->picnum;
		if (nTile < 0 || nTile >= kMaxTiles)
		{
			dprintf("tsprite[].cstat = %i\n", pTSprite->cstat);
			dprintf("tsprite[].shade = %i\n", pTSprite->shade);
			dprintf("tsprite[].pal = %i\n", pTSprite->pal);
			dprintf("tsprite[].picnum = %i\n", pTSprite->picnum);
			dprintf("tsprite[].ang = %i\n", pTSprite->ang);
			dprintf("tsprite[].owner = %i\n", pTSprite->owner);
			dprintf("tsprite[].sectnum = %i\n", pTSprite->sectnum);
			dprintf("tsprite[].statnum = %i\n", pTSprite->statnum);
			dprintf("tsprite[].type = %i\n", pTSprite->type);
			dprintf("tsprite[].flags = %i\n", pTSprite->flags);
			dprintf("tsprite[].extra = %i\n", pTSprite->extra);
			dassert(nTile >= 0 && nTile < kMaxTiles);
		}

		// only interpolate certain moving things
		if ( pTSprite->flags & kAttrMove )
		{
			long x = gPrevSpriteLoc[pTSprite->owner].x;
			long y = gPrevSpriteLoc[pTSprite->owner].y;
			long z = gPrevSpriteLoc[pTSprite->owner].z;
			short nAngle = gPrevSpriteLoc[pTSprite->owner].ang;

			// interpolate sprite position
			x += mulscale16(pTSprite->x - gPrevSpriteLoc[pTSprite->owner].x, gInterpolate);
			y += mulscale16(pTSprite->y - gPrevSpriteLoc[pTSprite->owner].y, gInterpolate);
			z += mulscale16(pTSprite->z - gPrevSpriteLoc[pTSprite->owner].z, gInterpolate);
			nAngle += mulscale16(((pTSprite->ang - gPrevSpriteLoc[pTSprite->owner].ang + kAngle180) & kAngleMask) - kAngle180, gInterpolate);

			pTSprite->x = x;
			pTSprite->y = y;
			pTSprite->z = z;
			pTSprite->ang = nAngle;
		}

		int nFrames = picanm[nTile].frames + 1;

		switch ( picanm[nTile].view )
		{
			case kSpriteViewSingle:

				if (nXSprite > 0)
				{
					dassert(nXSprite < kMaxXSprites);
					switch ( pTSprite->type )
					{
						case kSwitchToggle:
						case kSwitchMomentary:
							if ( xsprite[nXSprite].state )
								pTSprite->picnum = (short)(nTile + nFrames);
								break;

						case kSwitchCombination:
							pTSprite->picnum = (short)(nTile + xsprite[nXSprite].data1 * nFrames);
							break;

					}
				}
				break;

			case kSpriteView5Full:

				// Calculate which of the 8 angles of the sprite to draw (0-7)
				dx = nViewX - pTSprite->x;
				dy = nViewY - pTSprite->y;
				RotateVector(&dx, &dy, -pTSprite->ang + kAngle45 / 2);
				nOctant = GetOctant(dx, dy);

				if (nOctant <= 4)
				{
					pTSprite->picnum = (short)(nTile + nOctant);
					pTSprite->cstat &= ~4;   // clear x-flipping bit
				}
				else
				{
					pTSprite->picnum = (short)(nTile + 8 - nOctant);
					pTSprite->cstat |= 4;   // set x-flipping bit
				}
				break;

			case kSpriteView8Full:
				// Calculate which of the 8 angles of the sprite to draw (0-7)
				dx = nViewX - pTSprite->x;
				dy = nViewY - pTSprite->y;
				RotateVector(&dx, &dy, -pTSprite->ang + kAngle45 / 2);
				nOctant = GetOctant(dx, dy);

				pTSprite->picnum = (short)(nTile + nOctant);
				break;

			case kSpriteView5Half:
				break;
		}

		if ( spritesortcnt < kMaxViewSprites )
		{
			if (pTSprite->statnum == kStatDude || pTSprite->statnum == kStatThing || pTSprite->statnum == kStatMissile)
			{
				if ( pTXSprite && actGetBurnTime(pTXSprite) > 0 )
					viewAddEffect( nTSprite, kViewEffectSmokeLow );

				if ( pTSprite->flags & kAttrSmoke )
					viewAddEffect(nTSprite, kViewEffectSmokeHigh);

				if (powerupCheck(gView, kItemRoseGlasses - kItemBase) > 0 && pTSprite->statnum == kStatDude)
					pTSprite->shade = kMaxShade;

				if (pTSprite->type >= kDudePlayer1 && pTSprite->type <= kDudePlayer8)
				{
					PLAYER *pPlayer = &gPlayer[pTSprite->type - kDudePlayer1];

					if ( powerupCheck(pPlayer, kItemLtdInvisibility - kItemBase) > 0
					&& powerupCheck(gView, kItemRoseGlasses - kItemBase) == 0 )
					{
						pTSprite->cstat |= kSpriteTranslucent; // kSpriteInvisible;
						pTSprite->pal = kPLUGray;
					}
					// if not invisible, show the temporary player ID
					if ( !(pTSprite->cstat & kSpriteTranslucent) )
						pTSprite->pal = (uchar )(kPLUPlayer1 + ( pPlayer->teamID & 3 ));
				}

				switch (pTSprite->type)
				{
					case kMissileButcherKnife:
						viewAddEffect( nTSprite, kViewEffectPhase );
						break;

					case kMissileSpear:
						viewAddEffect( nTSprite, kViewEffectSpear );
						break;

					case kMissileFlare:
					{
						viewAddEffect( nTSprite, kViewEffectFlareHalo );
						SECTOR *pSector = &sector[pTSprite->sectnum];
						int zDist = (pTSprite->z - pSector->ceilingz) >> 8;
						if ( !(pSector->ceilingstat & kSectorParallax) && zDist < 64 )
							viewAddEffect( nTSprite, kViewEffectCeilGlow );
						zDist = (pSector->floorz - pTSprite->z) >> 8;
						if ( !(pSector->floorstat & kSectorParallax) && zDist < 64 )
							viewAddEffect( nTSprite, kViewEffectFloorGlow );
						break;
					}

					case kThingTNTStick:
						if ( gSpriteHit[nXSprite].floorHit == 0 )
						{
							pTSprite->picnum = 2036;
							viewAddEffect(nTSprite, kViewEffectShadow);
						}
						break;

					case kThingTNTBundle:
						if ( gSpriteHit[nXSprite].floorHit == 0 )
						{
							pTSprite->picnum = 2028;
							viewAddEffect(nTSprite, kViewEffectShadow);
						}
						break;

					default:
						// only add shadow if will be below the view z
						if ( getflorzofslope(pTSprite->sectnum, pTSprite->x, pTSprite->y) >= nViewZ)
						{
							// don't add to kThings already on the ground
							if (pTSprite->type >= kThingBase && pTSprite->type < kThingMax)
							{
								if ( gSpriteHit[nXSprite].floorHit != 0 )
									break;
							}
							viewAddEffect( nTSprite, kViewEffectShadow );
						}
						break;
				}
			}
			else switch ( pTSprite->type )
			{
				case kMiscTorch:
					if ( pTXSprite )
					{
						if ( pTXSprite->state > 0 )
						{
							pTSprite->picnum++;
							viewAddEffect( nTSprite, kViewEffectTorchHigh );
						} else {
							viewAddEffect( nTSprite, kViewEffectSmokeHigh );
						}
					}
					else
					{
						pTSprite->picnum++;
						viewAddEffect( nTSprite, kViewEffectTorchHigh );
					}
					break;
				case kMiscHangingTorch:
					if ( pTXSprite )
					{
						if ( pTXSprite->state > 0 )
						{
							pTSprite->picnum++;
							viewAddEffect( nTSprite, kViewEffectTorchLow );
						} else {
							viewAddEffect( nTSprite, kViewEffectSmokeLow );
						}
					}
					else
					{
						pTSprite->picnum++;
						viewAddEffect( nTSprite, kViewEffectTorchLow );
					}
					break;
				case kTrapSawBlade:
					if ( pTXSprite->state )
					{
						if ( pTXSprite->data1 )	// has been bloodied
						{
							pTSprite->picnum = kAnmCircSaw2;
							if ( pTXSprite->data2 )
								viewAddEffect( nTSprite, kViewEffectBloodSpray );
						}
					}
					else
					{
						if ( pTXSprite->data1 )	// this blade has been bloodied
							pTSprite->picnum = kAnmCircSaw2 + 1;
						else
							pTSprite->picnum = kAnmCircSaw1 + 1;
					}
					break;
			}
		}
	}
}


#define kViewDistance	(80 << 4)
extern void viewCalcPosition( SPRITE *pSprite, long *px, long *py, long *pz, short *pAngle, short *pSector )
{
	int nAngle = (*pAngle + gViewPosAngle[gViewPos]) & kAngleMask;
	int dx = mulscale30(kViewDistance, Cos(nAngle));
	int dy = mulscale30(kViewDistance, Sin(nAngle));

	*pAngle = (short)((nAngle + kAngle180) & kAngleMask);

	ushort cstat = pSprite->cstat;
	pSprite->cstat &= ~kSpriteBlocking;
	ClipMove(px, py, pz, pSector, dx, dy, 4 << 4, 1, 1, 0);
	pSprite->cstat = cstat;
}


void viewBackupPlayerLoc( int nPlayer )
{
	SPRITE *pSprite = gPlayer[nPlayer].sprite;
	PLOCATION *pPLocation = &gPrevPlayerLoc[nPlayer];
	pPLocation->x = pSprite->x;
	pPLocation->y = pSprite->y;
	pPLocation->z = pSprite->z;
	pPLocation->ang = pSprite->ang;
	pPLocation->horiz = gPlayer[nPlayer].horiz;
	pPLocation->bobHeight = gPlayer[nPlayer].bobHeight;
	pPLocation->bobWidth = gPlayer[nPlayer].bobWidth;
	pPLocation->swayHeight = gPlayer[nPlayer].swayHeight;
	pPLocation->swayWidth = gPlayer[nPlayer].swayWidth;
}


void viewBackupSpriteLoc( int nSprite, SPRITE *pSprite )
{
	LOCATION *pLocation = &gPrevSpriteLoc[nSprite];
	pLocation->x = pSprite->x;
	pLocation->y = pSprite->y;
	pLocation->z = pSprite->z;
	pLocation->ang = pSprite->ang;
}


void viewBackupAllSpriteLoc( void )
{
	SPRITE *pSprite = &sprite[0];
	LOCATION *pLocation = &gPrevSpriteLoc[0];
	for (int i = 0; i < kMaxSprites; i++, pSprite++, pLocation++)
	{
		if ( pSprite->flags & kAttrMove )
		{
			pLocation->x = pSprite->x;
			pLocation->y = pSprite->y;
			pLocation->z = pSprite->z;
			pLocation->ang = pSprite->ang;
		}
	}
}


/***********************************************************************
 * viewDrawSprite()
 *
 * Draws tiles to the screen using a unified set of flags
 * similar to those in overwritesprite/rotatesprite
 **********************************************************************/
void viewDrawSprite( long sx, long sy,
	long nZoom, int nAngle, short nTile, schar nShade, char nPLU, ushort nFlags,
	long wx1, long wy1, long wx2, long wy2 )
{
	// convert x-flipping
	if ( nFlags & kDrawXFlip )
	{
		nAngle = (nAngle + kAngle180) & kAngleMask;
		nFlags ^= kDrawYFlip;
	}

	// call rotatesprite passing only compatible bits in nFlags
	rotatesprite( sx, sy, nZoom, (short)nAngle, nTile, nShade, nPLU,
		(char)nFlags, wx1, wy1, wx2, wy2 );
}

#define kMaxViewFlames	9

static struct {
	short	tile;
	uchar	flags;
	char	pal;
	long	zoom;
	short	x;
	short	y;
} burnTable[ kMaxViewFlames ] = {
	{	kAnmFlame2,	(uchar)(kDrawScale),			0,	0x22000,	10,		200 },
	{	kAnmFlame2,	(uchar)(kDrawScale|kDrawXFlip),	0,	0x20000,	40,		200 },
	{	kAnmFlame2,	(uchar)(kDrawScale),			0,	0x19000,	85,		200 },
	{	kAnmFlame2,	(uchar)(kDrawScale),			0,	0x16000,	120,	200 },
	{	kAnmFlame2,	(uchar)(kDrawScale|kDrawXFlip),	0,	0x14000,	160,	200 },
	{	kAnmFlame2,	(uchar)(kDrawScale|kDrawXFlip),	0,	0x17000,	200,	200 },
	{	kAnmFlame2,	(uchar)(kDrawScale),			0,	0x18000,	235,	200 },
	{	kAnmFlame2,	(uchar)(kDrawScale),			0,	0x20000,	275,	200 },
	{	kAnmFlame2,	(uchar)(kDrawScale|kDrawXFlip),	0,	0x23000,	310,	200 },
};


/***********************************************************************
 * viewBurnTime()
 **********************************************************************/
void viewBurnTime( int nTicks )
{
	if ( nTicks == 0 )
		return;

	for ( int i = 0; i < kMaxViewFlames; i++ )
	{
		// fake a sprite type for animateoffs to derive an index into an animation sequence
		short nTile = burnTable[i].tile;
		nTile += animateoffs( nTile, (ushort)(0x8000 + i));
		long nZoom = burnTable[i].zoom;

		// size diminishes during last 5 seconds
		if ( nTicks < 5 * kTimerRate )
			nZoom = muldiv(nZoom, nTicks, 5 * kTimerRate);

		viewDrawSprite( burnTable[i].x << 16, burnTable[i].y << 16, nZoom, 0, nTile, 0,
			burnTable[i].pal, burnTable[i].flags, windowx1, windowy1, windowx2, windowy2);
	}
}


void viewSetMessage( char *s )
{
	messageTime = gFrameClock;
	strcpy(message, s);
}


void viewDisplayMessage( void )
{

	if (gFrameClock < messageTime + kTimerRate * 2)
		viewDrawText(kFontMessage, message, windowx1 + 1, windowy1 + 1, 0, 0);
}


void viewDrawScreen( void )
{
	static lastDacUpdate = 0;
	BOOL fInterpolateRangeError = FALSE;

	if ( gViewMode == kView3D || gViewMode == kView2DIcon || gOverlayMap )
	{
	 	DoSectorLighting();
		DoSectorPanning();
	}

	if ((gViewMode == kView3D) || (gOverlayMap))
	{
		int theView = gViewIndex;
		if ( powerupCheck(gMe, kItemCrystalBall - kItemBase) )
			theView = myconnectindex;

		short nSector = gView->sprite->sectnum;

		if (gInterpolate < 0 || gInterpolate > 0x10000)
		{
			fInterpolateRangeError = TRUE;
			gInterpolate = ClipRange(gInterpolate, 0, 0x10000);
		}

		long x = gPrevPlayerLoc[theView].x;
		long y = gPrevPlayerLoc[theView].y;
		long z = gPrevPlayerLoc[theView].z;
		short nAngle = gPrevPlayerLoc[theView].ang;
		int nHoriz = gPrevPlayerLoc[theView].horiz;
		int bobWidth = gPrevPlayerLoc[theView].bobWidth;
		int bobHeight = gPrevPlayerLoc[theView].bobHeight;
		int swayWidth = gPrevPlayerLoc[theView].swayWidth;
		int swayHeight = gPrevPlayerLoc[theView].swayHeight;

		// interpolate player position
		x += mulscale16(gView->sprite->x - gPrevPlayerLoc[theView].x, gInterpolate);
		y += mulscale16(gView->sprite->y - gPrevPlayerLoc[theView].y, gInterpolate);
		z += mulscale16(gView->sprite->z - gPrevPlayerLoc[theView].z, gInterpolate);
		nAngle += mulscale16(((gView->sprite->ang - gPrevPlayerLoc[theView].ang + kAngle180) & kAngleMask) - kAngle180, gInterpolate);
		nHoriz += mulscale16(gView->horiz - gPrevPlayerLoc[theView].horiz, gInterpolate);
		bobWidth += mulscale16(gView->bobWidth - gPrevPlayerLoc[theView].bobWidth, gInterpolate);
		bobHeight += mulscale16(gView->bobHeight - gPrevPlayerLoc[theView].bobHeight, gInterpolate);
		swayWidth += mulscale16(gView->swayWidth - gPrevPlayerLoc[theView].swayWidth, gInterpolate);
		swayHeight += mulscale16(gView->swayHeight - gPrevPlayerLoc[theView].swayHeight, gInterpolate);

//		nAngle = gPrevPlayerLoc[theView].ang;

		if ( gViewPos == kViewPosCenter )
		{
			x -= mulscale30(bobWidth, Sin(nAngle)) >> 4;
			y += mulscale30(bobWidth, Cos(nAngle)) >> 4;
			z += bobHeight + gView->compression - gView->eyeAboveZ + nHoriz * 10;
		}
		else
		{
			viewCalcPosition( gView->sprite, &x, &y, &z, &nAngle, &nSector );
			z -= gView->eyeAboveZ + (16 << 8);
		}

// 		should modify the z to handle earthquake type effects

//		something like this might come in handy for the crystal ball
//		need to mod based on number of players, and exclude self
//		int nEyes = (gGameGlock & 0x700) >> 8; // show through new "eyes" every two seconds
//		then render the other view to a tile and draw the tile (after the screen) using a lens effect

		long tiltLock = gScreenTilt;

		BOOL bDelirious   = FALSE;
		BOOL bClairvoyant = FALSE;

		bDelirious = ( powerupCheck(gView, kItemShroomDelirium - kItemBase) > 0 );
		if ( tiltLock != 0 || bDelirious )
		{
			if ( waloff[ TILTBUFFER ] == NULL )
				allocatepermanenttile( TILTBUFFER, 320L, 320L );

			dassert( waloff[ TILTBUFFER ] != NULL );
			setviewtotile( TILTBUFFER, 320L, 320L );
		}
		else
		{
			bClairvoyant = ( powerupCheck(gView, kItemCrystalBall - kItemBase) > 0 );
			if ( bClairvoyant && gViewIndex != theView )
			{
				dprintf("CLAIR SETUP\n");
				PLAYER *pViewed = &gPlayer[gViewIndex];
				if ( waloff[ BALLBUFFER ] == NULL )
					allocatepermanenttile( BALLBUFFER, 128L, 128L );
				dassert( waloff[ BALLBUFFER ] != NULL );
				setviewtotile( BALLBUFFER, 128L, 128L );
				long x = gPrevPlayerLoc[gViewIndex].x;
				long y = gPrevPlayerLoc[gViewIndex].y;
				long z = gPrevPlayerLoc[gViewIndex].z;
				short nAngle = gPrevPlayerLoc[gViewIndex].ang;
				int bobWidth = gPrevPlayerLoc[gViewIndex].bobWidth;
				int bobHeight = gPrevPlayerLoc[gViewIndex].bobHeight;
				int swayWidth = gPrevPlayerLoc[gViewIndex].swayWidth;
				int swayHeight = gPrevPlayerLoc[gViewIndex].swayHeight;
				x += mulscale16(pViewed->sprite->x - gPrevPlayerLoc[gViewIndex].x, gInterpolate);
				y += mulscale16(pViewed->sprite->y - gPrevPlayerLoc[gViewIndex].y, gInterpolate);
				z += mulscale16(pViewed->sprite->z - gPrevPlayerLoc[gViewIndex].z, gInterpolate);
//				nAngle += mulscale16(((pViewed->sprite->ang - gPrevPlayerLoc[gViewIndex].ang + kAngle180) & kAngleMask) - kAngle180, gInterpolate);
//				bobWidth += mulscale16(pViewed->bobWidth - gPrevPlayerLoc[gViewIndex].bobWidth, gInterpolate);
//				bobHeight += mulscale16(pViewed->bobHeight - gPrevPlayerLoc[gViewIndex].bobHeight, gInterpolate);
//				swayWidth += mulscale16(pViewed->swayWidth - gPrevPlayerLoc[gViewIndex].swayWidth, gInterpolate);
//				swayHeight += mulscale16(pViewed->swayHeight - gPrevPlayerLoc[gViewIndex].swayHeight, gInterpolate);
//				viewCalcPosition( pViewed->sprite, &x, &y, &z, &nAngle, &nSector );
//				z -= pViewed->eyeAboveZ + (16 << 8);
				DrawMirrors(x, y, z, nAngle, kHorizDefault + pViewed->horiz );
				drawrooms(x, y, z, nAngle, kHorizDefault + pViewed->horiz, nSector);
				viewProcessSprites(x, y, z);
				drawmasks();
				setviewback();
			}
		}

		if ( !bDelirious )
		{
			deliriumTilt = 0;
			deliriumTurn = 0;
			deliriumPitch = 0;
		}

		nAngle = (short)((nAngle + deliriumTurn) & kAngleMask);

		// prevent the player from being drawn (not taken care of automatically if x,y different)
		ushort oldcstat = gView->sprite->cstat;
		if ( gViewPos == kViewPosCenter )
			gView->sprite->cstat |= kSpriteInvisible;

		DrawMirrors(x, y, z, nAngle, kHorizDefault + nHoriz + deliriumPitch);
		drawrooms(x, y, z, nAngle, kHorizDefault + nHoriz + deliriumPitch, nSector);
		viewProcessSprites(x, y, z);
		drawmasks();

		gView->sprite->cstat = oldcstat;

		// process gotpics
		if ( TestBitString(gotpic, kPicFire) )
		{
			FireProcess();
			ClearBitString(gotpic, kPicFire);
		}

		if ( tiltLock != 0 || bDelirious )
		{
			dassert( waloff[ TILTBUFFER ] != NULL );

			setviewback();

			uchar nFlags = kRotateScale | kRotateYFlip | kRotateNoMask;
			if ( bDelirious )
				nFlags |= kRotateTranslucent;
			rotatesprite( 320 << 15, 200 << 15, 0x10000, (short)(tiltLock + kAngle90), TILTBUFFER, 0, 0, nFlags,
				gViewX0, gViewY0, gViewX1, gViewY1);
		}
		else
		{
			if ( bClairvoyant && gViewIndex != theView )
			{
				dprintf("CLAIR DRAW\n");
				rotatesprite(0,0,0x10000,0,BALLBUFFER,0,0,0,gViewX0, gViewY0, gViewX1, gViewY1);
			}
		}

		if ( gViewPos == kViewPosCenter )
		{
			if ( gAimReticle )
			{
				// draw aiming reticle
				dassert(gView->relAim.dx > 0);

				x = 160 + gView->relAim.dy * 160 / gView->relAim.dx;
				y = kHorizDefault + nHoriz + (gView->relAim.dz >> 7);
				rotatesprite(x << 16, y << 16, 0x10000, 0, 2319, 0, 0, kRotateScale,
					gViewX0, gViewY0, gViewX1, gViewY1);
			}

			x = 160 + (swayWidth >> 8);
			y = 200 + gPosture[gView->lifemode].swayV + (swayHeight >> 8) + (gView->compression >> 8);
			int nShade = sector[nSector].floorshade;
			WeaponDraw(gView, nShade, x, y);
		}

		if ( gViewPos == kViewPosCenter && actGetBurnTime(gView->xsprite) > (kTimerRate / 2) )
			viewBurnTime( actGetBurnTime(gView->xsprite)) ;

		if ( powerupCheck(gView, kItemDivingSuit - kItemBase) > 0 )
		{
			uchar nFlags = kRotateScale|kRotateCorner;

			// upper-left
			rotatesprite( 0, 0, 0x10000, 0, kMaskDivingSuit, 0, 0, nFlags,
				gViewX0, gViewY0, gViewX1, gViewY1);

			// upper-right
			rotatesprite(  320 << 16, 0, 0x10000, 1024, kMaskDivingSuit, 0, 0, (uchar)(nFlags|kRotateYFlip),
				gViewX0, gViewY0, gViewX1, gViewY1);

			// lower-left
			rotatesprite(  0, 200 << 16, 0x10000, 0, kMaskDivingSuit, 0, 0, (uchar)(nFlags|kRotateYFlip),
				gViewX0, gViewY0, gViewX1, gViewY1);

			// lower-right
			rotatesprite(  320 << 16, 200 << 16, 0x10000, 1024, kMaskDivingSuit, 0, 0, nFlags,
				gViewX0, gViewY0, gViewX1, gViewY1);

			if ( gDetail >= kDetailLevelMax )
			{
				// upper-left reflection
				rotatesprite( 15 << 16, 3 << 16, 0x10000, 0, kMaskReflect1, 32, 0, (uchar)(nFlags|kRotateTranslucent),
					gViewX0, gViewY0, gViewX1, gViewY1);

				// lower-right reflection
				rotatesprite( 212 << 16, 77 << 16, 0x10000, 0, kMaskReflect2, 32, 0, (uchar)(nFlags|kRotateTranslucent),
					gViewX0, gViewY0, gViewX1, gViewY1);
			}
		}

		if ( powerupCheck(gView, kItemAsbestosArmor - kItemBase) > 0 )
		{
			uchar nFlags = kRotateScale|kRotateCorner;

			// upper-left
			rotatesprite( 0, 0, 0x10000, 0, kMaskFireSuit, 0, 0, nFlags,
				gViewX0, gViewY0, gViewX1, gViewY1);

			// upper-right
			rotatesprite(  320 << 16, 0, 0x10000, 1024, kMaskFireSuit, 0, 0, (uchar)(nFlags|kRotateYFlip),
				gViewX0, gViewY0, gViewX1, gViewY1);

			// lower-left
			rotatesprite(  0, 200 << 16, 0x10000, 0, kMaskFireSuit, 0, 0, (uchar)(nFlags|kRotateYFlip),
				gViewX0, gViewY0, gViewX1, gViewY1);

			// lower-right
			rotatesprite(  320 << 16, 200 << 16, 0x10000, 1024, kMaskFireSuit, 0, 0, nFlags,
				gViewX0, gViewY0, gViewX1, gViewY1);
		}
	}
	else
		clearview(0);  // Clear screen to specified color

	if ( gViewMode == kView2DIcon )
		drawmapview(gView->sprite->x, gView->sprite->y, gZoom >> 2, gView->sprite->ang);

	if ( gViewMode == kView2D /*|| gViewMode == kView2DIcon */)
	{
//		drawoverheadmap(gView->sprite->x, gView->sprite->y, gZoom, gView->sprite->ang);
		DrawMap(gView->sprite->x, gView->sprite->y, gView->sprite->ang, gZoom);
	}

	viewDrawInterface();

	viewDisplayMessage();

	CalcFrameRate();
	if ( gShowFrameRate )
	{
		char buffer[16];
		sprintf(buffer, "%3i", gFrameRate);
		printext256(gViewX1 - 12, gViewY0, 31, -1, buffer, 1);
	}
	if ( gShowFrags )
	{
		for ( int i = 0; i < numplayers; i++)
		{
			char buffer[16];
			sprintf(buffer, "%i:%i", i, gPlayer[i].fragCount);
			printext256(gViewX0 + 12, gViewY0+(i*8), 31, -1, buffer, 1);
		}
	}

	if ( gPaused )
		viewDrawText(kFontMenu, "PAUSED", gViewX1 / 2, gViewY1 / 2, 0, 0, TA_CENTER);

	if ( fInterpolateRangeError )
		gfxDrawText(windowx2 - 16, windowy1, gStdColor[kColorWhite], "I");

	if ( gView->xsprite->moveState == kMoveFall )
		gfxDrawText(windowx2 - 24, windowy1, gStdColor[kColorWhite], "F");

	scrSetDac(gGameClock - lastDacUpdate);
	lastDacUpdate = gGameClock;

	scrNextPage();
}


void Detail( int nType, BOOL bDetail )
{
	switch ( nType )
	{
		case kDetailSprites:
		{
			int nSprite, nNext;
			for (nSprite = headspritestat[kStatProximity]; nSprite >= 0; nSprite = nNext)
			{
				nNext = nextspritestat[nSprite];

				if (bDetail)
					sprite[nSprite].cstat &= ~kSpriteInvisible;
				else
					sprite[nSprite].cstat |= kSpriteInvisible;
			}
			break;
		}

		default:
			break;
	}
}


void viewSetDetail( int nDetail )
{
	dassert( nDetail >= kDetailLevelMin && nDetail <= kDetailLevelMax );

	switch ( nDetail )
	{
		// no kStatDetail sprites, no fake gibs, no shadows, no smoke trail effects, no glow or lens flares
		case kDetailLevelMin:
			Detail( kDetailSprites, FALSE );
			break;

		case kDetailLevel2:
			Detail( kDetailSprites, FALSE );
			break;

		case kDetailLevel3:
			Detail( kDetailSprites, FALSE );
			break;

		case kDetailLevel4:
			Detail( kDetailSprites, FALSE );
			break;

		case kDetailLevelMax:
			Detail( kDetailSprites, TRUE );
			break;
	}
	gDetail = nDetail;
}
