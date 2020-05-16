#include "RandomMemStuff.h"



RandomMemStuff::RandomMemStuff()
{

}


DWORD func_getStringFromCSV = 0x017B7A3E;
DWORD func_getLocalizedString = 0x01395763;
DWORD func_appendString = 0x01395488; //for reference purposes

DWORD patch_addedSpaces = 0x01529f98;
DWORD patch_addedNumbers = 0x0152a006;
DWORD patch_sprintfArg = 0x0183479C;

DWORD hookBackAddr_FakeTitles, hookBackAddr_CustomNames, hookBackAddr_missingLocalization;
DWORD hookAddr_ModifyLocalized = 0x01529F2B;
DWORD hookAddr_ModifyCleanString = 0x01529F61;
DWORD hookAddr_MissingLocalization = 0x01834790;

int len = 0;
Patch patch;
std::vector<std::string> songTitles(6);
std::string newish = "bratec";

void __declspec(naked) hook_fakeTitles() {
	//ESI = INDEX
	//EAX = char* str "$[36969]SONG LIST"
	__asm {
		mov ecx, dword ptr ds : [0x135CB7C]
		pushad
		mov ebx, 0x33 //sets the last number 
		add ebx, esi
		mov byte ptr[eax + 0x6], bl  
		mov byte ptr[eax + 0x2], 0x34 //third char (that is, the first digit); 0x30 = 0, 0x31 = 1, ... | with 0x34, there's no string with key [4696x] - it returns the whole thing back
		//mov byte ptr[eax + 0x3], 0x34 //adapt to your needs
		popad
		jmp[hookBackAddr_FakeTitles]
	}
}

//At 0x01529F61, into EAX is saved pointer to: either the clean string (if index exists) or a string without the $[] part (eg. either it becomes SONG LIST or #46967#SONG LIST
//so prolly a good place to hook - check if it contains # and check the last digit to determine which index to put out

/* My version for custom song list names, by using the 0x01529F61 method */

void __declspec(naked) hook_basicCustomTitles() {
	__asm {
		lea eax, dword ptr ss : [ebp - 0x80] // ebp-0x80 = pointer to the "clean" string address
		pushad

		mov ebx, [eax] //dereference it first
		cmp byte ptr[ebx], '#'
		jne GTFO
	
		mov byte ptr[ebx+3], 0x35 //very basic, not as developed as Koko's
		mov byte ptr[ebx+7], 0x44

		GTFO:
		popad
		jmp[hookBackAddr_CustomNames]
	}
}

/*Koko's version - hijacks the format string for printf & discards the parameters and then returns our (kkomrade) versions of the names
Quite likely, this one is better in our use case, since we want to grab titles from a file, which is done more conveniently in a regular CPP function without the ASM 
*/
char __stdcall missingLocalization(int number, char* text) {
	const int buffer_size = 10;
	char str[buffer_size];
	sprintf_s(&str[0], buffer_size, "%d", number);
	//MsgBoxA(str, "Missing locale str");

	switch (number)
	{
	case 46964: //either start from 46964 or change mov ebx, 0x33 to mov ebx, 0x2F in hook_fakeTitles to start from 0
		return (char)songTitles[0].c_str();
	case 46965:
		return (char)songTitles[1].c_str();
	case 46966:
		return (char)songTitles[2].c_str();
	case 46967:
		return (char)songTitles[3].c_str();
	case 46968:
		return (char)songTitles[4].c_str();
	case 46969:
		return (char)songTitles[5].c_str();
	default:
		return (char)str;
	}
}

void __declspec(naked) missingLocalizationHookFunc() {
	__asm {
		push ecx
		push edx
		push esp

		push[esp + 0x10]
		push eax
		call missingLocalization

		pop esp
		pop edx
		pop ecx

		add esp, 0x8
		push eax
		jmp[hookBackAddr_missingLocalization]
	}
}

void RandomMemStuff::PatchSongListAppendages() {
	patch.PatchAdr((BYTE*)patch_addedSpaces, (UINT*)"\x58\x58\x90\x90\x90", 5); //patch out " "
	patch.PatchAdr((BYTE*)patch_addedNumbers, (UINT*)"\x5A\x5A\x90\x90\x90", 5); //patch 1-6
}

void RandomMemStuff::SetFakeListNames() {
	PatchSongListAppendages();

	len = 6;

	hookBackAddr_FakeTitles = hookAddr_ModifyLocalized + len;
	patch.PlaceHook((void*)hookAddr_ModifyLocalized, hook_fakeTitles, len);
}

void RandomMemStuff::HookSongLists() {
	SetFakeListNames();

	len = 6;
	hookBackAddr_CustomNames = hookAddr_ModifyCleanString + len;

	patch.PlaceHook((void*)hookAddr_ModifyCleanString, hook_basicCustomTitles, len);
}

void RandomMemStuff::HookSongListsKoko() {
	SetFakeListNames();

	len = 5;
	hookBackAddr_missingLocalization = hookAddr_MissingLocalization + len;

	//Skip less printf parameters if those have been removed
	patch.PatchAdr((BYTE*)patch_sprintfArg, (BYTE*)"\x04", 1);

	patch.PlaceHook((void*)hookAddr_MissingLocalization, missingLocalizationHookFunc, len);
}

bool RandomMemStuff::LoadSettings() {
	INIReader reader("RSMods.ini");
	if (reader.ParseError() != 0) {
		//do sth
		return false;
	}

	for (int i = 0; i < 6; i++)
		songTitles[i] = reader.Get("SongListTitles", "SongListTitle_" + std::to_string(i+1), "SONG LIST");
}


RandomMemStuff::~RandomMemStuff()
{
}
