#include <stdlib.h>

#include "globals.h"
#include "resource.h"
#include "misc.h"
#include "i86.h"

#include <memcheck.h>

/***********************************************************************
 * Global variables
 **********************************************************************/
Resource gSysRes;
IniFile BloodINI("BLOOD.INI");

char gBuildDate[] = __DATE__;
int gOldDisplayMode;
long gFrameClock = 0;
int gFrameTicks = 0;
int gFrame = 0;
volatile long gGameClock = 0;
int gCacheMiss = 0;
int gFrameRate = 0;
int gGamma = 0;
int gSpring;					// û(m/k)
int gSpringPhaseInc;			// kAngle360 / T
BYTE gEndLevelFlag = 0;

BOOL	gPaused		= FALSE;
int		gNetPlayers	= 0;
BOOL	gNetGame	= FALSE;
int		gNetMode	= kNetModeCoop;

void ClockStrobe( void )
{
	gGameClock++;
}

void CLOCK_STROBE_END( void ) {};

void LockClockStrobe( void )
{
	dpmiLockMemory(FP_OFF(&ClockStrobe), FP_OFF(&CLOCK_STROBE_END) - FP_OFF(&ClockStrobe));
	dpmiLockMemory(FP_OFF(&gGameClock), sizeof(long));
}

void UnlockClockStrobe( void )
{
	dpmiLockMemory(FP_OFF(&ClockStrobe), FP_OFF(&CLOCK_STROBE_END) - FP_OFF(&ClockStrobe));
	dpmiLockMemory(FP_OFF(&gGameClock), sizeof(long));
}

