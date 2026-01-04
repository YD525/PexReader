#pragma once
#include "Record.h"

//https://github.com/Cutleast/sse-pex-interface
//https://en.uesp.net/wiki/Skyrim_Mod:Compiled_Script_File_Format#Debug_Info


#pragma pack(push, 1)
struct RecordHeader
{
	uint32_t magic;
	uint8_t majorVersion;
	uint8_t minorVersion;
	uint16_t gameId;
	uint64_t compilationTime;
	wstring	sourceFileName;
	wstring username;
	wstring machinename;
};
#pragma pack(pop)


