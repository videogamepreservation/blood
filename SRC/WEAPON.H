#ifndef __WEAPON_H
#define __WEAPON_H

#include "player.h"

void WeaponInit( void );
void WeaponDraw( PLAYER *pPlayer, int shade, int x, int y );
void WeaponProcess( PLAYER *pPlayer, INPUT *pInput );

#endif
