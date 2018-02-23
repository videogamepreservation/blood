/*******************************************************************************
	FILE:			SFX.H

	DESCRIPTION:	Sound Effects API

	AUTHOR:			Peter M. Freese
	CREATED:		11-07-95
	COPYRIGHT:		Copyright (c) 1995 Q Studios Corporation
*******************************************************************************/
#ifndef __SFX_H
#define __SFX_H
#include "typedefs.h"

enum
{
// WEAPON SOUNDS
	// lighter
	kSfxZipOpen = 0,
	kSfxZipClose,
	kSfxZipFlick,
	// sawed-off shotgun
	kSfxShotCock,
	kSfxShotFire,
	kSfxShotFire2,
	kSfxShotLoad,
	// flare gun
	kSfxFlareFire,
	kSfxFlareFireUW,
	// tommy gun
	kSfxTomCock,
	kSfxTomFire,
	// spray can
	kSfxSprayPaint,
	kSfxSprayFlame,
	// tnt
	kSfxTNTFuse,
	kSfxTNTDropProx,
	kSfxTNTArmProx,
	kSfxTNTDetProx,
	kSfxTNTDropRemote,
	kSfxTNTArmRemote,
	kSfxTNTDetRemote,
	kSfxTNTToss,
	// voodoo doll
	kSfxVoodooHit,
	kSfxVoodooHit2,
	kSfxVoodooHit3,
	kSfxVoodooHit4,
	kSfxVoodooChant,
	kSfxVoodooBreak,
	kSfxVoodooBurn,
	// spear gun
	kSfxSpearLoad,
	kSfxSpearFire,
	kSfxSpearFireUW,
	// ecto/shadowblaster
	kSfxBlasterFire,
	// pitchfork
	kSfxForkStone,
	kSfxForkMetal,
	kSfxForkWood,
	kSfxForkFlesh,
	kSfxForkWater,
	kSfxForkDirt,
	kSfxForkClay,
	kSfxForkSnow,
	kSfxForkIce,
	kSfxForkLeaves,
	kSfxForkPlant,
	kSfxForkGoo,
	kSfxForkLava,
	kSfxForkStoneUW,
	kSfxForkMetalUW,
	kSfxForkWoodUW,
	kSfxForkFleshUW,
	kSfxForkDirtUW,
	kSfxForkClayUW,
	kSfxForkIceUW,
	kSfxForkLeavesUW,
	kSfxForkPlantUW,
	// bullet ricochets
	kSfxBulletStone,
	kSfxBulletMetal,
	kSfxBulletWood,
	kSfxBulletFlesh,
	kSfxBulletWater,
	kSfxBulletDirt,
	kSfxBulletClay,
	kSfxBulletSnow,
	kSfxBulletIce,
	kSfxBulletLeaves,
	kSfxBulletPlant,
	kSfxBulletGoo,
	kSfxBulletLava,
	kSfxBulletStoneUW,
	kSfxBulletMetalUW,
	kSfxBulletWoodUW,
	kSfxBulletFleshUW,
	kSfxBulletDirtUW,
	kSfxBulletClayUW,
	kSfxBulletIceUW,
	kSfxBulletLeavesUW,
	kSfxBulletPlantUW,
	// axe ricochets
	kSfxAxeAir,
	kSfxAxeStone,
	kSfxAxeMetal,
	kSfxAxeWood,
	kSfxAxeFlesh,
	kSfxAxeWater,
	kSfxAxeDirt,
	kSfxAxeClay,
	kSfxAxeSnow,
	kSfxAxeIce,
	kSfxAxeLeaves,
	kSfxAxePlant,
	kSfxAxeGoo,
	kSfxAxeLava,
	kSfxAxeStoneUW,
	kSfxAxeMetalUW,
	kSfxAxeWoodUW,
	kSfxAxeFleshUW,
	kSfxAxeDirtUW,
	kSfxAxeClayUW,
	kSfxAxeIceUW,
	kSfxAxeLeavesUW,
	kSfxAxePlantUW,
	// claw ricochets
	kSfxClawStone,
	kSfxClawMetal,
	kSfxClawWood,
	kSfxClawFlesh,
	kSfxClawWater,
	kSfxClawDirt,
	kSfxClawClay,
	kSfxClawSnow,
	kSfxClawIce,
	kSfxClawLeaves,
	kSfxClawPlant,
	kSfxClawGoo,
	kSfxClawLava,
	kSfxClawStoneUW,
	kSfxClawMetalUW,
	kSfxClawWoodUW,
	kSfxClawFleshUW,
	kSfxClawDirtUW,
	kSfxClawClayUW,
	kSfxClawIceUW,
	kSfxClawLeavesUW,
	kSfxClawPlantUW,
// FOOTFALL SOUNDS
	kSfxFootStone1,
	kSfxFootStone2,
	kSfxFootMetal1,
	kSfxFootMetal2,
	kSfxFootWood1,
	kSfxFootWood2,
	kSfxFootFlesh1,
	kSfxFootFlesh2,
	kSfxFootWater1,
	kSfxFootWater2,
	kSfxFootDirt1,
	kSfxFootDirt2,
	kSfxFootClay1,
	kSfxFootClay2,
	kSfxFootSnow1,
	kSfxFootSnow2,
	kSfxFootIce1,
	kSfxFootIce2,
	kSfxFootLeaves,
	kSfxFootCloth1,
	kSfxFootCloth2,
	kSfxFootPlant1,
	kSfxFootPlant2,
	kSfxFootGoo1,
	kSfxFootGoo2,
	kSfxFootLava1,
	kSfxFootLava2,
	kSfxDudeLandStone,
	kSfxDudeLandMetal,
	kSfxDudeLandWood,
	kSfxDudeLandFlesh,
	kSfxDudeLandWater,
	kSfxDudeLandDirt,
	kSfxDudeLandClay,
	kSfxDudeLandSnow,
	kSfxDudeLandIce,
	kSfxDudeLandLeaves,
	kSfxDudeLandCloth,
	kSfxDudeLandPlant,
	kSfxDudeLandGoo,
	kSfxDudeLandLava,
	kSfxBodyLandStone,
	kSfxBodyLandMetal,
	kSfxBodyLandWood,
	kSfxBodyLandFlesh,
	kSfxBodyLandWater,
	kSfxBodyLandDirt,
	kSfxBodyLandClay,
	kSfxBodyLandSnow,
	kSfxBodyLandIce,
	kSfxBodyLandLeaves,
	kSfxBodyLandCloth,
	kSfxBodyLandPlant,
	kSfxBodyLandGoo,
	kSfxBodyLandLava,
	kSfxGibLandStone,
	kSfxGibLandMetal,
	kSfxGibLandWood,
	kSfxGibLandFlesh,
	kSfxGibLandWater,
	kSfxGibLandDirt,
	kSfxGibLandClay,
	kSfxGibLandSnow,
	kSfxGibLandIce,
	kSfxGibLandLeaves,
	kSfxGibLandCloth,
	kSfxGibLandPlant,
	kSfxGibLandGoo,
	kSfxGibLandLava,
	kSfxSplashS,
	kSfxSplashM,
	kSfxSplashL,
// HAZARD SOUNDS
	kSfxArc1,
	kSfxArc2,
	kSfxArc3,
	kSfxBladeDrop,
	kSfxBladeSwipe,
	kSfxBoulders,
	kSfxExplodeCS,
	kSfxExplodeCM,
	kSfxExplodeCL,
	kSfxExplodeFS,
	kSfxExplodeFM,
	kSfxExplodeFL,
	kSfxExplode1UW,
	kSfxExplode2UW,
	kSfxExplode3UW,
	kSfxFireballPrefire,
	kSfxFireballFire,
	kSfxMGFire,
	kSfxMGDie,
	kSfxSawStart,
	kSfxSawRun,
	kSfxSawStop,
	kSfxSawCut,
// AMBIENT SOUNDS
	kSfxAmbientCave,
	kSfxAmbientFauna,
	kSfxAmbientMechanical,
	kSfxAmbientRain,
	kSfxAmbientUW,
	kSfxAmbientWind,
	kSfxAmbientMansion,
	kSfxAmbientCastle,
	kSfxAmbientRainIndoors,
	kSfxAmbientCreepy1,
	kSfxAmbientCreepy2,
	kSfxAmbientCreepy3,
// SUB-AMBIENT SOUNDS
	kSfxBubbles,
	kSfxCrickets,
	kSfxDrip1,
	kSfxDrip2,
	kSfxDrip3,
	kSfxFrog,
	kSfxLeaves,
	kSfxOwl,
	kSfxRaven,
	kSfxRespawn,
	kSfxSewage,
	kSfxThunder,
	kSfxThunder2,
	kSfxTrees,
	kSfxWaterLap,
	kSfxWaterStream,
// MECHANICAL SOUNDS
	kSfxBurn,
	kSfxSizzle,
	kSfxTickTock,
	kSfxGust1,
	kSfxGust2,
	kSfxGust3,
	kSfxGust4,
	kSfxWhisper,
	kSfxLaugh,
	kSfxChains,
	kSfxMoan,
	kSfxSigh,
	kSfxCreak,
	kSfxSqueak,
	kSfxScuffle,
	kSfxRumble,
	kSfxScream,
	kSfxDoorCreak,
	kSfxDoorSlide,
	kSfxFloorCreak,
	kSfxGearStart,
	kSfxGearMove,
	kSfxGearStop,
	kSfxGlassHit,
	kSfxLiftStart,
	kSfxLiftMove,
	kSfxLiftStop,
	kSfxPadlock,
	kSfxPotteryHit,
	kSfxSlabMove,
	kSfxSplat,
	kSfxSwingOpen,
	kSfxSwingShut,
	kSfxSwitch1,
	kSfxSwitch2,
	kSfxSwitch3,
	kSfxSwitch4,
	kSfxWoodBreak,
// PLAYER SOUNDS
	kSfxPlayJump,
	kSfxPlayLand,
	kSfxPlayHunt,
	kSfxPlaySwim,
	kSfxPlaySwimUW,
	kSfxPlayBreathe1,
	kSfxPlayBreathe2,
	kSfxPlayPain,
	kSfxPlayDie,
	kSfxPlayDie2,
	kSfxPlayDie3,
	kSfxPlayFall,
	kSfxPlayChoke,
	kSfxPlayChoke2,
	kSfxPlayGasp,
	kSfxPlayHotFoot,
	kSfxPlayLaugh,
	kSfxPlaySubmerge,
	kSfxPlayEmerge,
	kSfxPlayItemUp,
	kSfxPlayPowerUp,
	kSfxPlayMessage,
// MONSTER SOUNDS
	// cultist sounds
	kSfxCultSpot,
	kSfxCultSpot2,
	kSfxCultSpot3,
	kSfxCultSpot4,
	kSfxCultSpot5,
	kSfxSCultRoam,
	kSfxSCultRoam2,
	kSfxSCultRoam3,
	kSfxSCultRoam4,
	kSfxCultPain,
	kSfxCultPain2,
	kSfxCultPain3,
	kSfxCultPain4,
	kSfxCultPain5,
	kSfxCultDie,
	kSfxCultDie2,
	kSfxCultDie3,
	kSfxCultToss,
	kSfxCultGloat,
	kSfxCultGloat2,
	kSfxCultGloat3,
	kSfxCultGloat4,
	kSfxCultGloat5,
	kSfxCultGloat6,
	kSfxCultGloat7,
	kSfxCultGloat8,
	kSfxCultGloat9,
	kSfxCultGloat10,
	kSfxSCultAttack,
	kSfxTCultAttack,
	// axe zombie sounds
	kSfxAZombSpot,
	kSfxAZombRoam,
	kSfxAZombPain,
	kSfxAZombDie,
	kSfxAZombDie2,
	kSfxAZombDie3,
	kSfxAZombAttack,
	kSfxAZombMorph,
	// fat zombie sounds
	kSfxFZombSpot,
	kSfxFZombRoam,
	kSfxFZombPain,
	kSfxFZombDie,
	kSfxFZombDie2,
	kSfxFZombDie3,
	kSfxFZombAttack,
	// hell hound sounds
	kSfxHoundSpot,
	kSfxHoundRoam,
	kSfxHoundPain,
	kSfxHoundDie,
	kSfxHoundDie2,
	kSfxHoundAttack,
	kSfxHoundAttack2,
	// gargoyle sounds
	kSfxGargSpot,
	kSfxGargRoam,
	kSfxGargPain,
	kSfxGargDie,
	kSfxGargDie2,
	kSfxGargDie3,
	kSfxGargAttack,
	kSfxGargAttack2,
	kSfxGargAttack3,
	kSfxGargMorph,
	// eel sounds
	kSfxEelSpot,
	kSfxEelRoam,
	kSfxEelPain,
	kSfxEelDie,
	kSfxEelAttack,
	// phantasm sounds
	kSfxPhantasmSpot,
	kSfxPhantasmRoam,
	kSfxPhantasmPain,
	kSfxPhantasmDie,
	kSfxPhantasmAttack,
	kSfxPhantasmMorph,
	// gill beast sounds
	kSfxGillSpot,
	kSfxGillRoam,
	kSfxGillPain,
	kSfxGillDie,
	kSfxGillAttack,
	// spider sounds
	kSfxSpiderSpot,
	kSfxSpiderRoam,
	kSfxSpiderPain,
	kSfxSpiderDie,
	kSfxSpiderAttack,
	kSfxSpiderBirth,
	// hell hand sounds
	kSfxHandRoam,
	kSfxHandDie,
	// bat sounds
	kSfxBatRoam,
	kSfxBatDie,
	// rat sounds
	kSfxRatRoam,
	kSfxRatDie,
	kSfxRatAttack,
	// pod sounds
	kSfxPodOpen,
	kSfxPodAttack,
	kSfxPodDie,
	kSfxPodDie2,
	// pod tentacle sounds
	kSfxTentUp,
	kSfxTentAttack,
	kSfxTentDie,
	// cerberus sounds
	kSfxCerbSpot,
	kSfxCerbSpot2,
	kSfxCerbRoam,
	kSfxCerbPain,
	kSfxCerbPain2,
	kSfxCerbDie,
	kSfxCerbDie2,
	kSfxCerbAttack,
	kSfxCerbAttack2,
	// tchernobog sounds
	kSfxBossSpot,
	kSfxBossRoam,
	kSfxBossPain,
	kSfxBossDie,
	kSfxBossAttack,
	kSfxMax
};


/*******************************************************************************
Sound definition file format:

SoundNN="NAME.RAW", volume [, startloop, endloop ]

Each sound has a relative volume for it in the definition file.  This allows
all samples to be recorded as hot as possible.

The startloop and endloop values are optional, and apply only to sounds which
are to be looped.  Unless we determine a way to dynamically stop a sample from
looping and cause it to continue playing all the way through, the endloop
position should be the end of the sample.  Currently, ASL does not support this
feature.

*******************************************************************************/


/***********************************************************************
 * Function prototypes
 **********************************************************************/
extern void sfxInit( void );
/* Initialize the sound effect system.  Read user settings (volume, mix
channels, mix rate, etc.), initialize low level driver, and load sound mapping
from definition file. */

extern void sfxTerm( void );
/* Shut down the sound system.  Stops all sounds and deinitializes driver. */

extern void sfxStart3DSound( int nXSprite, int soundId, int nPitchOffset = 0 );
/* Starts a sound playing for a given XSprite.  The sound will supercede any
sound already playing for the XSprite. */

extern void sfxCreate3DSound( int x, int y, int z, int soundId, int nPitch = 0 );
/* Create and start a 3D sound effect using a temporary structure. */

extern void sfxKill3DSound( int nXSprite );
/* Stops any sound playing for the specified XSprite.  Use this before warping
a sprite so it's sound doesn't get hosed from moving a large distance in a
short time period.*/

extern void sfxKillAll3DSounds( void );
/* Stops all 3D sound effects from playing. This might be used when ending a
level. */

extern void sfxUpdate3DSounds( void );
/* Update the 3D location of all 3D sound effects.  The warp flag indicates
whether the listener location actually moved or reappeared, and should be set
when teleporting and restarting the player. */

#endif //__SFX_H
