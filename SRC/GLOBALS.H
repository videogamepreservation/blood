#ifndef __GLOBALS_H
#define __GLOBALS_H

#include "engine.h"
#include "resource.h"
#include "inifile.h"

/***********************************************************************
 * Global constants
 **********************************************************************/

#define kRequiredFiles	30

#define kMaxPlayers		8

// Game clock timer resolution
#define kTimerRate      120

// number of clock ticks per game frame
#define kFrameTicks     4	// 30 frames/sec

// Physics constants
#define kZUnitsPerMeter (32 << 8)
#define kXUnitsPerMeter (32 << 4)
#define kRealGravity 	20.0     // meter/sec^2
//#define kRealGravity 	9.8     // meter/sec^2
#define M2Z(n)       (int)((n) * kZUnitsPerMeter)  // convert meters to z units
#define M2X(n)       (int)((n) * kXUnitsPerMeter)  // convert meters to x units
#define kGravity     (int)(M2Z(kRealGravity) / (kTimerRate * kTimerRate) + .5)

enum
{
	kNetModeOff			= 0,
	kNetModeCoop,
	kNetModeBloodBath,
	kNetModeTeams
};


/***********************************************************************
 * Global variables
 **********************************************************************/
extern Resource gSysRes;
extern IniFile BloodINI;

extern char gBuildDate[];
extern int gOldDisplayMode;
extern long gFrameClock;
extern int gFrameTicks;
extern int gFrame;
extern volatile long gGameClock;
extern int gCacheMiss;
extern int gFrameRate;
extern int gGamma;
extern int gSpring;					// û(m/k)
extern int gSpringPhaseInc;			// kAngle360 / T

extern BOOL gPaused;
extern BOOL gNetGame;
extern int gNetPlayers;
extern int gNetMode;

void ClockStrobe( void );
void LockClockStrobe( void );
void UnlockClockStrobe( void );

#endif
