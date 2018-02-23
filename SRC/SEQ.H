#ifndef __SEQ_H
#define __SEQ_H

#include "helix.h"
#include "typedefs.h"
#include "resource.h"

#define kSEQSig		"SEQ\x1A"
#define kSEQVersion	0x0300

struct SEQFRAME
{
	unsigned nTile			: 12;
	unsigned translucent	: 1;
	unsigned translucentR	: 1;
	unsigned blocking		: 1;
	unsigned hitscan		: 1;
	unsigned xrepeat		: 8;
	unsigned yrepeat		: 8;

	signed shade			: 8;
	unsigned pal			: 5;
	unsigned trigger		: 1;	// trigger callback
	unsigned smoke			: 1;	// add smoke tsprite

//	unsigned shadeRelative  : 1;
//	unsigned reserved		: 16;
	unsigned reserved		: 17;
};


typedef void (*SEQCALLBACK)( int type, int nXIndex );


struct Seq
{
	char 		signature[4];
	short 		version;
	short		nFrames;		// sequence length
	short		ticksPerFrame;	// inverted play rate
	short		soundID;
	uchar 		flags;
	char		pad[3];
	SEQFRAME	frame[];
	void		Preload( void );
};


// sequence flags
enum {
	kSeqLoop		= 1,
	kSeqRemove		= 2,
};


void seqSpawn( int nSeqID, int type, int nXIndex, SEQCALLBACK callback = NULL );

void seqKill( int type, int nXIndex );
/* use this when you need to delete a sprite which has a sequence playing. */

void seqKillAll( void );

int seqGetStatus( int type, int nXIndex );
/* find out whether a sequence is finished */

void seqProcess( int nTicks );
/* update all sequences */

#endif

