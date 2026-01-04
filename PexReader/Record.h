#pragma once
#include <string>
#include <vector>
#include <variant> 
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cstddef>

using namespace std;

inline uint16_t ReadUInt16BE(std::ifstream& stream)
{
    uint8_t bytes[2];
    stream.read(reinterpret_cast<char*>(bytes), 2);
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

inline string ReadString(std::ifstream& f)
{
    uint16_t length = ReadUInt16BE(f);
    if (length == 0) return "";

    vector<char> buffer(length);
    f.read(buffer.data(), length);

    return string(buffer.begin(), buffer.end());
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

    for (wchar_t wc : str)
    {
        char c = static_cast<char>(wc);
        f.write(&c, 1);
    }
}

//PEX's "WString" is not UTF-16, but rather ASCII/UTF-8 (1 byte per character).
inline std::wstring ReadWString(std::ifstream& f)
{
    uint16_t length = ReadUInt16BE(f);

    if (length == 0)
    {
        return L"";
    }

    std::vector<char> buffer(length);
    f.read(buffer.data(), length);

    std::wstring result;
    result.reserve(length);
    for (char c : buffer)
    {
        result.push_back(static_cast<wchar_t>(static_cast<unsigned char>(c)));
    }

    return result;
}

inline std::vector<byte> ReadBytes(std::ifstream& f) 
{
    uint16_t length = ReadUInt16BE(f);

    if (length == 0) 
    {
        return {};
    }

    std::vector<char> buffer(length);
    f.read(buffer.data(), length);

    std::vector<byte> byteArray;
    for (char c : buffer) {
        byteArray.push_back(static_cast<byte>(static_cast<unsigned char>(c)));  
    }

    return  byteArray;
}


inline std::pair<std::wstring, std::vector<byte>> ReadWStringR(std::ifstream& f) {
    uint16_t length = ReadUInt16BE(f);  

    if (length == 0) {
        return { L"", {} };  
    }

    std::vector<char> buffer(length); 
    f.read(buffer.data(), length);   


    std::wstring result;
    result.reserve(length);
    for (char c : buffer) {
        result.push_back(static_cast<wchar_t>(static_cast<unsigned char>(c)));  
    }


    std::vector<byte> byteArray;
    for (char c : buffer) {
        byteArray.push_back(static_cast<byte>(static_cast<unsigned char>(c))); 
    }


    return { result, byteArray };
}

inline std::vector<byte> ReadBytes(std::ifstream& f, uint16_t length)
{
    std::vector<byte> buffer(length);
    f.read(reinterpret_cast<char*>(buffer.data()), length);
    return buffer;
}

inline std::vector<byte> ReadBytesUntilNull(std::ifstream& f, size_t maxLength = 1024)
{
    std::vector<byte> buffer;
    byte byteRead;

    size_t bytesRead = 0;

    while (f.read(reinterpret_cast<char*>(&byteRead), 1))
    {
        if (bytesRead >= maxLength)
        {
            return {};
        }

        if (static_cast<unsigned char>(byteRead) == 0)
        {
            break;
        }

        buffer.push_back(byteRead);
        bytesRead++;
    }

    return buffer;
}


using byte = std::byte;

inline void PrintHexAndText(const std::vector<byte>& data) {

    std::cout << "Hex: ";
    for (byte b : data) 
    {
        unsigned char byteValue = static_cast<unsigned char>(b);
        std::cout << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(byteValue) << " ";
    }
    std::cout << std::dec << std::endl; 

    std::cout << "ASCII: ";
    for (byte b : data) 
    {
        unsigned char byteValue = static_cast<unsigned char>(b);
        if (byteValue >= 32 && byteValue <= 126) {
            std::cout << static_cast<char>(byteValue);
        }
        else {
            std::cout << '.';
        }
    }
    std::cout << std::endl;

    std::cout << "UTF-8: ";
    for (byte b : data) 
    {
        unsigned char byteValue = static_cast<unsigned char>(b);
        if (byteValue < 32 && byteValue != 9 && byteValue != 10 && byteValue != 13) {
            std::cout << '?';
        }
        else {
            std::cout << static_cast<char>(byteValue);
        }
    }
    std::cout << std::endl;
}