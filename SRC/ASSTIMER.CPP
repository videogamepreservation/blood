#include <stdlib.h>
#include <dos.h>
#include <conio.h>

#include "timer.h"
#include "globals.h"
#include "typedefs.h"
#include "misc.h"
#include "task_man.h"
#include "error.h"

#define kMaxClients		16

struct CLIENT_INFO
{
	TIMER_CLIENT function;
	TASK *task;				// TASK_MAN timer link
};

static CLIENT_INFO client[kMaxClients];
static int nClients = 0;
static BOOL timerActive = FALSE;

typedef void (* TASK_CLIENT)( TASK * );


void timerRemove( void )
{
	if ( timerActive )
	{
		for (int i = 0; i < nClients; i++)
		{
			TS_Terminate(client[i].task);
		}

		TS_Shutdown();

		nClients = 0;

		dpmiUnlockMemory(FP_OFF(client), sizeof(client));
		dpmiUnlockMemory(FP_OFF(&nClients), sizeof(nClients));

		timerActive = FALSE;
	}
}


void timerInstall( void )
{
	if ( !timerActive )
	{
		timerActive = TRUE;

		dpmiLockMemory(FP_OFF(client), sizeof(client));
		dpmiLockMemory(FP_OFF(&nClients), sizeof(nClients));

		TS_Dispatch();

		atexit(timerRemove);
	}
}


void timerSetRate( int /* rate */ )
{
	// this function not implemented for ASS interface
	ThrowError("Call to unimplemented glue function", ES_ERROR);
}


void timerRegisterClient( TIMER_CLIENT timerClient, int rate )
{
	if ( nClients < kMaxClients )
	{
		client[nClients].function = timerClient;
		client[nClients].task = TS_ScheduleTask((TASK_CLIENT)timerClient, rate, 1, NULL);
		nClients++;
	}
}


void timerRemoveClient( TIMER_CLIENT timerClient )
{
	for (int i = 0; i < nClients; i++)
	{
		if ( client[i].function == timerClient )
			break;
	}

	if ( i < nClients )
	{
		_disable();
		nClients--;
		client[i] = client[nClients];
		_enable();
	}
}


void timerSetClientRate( TIMER_CLIENT timerClient, int rate )
{
	for (int i = 0; i < nClients; i++)
	{
		if ( client[i].function == timerClient )
			break;
	}

	if ( i < nClients )
	{
		TS_SetTaskRate(client[i].task, rate);
	}
}
