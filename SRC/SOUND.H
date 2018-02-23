#ifndef __SOUND_H
#define __SOUND_H
#include "typedefs.h"


/***********************************************************************
 * Exported variables
 **********************************************************************/

extern int gFXDevice;
extern int gMusicDevice;


/***********************************************************************
 * Function prototypes
 **********************************************************************/
extern void sndPlaySong( char *song, BOOL loopflag );
extern BOOL sndIsSongPlaying( void );
extern void sndFadeSong( int nMilliseconds );
extern void sndStopSong( void );

extern void sndStartSample( char *sampleName, int nVol, int nChannel = -1 );
/* Play a sound at the given volume.  Use this function for interface and other
non-3D sound effects. */

extern void sndKillAllSounds( void );
/* Stop all interface sounds.  This could potentially kill all 3D sounds, too.
*/

extern void sndProcess( void );
/* Process sound events.  Must be called once per frame. */

extern void sndInit( void );
extern void sndTerm( void );

#endif //__SOUND_H
