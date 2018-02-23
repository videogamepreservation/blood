#include "horror.h"
#include "fire.h"

/****************************************************************************
** Jump parametrics
****************************************************************************/
#define kCrouchTime   45      	// ticks to attain highest jump
#define kTimeToApex   90      	// ticks to attain apex
#define kCrouchDist   (12 << 8)  // how far to crouch before jumping
#define kMaxJumpVel   (kTimeToApex * kNormalGravity / 100)
#define kJumpAccel	 	(kMaxJumpVel / kCrouchTime)
#define kDuckVel      (kCrouchDist / kCrouchTime)

/****************************************************************************
** Player status
****************************************************************************/
short gHealth    = 100;
short gDead      = FALSE;
short gJumping   = FALSE;
short gCrouching = FALSE;

/****************************************************************************
** Interface status
****************************************************************************/
short gViewStyle  = kViewNormal;
short gHasCompass = TRUE;
short gCompassUp  = FALSE;
short gPaused     = FALSE;
short gExit       = FALSE;
short gMenuMode   = !0;
int   gRandom;	 // reset every frame
char  gStatusShade = 0;

short startstate[] = {0,0,5,0,0,0};
short firearm=0, firestate=0, firetimer=0;

/****************************************************************************
** TEMPORARY GLOBALS
****************************************************************************/
char  boardfilename[20];
short lastsectnum;

/****************************************************************************
** ProcessIO - process keyboard i/o
**
** ARGUMENTS:
**	none
**
** RETURNS:
**	none
****************************************************************************/
void ProcessIO(void)
{
	static short doubvel;
	static long springVelocity = 0;

	// temp handling for escape key
	if (keystatus[kKeyEscape] > 0) {
		gExit = TRUE;
		return;
	}

	// PLAYER IS DEAD?
	if (gDead) {
		if (keystatus[kKeySpace] > 0) {
			gDead   = FALSE;
			gHealth = 100;
			keystatus[kKeySpace] = 0;
		}
		return;
	}

#if 0
	// COMPASS TOGGLE
	if (keystatus[ kKeyC ] > 0) {
		if (gHasCompass) {
			gCompassUp = BOOL(!gCompassUp);
		}
		keystatus[ kKeyC ] = 0;
	}
#endif

	// FIRING & WEAPON SELECTION
	if (firestate == startstate[firearm]) {
		if (keystatus[kKeyLCtrl] > 0) {
			firestate++;
		}
		else if (keystatus[kKey1] > 0) {
			firearm = 0;
			firestate = startstate[firearm];
		}
		else if (keystatus[kKey2] > 0) {
			firearm = 1;
			firestate = startstate[firearm];
		}
		else if (keystatus[kKey3] > 0) {
			firearm = 2;
			firestate = startstate[firearm];
		}
		else if (keystatus[kKey4] > 0) {
			firearm = 3;
			firestate = startstate[firearm];
		}
		else if (keystatus[kKey5] > 0) {
			firearm = 4;
			firestate = startstate[firearm];
		}
		else if (keystatus[kKey6] > 0) {
			firearm = 5;
			firestate = startstate[firearm];
		}
	}

	// PLAYER PUSHED SOMETHING
	if (keystatus[kKeySpace] > 0) {
		P_Push();
		keystatus[kKeySpace] = 0;
	}

  // CHANGE HORIZON
	if ((keystatus[0x4a] > 0) && (horiz >= 8)) horiz -= 8;     //-
	if ((keystatus[0x4e] > 0) && (horiz <= 180)) horiz += 8;   //+
	if (keystatus[0x4C] > 0) {
		horiz = 100;
		keystatus[0x4C] = 0;
	}

  // CHANGE PARALLAX SKY TYPE
	if (keystatus[0x19] > 0)  //P
	{
		keystatus[0x19] = 0;
		parallaxtype++;
		if (parallaxtype > 2) parallaxtype = 0;
	}

  // CHANGE VISIBILITY
	if (keystatus[0x2f] > 0) {
		keystatus[0x2f] = 0;
		visibility++;
		if (visibility == 17)
			visibility = 5;
	}

	// PROCESS DIRECTIONAL MOVEMENT
	if (angvel != 0) {
		//ang += angvel * constant
		//ENGINE calculates angvel for you
		doubvel = lockspeed;

		//Left shift makes turn velocity 50% faster
		if (keystatus[kKeyLShift] > 0)
			doubvel += (lockspeed>>1);

		ang += ((angvel*doubvel)>>5);
		ang = (ang+2048)&2047;
	}

	doubvel = lockspeed;
	//Left shift doubles forward velocity
	if (keystatus[0x2a] > 0)
		doubvel += lockspeed;

	//vel = forward velocity
	if (vel != 0) {
		//Change posx/posy with ang & clip
		clipmove( &posx, &posy, &posz, &cursectnum,
			(long)(vel * doubvel * sintable[(ang + 2560) & 2047]) >> 4,
			(long)(vel * doubvel * sintable[ang & 2047]) >> 4, 128L);
	}
	//svel = strafing velocity
	if (svel != 0) {
		//Change posx/posy with ang & clip
		clipmove( &posx, &posy, &posz, &cursectnum,
			(long)(svel * doubvel * sintable[ang & 2047]) >> 4,
			(long)(svel * doubvel * sintable[(ang + 1536) & 2047]) >> 4, 128L);
	}

	// LeftAlt: PLAYER IS JUMPING
	if (keystatus[kKeyLAlt] > 0) {
		if (!gJumping) {
			springVelocity = 0;
			gJumping = TRUE;
		}
		if (springVelocity < kMaxJumpVel) {
			heightofffloor -= lockspeed * kDuckVel;
			springVelocity += lockspeed * kJumpAccel;
		}
	}
	else if (gJumping) {
		if (posz > sector[cursectnum].floorz - kNormHeightOffFloor) {
			if (hvel > 0) hvel = 0;
			hvel -= springVelocity;
			/* this is a kludge here because Ken's engine doesn't properly use
	 		* the value in hvel when posz is below heightofffloor. */
			posz = sector[cursectnum].floorz - heightofffloor - 1;
		}
		gJumping = FALSE;
	}

	// 'Z': PLAYER IS DUCKING
	if (keystatus[0x2c] > 0) {
		heightofffloor -= lockspeed * kDuckVel;
		//Either shift key
		if ((keystatus[0x2a]|keystatus[0x36]) > 0) {
			heightofffloor -= lockspeed * kDuckVel;
		}
		gJumping = FALSE;
 		gCrouching = TRUE;
	}
	else if (gCrouching) {
		gCrouching = FALSE;
		heightofffloor = kNormHeightOffFloor;
	}

	if (!gJumping && !gCrouching && hvel > 0) {
		heightofffloor = kNormHeightOffFloor;
	}

	P_TransitSectors(); // entry/exit sector triggers
	P_GetItems();       // get items in sector
}

/****************************************************************************
** Draw3DScreen
**
** ARGUMENTS:
**	none
**
** RETURNS:
**	none
****************************************************************************/
void Draw3DScreen(void)
{
	short i, j, isprite, dang;
	struct spritetype *pSprite;


	drawrooms();

	// reset picnum for multi-face sprites
	for (i=0; i<spritesortcnt; i++)
	{
		if ((isprite = thesprite[i]) >= 0) {
			pSprite = &sprite[isprite];
			switch ( pSprite->tag )
			{
				case kTagGoldRing:
					dang = getangle(pSprite->x-posx,pSprite->y-posy);
					j = (((dang + 64) & kAngleMask) << 4) / 1365;
					if (j <= 6) {
						pSprite->picnum = kAnmGoldRing + j;
						pSprite->cstat |= kSpriteFlipX;
					} else if (j <= 12) {
						pSprite->picnum = kAnmGoldRing + 12 - j;
						pSprite->cstat &= ~kSpriteFlipX;
					}
					break;
				case kTagGargoyle:
					break;
				case kTagPlayer1:
					sprite[isprite].picnum = kInvisible;
					break;
			}
		}
	}
	drawmasks();
}

/****************************************************************************
** DrawStatus
**
** ARGUMENTS:
**	none
**
** RETURNS:
**	none
****************************************************************************/
void DrawStatus(void)
{
	signed char  gunshade;
	short i, daang, hitsect, hitwall=-1, hitsprite=-1;
	long hitx, hity, hitz, daz;
	short damage;

	// set the weapon shade
	gunshade = 0;
#if 0
	gunshade = sector[cursectnum].floorshade;
	if (gunshade < 0) {
		gunshade = 0;
	} else {
		gunshade &= 31;
	}
#endif

	switch (firestate) {
		// PISTOL
		case 0:
			overwritesprite(135,134,kAnmPistol,gunshade,0);
			break;
		case 1:
		case 3:
			if (firestate == 1) {
				if (firetimer == 0) {
					visibility++;
				} else {
					visibility--;
				}
			}
			overwritesprite(105,134-20,kAnmPistol+1,visibility,0);
			if ((firestate == 1) && (firetimer==0)) {
				PlaySound(kPISTOL,255);
				overwritesprite(160,114,kAnmPistolFlash,0,1);

				daang = ((ang + (gRandom & 63) + 2048 - 32 ) & 2047);
				daz   = ((100 - horiz) * 2621);
				daz   += (((((long)rand()) & 32767L) - 16384L) << 1);
				hitscan(posx, posy, posz, cursectnum, //Start position
					sintable[(daang + 2560) & 2047],    //X vector of 3D ang
					sintable[(daang + 2048) & 2047],    //Y vector of 3D ang
					daz,                                //Z vector of 3D ang
					&hitsect, &hitwall, &hitsprite, &hitx, &hity, &hitz);
				//A_NoiseAlert(hitsect);
				if (hitsprite > -1) {
					damage = ((gRandom % 8) + 1) * 2;
					FX_TriggerSprite(hitsprite,kSpriteImpact,&damage);
//					P_DamageSprite(hitsprite,(gRandom%8+1)*2);
				}
				else if (hitwall > -1) {
					FX_SpawnImpact(hitsect,hitx,hity,hitz,kTagPuff);
					if (wall[hitwall].tag & kWallImpact) {
						FX_TriggerWall(hitwall,kWallImpact, NULL);
					}
				}
				else if (sector[hitsect].tag & kSectImpact) {
					FX_TriggerSector(hitsect,kSectImpact, NULL);
				}
			}
			break;
		case 2:
			overwritesprite(105,134-41,kAnmPistol+2,visibility,0);
			break;
		case 4:
			overwritesprite(135,134,kAnmPistol,visibility,0);
			firestate = startstate[firearm];
			firetimer = 0;
			break;

		// SHOTGUN
		case 5:
			overwritesprite(160,165,kAnmShotgun,gunshade,1);
			break;
		case 6:
			if (firetimer==0) {
				visibility++;
				PlaySound(kSHOTGUN,255);
				for (i=0;i<5;i++)
				{
					daang = ((ang + (rand() & 63) + 2048 - 32 ) & 2047);
					daz   = ((100 - horiz) * 2621);
					daz   += (((((long)rand()) & 32767L) - 16384L) << 1);
					hitscan(posx, posy, posz, cursectnum, //Start position
						sintable[(daang + 2560) & 2047],    //X vector of 3D ang
						sintable[(daang + 2048) & 2047],    //Y vector of 3D ang
						daz,                                //Z vector of 3D ang
						&hitsect, &hitwall, &hitsprite, &hitx, &hity, &hitz);
					//A_NoiseAlert(hitsect);
					if (hitsprite > -1) {
						damage = ((gRandom % 8) + 1) * 2;
						FX_TriggerSprite(hitsprite,kSpriteImpact,&damage);
//						P_DamageSprite(hitsprite,(gRandom%8+1)*2);
					}
					else if (hitwall > -1) {
						FX_SpawnImpact(hitsect,hitx,hity,hitz,kTagPuff);
						if (wall[hitwall].tag & kWallImpact) {
							// COULD ADD WALL DAMAGE HERE
							FX_TriggerWall(hitwall,kWallImpact,NULL);
						}
					}
					else if (sector[hitsect].tag & kSectImpact) {
							// COULD ADD SECTOR DAMAGE HERE
						FX_TriggerSector(hitsect,kSectImpact, NULL);
					}
				}
			} else {
				visibility--;
			}
			overwritesprite(160,170,kAnmShotgun,visibility,1);
			overwritesprite(163,125,kAnmShotgun+4,0,1);
			break;
		case 7:
			overwritesprite(160,170,kAnmShotgun,visibility,1);
			overwritesprite(163,125,kAnmShotgun+5,0,1);
			break;
		case 8:
		case 12:
			overwritesprite(80-24,100-41,kAnmShotgun+1,visibility,0);
			if (firestate == 12) {
				firestate = startstate[firearm];
				firetimer = 0;
			}
			break;
		case 9:
		case 11:
			overwritesprite(80-24,100-20,kAnmShotgun+2,visibility,0);
			break;
		case 10:
			if (firetimer==0) PlaySound(kSHOTCOCK,255);
			overwritesprite(80,80,kAnmShotgun+3,visibility,0);
			break;
	}
	// advance the firing state, reset firing timer
	if (firestate != startstate[firearm]) {
		if (++firetimer > 1) {
			firestate++;
			firetimer=0;
		}
	}

	// draw the compass
#if 0
	if (gHasCompass && gCompassUp) {
		overwritesprite(160, YDIM-60,	kCompass, visibility, 1);
	}

	if (gViewStyle == kViewFullScreen) {
		overwritesprite(0, YDIM-tilesizy[kStatusFace],
			kStatusFace, gStatusShade, 0);
		overwritesprite(XDIM-tilesizx[kStatusCompass], YDIM-tilesizy[kStatusCompass],
			kStatusCompass + (((ang + kAngle90) & kAngleMask) >> 8), gStatusShade, 0);
	}
#endif
}

/****************************************************************************
** main - it all starts here...
****************************************************************************/
int main(int argc, char *argv[])
{
	UBYTE *CLU[4];

	InstallBuild();
	GetArgs(argc,argv);	// also get map from command-line
	InstallSound();

	CLU[0] = LoadFile( "red.clu" );
	CLU[1] = LoadFile( "green.clu" );
	CLU[2] = LoadFile( "blue.clu" );
	CLU[3] = LoadFile( "grey.clu" );
	// add error checks

	FireInit( 128 ); // create 128x128 combustion chamber

	if (loadboard(boardfilename) == -1) {
		Abendf( "Board \"%s\" not found.\n", boardfilename );
	}
	loadpics("tiles000.art");
	FX_InitPanning();
	FX_InitLights();
	FX_InitSprites();
	FX_InitAlways();

	scrSetGameMode();
	SetViewStyle(kViewFullScreen);

	while (gExit == FALSE)
	{
		gRandom = rand();
		FireProcess();
		FireUpdateTile( CLU[0], (UBYTE *)waloff[ kAnmRedFire ] );
		FireUpdateTile( CLU[1], (UBYTE *)waloff[ kAnmGreenFire ] );
 		FireUpdateTile( CLU[2], (UBYTE *)waloff[ kAnmBlueFire ] );
 		FireUpdateTile( CLU[3], (UBYTE *)waloff[ kAnmGreyFire ] );
		lastsectnum = cursectnum;
		Draw3DScreen();
		ProcessIO();

		if (gPaused == FALSE) {
			FX_DoPanning();
			FX_DoLights();
			FX_DoAlways();
			// other effects here
		}
		engineinput();
		DrawStatus();
		FX_MoveSprites();
		nextpage();
	}
	RemoveBuild();

	FireDeinit();

#ifdef DEBUG
	showengineinfo();
#endif
	return 0;
}


