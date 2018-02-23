#ifndef __MULTI_H
#define __MULTI_H

#define MAXPLAYERS 16

/***********************************************************************
 * Multiplayer functions
 **********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

extern short numplayers, myconnectindex;
extern short connecthead, connectpoint2[MAXPLAYERS];
extern char syncstate;	// if becomes non-zero, then a packet was dropped
extern long *lastpacket2clock;

void initmultiplayers(char mode, char comRate, char priority);
/* Call this right after initengine.
mode =
	1 = COM1
	2 = COM2
	3 = COM3
	4 = COM4
	5 = IPX 2 player
	6 = IPX 3 player
	7 = IPX 4 player
comRate =
	upper 4 bits = IRQ - 2
	lower 4 bits =
		0 = 2400
		1 = 4800
		2 = 9600
		3 = 14400
		4 = 19200
		5 = 28800
*/

void uninitmultiplayers( void );
/* Call this right before uninitengine. */

void sendlogon( void );
/* Use this function after everything's initialized, but before you go into the
main game loop.  Right after you call sendlogon(), you should run a loop that
will wait until a specified number of players.  Here's some example code:

	sendlogon();
	while (numplayers < waitplayers)
	{
		getpackets();
	}
	screenpeek = myconnectindex;

Getpackets reserves the packet header range from 200-255.  If you keep calling
getpackets after sendlogon, the numplayers variable will automatically be
incremented when other people log on. */

void sendlogoff( void );
/* Call this before leaving, before uninitializing the multiplayer code. */

void sendpacket(short nIndex, char *bufptr, short buflen);
/* For COM(modem) communications, the otherconnectindex doesn't matter.  For
network communcations, you can specify which computer to send to by setting
nIndex to the proper index number.  You can also do a broadcast by setting
nIndex to -1.  Also pass the buffer and length parameters.  The first character
of the buffer normally contains the packet type.  Packets types 200-255 are
used internally.  255 is send logoff.  */

short getpacket (short *nIndex, char *bufptr);
/* returns bufleng.  When using getpacket, first check the value it returns.
If the value is 0, then the buffer length is 0 which means there are no packets
available.  If the buffer length is greater than 0, then use that value as the
length of the buffer.  Getpacket also tells you what computer the message was
received from in nIndex.  */

long getoutputcirclesize( void );
/* This function returns the number of bytes that have not yet been copied.  If
there are still more than say, 16 bytes, then you may be sending too many bytes
per second.  This can happen if the frame rate of a computer is faster than the
speed of the serial mode (Ex: Try 2400 baud with a Pentium 90!) this function
will tell you how many bytes are left to copy In other words, if
getoutputcirclesize() < 16 then it is safe to send a packet.  If you already
have serial mode working all you have to do to update your code is to copy the
lines in my sync() function the deal with the getoutputcirclesize function.
Everything else in sync() and getpackets() is pretty much the same. */

#ifdef __cplusplus
}
#endif

#endif


