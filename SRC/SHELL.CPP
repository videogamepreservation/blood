#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <dos.h>

#include "getopt.h"
#include "misc.h"

void ShowUsage(void)
{
	printf("");
	exit(0);
}

void QuitMessage(char * fmt, ...)
{
	char msg[80];
	va_list argptr;
	va_start( argptr, fmt );
	vsprintf( msg, fmt, argptr );
	va_end(argptr);
	printf(msg);
	exit(1);
}


void ProcessFile(char *filespec)
{
	printf("%s: ", filespec);
	printf("done.\n");
}


void ProcessArgument(char *s)
{
	char filespec[_MAX_PATH];
	char buffer[_MAX_PATH2];
	char path[_MAX_PATH];

	strcpy(filespec, s);

	char *drive, *dir;

	// separate the path from the filespec
	_splitpath2(s, buffer, &drive, &dir, NULL, NULL);
	_makepath(path, drive, dir, NULL, NULL);

	struct find_t fileinfo;

	unsigned r = _dos_findfirst(s, _A_NORMAL, &fileinfo);

	while ( r == 0 )
	{
		strcpy(filespec, path);
		strcat(filespec, fileinfo.name);
		ProcessFile(filespec);

		r = _dos_findnext( &fileinfo );
	}

	_dos_findclose(&fileinfo);
}


/***********************************************************************
 * Process command line arguments
 **********************************************************************/
void ParseOptions( int argc, char *argv[])
{
	int c;
	while ( (c = GetOptions(argc, argv, "")) != GO_EOF ) {
		switch (c) {
			case GO_INVALID:
				QuitMessage("Invalid argument: %s", OptArgument);

			case GO_FULL:
				ProcessArgument(OptArgument);
				break;
		}
	}
}


void main(int argc, char *argv[])
{
	printf("Conversion shell Copyright (c) 1995 Q Studios Corporation\n");

	if (argc < 2) ShowUsage();

	ParseOptions(argc, argv);
}



