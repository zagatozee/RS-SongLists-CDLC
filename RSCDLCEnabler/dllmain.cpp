#include <windows.h>
#include <string>

#include "d3dx9_42.h"
#include "FindSignature.h"
#include "Patch.h"
#include <cstdlib>
#include "RandomMemStuff.h"
 
/* ZZ Based on Lovrom8 and Kokos work, I've just slimmed out some functions to make a basic version that JUST does the songlist mod and enables CDLC.
If maingame.csv is modified with entries in 46964-9 - then those will be used, If NOT, the RSMods.ini ill be used.
*/

DWORD WINAPI MainThread(void*) {	
	RandomMemStuff mem;
	Patch patch;

	uint8_t* VerifySignatureOffset = FindPattern(0x01377000, 0x00DDE000, (uint8_t*)"\xE8\x00\x00\x00\x00\x83\xC4\x20\x88\xC3", "x????xxxxx");

	if (VerifySignatureOffset) {
		if (!patch.PatchAdr(VerifySignatureOffset + 8, "\xB3\x01", 2))
			printf("Failed to patch verify_signature!\n");
		else
			printf("Patch verify_signature success!\n");
	}

	// bool loftEnabled = true;

	mem.LoadSettings();

	while (true) {

		Sleep(100);

		{
			mem.HookSongListsKoko();
		}
		 
	}
	

	return 0;
}


void Initialize(void) {
	CreateThread(NULL, 0, MainThread, NULL, NULL, 0);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
		InitProxy();
		Initialize();
		return TRUE;
	case DLL_PROCESS_DETACH:
		ShutdownProxy();
		return TRUE;
	}
	return TRUE;
}
