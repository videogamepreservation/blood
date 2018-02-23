#include <fcntl.h>
#include <io.h>
#include <sys\stat.h>

#include "engine.h"
#include "options.h"
#include "debug4g.h"
#include "key.h"
#include "resource.h"
#include "globals.h"
#include "helix.h"
#include "error.h"
#include "gfx.h"
#include "trig.h"

#include <memcheck.h>

BOOL gFallingDamage		= TRUE;
BOOL gViewBobbing		= TRUE;
BOOL gRunBackwards		= FALSE;
BOOL gFriendlyFire		= TRUE;
BOOL gRespawnItems		= FALSE;
BOOL gRespawnEnemies	= FALSE;
BOOL gOverlayMap		= FALSE;
BOOL gRotateMap			= FALSE;
BOOL gGraphNumbers		= FALSE;
BOOL gAutoAim			= TRUE;
BOOL gInfiniteAmmo		= FALSE;
BOOL gFullMap			= FALSE;
BOOL gNoClip			= FALSE;
BOOL gAimReticle		= FALSE;

int		gDetail = kDetailLevelMin;

extern "C" void initgroupfile(char *);


/*******************************************************************************
	FUNCTION:		InitEngine()

	DESCRIPTION:	Initialize the Build engine and pick the graphics mode

	PARAMETERS:		You can override the mode settings in BLOOD.INI by passing
					them in here.

	NOTES:
		; 0 = Unchained mode
		; 1 = VESA 2.0 linear buffer
		; 2 = Buffered Screen/VESA mode
		; 3 = TSENG mode
		; 4 = Paradise mode
		; 5 = S3
		; 6 = Crystal Eyes mode
		; 7 = Red-Blue mode
*******************************************************************************/
void InitEngine( int nMode, int xRes, int yRes )
{
	atexit(uninitengine);	// restores key interrupt on crash

	if ( nMode == -1 )
	{
		nMode = BloodINI.GetKeyInt("Video", "Mode", 2);
		xRes = BloodINI.GetKeyInt("Video", "XRes", 320);
		yRes = BloodINI.GetKeyInt("Video", "YRes", 200);
	}

	if ( nMode >= 3 && nMode <= 7 )
	{
		if (xRes != 320 || yRes != 200)
			ThrowError("You wanker!  This video mode doesn't support that resolution.", ES_ERROR);
	}

	initengine(nMode, xRes, yRes);

	switch (nMode)
	{
		case 0:	//	non-chained
			if (chainnumpages > 1)
			{
				if (!gFindMode(320, 240, 256, NONCHAIN256))
					ThrowError("Helix driver not found", ES_ERROR);
				gPageTable[0].bytesPerRow = xdim / 4;
				gPageTable[0].size = chainsiz;
			}
			else	// memory buffered
			{
				if (!gFindMode(320, 200, 256, CHAIN256))
					ThrowError("Helix driver not found", ES_ERROR);
				gPageTable[0].begin = (unsigned)frameplace;
				gPageTable[0].bytesPerRow = xdim;
				gPageTable[0].size = xdim * ydim;
			}
			break;

		default:
			// generic linear frame buffer
			if (!gFindMode(320, 200, 256, CHAIN256))
				ThrowError("Helix driver not found", ES_ERROR);
			gPageTable[0].begin = (unsigned)frameplace;
			gPageTable[0].bytesPerRow = xdim;
			gPageTable[0].size = xdim * ydim;
	}

	gfxSetClip(0, 0, xRes, yRes);
}


void optLoadINI( void )
{
	gSpring 		= BloodINI.GetKeyInt("Player", "SpringCoefficient", 12);
	gSpringPhaseInc = (int)(kAngle360 / (2 * 3.141592654 * (double)gSpring));

	gGamma 			= BloodINI.GetKeyInt("View", "Gamma", 0);

	// load the play options
	gDetail			= BloodINI.GetKeyInt("Options", "Detail", gDetail);

	gFallingDamage  = BloodINI.GetKeyBool("Options", "FallingDamage",  	gFallingDamage);
	gViewBobbing    = BloodINI.GetKeyBool("Options", "ViewBobbing",    	gViewBobbing);
	gRunBackwards   = BloodINI.GetKeyBool("Options", "RunBackwards",   	gRunBackwards);
	gFriendlyFire   = BloodINI.GetKeyBool("Options", "FriendlyFire",   	gFriendlyFire);
	gRespawnItems   = BloodINI.GetKeyBool("Options", "RespawnItems",   	gRespawnItems);
	gRespawnEnemies = BloodINI.GetKeyBool("Options", "RespawnEnemies", 	gRespawnEnemies);
	gOverlayMap     = BloodINI.GetKeyBool("Options", "OverlayMap",     	gOverlayMap);
	gRotateMap      = BloodINI.GetKeyBool("Options", "RotateMap",      	gRotateMap);
	gGraphNumbers   = BloodINI.GetKeyBool("Options", "GraphNumbers",   	gGraphNumbers);
	gAutoAim		= BloodINI.GetKeyBool("Options", "AutoAim",   		gAutoAim);
	gInfiniteAmmo	= BloodINI.GetKeyBool("Options", "InfiniteAmmo",   	gInfiniteAmmo);
}

