#ifndef __EVENTQ_H
#define __EVENTQ_H

#include <stdlib.h>
#include "typedefs.h"

// command types
enum {
	kCommandOff			= 0,
	kCommandOn			= 1,
	kCommandState		= 2,
	kCommandToggle		= 3,
	kCommandNotState	= 4,
	kCommandLink		= 5,
	kCommandLock		= 6,
	kCommandUnlock		= 7,
	kCommandToggleLock	= 8,

	kCommandCallback,
	kCommandRespawn,

	kCommandSpritePush		= 16,
	kCommandSpriteImpact,
	kCommandSpritePickup,
	kCommandSpriteTouch,
	kCommandSpriteSight,
	kCommandSpriteProximity,
	kCommandSpriteExplode,

	kCommandSectorPush      = 32,
	kCommandSectorImpact,
	kCommandSectorEnter,
	kCommandSectorExit,

	kCommandWallPush        = 48,
	kCommandWallImpact,
	kCommandWallCross,

	kCommandNumbered		= 64,
};

// txId constants
enum {
	kChannelNull			= 0,

	kChannelTextOver 		= 3,

	kChannelEndLevelA		= 4,
	kChannelEndLevelB		= 5,

	kChannelLightning		= 6,

	kChannelTriggerStart	= 7,	// channel triggered at startup
	kChannelTriggerMatch	= 8,	// channel triggered at startup for BloodBath mode
	kChannelTriggerCoop		= 9,	// channel triggered at startup for Coop mode

	// REMOTE BOMBS: Channels 90-99 reserved
	kChannelRemoteFire1		= 90,
	kChannelRemoteFire2,
	kChannelRemoteFire3,
	kChannelRemoteFire4,
	kChannelRemoteFire5,
	kChannelRemoteFire6,
	kChannelRemoteFire7,
	kChannelRemoteFire8,

	kUserChannelStart		= 100
};

struct EVENT {
	unsigned index 		: 13; 	// object array index (sprite[], sector[], wall[])
	unsigned type  		:  3;	// 0=sprite, 1=sector, 2=wall
	unsigned to       	: 10; 	// objects with matching rxID will receive message
	unsigned command  	:  6; 	// kCommandOn, kCommandOff, etc.
};

void evInit( void );
void evSend( int index, int type, int to, int command );
void evPost( int index, int type, ulong time, int command = kCommandCallback );
void evProcess( ulong time );

#endif // __EVENTQ_H
