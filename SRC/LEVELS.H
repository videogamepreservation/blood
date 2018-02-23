#ifndef __LEVELS_H
#define __LEVELS_H

#include <stdlib.h>

/***********************************************************************
 *
 * Enumerated Types
 *
 **********************************************************************/
enum {
	kMinEpisode = 1,
	kMaxEpisode = 3,
	kMinMap     = 1,
	kMaxMap     = 15,
};


/***********************************************************************
 *
 * Global Level Data
 *
 **********************************************************************/
extern char gLevelName[_MAX_PATH];
extern int  gEpisodeId;
extern int  gMapId;
extern int  gEndingA, gEndingB;
extern int  gEndLevelFlag;
extern char gLevelAuthor[128];
extern char gLevelDescription[128];
extern char gLevelSong[128];
extern char *gLevelMessage[64];


/***********************************************************************
 *
 * Global Lightning Data
 *
 **********************************************************************/
#define kMaxLightning	16

struct LIGHTNING {
	int slot;
	int offset;
};

extern LIGHTNING gLightningInfo[ kMaxLightning ];


/***********************************************************************
 *
 * Function Prototypes
 *
 **********************************************************************/
void levelGetName(int episodeId, int mapId);
void levelLoadDef( void );

#endif // __LEVELS_H
