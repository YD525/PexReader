#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <variant>
#include <vector>

using namespace std;

class PexBinaryReader
{
public:
    static constexpr std::uint64_t MaxFileSize = 64ULL * 1024ULL * 1024ULL;
    static constexpr std::size_t MaxAggregateElements = 1'000'000;

    explicit PexBinaryReader(std::ifstream& stream)
        : _stream(stream), _size(0), _position(0), _aggregateElements(0), _failed(false)
    {
        _stream.seekg(0, std::ios::end);
        const std::streampos end = _stream.tellg();
        if (end < 0)
            throw std::runtime_error("Unable to determine the PEX file size.");

        _size = static_cast<std::uint64_t>(end);
        if (_size > MaxFileSize)
            throw std::runtime_error("PEX file exceeds the configured size limit.");

        _stream.seekg(0, std::ios::beg);
        if (!_stream)
            throw std::runtime_error("Unable to seek to the start of the PEX file.");
    }

    [[nodiscard]] std::uint64_t Position() const noexcept
    {
        return _position;
    }

    [[nodiscard]] std::uint64_t Remaining() const noexcept
    {
        return _size - _position;
    }

    [[nodiscard]] bool Failed() const noexcept
    {
        return _failed;
    }

    void Read(void* destination, std::size_t byteCount, const char* context)
    {
        if (byteCount > Remaining())
            Fail(std::string(context) + " exceeds the remaining PEX input.");

        if (byteCount == 0)
            return;

        if (byteCount > static_cast<std::size_t>((std::numeric_limits<std::streamsize>::max)()))
            Fail(std::string(context) + " exceeds the stream read limit.");

        _stream.read(static_cast<char*>(destination), static_cast<std::streamsize>(byteCount));
        if (_stream.gcount() != static_cast<std::streamsize>(byteCount) || !_stream)
            Fail(std::string(context) + " could not be read completely.");

        _position += byteCount;
    }

    void ValidateCount(
        std::size_t count,
        std::size_t minimumBytesPerElement,
        const char* context)
    {
        if (count > MaxAggregateElements - _aggregateElements)
            Fail(std::string(context) + " exceeds the aggregate element limit.");

        if (minimumBytesPerElement != 0 && count > Remaining() / minimumBytesPerElement)
            Fail(std::string(context) + " exceeds the remaining PEX input.");

        _aggregateElements += count;
    }

    void ValidateByteLength(std::size_t byteCount, const char* context)
    {
        if (byteCount > Remaining())
            Fail(std::string(context) + " exceeds the remaining PEX input.");
    }

private:
    [[noreturn]] void Fail(const std::string& message)
    {
        _failed = true;
        throw std::runtime_error(message);
    }

    std::ifstream& _stream;
    std::uint64_t _size;
    std::uint64_t _position;
    std::size_t _aggregateElements;
    bool _failed;
};

inline std::uint8_t ReadUInt8(PexBinaryReader& reader)
{
    std::uint8_t value = 0;
    reader.Read(&value, sizeof(value), "8-bit value");
    return value;
}

inline std::uint16_t ReadUInt16BE(PexBinaryReader& reader)
{
    std::uint8_t bytes[2]{};
    reader.Read(bytes, sizeof(bytes), "16-bit value");
    return (static_cast<std::uint16_t>(bytes[0]) << 8) | bytes[1];
}

inline std::uint32_t ReadUInt32BE(PexBinaryReader& reader)
{
    std::uint8_t bytes[4]{};
    reader.Read(bytes, sizeof(bytes), "32-bit value");
    return (static_cast<std::uint32_t>(bytes[0]) << 24) |
        (static_cast<std::uint32_t>(bytes[1]) << 16) |
        (static_cast<std::uint32_t>(bytes[2]) << 8) |
        bytes[3];
}

inline std::uint64_t ReadUInt64BE(PexBinaryReader& reader)
{
    std::uint8_t bytes[8]{};
    reader.Read(bytes, sizeof(bytes), "64-bit value");
    return (static_cast<std::uint64_t>(bytes[0]) << 56) |
        (static_cast<std::uint64_t>(bytes[1]) << 48) |
        (static_cast<std::uint64_t>(bytes[2]) << 40) |
        (static_cast<std::uint64_t>(bytes[3]) << 32) |
        (static_cast<std::uint64_t>(bytes[4]) << 24) |
        (static_cast<std::uint64_t>(bytes[5]) << 16) |
        (static_cast<std::uint64_t>(bytes[6]) << 8) |
        bytes[7];
}

inline std::int32_t ReadInt32BE(PexBinaryReader& reader)
{
    return static_cast<std::int32_t>(ReadUInt32BE(reader));
}

inline float ReadFloatBE(PexBinaryReader& reader)
{
    const std::uint32_t intValue = ReadUInt32BE(reader);
    float result = 0.0F;
    std::memcpy(&result, &intValue, sizeof(result));
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

inline std::uint16_t CheckedUInt16Length(std::size_t length, const char* context)
{
    if (length > (std::numeric_limits<std::uint16_t>::max)())
        throw std::length_error(std::string(context) + " exceeds the PEX 16-bit length limit.");

    return static_cast<std::uint16_t>(length);
}

inline void WriteWString(std::ofstream& f, const wstring& str)
{
    const uint16_t length = CheckedUInt16Length(str.length(), "PEX string");
    WriteUInt16BE(f, length);

    for (wchar_t wc : str)
    {
        char c = static_cast<char>(wc);
        f.write(&c, 1);
    }
}

// PEX strings use length-prefixed bytes rather than UTF-16 code units.
inline std::wstring ReadWString(PexBinaryReader& reader)
{
    const std::uint16_t length = ReadUInt16BE(reader);
    if (length == 0)
        return L"";

    reader.ValidateByteLength(length, "PEX string");
    std::vector<char> buffer(length);
    reader.Read(buffer.data(), buffer.size(), "PEX string");

    std::wstring result;
    result.reserve(length);
    for (char c : buffer)
    {
        result.push_back(static_cast<wchar_t>(static_cast<unsigned char>(c)));
    }

    return result;
}

inline std::vector<std::byte> ReadBytes(PexBinaryReader& reader)
{
    const std::uint16_t length = ReadUInt16BE(reader);
    if (length == 0)
        return {};

    reader.ValidateByteLength(length, "PEX byte string");
    std::vector<std::byte> buffer(length);
    reader.Read(buffer.data(), buffer.size(), "PEX byte string");
    return buffer;
}
