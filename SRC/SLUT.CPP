#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <dos.h>
#include <io.h>
#include <fcntl.h>

#include "getopt.h"
#include "misc.h"
#include "typedefs.h"
#include "seq.h"
#include "debug4g.h"

#define kMaxFrames	1024

struct SEQFRAME1
{
	unsigned 	nTile : 16;
	signed		shade : 8;
	unsigned	translucent : 1;
	unsigned	blocking : 1;
	unsigned	hitscan : 1;
	unsigned	pal : 4;
};

struct Seq1
{
	char 		signature[4];
	short 		version;
	short		nFrames;		// sequence length
	short		ticksPerFrame;	// inverted play rate
	short		keyFrame;
	uchar 		flags;
	char		pad[3];
	SEQFRAME1	frame[1];
};


struct SEQFRAME2
{
	unsigned 	nTile : 12;
	unsigned 	reserved1 : 3;
	unsigned	translucentR : 1;
	signed		shade : 8;
	unsigned	translucent : 1;
	unsigned	blocking : 1;
	unsigned	hitscan : 1;
	unsigned	pal : 5;
	unsigned	xrepeat : 8;
	unsigned	yrepeat : 8;
	unsigned	reserved2 : 16;
};

struct Seq2
{
	char 		signature[4];
	short 		version;
	short		nFrames;		// sequence length
	short		ticksPerFrame;	// inverted play rate
	short		keyFrame;
	uchar 		flags;
	char		pad[3];
	SEQFRAME2	frame[1];
};


struct FNODE
{
	FNODE *next;
	char name[1];
};


FNODE head = { &head, "" };
FNODE *tail = &head;

BOOL bForcePal = FALSE;
uchar nPal;
BOOL bForceShade = FALSE;
char nShade;
BOOL bForceXRepeat = FALSE;
uchar xrepeat;
BOOL bForceYRepeat = FALSE;
uchar yrepeat;

BOOL bForceTranslucent = FALSE;
BOOL bIsTranslucent;
BOOL bIsTranslucentR;

Seq *pSeq = NULL;


/*******************************************************************************
	FUNCTION:		ShowBanner()

	DESCRIPTION:	Show application banner
*******************************************************************************/
void ShowBanner( void )
{
    printf("컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�\n");
	printf("SEQ Loathing Update Tool  Version 3.0  Copyright (c) 1995 Q Studios Corporation\n");
    printf("컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�\n");
}


/*******************************************************************************
	FUNCTION:		ShowUsage()

	DESCRIPTION:	Display command-line parameter usage, then exit
*******************************************************************************/
void ShowUsage(void)
{
	printf("Syntax:  SLUT [options] files[.seq]\n");
	printf("-pN           force all palookups to N\n");
	printf("-sN           force all shade values to N\n");
	printf("-t[0|1|2]     force translucency level\n");
	printf("-xN           force all x repeats to N\n");
	printf("-yN           force all y repeats to N\n");
	printf("-?            This help\n");
	exit(0);
}


/*******************************************************************************
	FUNCTION:		QuitMessage()

	DESCRIPTION:	Display a printf() style message, the exit with code 1
*******************************************************************************/
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


void ProcessFile(char *filename)
{
	char bakname[_MAX_PATH], tempname[_MAX_PATH];
	int i, hFile, nSize;

	strcpy(bakname, filename);
	ChangeExtension(bakname, ".BAK");
	tmpnam(tempname);

	printf("%s: ", filename);

	hFile = open(filename, O_RDONLY | O_BINARY);
	if ( hFile == -1 )
		QuitMessage("Couldn't open file %s, errno=%d\n", filename, errno);

	nSize = filelength(hFile);
	Seq *pTempSeq = (Seq *)malloc(nSize);
	dassert(pTempSeq != NULL);

	if ( !FileRead(hFile, pTempSeq, nSize) )
	{
		close(hFile);
		QuitMessage("Error reading SEQ file");
	}

	close(hFile);
	memcpy(pSeq, pTempSeq, nSize);

	if (memcmp(pSeq->signature, kSEQSig, sizeof(pSeq->signature)) != 0)
		QuitMessage("SEQ file corrupted");

	switch ( pTempSeq->version >> 8 )
	{
		case 3:
			break;	// current version

		case 2:
		{
			// convert old sequence format
			Seq2 *pSeq2 = (Seq2 *)pTempSeq;

			memset(pSeq, 0, sizeof(Seq));
			pSeq->nFrames = pSeq2->nFrames;
			pSeq->ticksPerFrame = pSeq2->ticksPerFrame;

			// check version 2 minor version
			switch ( pTempSeq->version & 0xFF )
			{
				case 2:
					pSeq->flags = pSeq2->flags;
					for (i = 0; i < pSeq2->nFrames; i++)
					{
						memset(&pSeq->frame[i], 0, sizeof(SEQFRAME));
						pSeq->frame[i].nTile = pSeq2->frame[i].nTile;
						pSeq->frame[i].translucent = pSeq2->frame[i].translucent;
						pSeq->frame[i].translucentR = pSeq2->frame[i].translucentR;
						pSeq->frame[i].blocking = pSeq2->frame[i].blocking;
						pSeq->frame[i].hitscan = pSeq2->frame[i].hitscan;
						pSeq->frame[i].xrepeat = pSeq2->frame[i].xrepeat;
						pSeq->frame[i].yrepeat = pSeq2->frame[i].yrepeat;;
						pSeq->frame[i].shade = pSeq2->frame[i].shade;
						pSeq->frame[i].pal = pSeq2->frame[i].pal;
					}
					break;

				case 1:
					pSeq->flags = pSeq2->flags;
					for (i = 0; i < pSeq2->nFrames; i++)
					{
						memset(&pSeq->frame[i], 0, sizeof(SEQFRAME));
						pSeq->frame[i].nTile = pSeq2->frame[i].nTile;
						pSeq->frame[i].translucent = pSeq2->frame[i].translucent;
						pSeq->frame[i].translucentR = 0;
						pSeq->frame[i].blocking = pSeq2->frame[i].blocking;
						pSeq->frame[i].hitscan = pSeq2->frame[i].hitscan;
						pSeq->frame[i].xrepeat = pSeq2->frame[i].xrepeat;
						pSeq->frame[i].yrepeat = pSeq2->frame[i].yrepeat;;
						pSeq->frame[i].shade = pSeq2->frame[i].shade;
						pSeq->frame[i].pal = pSeq2->frame[i].pal;
					}
					break;


				case 0:
					pSeq->flags = 0;
					for (i = 0; i < pSeq2->nFrames; i++)
					{
						memset(&pSeq->frame[i], 0, sizeof(SEQFRAME));
						pSeq->frame[i].nTile = pSeq2->frame[i].nTile;
						pSeq->frame[i].translucent = pSeq2->frame[i].translucent;
						pSeq->frame[i].translucentR = 0;
						pSeq->frame[i].blocking = pSeq2->frame[i].blocking;
						pSeq->frame[i].hitscan = pSeq2->frame[i].hitscan;
						pSeq->frame[i].xrepeat = pSeq2->frame[i].xrepeat;
						pSeq->frame[i].yrepeat = pSeq2->frame[i].yrepeat;;
						pSeq->frame[i].shade = pSeq2->frame[i].shade;
						pSeq->frame[i].pal = pSeq2->frame[i].pal;
					}
					break;

			}
			break;
		}

		case 1:
		{
			// convert old sequence format
			Seq1 *pSeq1 = (Seq1 *)pTempSeq;

			memset(pSeq, 0, sizeof(Seq));
			pSeq->nFrames = pSeq1->nFrames;
			pSeq->ticksPerFrame = pSeq1->ticksPerFrame;
			for (i = 0; i < pSeq1->nFrames; i++)
			{
				memset(&pSeq->frame[i], 0, sizeof(SEQFRAME));
				pSeq->frame[i].nTile = pSeq1->frame[i].nTile;
				pSeq->frame[i].translucent = pSeq1->frame[i].translucent;
				pSeq->frame[i].blocking = pSeq1->frame[i].blocking;
				pSeq->frame[i].hitscan = pSeq1->frame[i].hitscan;
				pSeq->frame[i].xrepeat = 64;
				pSeq->frame[i].yrepeat = 64;
				pSeq->frame[i].shade = pSeq1->frame[i].shade;
				pSeq->frame[i].pal = pSeq1->frame[i].pal;
			}
			break;
		}

		default:
			QuitMessage("Obsolete SEQ version");
	}

	free(pTempSeq);

	// set forced frame attributes
	for (i = 0; i < pSeq->nFrames; i++)
	{
		if ( bForcePal )
			pSeq->frame[i].pal = nPal;

		if ( bForceShade )
			pSeq->frame[i].shade = nShade;

		if ( bForceTranslucent )
		{
			if (bIsTranslucent)
				pSeq->frame[i].translucent = 1;
			else
				pSeq->frame[i].translucent = 0;

			if (bIsTranslucentR)
				pSeq->frame[i].translucentR = 1;
			else
				pSeq->frame[i].translucentR = 0;
		}

		if ( bForceXRepeat )
			pSeq->frame[i].xrepeat = xrepeat;

		if ( bForceYRepeat )
			pSeq->frame[i].yrepeat = yrepeat;
	}


	hFile = open(tempname, O_CREAT | O_WRONLY | O_BINARY | O_TRUNC, S_IWUSR);
	if ( hFile == -1 )
		QuitMessage("Error creating temporary file");

	memcpy(pSeq->signature, kSEQSig, sizeof(pSeq->signature));
	pSeq->version = kSEQVersion;

	if ( !FileWrite(hFile, pSeq, sizeof(Seq) + pSeq->nFrames * sizeof(SEQFRAME)) )
	{
		close(hFile);
		QuitMessage("Error writing temporary file");
	}

	close(hFile);

	// backup the existing sequence
	unlink(bakname);
	rename(filename, bakname);
	rename(tempname, filename);

	printf("done.\n");
}


void InsertFilename( char *fname )
{
	FNODE *n = (FNODE *)malloc(sizeof(FNODE) + strlen(fname));
	strcpy(n->name, fname);

	// insert the node at the tail, so it stays in order
	n->next = tail->next;
	tail->next = n;
	tail = n;
}


void ProcessArgument(char *s)
{
	char filespec[_MAX_PATH];
	char buffer[_MAX_PATH2];
	char path[_MAX_PATH];

	strcpy(filespec, s);
	AddExtension(filespec, ".SEQ");

	char *drive, *dir;

	// separate the path from the filespec
	_splitpath2(s, buffer, &drive, &dir, NULL, NULL);
	_makepath(path, drive, dir, NULL, NULL);

	struct find_t fileinfo;

	unsigned r = _dos_findfirst(s, _A_NORMAL, &fileinfo);
	if (r != 0)
		printf("%s not found\n", s);

	while ( r == 0 )
	{
		strcpy(filespec, path);
		strcat(filespec, fileinfo.name);
		InsertFilename(filespec);

		r = _dos_findnext( &fileinfo );
	}

	_dos_findclose(&fileinfo);
}


/***********************************************************************
 * Process command line arguments
 **********************************************************************/
void ParseOptions( void )
{
	enum {
		kSwitchHelp,
		kSwitchForcePal,
		kSwitchForceShade,
		kSwitchForceTrans,
		kSwitchXRepeat,
		kSwitchYRepeat,
	};
	static SWITCH switches[] = {
		{ "?", kSwitchHelp, FALSE },
		{ "P", kSwitchForcePal, TRUE },
		{ "S", kSwitchForceShade, TRUE },
		{ "T", kSwitchForceTrans, TRUE },
		{ "X", kSwitchXRepeat, TRUE },
		{ "Y", kSwitchYRepeat, TRUE },
		{ NULL, 0, FALSE },
	};
	int value;
	int r;
	while ( (r = GetOptions(switches)) != GO_EOF )
	{
		switch (r)
		{
			case GO_INVALID:
				QuitMessage("Invalid argument: %s", OptArgument);

			case GO_FULL:
				ProcessArgument(OptArgument);
				break;

			case kSwitchForcePal:
				value = strtol(OptArgument, NULL, 0);
				bForcePal = TRUE;
				nPal = (uchar)value;
				break;

			case kSwitchForceShade:
				value = strtol(OptArgument, NULL, 0);
				bForceShade = TRUE;
				nShade = (char)value;
				break;

			case kSwitchForceTrans:
				value = strtol(OptArgument, NULL, 0);
				bForceTranslucent = TRUE;
				switch(value)
				{
					case 0:
						bIsTranslucent = bIsTranslucentR = FALSE;
						break;
					case 1:
						bIsTranslucent = TRUE;
						bIsTranslucentR = FALSE;
						break;
					case 2:
						bIsTranslucent = bIsTranslucentR = TRUE;
						break;
					default:
						QuitMessage("Invalid translucency argument: %s", OptArgument);
						break;
				}
				break;

			case kSwitchXRepeat:
				value = strtol(OptArgument, NULL, 0);
				bForceXRepeat = TRUE;
				xrepeat = (uchar)value;
				break;

			case kSwitchYRepeat:
				value = strtol(OptArgument, NULL, 0);
				bForceYRepeat = TRUE;
				yrepeat = (uchar)value;
				break;
		}
	}
}


void main( int argc )
{
	ShowBanner();

	pSeq = (Seq *)malloc(sizeof(Seq) + kMaxFrames * sizeof(SEQFRAME));
	dassert(pSeq != NULL);

	if (argc < 2) ShowUsage();

	ParseOptions();

	// process the file list
	for (FNODE *n = head.next; n != &head; n = n->next)
		ProcessFile(n->name);
}
