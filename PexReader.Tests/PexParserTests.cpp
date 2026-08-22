#include "CppUnitTest.h"

#include "../PexReader/PexHelper.cpp"

#include <algorithm>
#include <atomic>
#include <cctype>
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
        std::size_t debugFunctionCountOffset = 0;
        std::size_t userFlagCountOffset = 0;
        std::size_t objectCountOffset = 0;
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

        std::vector<std::uint8_t> Read() const
        {
            std::ifstream input(_path, std::ios::binary);
            if (!input)
                throw std::runtime_error("Unable to read the temporary PEX fixture.");

            return std::vector<std::uint8_t>(
                std::istreambuf_iterator<char>(input),
                std::istreambuf_iterator<char>());
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

    Fixture CreateFixture(std::uint8_t minorVersion = 2, bool includeUnicode = true)
    {
        Fixture fixture;
        std::vector<std::uint8_t>& bytes = fixture.bytes;

        AppendUInt32(bytes, 0xFA57C0DE);
        AppendUInt8(bytes, 3);
        AppendUInt8(bytes, minorVersion);
        AppendUInt16(bytes, 1);
        AppendUInt64(bytes, 123456789);
        fixture.headerStringLengthOffset = bytes.size();
        AppendString(bytes, "Fixture.psc");
        AppendString(bytes, "Tester");
        AppendString(bytes, "BuildHost");

        fixture.stringCountOffset = bytes.size();
        const std::string unicodeState =
            "Gr\xC3\xBC\xC3\x9F" "e \xE6\x9D\xB1\xE4\xBA\xAC";
        const std::vector<std::string> strings{
            "ObjectName",
            includeUnicode ? unicodeState : "StateName",
            "FunctionName",
            "Int",
            "Variable",
            "Method",
            "Flag"
        };
        AppendUInt16(bytes, static_cast<std::uint16_t>(strings.size()));
        for (const std::string& value : strings)
            AppendString(bytes, value);

        AppendUInt8(bytes, 1);
        AppendUInt64(bytes, 987654321);
        fixture.debugFunctionCountOffset = bytes.size();
        AppendUInt16(bytes, 1);
        AppendUInt16(bytes, 0);
        AppendUInt16(bytes, 1);
        AppendUInt16(bytes, 2);
        AppendUInt8(bytes, 0);
        AppendUInt16(bytes, 2);
        AppendUInt16(bytes, 42);
        AppendUInt16(bytes, 43);

        fixture.userFlagCountOffset = bytes.size();
        AppendUInt16(bytes, 1);
        AppendUInt16(bytes, 6);
        AppendUInt8(bytes, 3);

        fixture.objectCountOffset = bytes.size();
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

        AppendUInt16(bytes, 2);

        AppendUInt16(bytes, 4);
        AppendUInt16(bytes, 3);
        AppendUInt16(bytes, 1);
        AppendUInt32(bytes, 0x11);
        AppendUInt8(bytes, 3);

        AppendUInt16(bytes, 3);
        AppendUInt16(bytes, 1);
        AppendUInt32(bytes, 0x22);
        AppendUInt8(bytes, 1);
        AppendUInt16(bytes, 0);
        AppendUInt16(bytes, 0);
        AppendUInt16(bytes, 0);

        AppendUInt16(bytes, 3);
        AppendUInt16(bytes, 1);
        AppendUInt32(bytes, 0x33);
        AppendUInt8(bytes, 2);
        AppendUInt16(bytes, 0);
        AppendUInt16(bytes, 0);
        AppendUInt16(bytes, 0);

        AppendUInt16(bytes, 5);
        AppendUInt16(bytes, 3);
        AppendUInt16(bytes, 1);
        AppendUInt32(bytes, 0x44);
        AppendUInt8(bytes, 4);
        AppendUInt16(bytes, 4);

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

        AppendUInt16(bytes, 2);
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

        AppendUInt8(bytes, static_cast<std::uint8_t>(Opcode::fadd));
        AppendUInt8(bytes, 4);
        AppendUInt32(bytes, 0x3FC00000);
        AppendUInt8(bytes, 4);
        AppendUInt32(bytes, 0xC0000000);
        AppendUInt8(bytes, 4);
        AppendUInt32(bytes, 0x3F000000);

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

    int HexValue(char value)
    {
        if (value >= '0' && value <= '9')
            return value - '0';
        if (value >= 'A' && value <= 'F')
            return value - 'A' + 10;
        if (value >= 'a' && value <= 'f')
            return value - 'a' + 10;
        return -1;
    }

    std::vector<std::uint8_t> ReadHexFixture(const wchar_t* name)
    {
        const std::filesystem::path path = GetTestModuleDirectory() / L"Fixtures" / name;
        std::ifstream input(path, std::ios::binary);
        if (!input)
            throw std::runtime_error("Unable to open a documented PEX fixture.");

        std::vector<std::uint8_t> bytes;
        int highNibble = -1;
        char character = 0;
        while (input.get(character))
        {
            if (std::isspace(static_cast<unsigned char>(character)))
                continue;

            const int value = HexValue(character);
            if (value < 0)
                throw std::runtime_error("A PEX fixture contains a non-hexadecimal character.");

            if (highNibble < 0)
                highNibble = value;
            else
            {
                bytes.push_back(static_cast<std::uint8_t>((highNibble << 4) | value));
                highNibble = -1;
            }
        }

        if (highNibble >= 0 || bytes.empty())
            throw std::runtime_error("A PEX fixture contains an incomplete hexadecimal byte.");
        return bytes;
    }

    void AssertVariableDataEqual(const VariableData& expected, const VariableData& actual)
    {
        Assert::AreEqual(expected.type, actual.type);
        Assert::IsTrue(expected.data == actual.data);
    }

    void AssertFunctionEqual(const Function& expected, const Function& actual)
    {
        Assert::AreEqual(expected.returnType, actual.returnType);
        Assert::AreEqual(expected.docString, actual.docString);
        Assert::AreEqual(expected.userFlags, actual.userFlags);
        Assert::AreEqual(expected.flags, actual.flags);
        Assert::AreEqual(expected.numParams, actual.numParams);
        Assert::AreEqual(expected.params.size(), actual.params.size());
        for (std::size_t index = 0; index < expected.params.size(); ++index)
        {
            Assert::AreEqual(expected.params[index].name, actual.params[index].name);
            Assert::AreEqual(expected.params[index].type, actual.params[index].type);
        }

        Assert::AreEqual(expected.numLocals, actual.numLocals);
        Assert::AreEqual(expected.locals.size(), actual.locals.size());
        for (std::size_t index = 0; index < expected.locals.size(); ++index)
        {
            Assert::AreEqual(expected.locals[index].name, actual.locals[index].name);
            Assert::AreEqual(expected.locals[index].type, actual.locals[index].type);
        }

        Assert::AreEqual(expected.numInstructions, actual.numInstructions);
        Assert::AreEqual(expected.instructions.size(), actual.instructions.size());
        for (std::size_t instructionIndex = 0;
            instructionIndex < expected.instructions.size();
            ++instructionIndex)
        {
            const Instruction& expectedInstruction = expected.instructions[instructionIndex];
            const Instruction& actualInstruction = actual.instructions[instructionIndex];
            Assert::AreEqual(
                static_cast<std::uint8_t>(expectedInstruction.op),
                static_cast<std::uint8_t>(actualInstruction.op));
            Assert::AreEqual(expectedInstruction.arguments.size(), actualInstruction.arguments.size());
            for (std::size_t argumentIndex = 0;
                argumentIndex < expectedInstruction.arguments.size();
                ++argumentIndex)
            {
                AssertVariableDataEqual(
                    expectedInstruction.arguments[argumentIndex],
                    actualInstruction.arguments[argumentIndex]);
            }
        }
    }

    void AssertPexDataEqual(const PexData& expected, const PexData& actual)
    {
        Assert::AreEqual(expected.Header.magic, actual.Header.magic);
        Assert::AreEqual(expected.Header.majorVersion, actual.Header.majorVersion);
        Assert::AreEqual(expected.Header.minorVersion, actual.Header.minorVersion);
        Assert::AreEqual(expected.Header.gameId, actual.Header.gameId);
        Assert::AreEqual(expected.Header.compilationTime, actual.Header.compilationTime);
        Assert::IsTrue(expected.Header.sourceFileName == actual.Header.sourceFileName);
        Assert::IsTrue(expected.Header.username == actual.Header.username);
        Assert::IsTrue(expected.Header.machinename == actual.Header.machinename);

        Assert::AreEqual(expected.stringTable.count, actual.stringTable.count);
        Assert::AreEqual(expected.stringTable.strings.size(), actual.stringTable.strings.size());
        for (std::size_t index = 0; index < expected.stringTable.strings.size(); ++index)
            Assert::IsTrue(expected.stringTable.strings[index] == actual.stringTable.strings[index]);

        Assert::AreEqual(expected.debugInfo.hasDebugInfo, actual.debugInfo.hasDebugInfo);
        Assert::AreEqual(expected.debugInfo.modificationTime, actual.debugInfo.modificationTime);
        Assert::AreEqual(expected.debugInfo.functionCount, actual.debugInfo.functionCount);
        Assert::AreEqual(expected.debugInfo.functions.size(), actual.debugInfo.functions.size());
        for (std::size_t index = 0; index < expected.debugInfo.functions.size(); ++index)
        {
            const DebugFunction& expectedFunction = expected.debugInfo.functions[index];
            const DebugFunction& actualFunction = actual.debugInfo.functions[index];
            Assert::AreEqual(expectedFunction.objectNameIndex, actualFunction.objectNameIndex);
            Assert::AreEqual(expectedFunction.stateNameIndex, actualFunction.stateNameIndex);
            Assert::AreEqual(expectedFunction.functionNameIndex, actualFunction.functionNameIndex);
            Assert::AreEqual(expectedFunction.functionType, actualFunction.functionType);
            Assert::AreEqual(expectedFunction.instructionCount, actualFunction.instructionCount);
            Assert::IsTrue(expectedFunction.lineNumbers == actualFunction.lineNumbers);
        }

        Assert::AreEqual(expected.userFlagCount, actual.userFlagCount);
        Assert::AreEqual(expected.userFlags.size(), actual.userFlags.size());
        for (std::size_t index = 0; index < expected.userFlags.size(); ++index)
        {
            Assert::AreEqual(expected.userFlags[index].flagNameIndex, actual.userFlags[index].flagNameIndex);
            Assert::AreEqual(expected.userFlags[index].flagIndex, actual.userFlags[index].flagIndex);
        }

        Assert::AreEqual(expected.objectCount, actual.objectCount);
        Assert::AreEqual(expected.objects.size(), actual.objects.size());
        for (std::size_t objectIndex = 0; objectIndex < expected.objects.size(); ++objectIndex)
        {
            const Object& expectedObject = expected.objects[objectIndex];
            const Object& actualObject = actual.objects[objectIndex];
            Assert::AreEqual(expectedObject.nameIndex, actualObject.nameIndex);
            Assert::AreEqual(expectedObject.size, actualObject.size);

            const ObjectData& expectedData = expectedObject.data;
            const ObjectData& actualData = actualObject.data;
            Assert::AreEqual(expectedData.parentClassName, actualData.parentClassName);
            Assert::AreEqual(expectedData.docString, actualData.docString);
            Assert::AreEqual(expectedData.userFlags, actualData.userFlags);
            Assert::AreEqual(expectedData.autoStateName, actualData.autoStateName);

            Assert::AreEqual(expectedData.numVariables, actualData.numVariables);
            Assert::AreEqual(expectedData.variables.size(), actualData.variables.size());
            for (std::size_t variableIndex = 0; variableIndex < expectedData.variables.size(); ++variableIndex)
            {
                const Variable& expectedVariable = expectedData.variables[variableIndex];
                const Variable& actualVariable = actualData.variables[variableIndex];
                Assert::AreEqual(expectedVariable.name, actualVariable.name);
                Assert::AreEqual(expectedVariable.typeName, actualVariable.typeName);
                Assert::AreEqual(expectedVariable.userFlags, actualVariable.userFlags);
                AssertVariableDataEqual(expectedVariable.data, actualVariable.data);
            }

            Assert::AreEqual(expectedData.numProperties, actualData.numProperties);
            Assert::AreEqual(expectedData.properties.size(), actualData.properties.size());
            for (std::size_t propertyIndex = 0; propertyIndex < expectedData.properties.size(); ++propertyIndex)
            {
                const Property& expectedProperty = expectedData.properties[propertyIndex];
                const Property& actualProperty = actualData.properties[propertyIndex];
                Assert::AreEqual(expectedProperty.name, actualProperty.name);
                Assert::AreEqual(expectedProperty.type, actualProperty.type);
                Assert::AreEqual(expectedProperty.docstring, actualProperty.docstring);
                Assert::AreEqual(expectedProperty.userFlags, actualProperty.userFlags);
                Assert::AreEqual(expectedProperty.flags, actualProperty.flags);
                Assert::AreEqual(expectedProperty.autoVarName, actualProperty.autoVarName);
                AssertFunctionEqual(expectedProperty.readHandler, actualProperty.readHandler);
                AssertFunctionEqual(expectedProperty.writeHandler, actualProperty.writeHandler);
            }

            Assert::AreEqual(expectedData.numStates, actualData.numStates);
            Assert::AreEqual(expectedData.states.size(), actualData.states.size());
            for (std::size_t stateIndex = 0; stateIndex < expectedData.states.size(); ++stateIndex)
            {
                const State& expectedState = expectedData.states[stateIndex];
                const State& actualState = actualData.states[stateIndex];
                Assert::AreEqual(expectedState.name, actualState.name);
                Assert::AreEqual(expectedState.numFunctions, actualState.numFunctions);
                Assert::AreEqual(expectedState.functions.size(), actualState.functions.size());
                for (std::size_t functionIndex = 0;
                    functionIndex < expectedState.functions.size();
                    ++functionIndex)
                {
                    Assert::AreEqual(
                        expectedState.functions[functionIndex].functionName,
                        actualState.functions[functionIndex].functionName);
                    AssertFunctionEqual(
                        expectedState.functions[functionIndex].function,
                        actualState.functions[functionIndex].function);
                }
            }
        }
    }
}

namespace PexReaderTests
{
    TEST_CLASS(PexParserTests)
    {
    public:
        TEST_METHOD(LoadsDocumentedSkyrimVersionFixtures)
        {
            const std::vector<std::uint8_t> classicBytes =
                ReadHexFixture(L"skyrim-3.1.pex.hex");
            const std::vector<std::uint8_t> specialEditionBytes =
                ReadHexFixture(L"skyrim-se-3.2-unicode.pex.hex");
            Assert::IsTrue(classicBytes == CreateFixture(1, false).bytes);
            Assert::IsTrue(specialEditionBytes == CreateFixture(2, true).bytes);

            TemporaryPexFile classicFile;
            classicFile.Write(classicBytes);
            PexData classicData;
            classicData.Load(classicFile.Path().wstring());
            Assert::AreEqual<std::uint8_t>(3, classicData.Header.majorVersion);
            Assert::AreEqual<std::uint8_t>(1, classicData.Header.minorVersion);
            Assert::AreEqual<std::uint16_t>(1, classicData.Header.gameId);

            TemporaryPexFile specialEditionFile;
            specialEditionFile.Write(specialEditionBytes);
            PexData specialEditionData;
            specialEditionData.Load(specialEditionFile.Path().wstring());
            Assert::AreEqual<std::uint8_t>(3, specialEditionData.Header.majorVersion);
            Assert::AreEqual<std::uint8_t>(2, specialEditionData.Header.minorVersion);
            Assert::AreEqual<std::uint16_t>(1, specialEditionData.Header.gameId);
            const std::string expectedUnicode =
                "Gr\xC3\xBC\xC3\x9F" "e \xE6\x9D\xB1\xE4\xBA\xAC";
            Assert::AreEqual(expectedUnicode, specialEditionData.stringTable.ToUtf8(1));
        }

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
            Assert::AreEqual<std::uint16_t>(2, data.objects[0].data.numProperties);
            Assert::AreEqual<std::uint16_t>(1, data.objects[0].data.states[0].numFunctions);
            Assert::AreEqual<std::uint16_t>(
                2,
                data.objects[0].data.states[0].functions[0].function.numInstructions);
        }

        TEST_METHOD(RejectsDocumentedMalformedCorpusSeed)
        {
            Assert::IsTrue(LoadFails(ReadHexFixture(L"malformed-truncated.pex.hex")));
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

        TEST_METHOD(RejectsImpossibleSectionCountsBeforeAllocation)
        {
            const Fixture source = CreateFixture();
            const std::size_t countOffsets[]{
                source.debugFunctionCountOffset,
                source.userFlagCountOffset,
                source.objectCountOffset
            };

            for (const std::size_t offset : countOffsets)
            {
                Fixture fixture = source;
                ReplaceUInt16(fixture.bytes, offset, 0xFFFF);
                Assert::IsTrue(LoadFails(fixture.bytes));
            }
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
            const std::vector<std::uint8_t> fixture =
                ReadHexFixture(L"skyrim-se-3.2-unicode.pex.hex");
            TemporaryPexFile input;
            TemporaryPexFile output;
            input.Write(fixture);

            PexData original;
            original.Load(input.Path().wstring());
            original.Save(output.Path().wstring());

            PexData reloaded;
            reloaded.Load(output.Path().wstring());

            AssertPexDataEqual(original, reloaded);
            Assert::IsTrue(fixture == output.Read());
        }

        TEST_METHOD(ModifiesUnicodeStringAndPreservesEveryOtherField)
        {
            TemporaryPexFile input;
            TemporaryPexFile output;
            input.Write(ReadHexFixture(L"skyrim-se-3.2-unicode.pex.hex"));

            PexData expected;
            expected.Load(input.Path().wstring());
            const std::string replacement =
                "Neu: Gr\xC3\xBC\xC3\x9F" "e \xE6\x9D\xB1\xE4\xBA\xAC";
            expected.ModifyStringTable(1, replacement);
            expected.Save(output.Path().wstring());

            PexData reloaded;
            reloaded.Load(output.Path().wstring());
            AssertPexDataEqual(expected, reloaded);
            Assert::AreEqual(replacement, reloaded.stringTable.ToUtf8(1));
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
