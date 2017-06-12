#include <iostream>
#include "hooker.hpp"

void start(HMODULE module)
{
	AllocConsole();
	AttachConsole(GetCurrentProcessId());
	freopen("CON", "w", stdout);
	printf("Lumai/o's #1 internal hack just $99 bitcoin.\n");
	initHook();
}

BOOL WINAPI DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
	if(dwReason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(hModule);
		CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)start, hModule, NULL, NULL);
	}
	else if(dwReason == DLL_PROCESS_DETACH)
	{
		unloadHook();
		FreeConsole();
	}
	return TRUE;
}
