#pragma once
#include "string.h";
#include <vector>;

using namespace std;

//https://github.com/Cutleast/sse-pex-interface
//https://en.uesp.net/wiki/Skyrim_Mod:Compiled_Script_File_Format#Debug_Info

#pragma pack(push, 1)
struct StringTable
{
	uint16_t count;  
	vector<wstring> strings;  

	StringTable(uint16_t _count) : count(_count)
	{
		strings.resize(count); 
	}
};
#pragma pack(pop)

#pragma pack(push, 1)
struct DebugFunction
{
};
#pragma pack(pop)

#pragma pack(push, 1)
struct DebugInfo
{
	uint8_t hasDebugInfo;
	uint64_t modificationTime;
	uint16_t functionCount;
	DebugFunction functions;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct UserFlag
{
};
#pragma pack(pop)

#pragma pack(push, 1)
struct Object
{
};
#pragma pack(pop)



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
	StringTable stringTable;
	DebugInfo debugInfo;
	uint16_t userFlagCount;
	UserFlag userFlags;
	uint16_t objectCount;
	Object objects;
};
#pragma pack(pop)


