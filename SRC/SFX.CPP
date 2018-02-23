#include <string.h>
#include <dos.h>

#include "sfx.h"
#include "db.h"
#include "globals.h"
#include "sound.h"
#include "engine.h"
#include "error.h"
#include "fx_man.h"
#include "player.h"
#include "gameutil.h"
#include "trig.h"
#include "globals.h"
#include "misc.h"
#include "debug4g.h"

// sample rate in samples / sec
#define kSampleRate		11110

// sound of sound in XY units / sec
#define kSoundVel		M2X(500.0)

// sound of sound in XY units / frame interval
#define kSoundVelFrame	(kSoundVel * kFrameTicks / kTimerRate)

// Intra-aural delay in seconds
#define kIntraAuralTime	0.0008

// Intra-aural distance based on delay and speed of sound
#define kIntraAuralDist	(kIntraAuralTime / 2 * kSoundVel)

// 16:16 fixed point constant for convert XY distance to sample phase
#define kDistPhase		((kSampleRate << 16) / kSoundVel)

// scale value for all 3D sounds
#define kVolScale		256

// volume different between sound in front and behind pinna focus
#define kBackFilter 	0x8000

// angle between forward and foci for pinna
#define kPinnaAngle 	kAngle15

static POINT2D earL, earR, earL0, earR0;
static VECTOR2D earVL, earVR;
static int lPhase, rPhase, lVol, rVol;
static int lVel, rVel;

struct SOUNDEFFECT
{
	char *name;
	int	relVol;
	int	pitch;
	int	pitchRange;
	RESHANDLE hResource;
};

SOUNDEFFECT soundEffect[ kSfxMax ] =
{
	{"zipclose",	80,		0x10000,	0},	// kSfxZipOpen
	{"zipclose",	80,		0x08000,	0},	// kSfxZipClose
	{"ziplight",	80,		0x10000,	0},	// kSfxZipFlick
	{"saw_cock",	80,		0x10000,	0},	// kSfxShotCock
	{"saw_fir",		160,	0x10000,	0},	// kSfxShotFire
	{"saw_fir2",	160,	0x10000,	0},	// kSfxShotFire2
	{"saw_load",	80,		0x10000,	0},	// kSfxShotLoad
	{"flar_fir",	60,		0x10000,	0},	// kSfxFlareFire
	{"flar_fir",	60,		0x10000,	0},	// kSfxFlareFireUW
	{"",			80,		0x10000,	0},	// kSfxTomCock
	{"_gunb",		200,	0x10000,	0},	// kSfxTomFire
	{"spraycan",	80,		0x10000,	0},	// kSfxSprayPaint
	{"sprayflm",	80,		0x10000,	0},	// kSfxSprayFlame
	{"tnt_fuse",	80,		0x10000,	0},	// kSfxTNTFuse
	{"dol_stab",	80,		0x10000,	0},	// kSfxTNTDropProx
	{"tnt_arm",		80,		0x10000,	0},	// kSfxTNTArmProx
	{"tnt_det",		80,		0x10000,	0},	// kSfxTNTDetProx
	{"dol_stab",	80,		0x10000,	0},	// kSfxTNTDropRemote
	{"tnt_arm",		80,		0x10000,	0},	// kSfxTNTArmRemote
	{"tnt_det",		80,		0x10000,	0},	// kSfxTNTDetRemote
	{"tnt_toss",	40,		0x10000,	0},	// kSfxTNTToss
	{"dol_stab",	80,		0x10000,	0},	// kSfxVoodooHit
	{"dol_stab",	80,		0x10000,	0},	// kSfxVoodooHit2
	{"dol_stab",	80,		0x10000,	0},	// kSfxVoodooHit3
	{"dol_stab",	80,		0x10000,	0},	// kSfxVoodooHit4
	{"cult2",		80,		0x18000,	0},	// kSfxVoodooChant
	{"dol_stab",	80,		0x10000,	0},	// kSfxVoodooBreak
	{"dol_burn",	80,		0x10000,	0},	// kSfxVoodooBurn
	{"spr_cham",	80,		0x10000,	0},	// kSfxSpearLoad
	{"spr_fire",	80,		0x10000,	0},	// kSfxSpearFire
	{"spr_uw",		80,		0x10000,	0},	// kSfxSpearFireUW
	{"",			80,		0x10000,	0},	// kSfxBlasterFire
	{"pf_rock",		10,		0x10000,	0},	// kSfxForkStone
	{"pf_metal",	15,		0x10000,	0},	// kSfxForkMetal
	{"pf_wood",		10,		0x10000,	0},	// kSfxForkWood
	{"pf_flesh",	15,		0x10000,	0},	// kSfxForkFlesh
	{"",			80,		0x10000,	0},	// kSfxForkWater
	{"",			80,		0x10000,	0},	// kSfxForkDirt
	{"",			80,		0x10000,	0},	// kSfxForkClay
	{"",			80,		0x10000,	0},	// kSfxForkSnow
	{"",			80,		0x10000,	0},	// kSfxForkIce
	{"",			80,		0x10000,	0},	// kSfxForkLeaves
	{"",			80,		0x10000,	0},	// kSfxForkPlant
	{"",			80,		0x10000,	0},	// kSfxForkGoo
	{"",			80,		0x10000,	0},	// kSfxForkLava
	{"pf_uw_rk",	10,		0x10000,	0},	// kSfxForkStoneUW
	{"pf_uw_mt",	10,		0x10000,	0},	// kSfxForkMetalUW
	{"pf_uw_wd",	10,		0x10000,	0},	// kSfxForkWoodUW
	{"pf_uw_fl",	10,		0x10000,	0},	// kSfxForkFleshUW
	{"",			80,		0x10000,	0},	// kSfxForkDirtUW
	{"",			80,		0x10000,	0},	// kSfxForkClayUW
	{"",			80,		0x10000,	0},	// kSfxForkIceUW
	{"",			80,		0x10000,	0},	// kSfxForkLeavesUW
	{"",			80,		0x10000,	0},	// kSfxForkPlantUW
	{"tom_ston",	80,		0x10000,	0},	// kSfxBulletStone
	{"tom_metl",	80,		0x10000,	0},	// kSfxBulletMetal
	{"tom_wood",	80,		0x10000,	0},	// kSfxBulletWood
	{"tom_flsh",	80,		0x10000,	0},	// kSfxBulletFlesh
	{"tom_watr",	80,		0x10000,	0},	// kSfxBulletWater
	{"tom_wood",	80,		0x10000,	0},	// kSfxBulletDirt
	{"tom_wood",	80,		0x10000,	0},	// kSfxBulletClay
	{"tom_ston",	80,		0x10000,	0},	// kSfxBulletSnow
	{"tom_ston",	80,		0x10000,	0},	// kSfxBulletIce
	{"tom_ston",	80,		0x10000,	0},	// kSfxBulletLeaves
	{"tom_ston",	80,		0x10000,	0},	// kSfxBulletPlant
	{"tom_ston",	80,		0x10000,	0},	// kSfxBulletGoo
	{"tom_wood",	80,		0x10000,	0},	// kSfxBulletLava
	{"",			80,		0x10000,	0},	// kSfxBulletStoneUW
	{"",			80,		0x10000,	0},	// kSfxBulletMetalUW
	{"",			80,		0x10000,	0},	// kSfxBulletWoodUW
	{"",			80,		0x10000,	0},	// kSfxBulletFleshUW
	{"",			80,		0x10000,	0},	// kSfxBulletDirtUW
	{"",			80,		0x10000,	0},	// kSfxBulletClayUW
	{"",			80,		0x10000,	0},	// kSfxBulletIceUW
	{"",			80,		0x10000,	0},	// kSfxBulletLeavesUW
	{"",			80,		0x10000,	0},	// kSfxBulletPlantUW
	{"dmhack2",		60,		0x10000,	0},	// kSfxAxeAir
	{"",			80,		0x10000,	0},	// kSfxAxeStone
	{"",			80,		0x10000,	0},	// kSfxAxeMetal
	{"",			80,		0x10000,	0},	// kSfxAxeWood
	{"",			80,		0x10000,	0},	// kSfxAxeFlesh
	{"",			80,		0x10000,	0},	// kSfxAxeWater
	{"",			80,		0x10000,	0},	// kSfxAxeDirt
	{"",			80,		0x10000,	0},	// kSfxAxeClay
	{"",			80,		0x10000,	0},	// kSfxAxeSnow
	{"",			80,		0x10000,	0},	// kSfxAxeIce
	{"",			80,		0x10000,	0},	// kSfxAxeLeaves
	{"",			80,		0x10000,	0},	// kSfxAxePlant
	{"",			80,		0x10000,	0},	// kSfxAxeGoo
	{"",			80,		0x10000,	0},	// kSfxAxeLava
	{"",			80,		0x10000,	0},	// kSfxAxeStoneUW
	{"",			80,		0x10000,	0},	// kSfxAxeMetalUW
	{"",			80,		0x10000,	0},	// kSfxAxeWoodUW
	{"",			80,		0x10000,	0},	// kSfxAxeFleshUW
	{"",			80,		0x10000,	0},	// kSfxAxeDirtUW
	{"",			80,		0x10000,	0},	// kSfxAxeClayUW
	{"",			80,		0x10000,	0},	// kSfxAxeIceUW
	{"",			80,		0x10000,	0},	// kSfxAxeLeavesUW
	{"",			80,		0x10000,	0},	// kSfxAxePlantUW
	{"",			80,		0x10000,	0},	// kSfxClawStone
	{"",			80,		0x10000,	0},	// kSfxClawMetal
	{"",			80,		0x10000,	0},	// kSfxClawWood
	{"",			80,		0x10000,	0},	// kSfxClawFlesh
	{"",			80,		0x10000,	0},	// kSfxClawWater
	{"",			80,		0x10000,	0},	// kSfxClawDirt
	{"",			80,		0x10000,	0},	// kSfxClawClay
	{"",			80,		0x10000,	0},	// kSfxClawSnow
	{"",			80,		0x10000,	0},	// kSfxClawIce
	{"",			80,		0x10000,	0},	// kSfxClawLeaves
	{"",			80,		0x10000,	0},	// kSfxClawPlant
	{"",			80,		0x10000,	0},	// kSfxClawGoo
	{"",			80,		0x10000,	0},	// kSfxClawLava
	{"",			80,		0x10000,	0},	// kSfxClawStoneUW
	{"",			80,		0x10000,	0},	// kSfxClawMetalUW
	{"",			80,		0x10000,	0},	// kSfxClawWoodUW
	{"",			80,		0x10000,	0},	// kSfxClawFleshUW
	{"",			80,		0x10000,	0},	// kSfxClawDirtUW
	{"",			80,		0x10000,	0},	// kSfxClawClayUW
	{"",			80,		0x10000,	0},	// kSfxClawIceUW
	{"",			80,		0x10000,	0},	// kSfxClawLeavesUW
	{"",			80,		0x10000,	0},	// kSfxClawPlantUW
	{"footstn1",	80,		0x10000,	0},	// kSfxFootStone1
	{"footstn2",	80,		0x10000,	0},	// kSfxFootStone2
	{"footmtl1",	80,		0x10000,	0},	// kSfxFootMetal1
	{"footmtl2",	80,		0x10000,	0},	// kSfxFootMetal2
	{"footwod1",	80,		0x10000,	0},	// kSfxFootWood1
	{"footwod2",	80,		0x10000,	0},	// kSfxFootWood2
	{"footwtr1",	80,		0x10000,	0},	// kSfxFootFlesh1
	{"footwtr2",	80,		0x10000,	0},	// kSfxFootFlesh2
	{"footwtr1",	80,		0x10000,	0},	// kSfxFootWater1
	{"footwtr2",	80,		0x10000,	0},	// kSfxFootWater2
	{"footdrt1",	80,		0x10000,	0},	// kSfxFootDirt1
	{"footdrt2",	80,		0x10000,	0},	// kSfxFootDirt2
	{"footdrt1",	80,		0x10000,	0},	// kSfxFootClay1
	{"footdrt2",	80,		0x10000,	0},	// kSfxFootClay2
	{"footgrs1",	80,		0x10000,	0},	// kSfxFootSnow1
	{"footgrs2",	80,		0x10000,	0},	// kSfxFootSnow2
	{"footwtr1",	80,		0x10000,	0},	// kSfxFootIce1
	{"footwtr2",	80,		0x10000,	0},	// kSfxFootIce2
	{"footgrs2",	80,		0x10000,	0},	// kSfxFootLeaves
	{"footdrt1",	80,		0x10000,	0},	// kSfxFootCloth1
	{"footdrt2",	80,		0x10000,	0},	// kSfxFootCloth2
	{"footgrs1",	80,		0x10000,	0},	// kSfxFootPlant1
	{"footgrs2",	80,		0x10000,	0},	// kSfxFootPlant2
	{"footwtr1",	80,		0x10000,	0},	// kSfxFootGoo1
	{"footwtr2",	80,		0x10000,	0},	// kSfxFootGoo2
	{"footgrs1",	80,		0x10000,	0},	// kSfxFootLava1
	{"footgrs2",	80,		0x10000,	0},	// kSfxFootLava2
	{"lnd_stn",		80,		0x10000,	0},	// kSfxDudeLandStone
	{"lnd_metl",	80,		0x10000,	0},	// kSfxDudeLandMetal
	{"lnd_wood",	80,		0x10000,	0},	// kSfxDudeLandWood
	{"lnd_wtr",		80,		0x10000,	0},	// kSfxDudeLandFlesh
	{"lnd_wtr",		80,		0x10000,	0},	// kSfxDudeLandWater
	{"lnd_dirt",	80,		0x10000,	0},	// kSfxDudeLandDirt
	{"",			80,		0x10000,	0},	// kSfxDudeLandClay
	{"",			80,		0x10000,	0},	// kSfxDudeLandSnow
	{"",			80,		0x10000,	0},	// kSfxDudeLandIce
	{"",			80,		0x10000,	0},	// kSfxDudeLandLeaves
	{"",			80,		0x10000,	0},	// kSfxDudeLandCloth
	{"",			80,		0x10000,	0},	// kSfxDudeLandPlant
	{"",			80,		0x10000,	0},	// kSfxDudeLandGoo
	{"",			80,		0x10000,	0},	// kSfxDudeLandLava
	{"",			80,		0x10000,	0},	// kSfxBodyLandStone
	{"",			80,		0x10000,	0},	// kSfxBodyLandMetal
	{"",			80,		0x10000,	0},	// kSfxBodyLandWood
	{"",			80,		0x10000,	0},	// kSfxBodyLandFlesh
	{"",			80,		0x10000,	0},	// kSfxBodyLandWater
	{"",			80,		0x10000,	0},	// kSfxBodyLandDirt
	{"",			80,		0x10000,	0},	// kSfxBodyLandClay
	{"",			80,		0x10000,	0},	// kSfxBodyLandSnow
	{"",			80,		0x10000,	0},	// kSfxBodyLandIce
	{"",			80,		0x10000,	0},	// kSfxBodyLandLeaves
	{"",			80,		0x10000,	0},	// kSfxBodyLandCloth
	{"",			80,		0x10000,	0},	// kSfxBodyLandPlant
	{"",			80,		0x10000,	0},	// kSfxBodyLandGoo
	{"",			80,		0x10000,	0},	// kSfxBodyLandLava
	{"",			80,		0x10000,	0},	// kSfxGibLandStone
	{"",			80,		0x10000,	0},	// kSfxGibLandMetal
	{"",			80,		0x10000,	0},	// kSfxGibLandWood
	{"",			80,		0x10000,	0},	// kSfxGibLandFlesh
	{"",			80,		0x10000,	0},	// kSfxGibLandWater
	{"",			80,		0x10000,	0},	// kSfxGibLandDirt
	{"",			80,		0x10000,	0},	// kSfxGibLandClay
	{"",			80,		0x10000,	0},	// kSfxGibLandSnow
	{"",			80,		0x10000,	0},	// kSfxGibLandIce
	{"",			80,		0x10000,	0},	// kSfxGibLandLeaves
	{"",			80,		0x10000,	0},	// kSfxGibLandCloth
	{"",			80,		0x10000,	0},	// kSfxGibLandPlant
	{"",			80,		0x10000,	0},	// kSfxGibLandGoo
	{"",			80,		0x10000,	0},	// kSfxGibLandLava
	{"",			80,		0x10000,	0},	// kSfxSplashS
	{"",			80,		0x10000,	0},	// kSfxSplashM
	{"",			80,		0x10000,	0},	// kSfxSplashL
	{"arc1",		80,		0x10000,	0},	// kSfxArc1
	{"arc2",		80,		0x10000,	0},	// kSfxArc2
	{"arc3",		80,		0x10000,	0},	// kSfxArc3
	{"",			80,		0x10000,	0},	// kSfxBladeDrop
	{"",			80,		0x10000,	0},	// kSfxBladeSwipe
	{"bould3",		80,		0x10000,	0},	// kSfxBoulders
	{"_exploa",		200,	0x10000,	0},	// kSfxExplodeCS	// exp_tnt
	{"exp_barl",	200,	0x10000,	0},	// kSfxExplodeCM
	{"exp_lrg",		400,	0x10000,	0},	// kSfxExplodeCL
	{"exp_tnt",		100,	0x10000,	0},	// kSfxExplodeFS
	{"exp_barl",	200,	0x10000,	0},	// kSfxExplodeFM
	{"exp_lrg",		400,	0x10000,	0},	// kSfxExplodeFL
	{"exp_uw1",		80,		0x10000,	0},	// kSfxExplode1UW
	{"exp_uw2",		80,		0x10000,	0},	// kSfxExplode2UW
	{"exp_uw1",		80,		0x10000,	0},	// kSfxExplode3UW
	{"",			80,		0x10000,	0},	// kSfxFireballPrefire
	{"",			80,		0x10000,	0},	// kSfxFireballFire
	{"dmpistol",	200,	0x10000,	0},	// kSfxMGFire
	{"_mgdie",		160,	0x10000,	0},	// kSfxMGDie
	{"",			80,		0x10000,	0},	// kSfxSawStart
	{"",			80,		0x10000,	0},	// kSfxSawRun
	{"",			80,		0x10000,	0},	// kSfxSawStop
	{"",			80,		0x10000,	0},	// kSfxSawCut
	{"",			80,		0x10000,	0},	// kSfxAmbientCave
	{"",			80,		0x10000,	0},	// kSfxAmbientFauna
	{"",			80,		0x10000,	0},	// kSfxAmbientMechanical
	{"rain2",		80,		0x10000,	0},	// kSfxAmbientRain
	{"undrwatr",	30,		0x10000,	0},	// kSfxAmbientUW
	{"",			80,		0x10000,	0},	// kSfxAmbientWind
	{"",			80,		0x10000,	0},	// kSfxAmbientMansion
	{"",			80,		0x10000,	0},	// kSfxAmbientCastle
	{"",			80,		0x10000,	0},	// kSfxAmbientRainIndoors
	{"",			80,		0x10000,	0},	// kSfxAmbientCreepy1
	{"",			80,		0x10000,	0},	// kSfxAmbientCreepy2
	{"",			80,		0x10000,	0},	// kSfxAmbientCreepy3
	{"bubrise",		80,		0x10000,	0},	// kSfxBubbles
	{"",			80,		0x10000,	0},	// kSfxCrickets
	{"dripsurf",	10,		0x10000,	0},	// kSfxDrip1
	{"dripwat4",	10,		0x10000,	0},	// kSfxDrip2
	{"dripwat5",	10,		0x10000,	0},	// kSfxDrip3
	{"",			80,		0x10000,	0},	// kSfxFrog
	{"",			80,		0x10000,	0},	// kSfxLeaves
	{"",			80,		0x10000,	0},	// kSfxOwl
	{"",			80,		0x10000,	0},	// kSfxRaven
	{"dmspawn",		80,		0x10000,	0},	// kSfxRespawn
	{"sewage",		80,		0x10000,	0},	// kSfxSewage
	{"thunder",		80,		0x10000,	0},	// kSfxThunder
	{"thunder2",	80,		0x10000,	0},	// kSfxThunder2
	{"",			80,		0x10000,	0},	// kSfxTrees
	{"waterlap",	80,		0x10000,	0},	// kSfxWaterLap
	{"water2",		80,		0x10000,	0},	// kSfxWaterStream
	{"bdy_burn",	50,		0x10000,	0},	// kSfxBurn
	{"flesh3",		80,		0x10000,	0},	// kSfxSizzle
	{"grandck3",	10,		0x10000,	0},	// kSfxTickTock
	{"gust1",		80,		0x10000,	0},	// kSfxGust1
	{"gust2",		80,		0x10000,	0},	// kSfxGust2
	{"gust3",		80,		0x10000,	0},	// kSfxGust3
	{"gust4",		80,		0x10000,	0},	// kSfxGust4
	{"",			80,		0x10000,	0},	// kSfxWhisper
	{"",			80,		0x10000,	0},	// kSfxLaugh
	{"chains",		80,		0x10000,	0},	// kSfxChains
	{"",			80,		0x10000,	0},	// kSfxMoan
	{"",			80,		0x10000,	0},	// kSfxSigh
	{"floorcr2",	80,		0x10000,	0},	// kSfxCreak
	{"",			80,		0x10000,	0},	// kSfxSqueak
	{"",			80,		0x10000,	0},	// kSfxScuffle
	{"",			80,		0x10000,	0},	// kSfxRumble
	{"",			80,		0x10000,	0},	// kSfxScream
	{"dorcreak",	80,		0x10000,	0},	// kSfxDoorCreak
	{"doorslid",	80,		0x10000,	0},	// kSfxDoorSlide
	{"floorcr2",	80,		0x10000,	0},	// kSfxFloorCreak
	{"gearstrt",	80,		0x10000,	0},	// kSfxGearStart
	{"geargrin",	80,		0x10000,	0},	// kSfxGearMove
	{"gearhalt",	80,		0x10000,	0},	// kSfxGearStop
	{"glashit2",	120,	0x10000,	0},	// kSfxGlassHit
	{"elevstrt",	80,		0x10000,	0},	// kSfxLiftStart
	{"elevmov",		80,		0x10000,	0},	// kSfxLiftMove
	{"elevstop",	80,		0x10000,	0},	// kSfxLiftStop
	{"chains",		80,		0x10000,	0},	// kSfxPadlock
	{"pottery",		80,		0x10000,	0},	// kSfxPotteryHit
	{"slabmove",	80,		0x10000,	0},	// kSfxSlabMove
	{"",			80,		0x10000,	0},	// kSfxSplat
	{"dooropen",	80,		0x10000,	0},	// kSfxSwingOpen
	{"doorclos",	80,		0x10000,	0},	// kSfxSwingShut
	{"switch5",		40,		0x10000,	0},	// kSfxSwitch1
	{"switch5",		40,		0x10000,	0},	// kSfxSwitch2
	{"switch5",		40,		0x10000,	0},	// kSfxSwitch3
	{"switch5",		40,		0x10000,	0},	// kSfxSwitch4
	{"",			80,		0x10000,	0},	// kSfxWoodBreak
	{"jump",		40,		0x10000,	0},	// kSfxPlayJump
	{"land",		40,		0x10000,	0},	// kSfxPlayLand
	{"",			80,		0x10000,	0},	// kSfxHunt
	{"pl_swim1",	80,		0x10000,	0},	// kSfxPlaySwim
	{"pl_uw_sw",	80,		0x10000,	0},	// kSfxPlaySwimUW
	{"breathe1",	80,		0x10000,	0},	// kSfxPlayBreathe1
	{"breathe2",	80,		0x10000,	0},	// kSfxPlayBreathe2
	{"_pain",		80,		0x10000,	0},	// kSfxPlayPain
	{"death",		80,		0x10000,	0},	// kSfxPlayDie
	{"bdy_splg",	80,		0x10000,	0},	// kSfxPlayDie2
	{"deathbrn",	80,		0x10000,	0},	// kSfxPlayDie3
	{"fall",		80,		0x10000,	0},	// kSfxPlayFall
	{"choke",		80,		0x10000,	0},	// kSfxPlayChoke
	{"choke2",		80,		0x10000,	0},	// kSfxPlayChoke2
	{"paingasp",	80,		0x10000,	0},	// kSfxPlayGasp
	{"hot_ouch",	80,		0x10000,	0},	// kSfxPlayHotFoot
	{"dol_lafs",	80,		0x10000,	0},	// kSfxPlayLaugh
	{"submerge",	80,		0x10000,	0},	// kSfxPlaySubmerge
	{"emerge",		80,		0x10000,	0},	// kSfxPlayEmerge
	{"pickup",		80,		0x10000,	0},	// kSfxPlayItemUp
	{"dmgetpow",	80,		0x10000,	0},	// kSfxPlayPowerUp
	{"dmradio",		80,		0x10000,	0},	// kSfxPlayMessage
	{"",			80,		0x10000,	0},	// kSfxCultSpot
	{"",			80,		0x10000,	0},	// kSfxCultSpot2
	{"",			80,		0x10000,	0},	// kSfxCultSpot3
	{"",			80,		0x10000,	0},	// kSfxCultSpot4
	{"",			80,		0x10000,	0},	// kSfxCultSpot5
	{"",			80,		0x10000,	0},	// kSfxSCultRoam
	{"",			80,		0x10000,	0},	// kSfxSCultRoam2
	{"",			80,		0x10000,	0},	// kSfxSCultRoam3
	{"",			80,		0x10000,	0},	// kSfxSCultRoam4
	{"",			80,		0x10000,	0},	// kSfxCultPain
	{"",			80,		0x10000,	0},	// kSfxCultPain2
	{"",			80,		0x10000,	0},	// kSfxCultPain3
	{"",			80,		0x10000,	0},	// kSfxCultPain4
	{"",			80,		0x10000,	0},	// kSfxCultPain5
	{"",			80,		0x10000,	0},	// kSfxCultDie
	{"",			80,		0x10000,	0},	// kSfxCultDie2
	{"",			80,		0x10000,	0},	// kSfxCultDie3
	{"tnt_toss",	40,		0x08000,	0},	// kSfxCultToss
	{"",			80,		0x10000,	0},	// kSfxCultGloat
	{"",			80,		0x10000,	0},	// kSfxCultGloat2
	{"",			80,		0x10000,	0},	// kSfxCultGloat3
	{"",			80,		0x10000,	0},	// kSfxCultGloat4
	{"",			80,		0x10000,	0},	// kSfxCultGloat5
	{"",			80,		0x10000,	0},	// kSfxCultGloat6
	{"",			80,		0x10000,	0},	// kSfxCultGloat7
	{"",			80,		0x10000,	0},	// kSfxCultGloat8
	{"",			80,		0x10000,	0},	// kSfxCultGloat9
	{"",			80,		0x10000,	0},	// kSfxCultGloat10
	{"",			80,		0x10000,	0},	// kSfxSCultAttack
	{"",			80,		0x10000,	0},	// kSfxTCultAttack
	{"",			80,		0x10000,	0},	// kSfxAZombSpot
	{"",			80,		0x10000,	0},	// kSfxAZombRoam
	{"",			80,		0x10000,	0},	// kSfxAZombPain
	{"",			80,		0x10000,	0},	// kSfxAZombDie
	{"",			80,		0x10000,	0},	// kSfxAZombDie2
	{"",			80,		0x10000,	0},	// kSfxAZombDie3
	{"dmhack2",		60,		0x10000,	0},	// kSfxAZombAttack
	{"",			80,		0x10000,	0},	// kSfxAZombMorph
	{"",			80,		0x10000,	0},	// kSfxFZombSpot
	{"",			80,		0x10000,	0},	// kSfxFZombRoam
	{"",			80,		0x10000,	0},	// kSfxFZombPain
	{"",			80,		0x10000,	0},	// kSfxFZombDie
	{"",			80,		0x10000,	0},	// kSfxFZombDie2
	{"",			80,		0x10000,	0},	// kSfxFZombDie3
	{"",			80,		0x10000,	0},	// kSfxFZombAttack
	{"",			80,		0x10000,	0},	// kSfxHoundSpot
	{"",			80,		0x10000,	0},	// kSfxHoundRoam
	{"",			80,		0x10000,	0},	// kSfxHoundPain
	{"",			80,		0x10000,	0},	// kSfxHoundDie
	{"",			80,		0x10000,	0},	// kSfxHoundDie2
	{"",			80,		0x10000,	0},	// kSfxHoundAttack
	{"",			80,		0x10000,	0},	// kSfxHoundAttack2
	{"",			80,		0x10000,	0},	// kSfxGargSpot
	{"",			80,		0x10000,	0},	// kSfxGargRoam
	{"",			80,		0x10000,	0},	// kSfxGargPain
	{"",			80,		0x10000,	0},	// kSfxGargDie
	{"",			80,		0x10000,	0},	// kSfxGargDie2
	{"",			80,		0x10000,	0},	// kSfxGargDie3
	{"gargoyle",	80,		0x10000,	0},	// kSfxGargAttack
	{"",			80,		0x10000,	0},	// kSfxGargAttack2
	{"",			80,		0x10000,	0},	// kSfxGargAttack3
	{"",			80,		0x10000,	0},	// kSfxGargMorph
	{"",			80,		0x10000,	0},	// kSfxEelSpot
	{"",			80,		0x10000,	0},	// kSfxEelRoam
	{"",			80,		0x10000,	0},	// kSfxEelPain
	{"",			80,		0x10000,	0},	// kSfxEelDie
	{"",			80,		0x10000,	0},	// kSfxEelAttack
	{"",			80,		0x10000,	0},	// kSfxPhantasmSpot
	{"",			80,		0x10000,	0},	// kSfxPhantasmRoam
	{"",			80,		0x10000,	0},	// kSfxPhantasmPain
	{"",			80,		0x10000,	0},	// kSfxPhantasmDie
	{"",			80,		0x10000,	0},	// kSfxPhantasmAttack
	{"",			80,		0x10000,	0},	// kSfxPhantasmMorph
	{"",			80,		0x10000,	0},	// kSfxGillSpot
	{"",			80,		0x10000,	0},	// kSfxGillRoam
	{"",			80,		0x10000,	0},	// kSfxGillPain
	{"",			80,		0x10000,	0},	// kSfxGillDie
	{"",			80,		0x10000,	0},	// kSfxGillAttack
	{"",			80,		0x10000,	0},	// kSfxSpiderSpot
	{"",			80,		0x10000,	0},	// kSfxSpiderRoam
	{"",			80,		0x10000,	0},	// kSfxSpiderPain
	{"",			80,		0x10000,	0},	// kSfxSpiderDie
	{"",			80,		0x10000,	0},	// kSfxSpiderAttack
	{"",			80,		0x10000,	0},	// kSfxSpiderBirth
	{"",			80,		0x10000,	0},	// kSfxHandRoam
	{"",			80,		0x10000,	0},	// kSfxHandDie
	{"bat",			80,		0x10000,	0},	// kSfxBatRoam
	{"",			80,		0x10000,	0},	// kSfxBatDie
	{"",			80,		0x10000,	0},	// kSfxRatRoam
	{"",			80,		0x10000,	0},	// kSfxRatDie
	{"",			80,		0x10000,	0},	// kSfxRatAttack
	{"",			80,		0x10000,	0},	// kSfxPodOpen
	{"",			80,		0x10000,	0},	// kSfxPodAttack
	{"",			80,		0x10000,	0},	// kSfxPodDie
	{"",			80,		0x10000,	0},	// kSfxPodDie2
	{"",			80,		0x10000,	0},	// kSfxTentUp
	{"",			80,		0x10000,	0},	// kSfxTentAttack
	{"",			80,		0x10000,	0},	// kSfxTentDie
	{"",			80,		0x10000,	0},	// kSfxCerbSpot
	{"",			80,		0x10000,	0},	// kSfxCerbSpot2
	{"",			80,		0x10000,	0},	// kSfxCerbRoam
	{"",			80,		0x10000,	0},	// kSfxCerbPain
	{"",			80,		0x10000,	0},	// kSfxCerbPain2
	{"",			80,		0x10000,	0},	// kSfxCerbDie
	{"",			80,		0x10000,	0},	// kSfxCerbDie2
	{"",			80,		0x10000,	0},	// kSfxCerbAttack
	{"",			80,		0x10000,	0},	// kSfxCerbAttack2
	{"",			80,		0x10000,	0},	// kSfxBossSpot
	{"",			80,		0x10000,	0},	// kSfxBossRoam
	{"",			80,		0x10000,	0},	// kSfxBossPain
	{"",			80,		0x10000,	0},	// kSfxBossDie
	{"",			80,		0x10000,	0}	// kSfxBossAttack
};

struct SAMPLE3D
{
	int hVoice;
	BYTE *pRaw;
};

struct VISBY
{
	SAMPLE3D left, right;
	int id;
	int x, y, z;
};

VISBY Visby[kMaxXSprites];


struct BONKLE
{
	SAMPLE3D left, right;
	int x, y, z;
};

#define kMaxBonkles	64
BONKLE Bonkle[kMaxBonkles];


void sfxInit( void )
{
	for (int i = 0; i < kSfxMax; i++)
	{
		if ( (soundEffect[i].hResource = gSysRes.Lookup(soundEffect[i].name, ".RAW")) == NULL )
		{
			dprintf("Missing sound resource: %s.RAW\n", soundEffect[i].name);
		}
	}
}


void sfxTerm( void )
{
}


void sfxKillAllSounds( void )
{
}


/*******************************************************************************

Here is the equation for determining the frequency shift of a sound:

	            Ú        ¿
	            ³ v + vL ³
	    fL = fS ³ ÄÄÄÄÄÄ ³
	            ³ v + vS ³
	            À        Ù

Where:

	fL	= frequency as heard by the listener
	fS	= frequency of the source
	v	= velocity of sound
	vL	= velocity of listener
	vS	= velocity of source

Velocities are measured on a vector from the listener to the source.

Speed of sound at STP = 343 m/s = 1126 ft/s.

*******************************************************************************/

/*
#define kSoundVelocity	((double)kSampleRate / kTimerRate)
int Freq( int velocity )
{
	return kSampleRate * kSoundVelocity / (ClipLow(kSoundVelocity - velocity, 1));
}
*/


inline int FreqShift( int nPitch, int vL )
{
	return muldiv(nPitch, kSoundVelFrame + vL, kSoundVelFrame);
}


static int Vol3d(int nAngle, int vol)
{
	return vol - mulscale16(vol, kBackFilter / 2 - mulscale30(kBackFilter / 2, Cos(nAngle)));
}


void Calc3DValues( int x, int y, int z, int relVol )
{
	int dx, dy, dz, dist, nAngle;

	dx = x - gMe->sprite->x;
	dy = y - gMe->sprite->y;
	dz = z - gMe->sprite->z;
	nAngle = getangle(dx, dy);
	dist = ClipLow(Dist3d(dx, dy, dz), 32);
	int monoVol = muldiv(relVol, kVolScale, dist);

	// normal vector for listener -> source
	dx = Cos(nAngle);
	dy = Sin(nAngle);

	int lDist = qdist(x - earL.x, y - earL.y);
	lVol = Vol3d(nAngle - (gMe->sprite->ang - kPinnaAngle), monoVol);
	lPhase = mulscale16r(lDist, kDistPhase);
	lVel = dmulscale30r(dx, earVL.dx, dy, earVL.dy);

	int rDist = qdist(x - earR.x, y - earR.y);
	rVol = Vol3d(nAngle - (gMe->sprite->ang + kPinnaAngle), monoVol);
	rPhase = mulscale16r(rDist, kDistPhase);
	rVel = dmulscale30r(dx, earVR.dx, dy, earVR.dy);
}

/*
void UpdateSound( void )
{
	if (lVoice > FX_Ok)
	{
		FX_SetPan(lVoice, lVol, lVol, 0);
		FX_SetFrequency(lVoice, Freq(lVel));
	}

	if (rVoice > FX_Ok)
	{
		FX_SetPan(rVoice, rVol, 0, rVol);
		FX_SetFrequency(rVoice, Freq(rVel));
	}
}
*/


void sfxStart3DSound( int nXSprite, int soundId, int nPitchOffset )
{
	if ( gFXDevice == -1 )
		return;

	if ( soundId < 0 || soundId >= kSfxMax )
		ThrowError("Invalid sound ID", ES_ERROR);

	SOUNDEFFECT *pSoundEffect = &soundEffect[soundId];

	dassert(nXSprite > 0 && nXSprite < kMaxXSprites);
	VISBY *pVisby = &Visby[nXSprite];

	// don't restart an actor sound if its already playing
	if ( (pVisby->left.hVoice > 0 | pVisby->right.hVoice > 0 ) && pVisby->id == soundId )
		return;

	pVisby->id = soundId;

	// if there is an active voice, kill the sound
	if ( pVisby->left.hVoice > 0 )
	{
		FX_StopSound(pVisby->left.hVoice);
		Resource::Free(pVisby->left.pRaw);
		pVisby->left.pRaw = NULL;
	}

	if ( pVisby->right.hVoice > 0 )
	{
		FX_StopSound(pVisby->right.hVoice);
		Resource::Free(pVisby->right.pRaw);
		pVisby->right.pRaw = NULL;
	}

	SPRITE *pSprite = &sprite[xsprite[nXSprite].reference];
	Calc3DValues(pSprite->x, pSprite->y, pSprite->z, pSoundEffect->relVol);

	RESHANDLE hResource = pSoundEffect->hResource;
	if ( hResource == NULL )
	{
		dprintf("sfxStart3DSound() called for missing sound #%d\n", soundId);
		return;
	}

	int sampleSize = gSysRes.Size(hResource);
	char *pRaw = (char *)gSysRes.Lock(hResource);

	int nPitch = kSampleRate;
	if ( nPitchOffset )
		nPitch += mulscale16(BiRandom(nPitchOffset), kSampleRate);

	pVisby->left.pRaw = (BYTE *)Resource::Alloc(sampleSize + lPhase);
	memset(pVisby->left.pRaw, 128, lPhase);
	memcpy(&pVisby->left.pRaw[lPhase], pRaw, sampleSize);

	pVisby->right.pRaw = (BYTE *)Resource::Alloc(sampleSize + rPhase);
	memset(pVisby->right.pRaw, 128, rPhase);
	memcpy(&pVisby->right.pRaw[rPhase], pRaw, sampleSize);

	gSysRes.Unlock(hResource);

	int nPriority = 1;	// ASS seems to choke if this is less than 1, e.g., 0
	if ( lVol > nPriority )
		nPriority = lVol;
	if ( rVol > nPriority )
		nPriority = rVol;

	_disable();	// make sure the samples start in the same interval

	pVisby->left.hVoice = FX_PlayRaw(
		pVisby->left.pRaw, sampleSize + lPhase, FreqShift(nPitch, lVel), 0,
		lVol, lVol, 0, nPriority, (ulong)(&pVisby->left.hVoice));

	pVisby->right.hVoice = FX_PlayRaw(
		pVisby->right.pRaw, sampleSize + rPhase, FreqShift(nPitch, rVel), 0,
		rVol, 0, rVol, nPriority, (ulong)(&pVisby->right.hVoice));

	_enable();	// allow them to start
}


void sfxCreate3DSound( int x, int y, int z, int soundId, int nPitchOffset )
{
	if ( gFXDevice == -1 )
		return;

	if ( soundId < 0 || soundId >= kSfxMax )
		ThrowError("Invalid sound ID", ES_ERROR);

	SOUNDEFFECT *pSoundEffect = &soundEffect[soundId];

	Calc3DValues(x, y, z, pSoundEffect->relVol);

	RESHANDLE hResource = pSoundEffect->hResource;
	if ( hResource == NULL )
	{
		dprintf("sfxCreate3DSound() called for missing sound #%d\n", soundId);
		return;
	}


	int sampleSize = gSysRes.Size(hResource);
	char *pRaw = (char *)gSysRes.Lock(hResource);

	int nPitch = kSampleRate;
	if ( nPitchOffset )
		nPitch += mulscale16(BiRandom(nPitchOffset), kSampleRate);

	for (int i = 0; i < kMaxBonkles; i++)
		if ( Bonkle[i].left.pRaw == NULL && Bonkle[i].right.pRaw == NULL )
			break;
	dassert(i < kMaxBonkles);
	BONKLE *pBonkle = &Bonkle[i];

	pBonkle->left.pRaw = (BYTE *)Resource::Alloc(sampleSize + lPhase);
	memset(pBonkle->left.pRaw, 128, lPhase);
	memcpy(&pBonkle->left.pRaw[lPhase], pRaw, sampleSize);

	pBonkle->right.pRaw = (BYTE *)Resource::Alloc(sampleSize + rPhase);
	memset(pBonkle->right.pRaw, 128, rPhase);
	memcpy(&pBonkle->right.pRaw[rPhase], pRaw, sampleSize);

	gSysRes.Unlock(hResource);

	int nPriority = 1;	// ASS seems to choke if this is less than 1, e.g., 0
	if ( lVol > nPriority )
		nPriority = lVol;
	if ( rVol > nPriority )
		nPriority = rVol;

	_disable();	// make sure the samples start in the same interval

	pBonkle->left.hVoice = FX_PlayRaw(
		pBonkle->left.pRaw, sampleSize + lPhase, FreqShift(nPitch, lVel), 0,
		lVol, lVol, 0, nPriority, (ulong)(&pBonkle->left.hVoice));

	pBonkle->right.hVoice = FX_PlayRaw(
		pBonkle->right.pRaw, sampleSize + rPhase, FreqShift(nPitch, rVel), 0,
		rVol, 0, rVol, nPriority, (ulong)(&pBonkle->right.hVoice));

	_enable();	// allow them to start
}


void sfxKill3DSound( int nXSprite )
{
	if ( gFXDevice == -1 )
		return;

	dassert(nXSprite > 0 && nXSprite < kMaxXSprites);
	VISBY *pVisby = &Visby[nXSprite];

	// if there is an active voice, kill the sound
	if ( pVisby->left.hVoice > 0 )
		FX_StopSound(pVisby->left.hVoice);
	if ( pVisby->right.hVoice > 0 )
		FX_StopSound(pVisby->right.hVoice);
}

/*
void sfxKillAll3DSounds( void )
{
}
*/

// ear distance (in samples)
#define kEarLX (int)(-kIntraAuralTime / 2 * kSampleRate)
#define kEarRX (int)(+kIntraAuralTime / 2 * kSampleRate)

void sfxUpdate3DSounds( void )
{
	// update listener ear positions
	int earDX, earDY;

	earDX = mulscale30(Cos(gMe->sprite->ang + kAngle90), kIntraAuralDist);
	earDY = mulscale30(Sin(gMe->sprite->ang + kAngle90), kIntraAuralDist);

	earL0 = earL;
	earL.x = gMe->sprite->x - earDX;
	earL.y = gMe->sprite->y - earDY;
	earR0 = earR;
	earR.x = gMe->sprite->x + earDX;
	earR.y = gMe->sprite->y + earDY;

	// update ear velocities
	earVL.dx = earL.x - earL0.x;
	earVL.dy = earL.y - earL0.y;

	earVR.dx = earR.x - earR0.x;
	earVR.dy = earR.y - earR0.y;

	int i;

	for (i = 0; i < kMaxXSprites; i++)
	{
		if ( Visby[i].left.pRaw != NULL && Visby[i].left.hVoice <= 0 )
		{
			Resource::Free(Visby[i].left.pRaw);
			Visby[i].left.pRaw = NULL;
		}
		if ( Visby[i].right.pRaw != NULL && Visby[i].right.hVoice <= 0 )
		{
			Resource::Free(Visby[i].right.pRaw);
			Visby[i].right.pRaw = NULL;
		}
	}

	for (i = 0; i < kMaxBonkles; i++)
	{
		if (Bonkle[i].left.hVoice <= 0 && Bonkle[i].left.pRaw != NULL )
		{
			Resource::Free(Bonkle[i].left.pRaw);
			Bonkle[i].left.pRaw = NULL;
		}
		if (Bonkle[i].right.hVoice <= 0 && Bonkle[i].right.pRaw != NULL )
		{
			Resource::Free(Bonkle[i].right.pRaw);
			Bonkle[i].right.pRaw = NULL;
		}
	}
}
