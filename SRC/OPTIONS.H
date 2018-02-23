#ifndef __OPTIONS_H
#define __OPTIONS_H


enum {
	// global detail level range
	kDetailLevelMin = 0,
	kDetailLevel2,
	kDetailLevel3,
	kDetailLevel4,
	kDetailLevelMax,

	// global detail types
	kDetailSprites = 0,
	kDetailShadows = 1,
	kDetailGiblets = 2,
};


extern BOOL gFallingDamage;
extern BOOL gViewBobbing;
extern BOOL gRunBackwards;
extern BOOL gFriendlyFire;
extern BOOL gRespawnItems;
extern BOOL gRespawnEnemies;
extern BOOL gOverlayMap;
extern BOOL gRotateMap;
extern BOOL gGraphNumbers;
extern BOOL gAutoAim;
extern BOOL gInfiniteAmmo;
extern BOOL gFullMap;
extern BOOL gNoClip;
extern BOOL gAimReticle;

extern int	gDetail;

void InitEngine( int nMode = -1, int xres = 320, int yres = 200 );

void optLoadINI( void );

#endif

