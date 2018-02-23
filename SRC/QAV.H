#ifndef __QAV_H
#define __QAV_H

#include "typedefs.h"
#include "gfx.h"


#define kQAVSig		"QAV\x1A"
#define kQAVVersion	0x0200

#define kMaxLayers	8

enum
{
	// begin rotatesprite compatible flags
	kQFrameNormal			= 0,		// no flags
	kQFrameTranslucent		= 0x01,		// frame is translucent
	kQFrameScale			= 0x02,		// frame is scaled to viewing window
	kQFrameYFlip			= 0x04,		// frame is y-flipped
	kQFrameUnclipped		= 0x08,		// frame is not clipped to umost/dmost
	kQFrameStatus			= 0x0A,		// frame is not masked (used for status bar)
	kQFrameCorner			= 0x10,		// frame is positioned by top left corner instead of origin
	kQFrameTranslucentR		= 0x20,		// frame is translucent, using reverse translucency table
	kQFrameNoMask			= 0x40,
	kQFrameAllPages			= 0x80,

	// begin internal flags
	kQFrameXFlip			= 0x100,	// frame is x-flipped
};


struct TRIGGER_FRAME
{
	int		id;
};


struct SOUND_FRAME
{
	int		id;
	uchar	priority;
	uchar	volume;
	ushort	soundAngle;
};


struct TILE_FRAME
{
	int		id;
	int		x, y;
	int		zoom;	// zoom in 16:16 fixed format
	int		flags;
	schar	shade;
	uchar	pal;
	ushort	angle;	// angle in Build units (0 is straight up)
};


struct FRAME
{
	TRIGGER_FRAME trigger;
	SOUND_FRAME sound;
	TILE_FRAME layer[kMaxLayers];
};


typedef void (*QAVCALLBACK)( int triggerId, void *data );


struct QAV
{
	char 	signature[4];
	short 	version;
	char	dummy[2];
	int		nFrames;
	int		ticksPerFrame;	// inverted play rate
	int		duration;		// length in ticks
	Point	origin;
	char	reserved[8];
	FRAME	frame[];

	void	Draw( long t, int shade, int flags );
	void	Play( long t0, long t1, QAVCALLBACK callback = NULL, void *data = NULL );
	void	Preload( void );
};

#endif
