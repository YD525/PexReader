#pragma once
#include "Record.h"
#include <codecvt>

class StringTable
{
public:
	uint16_t count;
	std::vector<std::wstring> strings;

	StringTable() : count(0) {}

	StringTable(uint16_t _count) : count(_count)
	{
		strings.resize(count);
	}

	std::string toUtf8(uint16_t index) const
	{
		if (index >= count || index >= strings.size())
		{
			return "";
		}

		// Simply convert wstring to string
		// Because the wstring we store was originally converted from ASCII.
		const std::wstring& wstr = strings[index];
		std::string result;
		result.reserve(wstr.length());

		for (wchar_t wc : wstr)
		{
			// Only process characters within the ASCII range.
			if (wc < 128)
			{
				result.push_back(static_cast<char>(wc));
			}
			else
			{
				//For non-ASCII characters, keep them as '?'.
				result.push_back('?');
			}
		}

		return result;
	}
};