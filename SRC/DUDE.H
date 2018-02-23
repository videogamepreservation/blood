#ifndef __DUDE_H
#define __DUDE_H

#include "actor.h"
#include "seq.h"

#define kGibMax		8

enum {
	kSeqDudeIdle	= 0,
	kSeqDudeDeath1,	// normal death
	kSeqDudeDeath2,	// exploding death
	kSeqDudeDeath3,	// burning death
	kSeqDudeRecoil,
	kSeqDudeCustomStart,
	kSeqDudeMax		= 16,

	kSeqCultistAttack1 = kSeqDudeCustomStart,
	kSeqCultistAttack2,
	kSeqCultistWalk,
	kSeqCultistJump1,
	kSeqCultistJump2,
	kSeqCultistJump3,

	kSeqAxeZombieAttack = kSeqDudeCustomStart,
	kSeqAxeZombieDeath4,
	kSeqAxeZombieWalk,
	kSeqEarthZombieUp,
	kSeqAxeZombieUp,
	kSeqEarthZombieIdle,

	kSeqFatZombieAttack	= kSeqDudeCustomStart,
	kSeqFatZombieDeath4,
	kSeqFatZombieWalk,

	kSeqGargoyleAttack	= kSeqDudeCustomStart,
	kSeqGargoyleFly,
	kSeqGargoyleSwoop,

	kSeqStoneStatueTurn	= kSeqDudeCustomStart,

	kSeqFleshStatueTurn	= kSeqDudeCustomStart,

	kSeqPhantasmAttack1	= kSeqDudeCustomStart,
	kSeqPhantasmMove1,
	kSeqPhantasmRecoil2,

	kSeqBatFly			= kSeqDudeCustomStart,

	kSeqRatWalk		= kSeqDudeCustomStart,

	kSeqSpiderBite	= kSeqDudeCustomStart,
	kSeqSpiderWalk,
	kSeqSpiderJump,

	kSeqGillAttack1		= kSeqDudeCustomStart,
	kSeqGillAttack2,
	kSeqGillDeath4,
	kSeqGillMove1,
	kSeqGillMove2,
	kSeqGillRecoil2,

	kSeqHoundBite		= kSeqDudeCustomStart,
	kSeqHoundBurn,
	kSeqHoundWalk,

	kSeqEelMove1		= kSeqDudeCustomStart,

	kSeqCerberusAttack1	= kSeqDudeCustomStart,
	kSeqCerberusMove1,

	kSeqCerberus2Attack1	= kSeqDudeCustomStart,
	kSeqCerberus2Move1,

	kSeqBeastAttack1	= kSeqDudeCustomStart,
	kSeqBeastAttack2,	// no sequence yet!
	kSeqBeastMove1,

	kSeqHandWalk		= kSeqDudeCustomStart,
	kSeqHandJump,
};


struct GIB
{
	ushort	chance;		// likelihood (out of 256)
	ushort	tile;		// tile number
};


struct DUDEINFO
{
	short	seqStartID;
	short	startHealth;
	ushort	mass;			// in KG
	char	clipdist;
	int 	eyeHeight;		// (in pixels?) relative to origin
	int		hearDist;		// how far it can hear (?)
	int		seeDist;		// how far it can see
	int		periphery;		// peripheral vision in angle units
	int		meleeDist;		// how close it needs to be to melee attack
	int		fleeHealth;
	int		hinderDamage;	// amount of damage necessary to "hinder"
	int		changeTarget;	// probably of changing target per hit point (1/2^16)
	int		changeTargetKin;// probably of changing target to same species
	int		alertChance;	// chance of noticing player and activating
	int		frontAccel;
	int		sideAccel;
	int		backAccel;
	int		frontSpeed;
	int		sideSpeed;
	int		backSpeed;
	int		angSpeed;
	GIB		gib[kGibMax];
	int		damageShift[kDamageMax];	// use to indicate resistance to damage types

	// used internally -- do not initialize
//	Seq		*pSeq[kSeqDudeMax];
};

extern DUDEINFO dudeInfo[];

extern DUDEINFO gPlayerTemplate[2];	// player human/beast templates

#endif
