#pragma once
#include "Record.h"
#include <codecvt>

class StringTable
{
	public:
	uint16_t count;
	std::vector<std::vector<byte>> strings;
	StringTable() : count(0) {}

	StringTable(uint16_t _count) : count(_count)
	{
		strings.resize(count);
	}

	
};