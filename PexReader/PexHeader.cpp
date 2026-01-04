#pragma once
#include "string.h";
#include <vector>;

using namespace std;

//https://github.com/Cutleast/sse-pex-interface
//https://en.uesp.net/wiki/Skyrim_Mod:Compiled_Script_File_Format#Debug_Info

struct StringTable
{
	uint16_t count;  
	vector<wstring> strings;  

	StringTable(uint16_t _count) : count(_count)
	{
		strings.resize(count); 
	}
};



#pragma pack(push, 1)
struct RecordHeader
{
	//char     Sig[4];//Offse 0 ~ 3
	//uint32_t DataSize;// 4 ~ 7
	//uint32_t Flags;// 8 ~ 11
	//uint32_t FormID;// 12 ~ 15
	//uint32_t VersionCtrl;// 16 ~ 19
	//uint16_t Version;//20 ~ 21
	//uint16_t Unknown;//22 ~ 23
	uint32_t magic;
	uint8_t majorVersion;
	uint8_t minorVersion;
	uint16_t gameId;
	uint64_t compilationTime;
	wstring	sourceFileName;
	wstring username;
	wstring machinename;
	StringTable stringTable;

};
#pragma pack(pop)


