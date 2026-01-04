#pragma once
#include "Record.h"
#include <codecvt>


inline std::string Windows1252ToUTF8(const uint8_t* Data, size_t Size)
{
	std::string Result;
	Result.reserve(Size * 2);

	static const uint16_t CP1252_TABLE[32] =
	{
		0x20AC,0x0081,0x201A,0x0192,0x201E,0x2026,0x2020,0x2021,
		0x02C6,0x2030,0x0160,0x2039,0x0152,0x008D,0x017D,0x008F,
		0x0090,0x2018,0x2019,0x201C,0x201D,0x2022,0x2013,0x2014,
		0x02DC,0x2122,0x0161,0x203A,0x0153,0x009D,0x017E,0x0178
	};

	for (size_t i = 0; i < Size; ++i)
	{
		uint8_t C = Data[i];
		if (C == 0) break;

		if (C < 0x80)
		{
			Result += static_cast<char>(C);
		}
		else if (C >= 0x80 && C <= 0x9F)
		{
			uint16_t Unicode = CP1252_TABLE[C - 0x80];
			if (Unicode < 0x800)
			{
				Result += static_cast<char>(0xC0 | (Unicode >> 6));
				Result += static_cast<char>(0x80 | (Unicode & 0x3F));
			}
			else
			{
				Result += static_cast<char>(0xE0 | (Unicode >> 12));
				Result += static_cast<char>(0x80 | ((Unicode >> 6) & 0x3F));
				Result += static_cast<char>(0x80 | (Unicode & 0x3F));
			}
		}
		else
		{
			Result += static_cast<char>(0xC0 | (C >> 6));
			Result += static_cast<char>(0x80 | (C & 0x3F));
		}
	}

	return Result;
}

inline bool IsLikelyUTF8(const uint8_t* Data, size_t Size)
{
	for (size_t i = 0; i < Size && Data[i] != 0; ++i)
	{
		uint8_t C = Data[i];
		if (C >= 0x80)
		{
			if ((C & 0xE0) == 0xC0)
			{
				if (i + 1 >= Size || (Data[i + 1] & 0xC0) != 0x80) return false;
				i++;
			}
			else if ((C & 0xF0) == 0xE0)
			{
				if (i + 2 >= Size || (Data[i + 1] & 0xC0) != 0x80 || (Data[i + 2] & 0xC0) != 0x80) return false;
				i += 2;
			}
			else if ((C & 0xF8) == 0xF0)
			{
				if (i + 3 >= Size || (Data[i + 1] & 0xC0) != 0x80 || (Data[i + 2] & 0xC0) != 0x80 || (Data[i + 3] & 0xC0) != 0x80) return false;
				i += 3;
			}
			else
			{
				return false;
			}
		}
	}
	return true;
}

class RawString
{
	public:
	enum StrType
	{
		Char,
		WChar,
		BZString,
		BString,
		WString,
		WZString,
		ZString,
		String,
		List
	};

	std::string Data;
	std::string Encoding;

	RawString() {}
	RawString(const std::string& Str, const std::string& Enc = "utf8")
		: Data(Str), Encoding(Enc)
	{
	}

	static RawString Parse(const uint8_t* Bytes, size_t Size, StrType Type)
	{
		switch (Type)
		{
		case Char:
		{
			return RawString(std::string(reinterpret_cast<const char*>(Bytes), 1));
		}
		case WChar:
		case WString:
		{
			if (Size < 2) return RawString("");
			std::string Utf8;
			for (size_t i = 0; i + 1 < Size; i += 2)
			{
				uint16_t WC;
				std::memcpy(&WC, Bytes + i, 2);
				if (WC == 0) break;
				if (WC < 0x80)
				{
					Utf8 += static_cast<char>(WC);
				}
				else
				{
					if (WC < 0x800)
					{
						Utf8 += static_cast<char>(0xC0 | (WC >> 6));
						Utf8 += static_cast<char>(0x80 | (WC & 0x3F));
					}
					else
					{
						Utf8 += static_cast<char>(0xE0 | (WC >> 12));
						Utf8 += static_cast<char>(0x80 | ((WC >> 6) & 0x3F));
						Utf8 += static_cast<char>(0x80 | (WC & 0x3F));
					}
				}
			}
			return RawString(Utf8);
		}
		case WZString:
		{
			if (Size < 2) return RawString("");
			std::string Utf8;
			for (size_t i = 0; i + 1 < Size; i += 2)
			{
				uint16_t WC;
				std::memcpy(&WC, Bytes + i, 2);
				if (WC == 0) break;
				if (WC < 0x80)
				{
					Utf8 += static_cast<char>(WC);
				}
				else
				{
					if (WC < 0x800)
					{
						Utf8 += static_cast<char>(0xC0 | (WC >> 6));
						Utf8 += static_cast<char>(0x80 | (WC & 0x3F));
					}
					else
					{
						Utf8 += static_cast<char>(0xE0 | (WC >> 12));
						Utf8 += static_cast<char>(0x80 | ((WC >> 6) & 0x3F));
						Utf8 += static_cast<char>(0x80 | (WC & 0x3F));
					}
				}
			}
			return RawString(Utf8);
		}
		case BString:
		case BZString:
		case ZString:
		case String:
		default:
		{
			size_t ActualSize = 0;
			for (size_t i = 0; i < Size; ++i)
			{
				if (Bytes[i] == 0)
				{
					ActualSize = i;
					break;
				}
			}

			if (ActualSize == 0)
				ActualSize = Size;

			if (IsLikelyUTF8(Bytes, ActualSize))
			{
				return RawString(std::string(reinterpret_cast<const char*>(Bytes), ActualSize));
			}
			else
			{
				return RawString(Windows1252ToUTF8(Bytes, ActualSize));
			}
		}
		}
	}

	static RawString FromBytes(const std::vector<uint8_t>& Bytes, StrType Type = String)
	{
		return Parse(Bytes.data(), Bytes.size(), Type);
	}

	std::string ToUTF8String() const { return Data; }

	std::vector<uint8_t> Dump(StrType Type) const
	{
		switch (Type)
		{
		case Char:
		case String:
		{
			return std::vector<uint8_t>(Data.begin(), Data.end());
		}
		case BZString:
		{
			std::vector<uint8_t> Result;
			Result.push_back(static_cast<uint8_t>(Data.size()));
			Result.insert(Result.end(), Data.begin(), Data.end());
			Result.push_back(0);
			return Result;
		}
		default:
		{
			throw std::runtime_error("Dump not implemented for this type");
		}
		}
	}
};

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
		if (index >= count)
		{
			return "";
		}

		const uint8_t* data = reinterpret_cast<const uint8_t*>(strings[index].c_str());
		size_t size = strings[index].size() * sizeof(wchar_t); 

		RawString rawString = RawString::Parse(data, size, RawString::WString);
		return rawString.ToUTF8String();
	}
};