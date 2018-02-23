#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "engine.h"

#include "fire.h"
#include "typedefs.h"
#include "misc.h"
#include "names.h"
#include "error.h"
#include "debug4g.h"
#include "globals.h"
#include "resource.h"
#include "tile.h"

#include <memcheck.h>

/***********************************************************************
 * Defines
 **********************************************************************/
#define kSize				128
#define kSeedLines			3
#define kPorchLines			4
#define kSeedBuffers  		16


/***********************************************************************
 * Global variables
 **********************************************************************/
int gDamping = 7;	// use 7 for 64x64, 4 for 128x128


/***********************************************************************
 * Local variables
 **********************************************************************/
extern "C" BYTE CoolTable[];

BYTE CoolTable[1024];	// contains the new heat value per frame per cell
BYTE FrameBuffer[kSize * (kSize + kPorchLines + kSeedLines)];
BYTE SeedBuffer[kSeedBuffers][kSize];
int fireSize = kSize;
BYTE *gCLU;

/***********************************************************************
 * External declarations
 **********************************************************************/
extern "C" void cdecl CellularFrame(void * buffer, int width, int height);


/***********************************************************************
 * UpdateTile
 **********************************************************************/
void UpdateTile( BYTE *pCLU, BYTE *pTile );
#pragma aux UpdateTile = \
	"mov	esi,OFFSET FrameBuffer" \
	"xor	eax,eax" \
	"mov	edx,[fireSize]" \
	"rowLoop:" \
	"push	edi" \
	"mov	ecx,[fireSize]" \
	"colLoop:" \
	"mov	al,[esi]" \
	"inc	esi" \
	"mov	al,[ebx+eax]" \
	"mov	[edi],al" \
	"add	edi,[fireSize]" \
	"dec	ecx" \
	"jnz	colLoop" \
	"pop	edi" \
	"inc	edi" \
	"dec	edx" \
	"jnz	rowLoop" \
	parm [ EBX ] [ EDI ] \
	modify [ EAX EBX ECX EDX ESI EDI ];



/***********************************************************************
 * InitSeedBuffers
 **********************************************************************/
void InitSeedBuffers(void)
{
	int i, j;
	BYTE c;

	for ( i = 0; i < kSeedBuffers; i++ )
	{
		for ( j = 0; j < fireSize; j += 2 )
		{
			c = (BYTE)rand();
			SeedBuffer[i][j + 0] = c;
			SeedBuffer[i][j + 1] = c;
		}
	}
}


/***********************************************************************
 * BuildCoolTable
 **********************************************************************/
void BuildCoolTable( void )
{
	int i;

	for (i = 0; i < 1024; i++)
		CoolTable[i] = (BYTE)ClipLow((i - gDamping) / 4, 0);
}


/***********************************************************************
 * FireInit
 **********************************************************************/
void FireInit( void )
{
	memset(FrameBuffer, 0, sizeof(FrameBuffer));
	BuildCoolTable();
	InitSeedBuffers();

	RESHANDLE hCLU = gSysRes.Lookup("RFIRE", ".CLU");
	if (hCLU == NULL)
		ThrowError("RFIRE.CLU not found", ES_ERROR);

	gCLU = (BYTE *)gSysRes.Lock(hCLU);

	// run a few frame to start the fire
	for (int i = 0; i < 50; i++)
		FireProcess();
}


/***********************************************************************
 * FireProcess
 **********************************************************************/
#define kSeedLoc  (gFireSize * (gFireSize + 2))

void FireProcess( void )
{
	int i;
	int nSeed = RandShort() & (kSeedBuffers - 1);	// we need a secondary random number generator for view stuff!

	for (i = 0; i < kSeedLines; i++)
		memcpy(&FrameBuffer[(kSize + kPorchLines + i) * kSize], SeedBuffer[nSeed], kSize);

	CellularFrame(FrameBuffer, kSize, kSize + kPorchLines);
	UpdateTile(gCLU, tileLoadTile(kPicFire));
}
