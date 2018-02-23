#include <conio.h>
#include "engine.h"
#include "db.h"
#include "misc.h"
#include "textio.h"

#define kNumSprites		2048
#define kIterations		10000

short spriteId[kNumSprites];


void faketimerhandler( void ) {};


void main( void )
{
	int i;

	tioInit();

	tioPrint("Initializing");
	dbInit();

	tioPrint("Creating %d sprites", kNumSprites);
	for (i = 0; tioGauge(i, kNumSprites); i++)
		spriteId[i] = insertsprite((short)Random(kMaxSectors), (short)Random(kMaxStatus));

	while ( !kbhit() )
	{
		tioPrint("Changing sectors for %d sprites", kIterations);
		for (i = 0; tioGauge(i, kIterations); i++)
			changespritesect(spriteId[Random(kNumSprites)], (short)Random(kMaxSectors));

		tioPrint("Changing statnums for %d sprites", kIterations);
		for (i = 0; tioGauge(i, kIterations); i++)
			changespritestat(spriteId[Random(kNumSprites)], (short)Random(kMaxStatus));

		tioPrint("Deleting and reinserting %d sprites", kIterations);
		for (i = 0; tioGauge(i, kIterations); i++)
		{
			int j = Random(kNumSprites);
			deletesprite(spriteId[j]);
			spriteId[j] = insertsprite((short)Random(kMaxSectors), (short)Random(kMaxStatus));
		}
	}
}
