#include <conio.h>

#include "textio.h"


#define kBufSize	1024
#define kJoyPort	0x201
#define kNumAxes	4
#define kNumButtons	4

char data[kBufSize];

int axis[kNumAxes];
int button[kNumButtons];

void ReadJoystick( void )
{
	int i, j, mask;

	// write a dummy value to start resistive timing
	i = 0;
	outp(kJoyPort, 0xFF);
	while ( i < kBufSize )
		data[i++] = (char)inp(kJoyPort);

	for (j = 0, mask = 0x10; j < kNumButtons; j++, mask <<= 1)
		button[j] = (data[0] & mask) == 0;

	for (i = 0, mask = 1; i < kNumAxes; i++, mask <<= 1)
	{
		for (j = 0; j < kBufSize && (data[j] & mask) != 0; j++);
		axis[i] = j;
	}
}


void main( void )
{
	int i;

	tioInit(1);

	while ( !kbhit() )
	{
		ReadJoystick();
		tioSetPos(0, 0);
		for ( i = 0; i < kNumAxes; i++ )
			tioPrint("Axis[%i]=%4d", i, axis[i]);
		for ( i = 0; i < kNumButtons; i++ )
			tioPrint("Button[%i]=%d", i, button[i]);
	}
	getch();
}





