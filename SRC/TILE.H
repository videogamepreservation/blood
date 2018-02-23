#ifndef __TILE_H
#define __TILE_H

// 3d view types
enum {
	kSpriteViewSingle = 0,
	kSpriteView5Full,
	kSpriteView8Full,
	kSpriteView5Half,
	kSpriteView3Flat,
	kSpriteView4Flat
};

extern short tileIndex[kMaxTiles];
extern int tileIndexCount;
extern BYTE surfType[kMaxTiles];

void tileTerm( void );
int tileInit( void );
void tileSaveArt( void );
void tileSaveArtInfo( void );
void tileMarkDirty( int nTile );
void tileMarkDirtyAll( void );
void tilePurgeTile( int nTile );
void tileRotateTiles( int start, int length, int shift );

BYTE *tileLoadTile( int nTile );
BYTE *tileLockTile( int nTile );
void tileUnlockTile( int nTile );
BYTE *tileAllocTile( int nTile, int sizeX, int sizeY );
void tileFreeTile( int nTile );
void tilePreloadTile( int nTile );

short tileBuildHistogram( int type );
void tileDrawTileScreen( long nStart, long nCursor, int size, int nMax );
short tilePick( int nTile, int nDefault, int type );
BYTE tileGetSurfType( int nHit );

#endif

