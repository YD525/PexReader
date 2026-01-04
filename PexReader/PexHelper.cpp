#pragma once
#include "Record.h"
#include "PexHeader.cpp"
#include "PexSections.cpp"

//https://github.com/Cutleast/sse-pex-interface
//https://en.uesp.net/wiki/Skyrim_Mod:Compiled_Script_File_Format#Debug_Info

class PexData
{
	public:
	RecordHeader Header;
	StringTable stringTable;
	DebugInfo debugInfo;
	uint16_t userFlagCount;
	vector<UserFlag> userFlags;
	uint16_t objectCount;
	vector<Object> objects;

	void Load()
	{

	}

	void Save()
	{

	}
};