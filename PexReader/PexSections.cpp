#pragma once
#include "Record.h"

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

struct RecordSections
{
	StringTable stringTable;
	DebugInfo debugInfo;
	uint16_t userFlagCount;
	UserFlag userFlags;
	uint16_t objectCount;
	Object objects;
};

