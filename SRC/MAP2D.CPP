#include <string.h>

#include "engine.h"

#include "map2d.h"
#include "helix.h"
#include "gameutil.h"
#include "trig.h"
#include "misc.h"
#include "gfx.h"
#include "view.h"
#include "debug4g.h"
#include "actor.h"
#include "options.h"

#include <memcheck.h>

static scaleX, scaleY;


static void ScaleLine(long x0, long y0, long x1, long y1, int nColor)
{
	x0 = (gViewXCenter << 8) + mulscale16(x0, scaleX);
	y0 = (gViewYCenter << 8) + mulscale16(y0, scaleY);
	x1 = (gViewXCenter << 8) + mulscale16(x1, scaleX);
	y1 = (gViewYCenter << 8) + mulscale16(y1, scaleY);
	gfxDrawLine( x0, y0, x1, y1, nColor );
}

void DrawMap(int x, int y, int ang, int zoom)
{
	int i, j;
	long x0, y0, x1, y1, x2, y2;
	int nSector1, nSector2;
	char nColor;

	gfxSetClip(gViewX0, gViewY0, gViewX1, gViewY1);

	scaleX = zoom * xdim * 3 / 4;
	scaleY = zoom * ydim;

	for (i = 0; i < numwalls; i++)
	{
		j = wall[i].nextwall;

		// only draw one side of walls
		if ( i < j )
			continue;

		if ( gFullMap || TestBitString(show2dwall, i) )
		{
			nColor = 0;

			if (j == -1)
				nColor = 0x8F;	// red
			else
			{
				nSector1 = wall[j].nextsector;
				nSector2 = wall[i].nextsector;
				if (( sector[nSector1].ceilingz != sector[nSector2].ceilingz )
				&& (!(sector[nSector1].ceilingstat & 1) && !(sector[nSector2].ceilingstat & 1)) )
					nColor = 0xF6;	// yellow

				if ( sector[nSector1].floorz != sector[nSector2].floorz )
					nColor = 0x3B;	// light brown
			}

			if (nColor == 0)
				continue;

			ScaleLine(wall[i].x - x, wall[i].y - y,
				wall[wall[i].point2].x - x, wall[wall[i].point2].y - y,
				nColor);
		}
	}

	if ( zoom >= 256 )
	{
		for (i = 0; i < numsectors; i++)
		{
			j = headspritesect[i];
			while (j != -1)
			{
				if ( gFullMap || TestBitString(show2dsprite, j) )
				{
					SPRITE *pSprite = &sprite[j];

					nColor = 43;
					if ((pSprite->cstat & kSpriteBlocking) > 0)
						nColor = 69;

					switch ( pSprite->cstat & kSpriteRMask )
					{
						case kSpriteFloor: // floor sprite
							break;
						case kSpriteWall: // wall sprite
							break;
						default:
							break;
					}

					x0 = mulscale16(pSprite->x - x, scaleX);
					y0 = mulscale16(pSprite->y - y, scaleY);

					//Draw sprite
					x0 = x1 = x2 =pSprite->clipdist << 2;
					y0 = y1 = y2 = 0;
					RotateVector(&x0, &y0, pSprite->ang);
					RotateVector(&x1, &y1, pSprite->ang + kAngle120);
					RotateVector(&x2, &y2, pSprite->ang - kAngle120);
					x0 += pSprite->x - x;
					y0 += pSprite->y - y;
					x1 += pSprite->x - x;
					y1 += pSprite->y - y;
					x2 += pSprite->x - x;
					y2 += pSprite->y - y;
					ScaleLine(x0, y0, x1, y1, nColor);
					ScaleLine(x1, y1, x2, y2, nColor);
					ScaleLine(x2, y2, x0, y0, nColor);
				}

				j = nextspritesect[j];
			}
		}
	}

	//Draw player
	x0 = 0x200;
	y0 = 0;
	x1 = -0x200;
	y1 = 0;
	RotateVector(&x0, &y0, ang);
	RotateVector(&x1, &y1, ang);
	ScaleLine(x0, y0, x1, y1, 0x18);

	x1 = 0x180;
	y1 = -0x80;
	RotateVector(&x1, &y1, ang);
	ScaleLine(x0, y0, x1, y1, 0x18);

	x1 = 0x180;
	y1 = 0x80;
	RotateVector(&x1, &y1, ang);
	ScaleLine(x0, y0, x1, y1, 0x18);

	x0 = -0x200;
	y0 = 0;
	RotateVector(&x0, &y0, ang);

	x1 = -0x280;
	y1 = -0x80;
	RotateVector(&x1, &y1, ang);
	ScaleLine(x0, y0, x1, y1, 0x18);

	x1 = -0x280;
	y1 = 0x80;
	RotateVector(&x1, &y1, ang);
	ScaleLine(x0, y0, x1, y1, 0x18);

	x0 = -0x180;
	y0 = 0;
	RotateVector(&x0, &y0, ang);

	x1 = -0x200;
	y1 = -0x80;
	RotateVector(&x1, &y1, ang);
	ScaleLine(x0, y0, x1, y1, 0x18);

	x1 = -0x200;
	y1 = 0x80;
	RotateVector(&x1, &y1, ang);
	ScaleLine(x0, y0, x1, y1, 0x18);

	x0 = -0x100;
	y0 = 0;
	RotateVector(&x0, &y0, ang);

	x1 = -0x180;
	y1 = -0x80;
	RotateVector(&x1, &y1, ang);
	ScaleLine(x0, y0, x1, y1, 0x18);

	x1 = -0x180;
	y1 = 0x80;
	RotateVector(&x1, &y1, ang);
	ScaleLine(x0, y0, x1, y1, 0x18);
}


#if 0
int drawoverheadmap(long cposx, long cposy, long czoom, short cang)
{
	long i, j, k, l, x1, y1, x2, y2, x3, y3, x4, y4, ox, oy, xoff, yoff;
	long xvect, yvect, dax, day, cosang, sinang, xspan, yspan;
	long xrepeat, yrepeat, z1, z2, startwall, endwall, tilenum;
	short daang;
	char col;
	WALL *wal, *wal2;
	SPRITE *spr;

	xvect = mulscale(Sin(-cang), czoom, 16);
	yvect = mulscale(Cos(-cang), czoom, 16);

		//Draw red lines
	for(i=0;i<numsectors;i++)
	{
		startwall = sector[i].wallptr;
		endwall = sector[i].wallptr + sector[i].wallnum - 1;

		z1 = sector[i].ceilingz; z2 = sector[i].floorz;

		for(j=startwall,wal=&wall[startwall];j<=endwall;j++,wal++)
		{
			k = wal->nextwall; if (k < 0) continue;

			if ( !TestBitString(show2dwall, j) )
				continue;
			if ((k > j) && ShowBitString(show2dwall, k) )
				continue;

			if (sector[wal->nextsector].ceilingz == z1)
				if (sector[wal->nextsector].floorz == z2)
					if (((wal->cstat|wall[wal->nextwall].cstat)&(16+32)) == 0) continue;

			col = 152;

			if (gViewMode == kView2DIcon)
			{
				if (sector[i].floorz != sector[i].ceilingz)
					if (sector[wal->nextsector].floorz != sector[wal->nextsector].ceilingz)
						if (((wal->cstat|wall[wal->nextwall].cstat)&(16+32)) == 0)
							if (sector[i].floorz == sector[wal->nextsector].floorz) continue;
				if (sector[i].floorpicnum != sector[wal->nextsector].floorpicnum) continue;
				if (sector[i].floorshade != sector[wal->nextsector].floorshade) continue;
				col = 12;
			}

			ox = wal->x-cposx; oy = wal->y-cposy;
			x1 = mulscale16(ox, xvect) - mulscale16(oy,yvect);
			y1 = mulscale16(oy, xvect) + mulscale16(ox,yvect);

			wal2 = &wall[wal->point2];
			ox = wal2->x-cposx; oy = wal2->y-cposy;
			x2 = mulscale16(ox,xvect) - mulscale16(oy,yvect);
			y2 = mulscale16(oy,xvect) + mulscale16(ox,yvect);

			drawline256(x1+(xdim<<11),y1+(ydim<<11),x2+(xdim<<11),y2+(ydim<<11),col);
		}
	}

		//Draw sprites
	k = gView->nSprite;
	for(i=0;i<numsectors;i++)
		for(j=headspritesect[i];j>=0;j=nextspritesect[j])
			if ( TestBitString(show2dsprite, j) )
			{
				spr = &sprite[j];
				col = 56;
				if ((spr->cstat&1) > 0) col = 248;
				if (j == k) col = 31;

				switch (spr->cstat&48)
				{
					case 0:
						ox = spr->x-cposx; oy = spr->y-cposy;
						x1 = mulscale16(ox,xvect) - mulscale16(oy,yvect);
						y1 = mulscale16(oy,xvect) + mulscale16(ox,yvect);

						if (gViewMode == kView2D)
						{
							ox = Cos(spr->ang) >> 23;
							oy = Sin(spr->ang) >> 23;
							x2 = mulscale16(ox,xvect) - mulscale16(oy,yvect);
							y2 = mulscale16(oy,xvect) + mulscale16(ox,yvect);

							if (j == gView->nSprite)
							{
								x1 = 0L;
								y1 = (64L<<12);
								x2 = 0L;
								y2 = -(czoom<<5);
							}

							drawline256(x1-x2+(xdim<<11),y1-y2+(ydim<<11),
											x1+x2+(xdim<<11),y1+y2+(ydim<<11),col);
							drawline256(x1-y2+(xdim<<11),y1+x2+(ydim<<11),
											x1+x2+(xdim<<11),y1+y2+(ydim<<11),col);
							drawline256(x1+y2+(xdim<<11),y1-x2+(ydim<<11),
											x1+x2+(xdim<<11),y1+y2+(ydim<<11),col);
						}
						else if (gViewMode == kView2DIcon)
						{
							if (((gotsector[i>>3]&(1<<(i&7))) > 0) && (czoom > 192))
							{
								daang = (short)((spr->ang-cang) & kAngleMask);
								if (j == gView->nSprite)
									{ x1 = 0; y1 = (4096*64); daang = 0; }
								rotatesprite((x1<<4)+(xdim<<16),(y1<<4)+(ydim<<16),czoom*spr->yrepeat,daang,spr->picnum,spr->shade,spr->pal,
									spr->cstat & kSpriteTranslucent ? 1 : 0);
							}
						}
						break;
					case 16:
						x1 = spr->x; y1 = spr->y;
						tilenum = spr->picnum;
						xoff = picanm[tilenum].xcenter + spr->xoffset;
						if ((spr->cstat&4) > 0) xoff = -xoff;
						k = spr->ang; l = spr->xrepeat;
						dax = Sin(k) * l >> 16;
						day = Cos(k) * l >> 16;
						l = tilesizx[tilenum]; k = (l>>1)+xoff;
						x1 -= mulscale16(dax,k); x2 = x1+mulscale16(dax,l);
						y1 -= mulscale16(day,k); y2 = y1+mulscale16(day,l);

						ox = x1-cposx; oy = y1-cposy;
						x1 = mulscale16(ox,xvect) - mulscale16(oy,yvect);
						y1 = mulscale16(oy,xvect) + mulscale16(ox,yvect);

						ox = x2-cposx; oy = y2-cposy;
						x2 = mulscale16(ox,xvect) - mulscale16(oy,yvect);
						y2 = mulscale16(oy,xvect) + mulscale16(ox,yvect);

						drawline256(x1+(xdim<<11),y1+(ydim<<11),
										x2+(xdim<<11),y2+(ydim<<11),col);

						break;
					case 32:
						if (gViewMode == kView2D)
						{
							tilenum = spr->picnum;
							xoff = (long)(picanm[tilenum].xcenter + spr->xoffset);
							yoff = (long)(picanm[tilenum].ycenter + spr->yoffset);
							if ((spr->cstat&4) > 0) xoff = -xoff;
							if ((spr->cstat&8) > 0) yoff = -yoff;

							k = spr->ang;
							cosang = Cos(k) >> 16;
							sinang = Sin(k) >> 16;
							xspan = tilesizx[tilenum]; xrepeat = spr->xrepeat;
							yspan = tilesizy[tilenum]; yrepeat = spr->yrepeat;

							dax = ((xspan>>1)+xoff)*xrepeat; day = ((yspan>>1)+yoff)*yrepeat;
							x1 = spr->x + mulscale16(sinang,dax) + mulscale16(cosang,day);
							y1 = spr->y + mulscale16(sinang,day) - mulscale16(cosang,dax);
							l = xspan*xrepeat;
							x2 = x1 - mulscale16(sinang,l);
							y2 = y1 + mulscale16(cosang,l);
							l = yspan*yrepeat;
							k = -mulscale16(cosang,l); x3 = x2+k; x4 = x1+k;
							k = -mulscale16(sinang,l); y3 = y2+k; y4 = y1+k;

							ox = x1-cposx; oy = y1-cposy;
							x1 = mulscale16(ox,xvect) - mulscale16(oy,yvect);
							y1 = mulscale16(oy,xvect) + mulscale16(ox,yvect);

							ox = x2-cposx; oy = y2-cposy;
							x2 = mulscale16(ox,xvect) - mulscale16(oy,yvect);
							y2 = mulscale16(oy,xvect) + mulscale16(ox,yvect);

							ox = x3-cposx; oy = y3-cposy;
							x3 = mulscale16(ox,xvect) - mulscale16(oy,yvect);
							y3 = mulscale16(oy,xvect) + mulscale16(ox,yvect);

							ox = x4-cposx; oy = y4-cposy;
							x4 = mulscale16(ox,xvect) - mulscale16(oy,yvect);
							y4 = mulscale16(oy,xvect) + mulscale16(ox,yvect);

							drawline256(x1+(xdim<<11),y1+(ydim<<11),
											x2+(xdim<<11),y2+(ydim<<11),col);

							drawline256(x2+(xdim<<11),y2+(ydim<<11),
											x3+(xdim<<11),y3+(ydim<<11),col);

							drawline256(x3+(xdim<<11),y3+(ydim<<11),
											x4+(xdim<<11),y4+(ydim<<11),col);

							drawline256(x4+(xdim<<11),y4+(ydim<<11),
											x1+(xdim<<11),y1+(ydim<<11),col);

						}
						break;
				}
			}

		//Draw white lines
	for(i=0;i<numsectors;i++)
	{
		startwall = sector[i].wallptr;
		endwall = sector[i].wallptr + sector[i].wallnum - 1;

		for(j=startwall,wal=&wall[startwall];j<=endwall;j++,wal++)
		{
			if (wal->nextwall >= 0) continue;

			if ( !TestBitString(show2dwall, j) ) continue;

			ox = wal->x-cposx; oy = wal->y-cposy;
			x1 = mulscale16(ox,xvect) - mulscale16(oy,yvect);
			y1 = mulscale16(oy,xvect) + mulscale16(ox,yvect);

			wal2 = &wall[wal->point2];
			ox = wal2->x-cposx; oy = wal2->y-cposy;
			x2 = mulscale16(ox,xvect) - mulscale16(oy,yvect);
			y2 = mulscale16(oy,xvect) + mulscale16(ox,yvect);

			drawline256(x1+(xdim<<11),y1+(ydim<<11),x2+(xdim<<11),y2+(ydim<<11),24);
		}
	}
	return 0;
}
#endif
