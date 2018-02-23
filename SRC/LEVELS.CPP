#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug4g.h"
#include "inifile.h"
#include "levels.h"
#include "globals.h"
#include "resource.h"
#include "misc.h"
#include "screen.h"

#include <memcheck.h>

/***********************************************************************
 *
 **********************************************************************/
char gLevelName[_MAX_PATH];
int  gEpisodeId = kMinEpisode;
int  gMapId     = kMinMap;
int  gEndingA, gEndingB;
int  gEndLevelFlag = 0;
char gLevelAuthor[128];
char gLevelDescription[128];
char gLevelSong[128];
char *gLevelMessage[64];

LIGHTNING gLightningInfo[ kMaxLightning ];

/***********************************************************************
 * Process command line arguments
 **********************************************************************/
void levelGetName(int episodeId, int mapId)
{
	char zEpisode[13], zMap[13];

	sprintf(zEpisode, "EPISODE%1d.DEF", episodeId);

	IniFile EpisodeINI(zEpisode);

	sprintf( zMap, "Map%d", mapId );
	char *key = EpisodeINI.GetKeyString(NULL, zMap, NULL);

	if (key == NULL)
	{
		dprintf("%s not found in level definition file\n", zMap);
		return;
	}

	dprintf("%s=%s\n", zMap, key);
	sscanf(key, "%[^, ],%i,%i", gLevelName, &gEndingA, &gEndingB);
}

void levelLoadDef( void )
{
	char defName[13];
	strcpy(defName, gLevelName);
	ChangeExtension(defName, ".DEF");

	IniFile levelDef(defName);

	strcpy(gLevelAuthor, levelDef.GetKeyString(NULL, "Author", ""));
	strcpy(gLevelDescription, levelDef.GetKeyString(NULL, "Description", ""));
	strcpy(gLevelSong, levelDef.GetKeyString(NULL, "Song", ""));

	gFogMode = levelDef.GetKeyBool(NULL, "Fog", FALSE);

	// load the level messages
	for (int i = 0; i < 64; i++)
	{
		if ( gLevelMessage[i] != NULL )
		{
			Resource::Free(gLevelMessage[i]);
			gLevelMessage[i] = NULL;
		}

		char messageKey[10];
		sprintf(messageKey, "Message%d", i);
		char *msg = levelDef.GetKeyString(NULL, messageKey, NULL);
		if (msg != NULL)
		{
			gLevelMessage[i] = (char *)Resource::Alloc(strlen(msg));
			strcpy(gLevelMessage[i], msg);
		}
	}

	// load the level lightning definitions
	for (i = 0; i < kMaxLightning; i++)
	{
		// preinitialize lightning
		gLightningInfo[i].slot = -1;
		gLightningInfo[i].offset = -1;

		char forkKey[16];
		sprintf(forkKey, "Lightning%d", i);
		char *msg = levelDef.GetKeyString(NULL, forkKey, NULL);
		if (msg != NULL)
			sscanf(msg, "%i,%i", &gLightningInfo[i].slot, &gLightningInfo[i].offset);
	}

}
