#include "CppUnitTest.h"

#include "../PexReader/PexHelper.cpp"

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace
{
    struct Fixture
    {
        std::vector<std::uint8_t> bytes;
        std::size_t headerStringLengthOffset = 0;
        std::size_t stringCountOffset = 0;
        std::size_t opcodeOffset = 0;
        std::size_t callArgumentTypeOffset = 0;
        std::size_t callArgumentCountOffset = 0;
    };

    class TemporaryPexFile
    {
    public:
        TemporaryPexFile()
        {
            static std::atomic<unsigned long> sequence{ 0 };
            _path = std::filesystem::temp_directory_path() /
                (L"PexReaderTests-" + std::to_wstring(++sequence) + L".pex");
        }

        ~TemporaryPexFile()
        {
            std::error_code error;
            std::filesystem::remove(_path, error);
        }

        const std::filesystem::path& Path() const noexcept
        {
            return _path;
        }

        void Write(const std::vector<std::uint8_t>& bytes) const
        {
            std::ofstream output(_path, std::ios::binary | std::ios::trunc);
            if (!output)
                throw std::runtime_error("Unable to create the temporary PEX fixture.");

            output.write(
                reinterpret_cast<const char*>(bytes.data()),
                static_cast<std::streamsize>(bytes.size()));
            if (!output)
                throw std::runtime_error("Unable to write the temporary PEX fixture.");
        }

    private:
        std::filesystem::path _path;
    };

    void AppendUInt8(std::vector<std::uint8_t>& bytes, std::uint8_t value)
    {
        bytes.push_back(value);
    }

    void AppendUInt16(std::vector<std::uint8_t>& bytes, std::uint16_t value)
    {
        bytes.push_back(static_cast<std::uint8_t>(value >> 8));
        bytes.push_back(static_cast<std::uint8_t>(value));
    }

    void AppendUInt32(std::vector<std::uint8_t>& bytes, std::uint32_t value)
    {
        bytes.push_back(static_cast<std::uint8_t>(value >> 24));
        bytes.push_back(static_cast<std::uint8_t>(value >> 16));
        bytes.push_back(static_cast<std::uint8_t>(value >> 8));
        bytes.push_back(static_cast<std::uint8_t>(value));
    }

    void AppendUInt64(std::vector<std::uint8_t>& bytes, std::uint64_t value)
    {
        AppendUInt32(bytes, static_cast<std::uint32_t>(value >> 32));
        AppendUInt32(bytes, static_cast<std::uint32_t>(value));
    }

    void AppendString(std::vector<std::uint8_t>& bytes, const std::string& value)
    {
        AppendUInt16(bytes, static_cast<std::uint16_t>(value.size()));
        bytes.insert(bytes.end(), value.begin(), value.end());
    }

    void ReplaceUInt16(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint16_t value)
    {
        bytes.at(offset) = static_cast<std::uint8_t>(value >> 8);
        bytes.at(offset + 1) = static_cast<std::uint8_t>(value);
    }

    void ReplaceUInt32(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint32_t value)
    {
        bytes.at(offset) = static_cast<std::uint8_t>(value >> 24);
        bytes.at(offset + 1) = static_cast<std::uint8_t>(value >> 16);
        bytes.at(offset + 2) = static_cast<std::uint8_t>(value >> 8);
        bytes.at(offset + 3) = static_cast<std::uint8_t>(value);
    }

    Fixture CreateFixture()
    {
        Fixture fixture;
        std::vector<std::uint8_t>& bytes = fixture.bytes;

        AppendUInt32(bytes, 0xFA57C0DE);
        AppendUInt8(bytes, 3);
        AppendUInt8(bytes, 9);
        AppendUInt16(bytes, 1);
        AppendUInt64(bytes, 123456789);
        fixture.headerStringLengthOffset = bytes.size();
        AppendString(bytes, "Fixture.psc");
        AppendString(bytes, "Tester");
        AppendString(bytes, "BuildHost");

        fixture.stringCountOffset = bytes.size();
        const std::vector<std::string> strings{
            "ObjectName", "StateName", "FunctionName", "Int", "Variable", "Method", "Flag"
        };
        AppendUInt16(bytes, static_cast<std::uint16_t>(strings.size()));
        for (const std::string& value : strings)
            AppendString(bytes, value);

        AppendUInt8(bytes, 1);
        AppendUInt64(bytes, 987654321);
        AppendUInt16(bytes, 1);
        AppendUInt16(bytes, 0);
        AppendUInt16(bytes, 1);
        AppendUInt16(bytes, 2);
        AppendUInt8(bytes, 0);
        AppendUInt16(bytes, 1);
        AppendUInt16(bytes, 42);

        AppendUInt16(bytes, 1);
        AppendUInt16(bytes, 6);
        AppendUInt8(bytes, 3);

        AppendUInt16(bytes, 1);
        AppendUInt16(bytes, 0);
        const std::size_t objectSizeOffset = bytes.size();
        AppendUInt32(bytes, 0);
        const std::size_t objectDataOffset = bytes.size();

        AppendUInt16(bytes, 0);
        AppendUInt16(bytes, 0);
        AppendUInt32(bytes, 0);
        AppendUInt16(bytes, 1);

        AppendUInt16(bytes, 1);
        AppendUInt16(bytes, 4);
        AppendUInt16(bytes, 3);
        AppendUInt32(bytes, 0);
        AppendUInt8(bytes, 3);
        AppendUInt32(bytes, 7);

        AppendUInt16(bytes, 0);

        AppendUInt16(bytes, 1);
        AppendUInt16(bytes, 1);
        AppendUInt16(bytes, 1);
        AppendUInt16(bytes, 2);

        AppendUInt16(bytes, 3);
        AppendUInt16(bytes, 0);
        AppendUInt32(bytes, 0);
        AppendUInt8(bytes, 0);

        AppendUInt16(bytes, 1);
        AppendUInt16(bytes, 4);
        AppendUInt16(bytes, 3);

        AppendUInt16(bytes, 1);
        AppendUInt16(bytes, 4);
        AppendUInt16(bytes, 3);

        AppendUInt16(bytes, 1);
        fixture.opcodeOffset = bytes.size();
        AppendUInt8(bytes, static_cast<std::uint8_t>(Opcode::callmethod));
        AppendUInt8(bytes, 0);
        AppendUInt8(bytes, 1);
        AppendUInt16(bytes, 4);
        AppendUInt8(bytes, 2);
        AppendUInt16(bytes, 5);
        fixture.callArgumentTypeOffset = bytes.size();
        AppendUInt8(bytes, 3);
        fixture.callArgumentCountOffset = bytes.size();
        AppendUInt32(bytes, 1);
        AppendUInt8(bytes, 5);
        AppendUInt8(bytes, 1);

        const std::size_t objectSize = bytes.size() - objectDataOffset;
        ReplaceUInt32(bytes, objectSizeOffset, static_cast<std::uint32_t>(objectSize));
        return fixture;
    }

    bool LoadFails(const std::vector<std::uint8_t>& bytes)
    {
        TemporaryPexFile file;
        file.Write(bytes);

        try
        {
            PexData data;
            data.Load(file.Path().wstring());
            return false;
        }
        catch (const std::exception&)
        {
            return true;
        }
    }

    std::filesystem::path GetTestModuleDirectory()
    {
        HMODULE module = nullptr;
        const BOOL found = GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&GetTestModuleDirectory),
            &module);
        if (!found)
            throw std::runtime_error("Unable to locate the test module.");

        std::wstring modulePath(MAX_PATH, L'\0');
        const DWORD length = GetModuleFileNameW(module, &modulePath[0], static_cast<DWORD>(modulePath.size()));
        if (length == 0 || length == modulePath.size())
            throw std::runtime_error("Unable to resolve the test module path.");

        modulePath.resize(length);
        return std::filesystem::path(modulePath).parent_path();
    }
}

namespace PexReaderTests
{
    TEST_CLASS(PexParserTests)
    {
    public:
        TEST_METHOD(LoadsCompleteFixture)
        {
            const Fixture fixture = CreateFixture();
            TemporaryPexFile file;
            file.Write(fixture.bytes);

            PexData data;
            data.Load(file.Path().wstring());

            Assert::AreEqual<std::uint16_t>(7, data.stringTable.count);
            Assert::AreEqual<std::uint16_t>(1, data.debugInfo.functionCount);
            Assert::AreEqual<std::uint16_t>(1, data.userFlagCount);
            Assert::AreEqual<std::uint16_t>(1, data.objectCount);
            Assert::AreEqual<std::uint16_t>(1, data.objects[0].data.states[0].numFunctions);
        }

        TEST_METHOD(RejectsEveryTruncatedPrefix)
        {
            const Fixture fixture = CreateFixture();
            for (std::size_t length = 0; length < fixture.bytes.size(); ++length)
            {
                const std::vector<std::uint8_t> prefix(fixture.bytes.begin(), fixture.bytes.begin() + length);
                Assert::IsTrue(LoadFails(prefix), L"A truncated PEX prefix was accepted.");
            }
        }

        TEST_METHOD(RejectsImpossibleStringCountBeforeAllocation)
        {
            Fixture fixture = CreateFixture();
            ReplaceUInt16(fixture.bytes, fixture.stringCountOffset, 0xFFFF);

            Assert::IsTrue(LoadFails(fixture.bytes));
        }

        TEST_METHOD(RejectsImpossibleStringLengthBeforeAllocation)
        {
            Fixture fixture = CreateFixture();
            ReplaceUInt16(fixture.bytes, fixture.headerStringLengthOffset, 0xFFFF);

            Assert::IsTrue(LoadFails(fixture.bytes));
        }

        TEST_METHOD(RejectsInvalidOpcode)
        {
            Fixture fixture = CreateFixture();
            fixture.bytes[fixture.opcodeOffset] = 0xFF;

            Assert::IsTrue(LoadFails(fixture.bytes));
        }

        TEST_METHOD(RejectsNonIntegerCallArgumentCount)
        {
            Fixture fixture = CreateFixture();
            fixture.bytes[fixture.callArgumentTypeOffset] = 5;

            Assert::IsTrue(LoadFails(fixture.bytes));
        }

        TEST_METHOD(RejectsNegativeCallArgumentCount)
        {
            Fixture fixture = CreateFixture();
            ReplaceUInt32(fixture.bytes, fixture.callArgumentCountOffset, 0xFFFFFFFF);

            Assert::IsTrue(LoadFails(fixture.bytes));
        }

        TEST_METHOD(RejectsExcessiveCallArgumentCount)
        {
            Fixture fixture = CreateFixture();
            ReplaceUInt32(fixture.bytes, fixture.callArgumentCountOffset, 4097);

            Assert::IsTrue(LoadFails(fixture.bytes));
        }

        TEST_METHOD(SaveAndReloadPreservesParsedStructure)
        {
            const Fixture fixture = CreateFixture();
            TemporaryPexFile input;
            TemporaryPexFile output;
            input.Write(fixture.bytes);

            PexData original;
            original.Load(input.Path().wstring());
            original.Save(output.Path().wstring());

            PexData reloaded;
            reloaded.Load(output.Path().wstring());

            Assert::AreEqual(original.Header.magic, reloaded.Header.magic);
            Assert::AreEqual(original.stringTable.count, reloaded.stringTable.count);
            Assert::AreEqual(original.debugInfo.functionCount, reloaded.debugInfo.functionCount);
            Assert::AreEqual(original.objectCount, reloaded.objectCount);
            Assert::AreEqual(original.objects[0].size, reloaded.objects[0].size);
            Assert::AreEqual(
                original.objects[0].data.states[0].numFunctions,
                reloaded.objects[0].data.states[0].numFunctions);
        }

        TEST_METHOD(FailedLoadPreservesPreviousValidState)
        {
            const Fixture fixture = CreateFixture();
            TemporaryPexFile validFile;
            TemporaryPexFile invalidFile;
            validFile.Write(fixture.bytes);
            invalidFile.Write(std::vector<std::uint8_t>(fixture.bytes.begin(), fixture.bytes.end() - 1));

            PexData data;
            data.Load(validFile.Path().wstring());

            bool failed = false;
            try
            {
                data.Load(invalidFile.Path().wstring());
            }
            catch (const std::exception&)
            {
                failed = true;
            }

            Assert::IsTrue(failed);
            Assert::AreEqual<std::uint16_t>(7, data.stringTable.count);
            Assert::AreEqual<std::uint16_t>(1, data.objectCount);
            Assert::AreEqual<std::uint16_t>(1, data.objects[0].data.states[0].numFunctions);
        }

        TEST_METHOD(CAbiReportsFailureWithoutReplacingValidState)
        {
            using CreateInstance = std::intptr_t(*)();
            using DestroyInstance = void(*)(std::intptr_t);
            using ReadPex = int(*)(std::intptr_t, const wchar_t*);
            using GetStringTableCount = std::uint16_t(*)(std::intptr_t);

            const Fixture fixture = CreateFixture();
            TemporaryPexFile validFile;
            TemporaryPexFile invalidFile;
            validFile.Write(fixture.bytes);
            invalidFile.Write(std::vector<std::uint8_t>(fixture.bytes.begin(), fixture.bytes.end() - 1));

            const std::filesystem::path testDirectory = GetTestModuleDirectory();
            std::filesystem::path libraryPath = testDirectory / L"Pex.Interop.dll";
            if (!std::filesystem::exists(libraryPath))
            {
                libraryPath = testDirectory.parent_path().parent_path().parent_path() /
                    L"x64" / L"Release" / L"Pex.Interop.dll";
            }
            HMODULE library = LoadLibraryW(libraryPath.c_str());
            Assert::IsNotNull(library, L"Pex.Interop.dll could not be loaded.");

            const auto createInstance = reinterpret_cast<CreateInstance>(GetProcAddress(library, "C_CreateInstance"));
            const auto destroyInstance =
                reinterpret_cast<DestroyInstance>(GetProcAddress(library, "C_DestroyInstance"));
            const auto readPex = reinterpret_cast<ReadPex>(GetProcAddress(library, "C_ReadPex"));
            const auto getStringTableCount =
                reinterpret_cast<GetStringTableCount>(GetProcAddress(library, "C_GetStringTableCount"));
            Assert::IsNotNull(createInstance);
            Assert::IsNotNull(destroyInstance);
            Assert::IsNotNull(readPex);
            Assert::IsNotNull(getStringTableCount);

            const std::intptr_t handle = createInstance();
            Assert::AreNotEqual<std::intptr_t>(0, handle);
            Assert::AreEqual(1, readPex(handle, validFile.Path().c_str()));
            Assert::AreEqual<std::uint16_t>(7, getStringTableCount(handle));
            Assert::AreEqual(0, readPex(handle, invalidFile.Path().c_str()));
            Assert::AreEqual<std::uint16_t>(7, getStringTableCount(handle));
            Assert::AreEqual(0, readPex(0, validFile.Path().c_str()));

            destroyInstance(handle);
            FreeLibrary(library);
        }
    };
}
