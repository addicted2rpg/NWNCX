// (c) Addicted2rpg & Virusman (2013) - see core.cpp for License (GNU).

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include "gamelist.h"


FILE *logFile;
char logFileName[] = "logs/nwncx_gamelist.txt";

extern void WINAPI GameList();  // gamelist.cpp
extern void WINAPI InitHooks(); // gamelist.cpp
extern void __cdecl cleanup_gamelist_structure(); // gamelist.cpp

extern struct gamelist_struct **s_gamelist;  // gamelist.cpp

void LogMessage(char *s) {
	fprintf(logFile, s);
	fflush(logFile);
}


void InitPlugin()
{
	logFile = fopen(logFileName, "w");
	LogMessage("NWN Client Extender 1.0 - Game List\n");
	LogMessage("(c) Addicted2rpg made possible by Skywing, Virusman, Eeriegeek, and others.\n\n");

}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		InitPlugin();
		InitHooks();
		GameList();
	}
	else if (ul_reason_for_call == DLL_PROCESS_DETACH)
	{
		// cleanup
		fclose(logFile);

		if(s_gamelist != NULL) {
			cleanup_gamelist_structure();
		}

	}
	return TRUE;
}
