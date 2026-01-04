#pragma once
#include "Record.h"
#include <codecvt>

// 注意：这个 StringTable 类已经不需要复杂的转换了
// 因为我们直接用 wstring 存储，wstring 已经是正确编码的

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

		// 简单地将 wstring 转换为 string
		// 因为我们存储的 wstring 本来就是从 ASCII 转换来的
		const std::wstring& wstr = strings[index];
		std::string result;
		result.reserve(wstr.length());

		for (wchar_t wc : wstr)
		{
			// 只处理 ASCII 范围的字符
			if (wc < 128)
			{
				result.push_back(static_cast<char>(wc));
			}
			else
			{
				// 对于非 ASCII 字符，保持为 '?'
				result.push_back('?');
			}
		}

		return result;
	}
};