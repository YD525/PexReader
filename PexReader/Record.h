#pragma once
#include <string>
#include <vector>
#include <variant> 
#include <fstream>
#include <cstdint>

using namespace std;


inline uint16_t ReadUInt16BE(std::ifstream& f)
{
    uint8_t bytes[2];
    f.read(reinterpret_cast<char*>(bytes), 2);
    return (static_cast<uint16_t>(bytes[0]) << 8) | bytes[1];
}

inline uint32_t ReadUInt32BE(std::ifstream& f)
{
    uint8_t bytes[4];
    f.read(reinterpret_cast<char*>(bytes), 4);
    return (static_cast<uint32_t>(bytes[0]) << 24) |
        (static_cast<uint32_t>(bytes[1]) << 16) |
        (static_cast<uint32_t>(bytes[2]) << 8) |
        bytes[3];
}

inline uint64_t ReadUInt64BE(std::ifstream& f)
{
    uint8_t bytes[8];
    f.read(reinterpret_cast<char*>(bytes), 8);
    return (static_cast<uint64_t>(bytes[0]) << 56) |
        (static_cast<uint64_t>(bytes[1]) << 48) |
        (static_cast<uint64_t>(bytes[2]) << 40) |
        (static_cast<uint64_t>(bytes[3]) << 32) |
        (static_cast<uint64_t>(bytes[4]) << 24) |
        (static_cast<uint64_t>(bytes[5]) << 16) |
        (static_cast<uint64_t>(bytes[6]) << 8) |
        bytes[7];
}

inline uint8_t ReadUInt8(std::ifstream& f)
{
    uint8_t value;
    f.read(reinterpret_cast<char*>(&value), 1);
    return value;
}

inline int32_t ReadInt32BE(std::ifstream& f)
{
    return static_cast<int32_t>(ReadUInt32BE(f));
}

inline float ReadFloatBE(std::ifstream& f)
{
    uint32_t intVal = ReadUInt32BE(f);
    float result;
    std::memcpy(&result, &intVal, sizeof(float));
    return result;
}

inline wstring ReadWString(std::ifstream& f)
{
    uint16_t length = ReadUInt16BE(f);
    wstring result;
    result.reserve(length);
    for (uint16_t i = 0; i < length; ++i)
    {
        wchar_t ch;
        f.read(reinterpret_cast<char*>(&ch), sizeof(wchar_t));
        result.push_back(ch);
    }
    return result;
}

inline void WriteUInt16BE(std::ofstream& f, uint16_t value)
{
    uint8_t bytes[2];
    bytes[0] = (value >> 8) & 0xFF;
    bytes[1] = value & 0xFF;
    f.write(reinterpret_cast<char*>(bytes), 2);
}

inline void WriteUInt32BE(std::ofstream& f, uint32_t value)
{
    uint8_t bytes[4];
    bytes[0] = (value >> 24) & 0xFF;
    bytes[1] = (value >> 16) & 0xFF;
    bytes[2] = (value >> 8) & 0xFF;
    bytes[3] = value & 0xFF;
    f.write(reinterpret_cast<char*>(bytes), 4);
}

inline void WriteUInt64BE(std::ofstream& f, uint64_t value)
{
    uint8_t bytes[8];
    bytes[0] = (value >> 56) & 0xFF;
    bytes[1] = (value >> 48) & 0xFF;
    bytes[2] = (value >> 40) & 0xFF;
    bytes[3] = (value >> 32) & 0xFF;
    bytes[4] = (value >> 24) & 0xFF;
    bytes[5] = (value >> 16) & 0xFF;
    bytes[6] = (value >> 8) & 0xFF;
    bytes[7] = value & 0xFF;
    f.write(reinterpret_cast<char*>(bytes), 8);
}

inline void WriteUInt8(std::ofstream& f, uint8_t value)
{
    f.write(reinterpret_cast<char*>(&value), 1);
}

inline void WriteInt32BE(std::ofstream& f, int32_t value)
{
    WriteUInt32BE(f, static_cast<uint32_t>(value));
}

inline void WriteFloatBE(std::ofstream& f, float value)
{
    uint32_t intVal;
    std::memcpy(&intVal, &value, sizeof(float));
    WriteUInt32BE(f, intVal);
}

inline void WriteWString(std::ofstream& f, const wstring& str)
{
    uint16_t length = static_cast<uint16_t>(str.length());
    WriteUInt16BE(f, length);
    for (wchar_t ch : str)
    {
        f.write(reinterpret_cast<const char*>(&ch), sizeof(wchar_t));
    }
}