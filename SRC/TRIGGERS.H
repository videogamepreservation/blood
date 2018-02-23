#ifndef __TRIGGERS_H
#define __TRIGGERS_H

#include "eventq.h"
#include "db.h"

/***********************************************************************
 *
 * Enumerated Types
 *
 **********************************************************************/


/***********************************************************************
 *
 * Function Prototypes
 *
 **********************************************************************/
void trTriggerSector( unsigned nSector, XSECTOR *pXSector, int command );
void trTriggerWall( unsigned nWall, XWALL *pXWall, int command );
void trTriggerSprite( unsigned nSprite, XSPRITE *pXSprite, int command );

void trMessageSector( unsigned nSector, EVENT event );
void trMessageWall( unsigned nWall, EVENT event );
void trMessageSprite( unsigned nSprite, EVENT event );

void trTextOver( int textId );
void trLightning( int forkID );

void trProcessBusy( void );
void trInit( void );

#endif
