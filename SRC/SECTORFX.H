#ifndef __SECTORFX_H
#define __SECTORFX_H

extern int gDepth[];

enum {
	kWaveNone,
	kWaveSquare,
	kWaveSaw,
	kWaveRampup,
	kWaveRampdown,
	kWaveSine,
	kWaveFlicker1,
	kWaveFlicker2,
	kWaveFlicker3,
	kWaveFlicker4,
	kWaveStrobe,
	kWaveSearch,
};

void DoSectorLighting( void );
void UndoSectorLighting( void );
void DoSectorPanning( void );
void InitSectorFX( void );

#endif
