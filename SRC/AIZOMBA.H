#ifndef __AIZOMBA_H
#define __AIZOMBA_H

#include "ai.h"
#include "player.h"


extern AISTATE zombieAIdle;
extern AISTATE zombieAChase;
extern AISTATE zombieAGoto;
extern AISTATE zombieAHack;
extern AISTATE zombieASearch;
extern AISTATE zombieARecoil;

extern AISTATE zombieEIdle;
extern AISTATE zombieEUp2;
extern AISTATE zombieEUp;

void aiAlertLackey( PLAYER *pPlayer, int nLackey, int nSource, int nDamage );


#endif	// __AIZOMBA_H
