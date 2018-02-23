#include <stdio.h>
#include <string.h>

#include "engine.h"
#include "bstub.h"
#include "build.h"
#include "misc.h"
#include "trig.h"
#include "globals.h"
#include "textio.h"
#include "gameutil.h"
#include "db.h"
#include "debug4g.h"
#include "key.h"
#include "edit2d.h"


enum controlType
{
	TEXT,
	NUMBER,
	CHECKBOX,
	RADIOBUTTON,
	LIST,
	DIALOG
};

typedef struct DIALOG_ITEM *ITEM_PTR;
typedef BYTE ( *ITEM_PROC )( ITEM_PTR, BYTE key );

struct DIALOG_ITEM
{
	int x, y;
	int tabGroup;
	enum controlType type;
	char *label;
	int minValue;
	int maxValue;
	char **names;
	ITEM_PROC fieldHelpProc;
	int value;
};


// dialog helper function prototypes
BYTE GetNextUnusedID( DIALOG_ITEM *control, BYTE key );
BYTE DoSectorFXDialog( DIALOG_ITEM *control, BYTE key );


/*******************************************************************************
								Local Variables
*******************************************************************************/
static char buffer[256] = "";
short sectorhighlight;		// which sector mouse is inside in 2d mode

DIALOG_ITEM dlgSprite[] =
{
	{	0,	0,	1,	LIST,			"Type %4d: %-18.18s", 0, 1023, gSpriteNames },	// 0
	{ 	0,	1,	2,	NUMBER, 		"RX ID: %4d", 0, 1023, NULL, GetNextUnusedID },	// 1
	{ 	0,	2,	3,	NUMBER, 		"TX ID: %4d", 0, 1023, NULL, GetNextUnusedID },	// 2
	{ 	0,	3,	4,	LIST, 			"State %1d: %-3.3s", 0, 1, gBoolNames },		// 3
	{ 	0,	4,	5,	LIST, 			"Cmd: %3d: %-6.6s", 0, 255, gCommandNames },	// 4

	{	0,	5,	0,	TEXT, 			"Send when:" },									// 5
	{	0,	6,	6,	CHECKBOX,		"going ON" },									// 6
	{	0,	7,	7,	CHECKBOX,		"going OFF" },                                	// 7
	{	0,	8,	8,	NUMBER, 		"busyTime = %4d", 0, 4095 },                    // 8
	{	0,	9,	9,	NUMBER, 		"waitTime = %4d", 0, 4095 },                   	// 9
	{	0,	10,	10,	LIST,			"restState %1d: %-3.3s", 0, 1, gBoolNames },   	// 10

	{	30,	0,	0,	TEXT,			"Trigger On:" },								// 11
	{	30,	1,	11,	CHECKBOX,		"Push" },                                     	// 12
	{	30,	2,	12,	CHECKBOX,		"Impact" },                                   	// 13
	{	30,	3,	13,	CHECKBOX,		"Reserved" },                                  	// 14
	{	30,	4,	14,	CHECKBOX,		"Pickup" },                                   	// 15
	{	30,	5,	15,	CHECKBOX, 		"Touch" },                                    	// 16
	{	30,	6,	16,	CHECKBOX, 		"Sight" },                                    	// 17
 	{	30,	7,	17,	CHECKBOX, 		"Proximity" },                                	// 18

	{	46,	0,	0,	TEXT, 			"Trigger Flags:" },                           	// 19
	{	46,	1,	18,	CHECKBOX, 		"Decoupled" },                                	// 20
	{	46,	2,	19,	CHECKBOX,		"1-shot" },                                   	// 21
	{	46,	3,	20,	CHECKBOX,		"Locked" },                                   	// 22
	{	46,	4,	21,	CHECKBOX,		"Interruptable" },								// 23

	{ 	46,	5,	22,	NUMBER, 		"Data1: %5d", 0, 65535 },						// 24
	{ 	46,	6,	23,	NUMBER, 		"Data2: %5d", 0, 65535 },						// 25
	{ 	46,	7,	24,	NUMBER, 		"Data3: %5d", 0, 65535 },						// 26
	{ 	46,	8,	25,	NUMBER,			"Key: %1d", 0, 7 },                           	// 27
	{ 	46,	9,	26,	NUMBER,			"Sound: %3d", 0, 255 },							// 28
	{ 	46,	10,	27,	NUMBER,			"Difficulty: %1d", 0, 3 },                    	// 29

	{ 	62,	0,	0,	TEXT, 			"Respawn:" },									// 30
	{ 	62,	1,	28,	LIST, 			"When %1d: %-6.6s", 0, 3, gRespawnNames },		// 31
	{	62,	2,	29,	NUMBER, 		"Time = %4d", 0, 4095 },						// 32

	{	62,	4,	0,	TEXT, 			"Dude Flags:" },								// 33
	{	62,	5,	30,	CHECKBOX, 		"dudeDeaf" },									// 34
	{	62,	6,	31,	CHECKBOX,		"dudeAmbush" },									// 35
	{	62,	7,	32,	CHECKBOX,		"dudeGuard" },									// 36
	{	62,	8,	33,	CHECKBOX,		"reserved" },									// 37
};


DIALOG_ITEM dlgWall[] =
{
	{	0,	0,	1,	LIST, 			"Type %4d: %-18.18s", 0, 1023, gWallNames },	// 0
	{ 	0,	1,	2,	NUMBER, 		"RX ID: %4d", 0, 1023, NULL, GetNextUnusedID },	// 1
	{ 	0,	2,	3,	NUMBER, 		"TX ID: %4d", 0, 1023, NULL, GetNextUnusedID },	// 2
	{ 	0,	3,	4,	LIST, 			"State %1d: %-3.3s", 0, 1, gBoolNames },		// 3
	{ 	0,	4,	5,	LIST, 			"Cmd: %3d: %-6.6s", 0, 255, gCommandNames },	// 4

	{	0,	5,	0,	TEXT, 			"Send when:" },									// 5
	{	0,	6,	6,	CHECKBOX,		"going ON" },									// 6
	{	0,	7,	7,	CHECKBOX,		"going OFF" },									// 7
	{	0,	8,	8,	NUMBER, 		"busyTime = %4d", 0, 4095 },					// 8
	{	0,	9,	9,	NUMBER, 		"waitTime = %4d", 0, 4095 },					// 9
	{	0,	10,	10,	LIST,			"restState %1d: %-3.3s", 0, 1, gBoolNames },	// 10

	{	30,	0,	0,	TEXT,			"Trigger On:" },								// 11
	{	30,	1,	11,	CHECKBOX,		"Push" },										// 12
	{	30,	2,	12,	CHECKBOX,		"Impact" },										// 13
	{	30,	3,	13,	CHECKBOX,		"Reserved" },									// 14

	{	46,	0,	0,	TEXT, 			"Trigger Flags:" },								// 15
	{	46,	1,	14,	CHECKBOX, 		"Decoupled" },									// 16
	{	46,	2,	15,	CHECKBOX,		"1-shot" },										// 17
	{	46,	3,	16,	CHECKBOX, 		"Locked" },										// 18
	{	46,	4,	17,	CHECKBOX,		"Interruptable" },								// 19

	{ 	46,	6,	18,	NUMBER, 		"Data: %5d", 0, 65535 },						// 20
	{ 	46,	7,	19,	NUMBER,			"Key: %1d", 0, 7 },								// 21
	{	46,	8,	20,	NUMBER, 		"panX = %4d", -128, 127 },						// 22
	{	46,	9,	21,	NUMBER, 		"panY = %4d", -128, 127 },						// 23
	{	46,	10,	22,	CHECKBOX,		"panAlways" },									// 24
};


DIALOG_ITEM dlgSectorFX[] =
{
	{	0,	0,	0,	TEXT, 			"Lighting:" },                                           // 0
	{	0,	1,	1,	LIST, 			"Wave: %1d %-9.9s", 0, 15, gWaveNames },														// 1
	{	0,	2,	2,	NUMBER, 		"Amplitude: %+4d", -128, 127 },                          // 2
	{	0,	3,	3,	NUMBER, 		"Freq:    %3d", 0, 255 },                                // 3
	{	0,	4,	4,	NUMBER, 		"Phase:     %3d", 0, 255 },                              // 4
	{	0,	5,	5,	CHECKBOX,		"floor" },                                               // 5
	{	0,	6,	6,	CHECKBOX,		"ceiling" },                                             // 6
	{	0,	7,	7,	CHECKBOX,		"walls" },                                               // 7
	{	0,	8,	8,	CHECKBOX,		"shadeAlways" },                                         // 8

	{	20,	0,	0,	TEXT, 			"Motion FX:" },                                          // 9
	{	20,	1,	9,	NUMBER, 		"Speed = %4d", 0, 255 },                                 // 10
	{	20,	2,	10,	NUMBER, 		"Angle = %4d", 0, kAngle360-1 },                         // 11
	{	20,	3,	11,	CHECKBOX,		"pan floor" },                                           // 12
	{	20,	4,	12,	CHECKBOX,		"pan ceiling" },                                         // 13
	{	20,	5,	13,	CHECKBOX,		"panAlways" },                                           // 14
	{	20,	6,	14,	CHECKBOX,		"drag" },                                                // 15

	{	35,	2,	15,	LIST, 	   		"Depth = %1d: %-5.5s", 0, 3, gDepthNames },               // 16
	{	35,	3,	16,	LIST, 			"Under = %1d: %-3.3s", 0, 1, gBoolNames },                // 17
	{	35,	4,	17,	LIST,			"Wind  = %1d: %-3.3s", 0, 1, gBoolNames},                 // 18
};


DIALOG_ITEM dlgSector[] =
{
	{	0,	0,	1,	LIST, 		"Type %4d: %-18.18s", 0, 1023, gSectorNames },		// 0
	{ 	0,	1,	2,	NUMBER, 	"RX ID: %4d", 0, 1023, NULL, GetNextUnusedID },		// 1
	{ 	0,	2,	3,	NUMBER, 	"TX ID: %4d", 0, 1023, NULL, GetNextUnusedID },		// 2
	{ 	0,	3,	4,	LIST, 		"State %1d: %-3.3s", 0, 1, gBoolNames },			// 3
	{ 	0,	4,	5,	LIST, 		"Cmd: %3d: %-6.6s", 0, 255, gCommandNames },		// 4

	{	0,	5,	0,	TEXT, 		"Send when:" },										// 5
	{	0,	6,	6,	CHECKBOX,	"going ON" },										// 6
	{	0,	7,	7,	CHECKBOX,	"going OFF" },										// 7
	{	0,	8,	8,	NUMBER, 	"busyTime = %3d", 0, 255 },							// 8
	{	0,	9,	9,	NUMBER, 	"waitTime = %3d", 0, 255 },							// 9
	{	0,	10,	10,	LIST,		"restState %1d: %-3.3s", 0, 1, gBoolNames },		// 10

	{	30,	0,	0,	TEXT, 		"Trigger On:" },									// 11
	{	30,	1,	11,	CHECKBOX,	"Push" },											// 12
	{	30,	2,	12,	CHECKBOX,	"Impact" },                                     	// 13
	{	30,	3,	13,	CHECKBOX,	"Reserved" },										// 14
	{	30,	4,	14,	CHECKBOX,	"Enter" },                                      	// 15
	{	30,	5,	15,	CHECKBOX,	"Exit" },                                       	// 16
	{	30,	6,	16,	CHECKBOX,	"WallPush" },                                   	// 17

	{	46,	0,	0,	TEXT, 		"Trigger Flags:" },									// 18
	{	46,	1,	17,	CHECKBOX, 	"Decoupled" },										// 19
	{	46,	2,	18,	CHECKBOX,	"1-shot" },											// 20
	{	46,	3,	19,	CHECKBOX, 	"Locked" },											// 21
	{	46,	4,	20,	CHECKBOX, 	"Interruptable" },									// 22

	{ 	46,	6,	21,	NUMBER, 	"Data: %5d", 0, 65535 },							// 23
	{ 	46,	7,	22,	NUMBER,		"Key: %1d", 0, 7 },									// 24

	{	65,	10,	23,	DIALOG,		"EFFECTS...",  0, 0, NULL, DoSectorFXDialog },		// 25
};



void DialogText( int x, int y, short nFore, short nBack, char *string)
{
	printext16(x * 8 + 4, y * 8 + 28, nFore, nBack, string, 0);
}

/***********************************************************************
 * PaintDialogItem()
 *
 **********************************************************************/
void PaintDialogItem(DIALOG_ITEM *control, int focus)
{
	short foreColor = 11, backColor = 8;

	if ( focus )
	{
		foreColor = 14;
		backColor = 0;
	}

	if (control->tabGroup == 0)
	{
		foreColor = 15;
		backColor = 2;
	}

	switch ( control->type )
	{
		case TEXT:
		case DIALOG:
			DialogText(control->x, control->y, foreColor, backColor, control->label);
			break;

		case NUMBER:
			sprintf(buffer, control->label, control->value);
			DialogText(control->x, control->y, foreColor, backColor, buffer);
			break;

		case LIST:
			dassert(control->names != NULL);
			sprintf(buffer, control->label, control->value, control->names[control->value]);
			DialogText(control->x, control->y, foreColor, backColor, buffer);
			break;

		case CHECKBOX:
			sprintf(buffer, "%c %s", control->value ? 3 : 2, control->label);
			DialogText(control->x, control->y, foreColor, backColor, buffer);
			break;

		case RADIOBUTTON:
			sprintf(buffer, "%c %s", control->value ? 5 : 4, control->label);
			DialogText(control->x, control->y, foreColor, backColor, buffer);
			break;
	}
}


/***********************************************************************
 * PaintDialog()
 *
 **********************************************************************/
void PaintDialog(DIALOG_ITEM *dialog, int count)
{
	int i;

	clearmidstatbar16();
	for ( i = 0; i < count; i++ )
	{
		PaintDialogItem(&dialog[i], kFalse);
	}
}


/***********************************************************************
 * FindGroup()
 *
 **********************************************************************/
DIALOG_ITEM *FindGroup(DIALOG_ITEM *dialog, int count, int group)
{
	DIALOG_ITEM *control;

	for ( control = dialog; control < &dialog[count]; control++)
	{
		if ( control->tabGroup == group )
			break;
	}
	dassert(control < &dialog[count]);

	if ( control->type == RADIOBUTTON )
	{
		while ( control->value == kFalse )
		{
			control++;
			dassert(control < &dialog[count]);
			dassert(control->type == RADIOBUTTON);
		}
	}

	return control;
}


/***********************************************************************
 * SetControlValue()
 *
 **********************************************************************/
void SetControlValue(DIALOG_ITEM *dialog, int count, int group, int value )
{
	int i;

	for ( i = 0; i < count; i++)
	{
		if ( dialog[i].tabGroup == group )
			break;
	}
	dassert(i < count);

	dialog[i].value = value;
}


/***********************************************************************
 * GetControlValue()
 *
 **********************************************************************/
int GetControlValue(DIALOG_ITEM *dialog, int count, int group )
{
	int i;
	int value = 0;

	for ( i = 0; i < count; i++)
	{
		if ( dialog[i].tabGroup == group )
			break;
	}
	dassert(i < count);

	return dialog[i].value;
}


/***********************************************************************
 * SetRadioButton()
 *
 **********************************************************************/
void SetRadioButton(DIALOG_ITEM *dialog, int count, int group, int value )
{
	int i;

	for ( i = 0; i < count; i++)
	{
		if ( dialog[i].tabGroup == group )
			break;
	}
	dassert(i < count);

	while ( i < count && dialog[i].tabGroup == group )
	{
		dialog[i].value = (value == 0);
		i++;
		value--;
	}
	dassert(value < 0);
}


/***********************************************************************
 * GetRadioButton()
 *
 **********************************************************************/
int GetRadioButton(DIALOG_ITEM *dialog, int count, int group )
{
	int i;
	int value = 0;

	for ( i = 0; i < count; i++)
	{
		if ( dialog[i].tabGroup == group )
			break;
	}
	dassert(i < count);

	while ( i < count )
	{
		dassert(dialog[i].type == RADIOBUTTON);
		if ( dialog[i].value )
			break;
		i++;
		value++;
	}
	dassert(i < count);
	return value;
}


/*******************************************************************************
	FUNCTION:		EditDialog()

	DESCRIPTION:	This is the editing event loop for dialogs.

	PARAMETERS:		dialog is a pointer to the first control, count is the
					number of controls within the dialog.

	RETURNS:        1 if exited via Enter, 0 if exited with Esc
*******************************************************************************/
int EditDialog(DIALOG_ITEM *dialog, int count)
{
	int group = 1, lastGroup = 0;
	int i, value, oldValue;
	DIALOG_ITEM *control;
	BYTE key = 0;

	keyFlushStream();

	PaintDialog(dialog, count);

	for ( i = 0; i < count; i++ )
	{
		if ( dialog[i].tabGroup > lastGroup )
			lastGroup = dialog[i].tabGroup;
	}

	control = FindGroup(dialog, count, group);

	while ( 1 )
	{
		PaintDialogItem(control, kTrue);

		do
		{
			key = keyGet();
		} while ( key == 0 );

		if ( control->fieldHelpProc )
		{
			key = control->fieldHelpProc(control, key);

			// may need to repaint all in case of subdialogs
			PaintDialog(dialog, count);
		}

		switch ( key )
		{
			case KEY_ENTER:
			case KEY_PADENTER:
				keystatus[key] = 0;		// need to clear in 2D mode, got it?
				BeepOkay();
				return 1;

			case KEY_ESC:
				keystatus[key] = 0;
				return 0;

			case KEY_LEFT:
				if ( --group == 0 )
					group = lastGroup;
				PaintDialogItem(control, kFalse);
				control = FindGroup(dialog, count, group);
				break;

			case KEY_RIGHT:
				if ( ++group > lastGroup )
					group = 1;
				PaintDialogItem(control, kFalse);
				control = FindGroup(dialog, count, group);
				break;

			case KEY_TAB:
				if ( keystatus[KEY_LSHIFT] || keystatus[KEY_RSHIFT] )
				{
					if ( --group == 0 )
						group = lastGroup;
					PaintDialogItem(control, kFalse);
					control = FindGroup(dialog, count, group);
					break;
				}
				else
				{
					if ( ++group > lastGroup )
						group = 1;
					PaintDialogItem(control, kFalse);
					control = FindGroup(dialog, count, group);
					break;
				}

			default:
				switch ( control->type )
				{
					case NUMBER:

						oldValue = control->value;

						switch ( key )
						{
							case KEY_BACKSPACE:
								control->value /= 10;
								break;

							case KEY_PADPLUS:
							case KEY_UP:
								control->value++;
								break;

							case KEY_PADMINUS:
							case KEY_DOWN:
								control->value--;
								break;

							case KEY_PAGEUP:
								control->value = IncBy(control->value, 10);
								break;

							case KEY_PAGEDN:
								control->value = DecBy(control->value, 10);
								break;

							default:
								key = ScanToAscii[key];
								if ( key >= '0' && key <= '9' )
									control->value = control->value * 10 + key - '0';
						}

						if ( control->value > control->maxValue )
							control->value = control->maxValue;

						if ( control->value < control->minValue )
							control->value = control->minValue;

						break;

					case CHECKBOX:
						switch ( key )
						{
							case KEY_SPACE:
								control->value = !control->value;
								break;
						}
						break;

					case RADIOBUTTON:
						PaintDialogItem(control, kFalse);
						switch ( key )
						{
							case KEY_UP:
								control->value = kFalse;
								PaintDialogItem(control, kFalse);
								do
								{
									if (--control < dialog)
										control = &dialog[count - 1];

								} while ( control->tabGroup != group );
								control->value = kTrue;
								break;

							case KEY_DOWN:
								control->value = kFalse;
								PaintDialogItem(control, kFalse);
								do
								{
									if (++control == &dialog[count])
										control = dialog;
								} while ( control->tabGroup != group );
								control->value = kTrue;
								break;

						}
						break;

					case LIST:

						oldValue = control->value;
						value = control->value;

						switch ( key )
						{
							case KEY_PADPLUS:
							case KEY_UP:
								do
								{
									value++;
								} while ( value <= control->maxValue && control->names[value] == NULL );
								break;

							case KEY_PADMINUS:
							case KEY_DOWN:
								do
								{
									value--;
								} while ( value >= control->minValue && control->names[value] == NULL );
								break;

							case KEY_PAGEUP:
								do
								{
									value = IncBy(value, 10);
								} while ( value <= control->maxValue && control->names[value] == NULL );
								break;

							case KEY_PAGEDN:
								do
								{
									value = DecBy(value, 10);
								} while ( value >= control->minValue && control->names[value] == NULL );
								break;
						}

						if ( value >= control->minValue && value <= control->maxValue && control->names[value] != NULL )
							control->value = value;

						break;
				}
		}
	}
}


BYTE GetNextUnusedID( DIALOG_ITEM *control, BYTE key )
{
	BOOL used[1024];
	int i;

	switch ( key )
	{
		case KEY_F10:
			memset(used, FALSE, sizeof(used));

			for (i = 0; i < numsectors; i++)
			{
				int nXSector = sector[i].extra;
				if ( nXSector > 0 )
				{
					used[xsector[nXSector].txID] = TRUE;
					used[xsector[nXSector].rxID] = TRUE;
				}
			}

			for (i = 0; i < numwalls; i++)
			{
				int nXWall = wall[i].extra;
				if ( nXWall > 0 )
				{
					used[xwall[nXWall].txID] = TRUE;
					used[xwall[nXWall].rxID] = TRUE;
				}
			}

			for (i = 0; i < kMaxSprites; i++)
			{
				if ( sprite[i].statnum < kMaxStatus )
				{
					int nXSprite = sprite[i].extra;
					if ( nXSprite > 0 )
					{
						used[xsprite[nXSprite].txID] = TRUE;
						used[xsprite[nXSprite].rxID] = TRUE;
					}
				}
			}

			// find the first unused id
			for (i = 100; i < 1024; i++)
				if ( !used[i] )
					break;

			control->value = i;
			return 0;
	}

	return key;
}


BYTE DoSectorFXDialog( DIALOG_ITEM * /* control */, BYTE key )
{
	switch ( key )
	{
		case KEY_ENTER:
			if ( EditDialog(dlgSectorFX, LENGTH(dlgSectorFX)) )
				return KEY_ENTER;
			return 0;
	}

	return key;
}


/***********************************************************************
 * XWallToDialog()
 *
 **********************************************************************/
void XWallToDialog( int nWall )
{
	int nXWall = wall[nWall].extra;
	dassert(nXWall > 0 && nXWall < kMaxXWalls);

	dlgWall[0].value = wall[nWall].type;
	dlgWall[1].value = xwall[nXWall].rxID;
	dlgWall[2].value = xwall[nXWall].txID;
	dlgWall[3].value = xwall[nXWall].state;
	dlgWall[4].value = xwall[nXWall].command;

	dlgWall[6].value = xwall[nXWall].triggerOn;
	dlgWall[7].value = xwall[nXWall].triggerOff;
	dlgWall[8].value = xwall[nXWall].busyTime;
	dlgWall[9].value = xwall[nXWall].waitTime;
	dlgWall[10].value = xwall[nXWall].restState;

	dlgWall[12].value = xwall[nXWall].triggerPush;
	dlgWall[13].value = xwall[nXWall].triggerImpact;
	dlgWall[14].value = xwall[nXWall].triggerReserved0;

	dlgWall[16].value = xwall[nXWall].decoupled;
	dlgWall[17].value = xwall[nXWall].triggerOnce;
	dlgWall[18].value = xwall[nXWall].locked;
	dlgWall[19].value = xwall[nXWall].interruptable;

	dlgWall[20].value = xwall[nXWall].data;
	dlgWall[21].value = xwall[nXWall].key;
	dlgWall[22].value = xwall[nXWall].panXVel;
	dlgWall[23].value = xwall[nXWall].panYVel;
	dlgWall[24].value = xwall[nXWall].panAlways;
}


/***********************************************************************
 * DialogToXWall()
 *
 **********************************************************************/
void DialogToXWall( int nWall )
{
	int nXWall = wall[nWall].extra;
	dassert(nXWall > 0 && nXWall < kMaxXWalls);

	wall[nWall].type=				(short)dlgWall[0].value;
	xwall[nXWall].rxID=				dlgWall[1].value;
	xwall[nXWall].txID=             dlgWall[2].value;
	xwall[nXWall].state=            dlgWall[3].value;
	xwall[nXWall].command=          dlgWall[4].value;

	xwall[nXWall].triggerOn=        dlgWall[6].value;
	xwall[nXWall].triggerOff=       dlgWall[7].value;
	xwall[nXWall].busyTime=         dlgWall[8].value;
	xwall[nXWall].waitTime=         dlgWall[9].value;
	xwall[nXWall].restState=        dlgWall[10].value;

	xwall[nXWall].triggerPush=		dlgWall[12].value;
	xwall[nXWall].triggerImpact=    dlgWall[13].value;
	xwall[nXWall].triggerReserved0=	dlgWall[14].value;

	xwall[nXWall].decoupled=        dlgWall[16].value;
	xwall[nXWall].triggerOnce=      dlgWall[17].value;
	xwall[nXWall].locked=           dlgWall[18].value;
	xwall[nXWall].interruptable=	dlgWall[19].value;

	xwall[nXWall].data=             dlgWall[20].value;
	xwall[nXWall].key=              dlgWall[21].value;
	xwall[nXWall].panXVel=          dlgWall[22].value;
	xwall[nXWall].panYVel=          dlgWall[23].value;
	xwall[nXWall].panAlways=        dlgWall[24].value;
}

/***********************************************************************
 * XSpriteToDialog()
 *
 **********************************************************************/
void XSpriteToDialog( int nSprite )
{
	int nXSprite = sprite[nSprite].extra;
	dassert(nXSprite > 0 && nXSprite < kMaxXSprites);

	dlgSprite[0].value = sprite[nSprite].type;
	dlgSprite[1].value = xsprite[nXSprite].rxID;
	dlgSprite[2].value = xsprite[nXSprite].txID;
	dlgSprite[3].value = xsprite[nXSprite].state;
	dlgSprite[4].value = xsprite[nXSprite].command;

	dlgSprite[6].value = xsprite[nXSprite].triggerOn;
	dlgSprite[7].value = xsprite[nXSprite].triggerOff;
	dlgSprite[8].value = xsprite[nXSprite].busyTime;
	dlgSprite[9].value = xsprite[nXSprite].waitTime;
	dlgSprite[10].value = xsprite[nXSprite].restState;

	dlgSprite[12].value = xsprite[nXSprite].triggerPush;
	dlgSprite[13].value = xsprite[nXSprite].triggerImpact;
	dlgSprite[14].value = xsprite[nXSprite].triggerReserved0;
	dlgSprite[15].value = xsprite[nXSprite].triggerPickup;
 	dlgSprite[16].value = xsprite[nXSprite].triggerTouch;
 	dlgSprite[17].value = xsprite[nXSprite].triggerSight;
 	dlgSprite[18].value = xsprite[nXSprite].triggerProximity;

	dlgSprite[20].value = xsprite[nXSprite].decoupled;
	dlgSprite[21].value = xsprite[nXSprite].triggerOnce;
	dlgSprite[22].value = xsprite[nXSprite].locked;
	dlgSprite[23].value = xsprite[nXSprite].interruptable;

	dlgSprite[24].value = xsprite[nXSprite].data1;
	dlgSprite[25].value = xsprite[nXSprite].data2;
	dlgSprite[26].value = xsprite[nXSprite].data3;
	dlgSprite[27].value = xsprite[nXSprite].key;
	dlgSprite[28].value = xsprite[nXSprite].soundId;
	dlgSprite[29].value = xsprite[nXSprite].difficulty;

	dlgSprite[31].value = xsprite[nXSprite].respawn;
	dlgSprite[32].value = xsprite[nXSprite].respawnTime;

	dlgSprite[34].value = xsprite[nXSprite].dudeDeaf;
	dlgSprite[35].value = xsprite[nXSprite].dudeAmbush;
	dlgSprite[36].value = xsprite[nXSprite].dudeGuard;
	dlgSprite[37].value = xsprite[nXSprite].dudeFlag4;
}


/***********************************************************************
 * DialogToXSprite()
 *
 **********************************************************************/
void DialogToXSprite( int nSprite )
{
	int nXSprite = sprite[nSprite].extra;
	dassert(nXSprite > 0 && nXSprite < kMaxXSprites);

	sprite[nSprite].type		= (short)dlgSprite[0].value;
	xsprite[nXSprite].rxID		= dlgSprite[1].value;
	xsprite[nXSprite].txID		= dlgSprite[2].value;
	xsprite[nXSprite].state		= dlgSprite[3].value;
	xsprite[nXSprite].command	= dlgSprite[4].value;

	xsprite[nXSprite].triggerOn		= dlgSprite[6].value;
	xsprite[nXSprite].triggerOff	= dlgSprite[7].value;
	xsprite[nXSprite].busyTime		= dlgSprite[8].value;
	xsprite[nXSprite].waitTime		= dlgSprite[9].value;
	xsprite[nXSprite].restState		= dlgSprite[10].value;

	xsprite[nXSprite].triggerPush		= dlgSprite[12].value;
	xsprite[nXSprite].triggerImpact		= dlgSprite[13].value;
	xsprite[nXSprite].triggerReserved0	= dlgSprite[14].value;
	xsprite[nXSprite].triggerPickup		= dlgSprite[15].value;
 	xsprite[nXSprite].triggerTouch		= dlgSprite[16].value;
 	xsprite[nXSprite].triggerSight		= dlgSprite[17].value;
 	xsprite[nXSprite].triggerProximity	= dlgSprite[18].value;

	xsprite[nXSprite].decoupled			= dlgSprite[20].value;
	xsprite[nXSprite].triggerOnce		= dlgSprite[21].value;
	xsprite[nXSprite].locked			= dlgSprite[22].value;
	xsprite[nXSprite].interruptable		= dlgSprite[23].value;

	xsprite[nXSprite].data1			= dlgSprite[24].value;
	xsprite[nXSprite].data2			= dlgSprite[25].value;
	xsprite[nXSprite].data3			= dlgSprite[26].value;
	xsprite[nXSprite].key			= dlgSprite[27].value;
	xsprite[nXSprite].soundId		= dlgSprite[28].value;
	xsprite[nXSprite].difficulty	= dlgSprite[29].value;

	xsprite[nXSprite].respawn		= dlgSprite[31].value;
	xsprite[nXSprite].respawnTime	= dlgSprite[32].value;

	xsprite[nXSprite].dudeDeaf=		dlgSprite[34].value;
	xsprite[nXSprite].dudeAmbush=   dlgSprite[35].value;
	xsprite[nXSprite].dudeGuard=    dlgSprite[36].value;
	xsprite[nXSprite].dudeFlag4=    dlgSprite[37].value;
}


/***********************************************************************
 * XSectorToDialog()
 *
 **********************************************************************/
void XSectorToDialog( int nSector )
{
	int nXSector = sector[nSector].extra;
	dassert(nXSector > 0 && nXSector < kMaxXSectors);

	dlgSector[0].value = (short)sector[nSector].type;
	dlgSector[1].value = xsector[nXSector].rxID;
	dlgSector[2].value = xsector[nXSector].txID;
	dlgSector[3].value = xsector[nXSector].state;
	dlgSector[4].value = xsector[nXSector].command;

	dlgSector[6].value = xsector[nXSector].triggerOn;
	dlgSector[7].value = xsector[nXSector].triggerOff;
	dlgSector[8].value = xsector[nXSector].busyTime;
	dlgSector[9].value = xsector[nXSector].waitTime;
	dlgSector[10].value = xsector[nXSector].restState;

	dlgSector[12].value = xsector[nXSector].triggerPush;
	dlgSector[13].value = xsector[nXSector].triggerImpact;
	dlgSector[14].value = xsector[nXSector].triggerReserved0;
	dlgSector[15].value = xsector[nXSector].triggerEnter;
	dlgSector[16].value = xsector[nXSector].triggerExit;
	dlgSector[17].value = xsector[nXSector].triggerWPush;

	dlgSector[19].value = xsector[nXSector].decoupled;
	dlgSector[20].value = xsector[nXSector].triggerOnce;
	dlgSector[21].value = xsector[nXSector].locked;
	dlgSector[22].value = xsector[nXSector].interruptable;

	dlgSector[23].value = xsector[nXSector].data;
	dlgSector[24].value = xsector[nXSector].key;

	dlgSectorFX[1].value = xsector[nXSector].wave;
	dlgSectorFX[2].value = xsector[nXSector].amplitude;
	dlgSectorFX[3].value = xsector[nXSector].freq;
	dlgSectorFX[4].value = xsector[nXSector].phase;
	dlgSectorFX[5].value = xsector[nXSector].shadeFloor;
	dlgSectorFX[6].value = xsector[nXSector].shadeCeiling;
	dlgSectorFX[7].value = xsector[nXSector].shadeWalls;
	dlgSectorFX[8].value = xsector[nXSector].shadeAlways;

	dlgSectorFX[10].value = xsector[nXSector].panVel;
	dlgSectorFX[11].value = xsector[nXSector].panAngle;
	dlgSectorFX[12].value = xsector[nXSector].panFloor;
	dlgSectorFX[13].value = xsector[nXSector].panCeiling;
	dlgSectorFX[14].value = xsector[nXSector].panAlways;
	dlgSectorFX[15].value = xsector[nXSector].drag;

	dlgSectorFX[16].value = xsector[nXSector].depth;
	dlgSectorFX[17].value = xsector[nXSector].underwater;
	dlgSectorFX[18].value = xsector[nXSector].wind;
}


/***********************************************************************
 * DialogToXSector()
 *
 **********************************************************************/
void DialogToXSector( int nSector )
{
	int nXSector = sector[nSector].extra;
	dassert(nXSector > 0 && nXSector < kMaxXSectors);

	sector[nSector].type = (short)dlgSector[0].value;
	xsector[nXSector].rxID = dlgSector[1].value;
	xsector[nXSector].txID = dlgSector[2].value;
	xsector[nXSector].state = dlgSector[3].value;
	xsector[nXSector].command = dlgSector[4].value;

	xsector[nXSector].triggerOn  = dlgSector[6].value;
	xsector[nXSector].triggerOff = dlgSector[7].value;
	xsector[nXSector].busyTime = dlgSector[8].value;
	xsector[nXSector].waitTime = dlgSector[9].value;
	xsector[nXSector].restState = dlgSector[10].value;

	xsector[nXSector].triggerPush		= dlgSector[12].value;
	xsector[nXSector].triggerImpact		= dlgSector[13].value;
	xsector[nXSector].triggerReserved0	= dlgSector[14].value;
	xsector[nXSector].triggerEnter		= dlgSector[15].value;
	xsector[nXSector].triggerExit		= dlgSector[16].value;
	xsector[nXSector].triggerWPush		= dlgSector[17].value;

	xsector[nXSector].decoupled			= dlgSector[19].value;
	xsector[nXSector].triggerOnce		= dlgSector[20].value;
	xsector[nXSector].locked			= dlgSector[21].value;
	xsector[nXSector].interruptable		= dlgSector[22].value;

	xsector[nXSector].data				= dlgSector[23].value;
	xsector[nXSector].key				= dlgSector[24].value;

	xsector[nXSector].wave          = dlgSectorFX[1].value;
	xsector[nXSector].amplitude     = dlgSectorFX[2].value;
	xsector[nXSector].freq        	= dlgSectorFX[3].value;
	xsector[nXSector].phase         = dlgSectorFX[4].value;
	xsector[nXSector].shadeFloor    = dlgSectorFX[5].value;
	xsector[nXSector].shadeCeiling  = dlgSectorFX[6].value;
	xsector[nXSector].shadeWalls    = dlgSectorFX[7].value;
	xsector[nXSector].shadeAlways   = dlgSectorFX[8].value;

	xsector[nXSector].panVel		= dlgSectorFX[10].value;
	xsector[nXSector].panAngle		= dlgSectorFX[11].value;
	xsector[nXSector].panFloor		= dlgSectorFX[12].value;
	xsector[nXSector].panCeiling	= dlgSectorFX[13].value;
	xsector[nXSector].panAlways		= dlgSectorFX[14].value;
	xsector[nXSector].drag			= dlgSectorFX[15].value;

	xsector[nXSector].depth			= dlgSectorFX[16].value;
	xsector[nXSector].underwater	= dlgSectorFX[17].value;
	xsector[nXSector].wind			= dlgSectorFX[18].value;
}


int getpointhighlight(int x, int y)
{
	long i, d, closest, n;

	closest = divscale(gHighlightThreshold, zoom, 14);

	n = -1;

	for (i = 0; i < kMaxSprites; i++)
	{
		if (sprite[i].statnum < kMaxStatus)
		{
			d = qdist(x - sprite[i].x, y - sprite[i].y);
			if (d < closest)
			{
				closest = d;
				n = i | 0x4000;
			}
		}
	}

	for (i = 0; i < numwalls; i++)
	{
		d = qdist(x - wall[i].x, y - wall[i].y);
		if (d < closest)
		{
			closest = d;
			n = i;
		}
	}

	return n;
}


/***********************************************************************
 * ShowSectorData()
 *
 * This function displays the game specific sector data in the status
 * area.
 **********************************************************************/
void ShowSectorData( int nSector )
{
	int nXSector;

	dassert(nSector >= 0 && nSector < kMaxSectors);

	sprintf(buffer, "Sector %d", nSector);
	printmessage16(buffer);

	if (sector[nSector].extra != -1)
	{
		nXSector = sector[nSector].extra;
		dassert(nXSector < kMaxXSectors);

		XSectorToDialog(nSector);
		PaintDialog(dlgSector, LENGTH(dlgSector));
	}
	else
		clearmidstatbar16();
}


/***********************************************************************
 * EditSectorData()
 *
 * This function allows editing of the game specific sector data.
 **********************************************************************/
void EditSectorData( int nSector )
{
	int nXSector;

	dassert(nSector >= 0 && nSector < kMaxSectors);

	CleanUp();

	sprintf(buffer, "Sector %d", nSector);
	printmessage16(buffer);

	nXSector = GetXSector(nSector);
	XSectorToDialog(nSector);
	if ( EditDialog(dlgSector, LENGTH(dlgSector)) )
		DialogToXSector(nSector);

	ShowSectorData(nSector);
	vel = svel = angvel = 0;
	CleanUp();
}


/***********************************************************************
 * ShowWallData()
 *
 * This function displays the game specific wall data in the status
 * area.
 **********************************************************************/
void ShowWallData( int nWall )
{
	int nXWall;

	dassert(nWall >= 0 && nWall < kMaxWalls);
	int nWall2 = wall[nWall].point2;
	int length = qdist(wall[nWall2].x - wall[nWall].x, wall[nWall2].y - wall[nWall].y);

	sprintf(buffer, "Wall %d:  Length = %d", nWall, length);
	printmessage16(buffer);

	if (wall[nWall].extra != -1)
	{
		nXWall = wall[nWall].extra;
		dassert(nXWall < kMaxXWalls);

		XWallToDialog(nWall);
		PaintDialog(dlgWall, LENGTH(dlgWall));
	}
	else
	{
		clearmidstatbar16();
	}
}


/***********************************************************************
 * EditWallData()
 *
 * This function allows editing of the game specific wall data.
 **********************************************************************/
void EditWallData( int nWall )
{
	int nXWall;

	dassert(nWall >= 0 && nWall < kMaxWalls);

	CleanUp();

	sprintf(buffer, "Wall %d", nWall);
	printmessage16(buffer);

	nXWall = GetXWall(nWall);

	XWallToDialog(nWall);

	if ( EditDialog(dlgWall, LENGTH(dlgWall)) )
		DialogToXWall(nWall);

	ShowWallData(nWall);
	vel = svel = angvel = 0;
	CleanUp();
}


/***********************************************************************
 * ShowSpriteData()
 *
 * This function displays the game specific sprite data in the status
 * area.
 **********************************************************************/
void ShowSpriteData( int nSprite )
{
	int nXSprite;

	dassert(nSprite >= 0 && nSprite < kMaxSprites);

	if (sprite[nSprite].extra > 0)
	{
		nXSprite = sprite[nSprite].extra;
		sprintf(buffer, "Sprite %d  Extra %d XRef %d",
			nSprite, nXSprite, xsprite[nXSprite].reference);
	}
	else
	{
		sprintf(buffer, "Sprite %d", nSprite);
	}

	printmessage16(buffer);

	if ( sprite[nSprite].extra > 0 )
	{
		nXSprite = sprite[nSprite].extra;
		dassert(nXSprite < kMaxXSprites);

		XSpriteToDialog(nSprite);
		PaintDialog(dlgSprite, LENGTH(dlgSprite));
	}
	else
		clearmidstatbar16();
}


/***********************************************************************
 * EditSpriteData()
 *
 * This function allows editing of the game specific sprite data.
 **********************************************************************/
void EditSpriteData( int nSprite )
{
	int nXSprite;

	dassert(nSprite >= 0 && nSprite < kMaxSprites);

	CleanUp();

	sprintf(buffer, "Sprite %d", nSprite);
	printmessage16(buffer);

	nXSprite = GetXSprite(nSprite);
	XSpriteToDialog(nSprite);
	if ( EditDialog(dlgSprite, LENGTH(dlgSprite)) )
		DialogToXSprite(nSprite);

	ShowSpriteData(nSprite);
	vel = svel = angvel = 0;
	CleanUp();

	// fix sprite type attributes
	AutoAdjustSprites();
}


int getlinehighlight(int x, int y)
{
	long i, dst, dist, closest, xi, yi;

	dist = divscale(gHighlightThreshold, zoom, 14);
	closest = -1;

	for (i = 0; i < numwalls; i++)
	{
		int lx = wall[wall[i].point2].x - wall[i].x;
		int ly = wall[wall[i].point2].y - wall[i].y;
		int qx = x - wall[i].x;
		int qy = y - wall[i].y;

		// is point on wrong side of wall?
		if ( qx * ly > qy * lx )
			continue;

		int num = dmulscale(qx, lx, qy, ly, 4);
		int den = dmulscale(lx, lx, ly, ly, 4);

		if (num <= 0 || num >= den)
			continue;

		xi = wall[i].x + muldiv(lx, num, den);
		yi = wall[i].y + muldiv(ly, num, den);

		dst = qdist(xi - x, yi - y);
		if (dst <= dist)
		{
			dist = dst;
			closest = i;
		}
	}

	return closest;
}


void setcolor16(int color);
#pragma aux setcolor16 =\
	"mov dx, 0x3ce",\
	"shl ax, 8",\
	"out dx, ax",\
	parm [eax]\

void drawpixel16(int offset);
#pragma aux drawpixel16 =\
	"mov ecx, edi",\
	"and cl, 7",\
	"xor cl, 7",\
	"mov ah, 1",\
	"shl ah, cl",\
	"mov dx, 0x3ce",\
	"mov al, 0x8",\
	"out dx, ax",\
	"shr edi, 3",\
	"or byte ptr [edi+0xA0000], al",\
	parm [edi]\
	modify [eax ecx edx]\


void Draw2dWall( int x0, int y0, int x1, int y1, char color, int thick )
{
	if ( x0 < 0 && x1 < 0 ) return;
	if ( x0 >= 640 && x1 >= 640 ) return;
	if ( y0 < 0 && y1 < 0 ) return;
	if ( y0 >= ydim16 && y1 >= ydim16 ) return;

	drawline16(x0, y0, x1, y1, color);
	if (thick)
	{
		long dx = x1 - x0;
		long dy = y1 - y0;
		RotateVector(&dx, &dy, kAngle45 / 2);
		switch (GetOctant(dx, dy))
		{
			case 0:
			case 4:
				drawline16(x0, y0-1, x1, y1-1, color);
				drawline16(x0, y0+1, x1, y1+1, color);
				break;

			case 1:
			case 5:
				if (qabs(dx) < qabs(dy) )
				{
					drawline16(x0-1, y0+1, x1-1, y1+1, color);
					drawline16(x0-1, y0, x1, y1+1, color);
					drawline16(x0+1, y0, x1+1, y1, color);
				}
				else
				{
					drawline16(x0-1, y0+1, x1-1, y1+1, color);
					drawline16(x0, y0+1, x1, y1+1, color);
					drawline16(x0, y0+1, x1, y1+1, color);
				}
				break;

			case 2:
			case 6:
				drawline16(x0-1, y0, x1-1, y1, color);
				drawline16(x0+1, y0, x1+1, y1, color);
				break;

			case 3:
			case 7:
				if (qabs(dx) < qabs(dy) )
				{
					drawline16(x0-1, y0-1, x1-1, y1-1, color);
					drawline16(x0-1, y0, x1-1, y1, color);
					drawline16(x0+1, y0, x1+1, y1, color);
				}
				else
				{
					drawline16(x0-1, y0-1, x1-1, y1-1, color);
					drawline16(x0, y0-1, x1, y1-1, color);
					drawline16(x0, y0+1, x1, y1+1, color);
				}
				break;
		}
	}
}

void Draw2dVertex( int x, int y, int color )
{
	long templong = y * 640 + x + pageoffset;
	setcolor16(color);
	drawpixel16(templong-2-1280);
	drawpixel16(templong-1-1280);
	drawpixel16(templong+0-1280);
	drawpixel16(templong+1-1280);
	drawpixel16(templong+2-1280);

	drawpixel16(templong-2-640);
	drawpixel16(templong-2+0);
	drawpixel16(templong-2+640);

	drawpixel16(templong+2-640);
	drawpixel16(templong+2+0);
	drawpixel16(templong+2+640);

	drawpixel16(templong-2+1280);
	drawpixel16(templong-1+1280);
	drawpixel16(templong+0+1280);
	drawpixel16(templong+1+1280);
	drawpixel16(templong+2+1280);
}


void Draw2dFaceSprite( int x, int y, int color)
{
	long templong = y * 640 + x + pageoffset;
	setcolor16(color);

	drawpixel16(templong - 1 - 1920);
	drawpixel16(templong + 0 - 1920);
	drawpixel16(templong + 1 - 1920);

	drawpixel16(templong - 2 - 1280);
	drawpixel16(templong + 2 - 1280);

	drawpixel16(templong - 3 - 640);
	drawpixel16(templong + 3 - 640);

	drawpixel16(templong - 3 + 0);
	drawpixel16(templong + 3 + 0);

	drawpixel16(templong - 3 + 640);
	drawpixel16(templong + 3 + 640);

	drawpixel16(templong - 2 + 1280);
	drawpixel16(templong + 2 + 1280);

	drawpixel16(templong - 1 + 1920);
	drawpixel16(templong + 0 + 1920);
	drawpixel16(templong + 1 + 1920);
}


void Draw2dFloorSprite( int x, int y, char color)
{
	drawline16(x - 3, y - 3, x + 3, y - 3, color);
	drawline16(x - 3, y + 3, x + 3, y + 3, color);
	drawline16(x - 3, y - 3, x - 3, y + 3, color);
	drawline16(x + 3, y - 3, x + 3, y + 3, color);
}


void Draw2dMarker( int x, int y, int color)
{
	long templong = y * 640 + x + pageoffset;
	setcolor16(color);

	drawpixel16(templong - 1 - 2560);
	drawpixel16(templong + 0 - 2560);
	drawpixel16(templong + 1 - 2560);

	drawpixel16(templong - 3 - 1920);
	drawpixel16(templong - 2 - 1920);
	drawpixel16(templong + 2 - 1920);
	drawpixel16(templong + 3 - 1920);

	drawpixel16(templong - 3 - 1280);
	drawpixel16(templong - 2 - 1280);
	drawpixel16(templong + 2 - 1280);
	drawpixel16(templong + 3 - 1280);

	drawpixel16(templong - 4 - 640);
	drawpixel16(templong - 1 - 640);
	drawpixel16(templong + 1 - 640);
	drawpixel16(templong + 4 - 640);

	drawpixel16(templong - 4);
	drawpixel16(templong + 0);
	drawpixel16(templong + 4);

	drawpixel16(templong - 4 + 640);
	drawpixel16(templong - 1 + 640);
	drawpixel16(templong + 1 + 640);
	drawpixel16(templong + 4 + 640);

	drawpixel16(templong - 3 + 1280);
	drawpixel16(templong - 2 + 1280);
	drawpixel16(templong + 2 + 1280);
	drawpixel16(templong + 3 + 1280);

	drawpixel16(templong - 3 + 1920);
	drawpixel16(templong - 2 + 1920);
	drawpixel16(templong + 2 + 1920);
	drawpixel16(templong + 3 + 1920);

	drawpixel16(templong - 1 + 2560);
	drawpixel16(templong + 0 + 2560);
	drawpixel16(templong + 1 + 2560);
}


void draw2dscreen(long x, long y, short nAngle, long nZoom, short nGrid )
{
	int i, j, x0, y0, x1, y1;
	char nColor;
	char h = (gFrameClock & 0x08) ? 8 : 0;
	int thick;
	short flags;

	// steal a copy of these variables, because Ken made them static!
	grid = nGrid;
	zoom = nZoom;

	if (qsetmode == 200)
		return;

	for (i = 0; i < numwalls; i++)
	{
		// only draw one side of walls
		if ( wall[i].nextwall > i)
			continue;

		if (wall[i].nextwall == -1)
		{
			nColor = kColorWhite;
			flags = wall[i].cstat;
			thick = 0;
		}
		else
		{
			nColor = kColorRed;
			thick = 0;

			flags = wall[i].cstat | wall[wall[i].nextwall].cstat;

			// if one side is highlighted, only show that side's flags
			if (i == linehighlight)
				flags = wall[i].cstat;
			else if (wall[i].nextwall == linehighlight)
				flags = wall[wall[i].nextwall].cstat;

			if ( flags & kWallHitscan)
				nColor = kColorMagenta;
			if ( flags & kWallBlocking)
				thick = 1;
		}
		if ( flags & kWallMoveForward )
			nColor = kColorLightBlue;
		if ( flags & kWallMoveBackward )
			nColor = kColorLightGreen;

		if ( linehighlight >= 0 && (i == linehighlight || wall[i].nextwall == linehighlight) )
			nColor ^= h;

		x0 = 320 + mulscale(wall[i].x - x, nZoom, 14);
		y0 = 200 + mulscale(wall[i].y - y, nZoom, 14);
		x1 = 320 + mulscale(wall[wall[i].point2].x - x, nZoom, 14);
		y1 = 200 + mulscale(wall[wall[i].point2].y - y, nZoom, 14);

		Draw2dWall( x0, y0, x1, y1, nColor, thick);
		if (nZoom >= 256 && x0 > 4 && x0 < 635 && y0 > 4 && y0 < ydim16-5 )
			Draw2dVertex(x0, y0, kColorBrown);
	}

	// redraw highlighted vertices
	if (nZoom >= 256)
	{
		for (i = 0; i < numwalls; i++)
		{
			if ( i == pointhighlight || (highlightcnt > 0 && TestBitString(show2dwall, i)) )
			{
				x0 = 320 + mulscale(wall[i].x - x, nZoom, 14);
				y0 = 200 + mulscale(wall[i].y - y, nZoom, 14);
				if ( x0 > 4 && x0 < 635 && y0 > 4 && y0 < ydim16-5 )
					Draw2dVertex(x0, y0, kColorBrown ^ h);
			}
		}
	}

	if (nZoom >= 256)
	{
		for (i = 0; i < numsectors; i++)
		{
			j = headspritesect[i];
			while (j != -1)
			{
				x0 = 320 + mulscale(sprite[j].x - x, nZoom, 14);
				y0 = 200 + mulscale(sprite[j].y - y, nZoom, 14);

				if ( sprite[j].statnum == kStatMarker )
				{
					nColor = kColorYellow;

					if ( sprite[j].owner == sectorhighlight )
						nColor = kColorWhite;

					if ((j | 0x4000) == pointhighlight || (highlightcnt > 0 && TestBitString(show2dsprite, j)) )
						nColor ^= h;

					if (x0 > 4 && x0 < 635 && y0 > 4 && y0 < ydim16-5)
						switch( sprite[j].type )
						{
							case kMarkerOff:
							case kMarkerOn:
								Draw2dMarker(x0, y0, nColor);
								break;

							case kMarkerWarpDest:
							case kMarkerAxis:
								Draw2dMarker(x0, y0, nColor);
								x1 = mulscale30(nZoom / 128, Cos(sprite[j].ang));
								y1 = mulscale30(nZoom / 128, Sin(sprite[j].ang));
								Draw2dWall(x0, y0, x0 + x1, y0 + y1, nColor, thick);
								break;
						}

					if ( sprite[j].type == kMarkerOff )
					{
						// draw movement arrow
						int nSector = sprite[j].owner;
						int nXSector = sector[nSector].extra;
						dassert(nXSector > 0 && nXSector < kMaxXSectors);
						int nSprite2 = xsector[nXSector].marker1;
						x1 = 320 + mulscale(sprite[nSprite2].x - x, nZoom, 14);
						y1 = 200 + mulscale(sprite[nSprite2].y - y, nZoom, 14);
						drawline16(x0, y0, x1, y1, kColorLightBlue);
						int nAngle = getangle(x0 - x1, y0 - y1);
						x0 = mulscale30(nZoom / 64, Cos(nAngle + kAngle30));
						y0 = mulscale30(nZoom / 64, Sin(nAngle + kAngle30));
						drawline16(x1, y1, x1 + x0, y1 + y0, kColorLightBlue);
						x0 = mulscale30(nZoom / 64, Cos(nAngle - kAngle30));
						y0 = mulscale30(nZoom / 64, Sin(nAngle - kAngle30));
						drawline16(x1, y1, x1 + x0, y1 + y0, kColorLightBlue);

					}
				}
				else
				{
					nColor = kColorCyan;
					thick = 0;

					if ( sprite[j].cstat & kSpriteHitscan )
						nColor = kColorMagenta;

					if ( sprite[j].cstat & kSpriteMoveForward )
						nColor = kColorLightBlue;

					if ( sprite[j].cstat & kSpriteMoveReverse )
						nColor = kColorLightGreen;

					if ( sprite[j].cstat & kSpriteBlocking )
						thick = 1;

					if ((j | 0x4000) == pointhighlight || (highlightcnt > 0 && TestBitString(show2dsprite, j)) )
						nColor ^= h;

					if (x0 > 4 && x0 < 635 && y0 > 4 && y0 < ydim16-5)
					{
						switch (sprite[j].cstat & kSpriteRMask)
						{
							case kSpriteFace:
							case kSpriteSpin:
								Draw2dFaceSprite(x0, y0, nColor);
								x1 = mulscale30(nZoom / 128, Cos(sprite[j].ang));
								y1 = mulscale30(nZoom / 128, Sin(sprite[j].ang));
								Draw2dWall(x0, y0, x0 + x1, y0 + y1, nColor, thick);
								break;

							case kSpriteWall:
								Draw2dFaceSprite(x0, y0, nColor);
								x1 = mulscale30(nZoom / 128, Cos(sprite[j].ang + kAngle90));
								y1 = mulscale30(nZoom / 128, Sin(sprite[j].ang + kAngle90));
								Draw2dWall(x0 - x1, y0 - y1, x0 + x1, y0 + y1, nColor, thick);
								x1 = mulscale30(nZoom / 256, Cos(sprite[j].ang));
								y1 = mulscale30(nZoom / 256, Sin(sprite[j].ang));
								if (sprite[j].cstat & kSpriteOneSided)
									Draw2dWall(x0, y0, x0 + x1, y0 + y1, nColor, thick);
								else
									Draw2dWall(x0 - x1, y0 - y1, x0 + x1, y0 + y1, nColor, thick);
								break;

							case kSpriteFloor:
								Draw2dFloorSprite(x0, y0, nColor);
								x1 = mulscale30(nZoom / 256, Cos(sprite[j].ang));
								y1 = mulscale30(nZoom / 256, Sin(sprite[j].ang));
								Draw2dWall(x0, y0, x0 + x1, y0 + y1, nColor, thick);
								break;
						}
					}
				}
				j = nextspritesect[j];
			}
		}
	}

	x0 = 320 + mulscale30(nZoom / 128, Cos(nAngle)); //Draw white arrow
	y0 = 200 + mulscale30(nZoom / 128, Sin(nAngle));
	drawline16(x0, y0, 640-x0,400-y0,kColorWhite);
	drawline16(x0, y0, 120+y0,520-x0,kColorWhite);
	drawline16(x0, y0, 520-y0,-120+x0,kColorWhite);
}


void ProcessKeys2D( void )
{
	BYTE key;
	BYTE shift, ctrl, alt;
	int x, y;
	static int nPointLast = -1, nLineLast = -1, nSectorLast = -1;
	static int oldnumwalls, oldnumsectors, oldnumsprites;
	int numsprites = 0;

	// redetermine numsprites (build.c version is static!)
	numsprites = 0;
	for (int i = 0; i < kMaxSprites; i++)
		if ( sprite[i].statnum < kMaxStatus )
			numsprites++;

	// did anything important happen?
	if (numwalls != oldnumwalls || numsectors != oldnumsectors || numsprites != oldnumsprites)
	{
		CleanUp();

		// fix sprite type attributes
		AutoAdjustSprites();

		oldnumwalls = numwalls;
		oldnumsectors = numsectors;
		oldnumsprites = numsprites;
	}

	shift = keystatus[KEY_LSHIFT] | keystatus[KEY_RSHIFT];
	ctrl = keystatus[KEY_LCTRL] | keystatus[KEY_RCTRL];
	alt = keystatus[KEY_LALT] | keystatus[KEY_RALT];

	getpoint(searchx, searchy, &x, &y);

	updatesector(x, y, &sectorhighlight);

	if (pointhighlight != nPointLast || linehighlight != nLineLast || sectorhighlight != nSectorLast)
	{
		nPointLast = pointhighlight;
		nLineLast = linehighlight;
		nSectorLast = sectorhighlight;

		if ((pointhighlight & 0xC000) == 0x4000)
			ShowSpriteData(pointhighlight & 0x3FFF);
		else if (linehighlight >= 0)
			ShowWallData(linehighlight);
		else if (sectorhighlight >= 0)
			ShowSectorData(sectorhighlight);
		else
			clearmidstatbar16();
	}

	key = keyGet();

	switch (key)
	{
		case KEY_HOME:
			if (ctrl)
			{
				short nSprite = getnumber16("Locate sprite #: ",0,kMaxSprites);
				clearmidstatbar16();
				if (nSprite >= 0 && nSprite < kMaxSprites && sprite[nSprite].statnum < kMaxStatus)
				{
					posx	= sprite[nSprite].x;
					posy	= sprite[nSprite].y;
					posz	= sprite[nSprite].z;
					ang		= sprite[nSprite].ang;
					cursectnum = sprite[nSprite].sectnum;
				}
				else
					printmessage16("Sir Not Appearing In This Film");

			}
			break;

		case KEY_K:		// Kinetic sprites/walls
			if ( (pointhighlight & 0xC000) == 0x4000 )	// is a sprite highlighted
			{
				short flags = (short)(sprite[pointhighlight & 0x3FFF].cstat & kSpriteMoveMask);
				switch (flags)
				{
					case kSpriteMoveNone:
						flags = kSpriteMoveForward;
						break;

					case kSpriteMoveForward:
						flags = kSpriteMoveReverse;
						break;

					case kSpriteMoveReverse:
						flags = kSpriteMoveNone;
						break;

					default:
						flags = kSpriteMoveNone;
				}

				sprite[pointhighlight & 0x3FFF].cstat &= ~kSpriteMoveMask;
				sprite[pointhighlight & 0x3FFF].cstat |= flags;
				BeepOkay();
			}
			else if ( linehighlight >=0 )
			{
				short flags = (short)(wall[linehighlight].cstat & kWallMoveMask);
				switch (flags)
				{
					case kWallMoveNone:
						flags = kWallMoveForward;
						break;

					case kWallMoveForward:
						flags = kWallMoveBackward;
						break;

					case kWallMoveBackward:
						flags = kWallMoveNone;
						break;

					default:
						flags = kWallMoveNone;
				}

				wall[linehighlight].cstat &= ~kWallMoveMask;
				wall[linehighlight].cstat |= flags;
				BeepOkay();
			}
			else
				BeepError();
			break;

		case KEY_M:		// Masked walls
			if ( linehighlight >=0 )
			{
				int nWall = linehighlight;
				int nWall2 = wall[nWall].nextwall;

				if (nWall2 < 0)
				{
					BeepError();
					break;
				}

				wall[nWall].cstat |= kWallMasked;
				wall[nWall].cstat &= ~kWallFlipX;
				wall[nWall2].cstat |= kWallFlipX;           //auto other-side flip
				wall[nWall2].cstat |= kWallMasked;
				if (wall[nWall].overpicnum < 0)
					wall[nWall].overpicnum = 0;
				wall[nWall2].overpicnum = wall[nWall].overpicnum;
				wall[nWall].cstat &= ~kWallOneWay;
				wall[nWall2].cstat &= ~kWallOneWay;

				sprintf(buffer, "wall[%i] %s masked", searchwall,
					(wall[searchwall].cstat & kWallMasked) ? "is" : "not");
				printmessage16(buffer);

				BeepOkay();
				break;
			}
			else
				BeepError();
			break;

		case KEY_LBRACE:
			if ( (pointhighlight & 0xC000) == 0x4000 )	// is a sprite highlighted
			{
				int nSprite = pointhighlight & 0x3FFF;

				// rotate sprite ccw
				int step = shift ? 16 : 256;
				sprite[nSprite].ang = (short)DecNext(sprite[nSprite].ang, step);
				sprintf(buffer, "sprite[%i].ang: %i", nSprite, sprite[nSprite].ang);
				printmessage16(buffer);
				BeepOkay();
			}
			break;

		case KEY_RBRACE:
			if ( (pointhighlight & 0xC000) == 0x4000 )	// is a sprite highlighted
			{
				int nSprite = pointhighlight & 0x3FFF;

				// rotate sprite ccw
				int step = shift ? 16 : 256;
				sprite[nSprite].ang = (short)IncNext(sprite[nSprite].ang, step);
				sprintf(buffer, "sprite[%i].ang: %i", nSprite, sprite[nSprite].ang);
				printmessage16(buffer);
				BeepOkay();
			}
			break;
	}

	// let's see if this fixes the input buffering problem
	if (key != 0)
		keyFlushStream();
}
