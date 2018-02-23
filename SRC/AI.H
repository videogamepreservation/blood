#ifndef __AI_H
#define __AI_H

#include "actor.h"
#include "db.h"
#include "seq.h"

extern int gDudeSlope[kMaxXSprites];

/*
// Movement states
enum {
	kMoveDead = 0,
	kMoveStill,
	kMoveWalk,
	kMoveFall,
	kMoveLand,
	kMoveStand,
	kMoveSwim,
	kMoveFly,
	kMoveHang,
};

	// this stuff needed for dudes
	unsigned moveState :		8;	// same as player move states
	unsigned aiState :			8;
	unsigned health :			12;
	unsigned dudeDeaf :			1;
	unsigned dudeAmbush :		1;
	unsigned dudeGuard :		1;
	unsigned dudeFlag4 :		1;
	signed target :				16;
	signed targetX :			32;
	signed targetY :			32;
	signed targetZ :			32;
	signed avel :				16;
	unsigned weaponTimer :		16;
	unsigned stateTimer :		16;
*/


/** Monster AI States **/
enum {
	kAIScan		= 0,	// wait to be activated
	kAIGoto		= 1,	// move to target x,y,z
	kAIFollow	= 2,	// move to target sprite
	kAIFlee		= 3,	// move away from target sprite
	kAIMorph	= 4,	// transforming (i.e., statue to gargoyle)
	kAIRecoil	= 5,
	kAISearch	= 6,
	kAIAttack	= 7,
};

typedef void (*AISTATEFUNC)( SPRITE *sprite, XSPRITE *xsprite);


struct AISTATE
{
	int				seqId;
	SEQCALLBACK		seqCallback;
	int				ticks;
	AISTATEFUNC		enter;
	AISTATEFUNC		move;
	AISTATEFUNC		think;
	AISTATE			*next;
};


extern void aiInit( void );
extern void aiProcessDudes( void );

extern void aiActivateDude( SPRITE *pSprite, XSPRITE *pXSprite );
extern void aiMoveForward( SPRITE *pSprite, XSPRITE *pXSprite );
extern void aiMoveTurn( SPRITE *pSprite, XSPRITE *pXSprite );
extern void aiMoveDodge( SPRITE *pSprite, XSPRITE *pXSprite );
extern void aiThinkTarget( SPRITE *pSprite, XSPRITE *pXSprite );
extern void aiChooseDirection( SPRITE *pSprite, XSPRITE *pXSprite, int ang );
extern void aiNewState( SPRITE *pSprite, XSPRITE *pXSprite, AISTATE *pState );
extern void aiSetTarget( XSPRITE *pXSprite, int x, int y, int z );
extern void aiSetTarget( XSPRITE *pXSprite, int nTarget );
extern void aiDamageSprite( SPRITE *pSprite, XSPRITE *pXSprite, int nSource, DAMAGE_TYPE nDamageType, int nDamage );


#endif


