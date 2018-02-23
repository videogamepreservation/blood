#ifndef __BSTUB_H
#define __BSTUB_H

extern int gHighlightThreshold;
extern int gStairHeight;
extern int gLBIntensity;
extern int gLBAttenuation;
extern int gLBReflections;
extern int gLBMaxBright;
extern int gLBRampDist;
extern int gPalette;
extern BOOL gBeep;
extern BOOL gOldKeyMapping;
extern BOOL gPanning;
extern char *gBoolNames[2];
extern char *gSpriteNames[1024];
extern char *gWallNames[1024];
extern char *gSectorNames[1024];
extern char *gWaveNames[16];
extern char *gDepthNames[4];
extern char *gCommandNames[256];
extern char *gRespawnNames[4];
extern short grid, gridlock;
extern long zoom;

#ifdef __cplusplus
extern "C" {
#endif

const char *ExtGetSectorCaption( short nSector );
const char *ExtGetWallCaption( short nWall );
const char *ExtGetSpriteCaption( short nSprite );
void ExtShowSectorData( short nSector );
void ExtShowWallData( short nWall );
void ExtShowSpriteData( short nSprite );
void ExtEditSectorData( short nSector );
void ExtEditWallData( short nWall );
void ExtEditSpriteData( short nSprite );
void ExtLoadMap( char *fname );
void ExtSaveMap( char *fname );
void ExtPreCheckKeys( void );
void ExtCheckKeys( void );
void ExtInit( void );
void ExtUnInit( void );
void ExtAnalyzeSprites( void );

#ifdef __cplusplus
}
#endif

void Wait(long time);
void BeepOkay( void );
void BeepError( void );
void CleanUp( void );
int GetXSector( int nSector );
int GetXWall( int nWall );
int GetXSprite( int nSprite );
void AutoAdjustSprites( void );

#endif
