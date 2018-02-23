#include "engine.h"

#include "qav.h"
#include "error.h"
#include "misc.h"
#include "tile.h"
#include "trig.h"
#include "debug4g.h"


static void DrawFrame( int x, int y, TILE_FRAME *f, int shade, int flags )
{
	short nAngle = f->angle;
	flags |= f->flags;

	if ( flags & kQFrameXFlip )
	{
		nAngle = (short)((nAngle + kAngle180) & kAngleMask);
		flags &= ~kQFrameXFlip;
		flags ^= kQFrameYFlip;
	}

	rotatesprite(
		x + f->x << 16,
		y + f->y << 16,
		f->zoom,
		nAngle,
		(short)f->id,
		(schar)ClipRange(f->shade + shade, -128, 127),
		(char)f->pal,
		(char)flags,
		windowx1, windowy1, windowx2, windowy2);
}


// render a complete frame for time t
void QAV::Draw( long t, int shade, int flags )
{
	dassert(ticksPerFrame > 0);
	int nFrame = t / ticksPerFrame;
	dassert(nFrame >= 0 && nFrame < nFrames);
	FRAME *f = &frame[nFrame];
	for (int i = 0; i < kMaxLayers; i++)
	{
		if ( f->layer[i].id > 0 )
		 	DrawFrame(origin.x, origin.y, &f->layer[i], shade, flags);
	}
}


// play a frame for time interval t0 < t <= t1
void QAV::Play( long t0, long t1, QAVCALLBACK callback, void *data)
{
	dassert(ticksPerFrame > 0);
	int nFrame;

	// need to truncate toward -infinity
	if ( t0 < 0 )
		nFrame = (t0 + 1) / ticksPerFrame;
	else
		nFrame = t0 / ticksPerFrame + 1;

	int t = nFrame * ticksPerFrame;
	for ( ; t <= t1; t += ticksPerFrame, nFrame++ )
	{
		if ( nFrame >= 0 && nFrame < nFrames )
		{
			FRAME *f = &frame[nFrame];
			// Play the sound
			// .
			// .
			// .

			// Callback triggers
			if ( f->trigger.id > 0 && callback != NULL )
				callback( f->trigger.id, data );
		}
	}
}


void QAV::Preload( void )
{
	for (int i = 0; i < nFrames; i++)
	{
		for (int j = 0; j < kMaxLayers; j++)
		{
			if ( frame[i].layer[j].id >= 0 )
				tilePreloadTile(frame[i].layer[j].id);
		}
	}
}
