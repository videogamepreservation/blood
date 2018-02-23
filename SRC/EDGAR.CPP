/*******************************************************************************
	FILE:			EDGAR.CPP

	DESCRIPTION:	GUI Editor

	AUTHOR:			Peter M. Freese
	CREATED:		06-30-95
	COPYRIGHT:		Copyright (c) 1995 Q Studios Corporation
*******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <helix.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>

#include "typedefs.h"
#include "engine.h"
#include "key.h"
#include "misc.h"
#include "gameutil.h"
#include "gfx.h"
#include "globals.h"
#include "debug4g.h"
#include "error.h"
#include "gui.h"
#include "screen.h"
#include "textio.h"
#include "inifile.h"
#include "options.h"
#include "timer.h"
#include "edgar.h"
#include "mouse.h"

#include <memcheck.h>

#define kAttrTitle      (kColorGreen * 16 + kColorYellow)

char buffer[256];
EHF prevErrorHandler;


void faketimerhandler( void )
{ }


virtual void EButton::HandleEvent( GEVENT *event )
{
	if ( event->type & evMouse )
	{
		if (event->mouse.button == 1)
		{
			switch (event->type)
			{
				case evMouseDrag:
					left = ClipRange((event->mouse.x - width / 2) & ~3, 0, owner->width - width);
					top = ClipRange((event->mouse.y - height / 2) & ~3, 0, owner->height - height);
					break;

				case evMouseUp:
					dprintf("Button dropped at %i, %i\n", left, top);
					break;
			}
			return;
		}
	}
	TextButton::HandleEvent(event);
}



/***********************************************************************
 * EditorErrorHandler()
 *
 * Terminate from error condition, displaying a message in text mode.
 *
 **********************************************************************/
ErrorResult EditorErrorHandler( const Error& error )
{
	uninitengine();
	keyRemove();
	setvmode(gOldDisplayMode);

	// chain to the default error handler
	return prevErrorHandler(error);
};


void main( void )
{
	char title[256];

	gOldDisplayMode = getvmode();

	sprintf(title, "Edgar [%s] -- DO NOT DISTRIBUTE", gBuildDate);
	tioInit();
	tioCenterString(0, 0, tioScreenCols - 1, title, kAttrTitle);
	tioCenterString(tioScreenRows - 1, 0, tioScreenCols - 1,
		"Copyright (c) 1994, 1995 Q Studios Corporation", kAttrTitle);

	tioWindow(1, 0, tioScreenRows - 2, tioScreenCols);

	if ( _grow_handles(kRequiredFiles) < kRequiredFiles )
		ThrowError("Not enough file handles available", ES_ERROR);

	tioPrint("Initializing heap and resource system");
	Resource::heap = new QHeap(dpmiDetermineMaxRealAlloc());

	tioPrint("Initializing resource archive");
	gSysRes.Init("BLOOD.RFF", "*.*");
	gGuiRes.Init("GUI.RFF", NULL);

	tioPrint("Initializing mouse");
	if ( !initmouse() )
		tioPrint("Mouse not detected");

	// install our error handler
	prevErrorHandler = errSetHandler(EditorErrorHandler);

	InitEngine();

	Mouse::SetRange(xdim, ydim);

	tioPrint("Initializing screen");
	scrInit();

	tioPrint("Installing keyboard handler");
	keyInstall();

	scrCreateStdColors();

	tioPrint("Installing timer");
	timerRegisterClient(ClockStrobe, kTimerRate);
	timerInstall();

	tioPrint("Engaging graphics subsystem...");

	scrSetGameMode();

	scrSetGamma(gGamma);
	scrSetDac(0);
	clearview(0);
	scrNextPage();

	// create the dialog
	Window dialog(0, 0, 240, 80, "Edgar");

	dialog.Insert(new EButton( 4, 4, 80,  20, "&Button", mrNone));
 	dialog.Insert(new ScrollBar(120, 4, 50, 0, 100, 0));

	// pressed esape
	ShowModal(&dialog);

	setvmode(gOldDisplayMode);

	dprintf("Removing timer\n");
	timerRemove();

 	dprintf("uninitengine()\n");
	uninitengine();

	dprintf("All subsystems shut down.  Processing exit functions\n");

	errSetHandler(prevErrorHandler);
}
