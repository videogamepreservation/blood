#include <io.h>
#include <fcntl.h>
#include <math.h>

#include "typedefs.h"
#include "textio.h"
#include "engine.h"
#include "screen.h"
#include "trig.h"
#include "misc.h"

#define kPi				3.141592654
#define kAttrTitle      (kColorMagenta * 16 + kColorWhite)
#define kGammaLevels	16

uchar gammaTable[kGammaLevels][256];
long costable[kAngle360];


BOOL FileSave(char *fname, void *buffer, ulong size)
{
	int hFile, n;

	hFile = open(fname, O_CREAT | O_WRONLY | O_BINARY, S_IWUSR);

	if ( hFile == -1 )
		return FALSE;

	n = write(hFile, buffer, size);
	close(hFile);

	return n == size;
}


void BuildGammaTable( void )
{
	double nGamma;
	double nScale;
	int i, j;

	for (i = 0; tioGauge(i, kGammaLevels); i++)
	{
		nGamma = 1.0 / (1.0 + i * 0.1);
		nScale = (double)255 / pow(255, nGamma);

		for (j = 0; j < 256; j++)
		{
			gammaTable[i][j] = pow(j, nGamma) * nScale;
		}
	}
}


void BuildCosTable( void )
{
	int i;

	for (i = 0; tioGauge(i, kAngle90); i++)
	{
		costable[i] = cos(i * 2 * kPi / kAngle360) * (1 << 30);
	}
}


void main( void )
{
	tioInit();
	tioCenterString(0, 0, tioScreenCols - 1, "Blood math table builder", kAttrTitle);
	tioCenterString(tioScreenRows - 1, 0, tioScreenCols - 1,
		"Copyright (c) 1994, 1995 Q Studios Corporation", kAttrTitle);

	tioWindow(1, 0, tioScreenRows - 2, tioScreenCols - 1);

	tioPrint("Building gamma correction table");
	BuildGammaTable();
	FileSave("gamma.dat", gammaTable, sizeof(gammaTable));

	tioPrint("Building cosine table");
	BuildCosTable();
	FileSave("cosine.dat", costable, kAngle90 * sizeof(costable[0]));
	tioTerm();
}


