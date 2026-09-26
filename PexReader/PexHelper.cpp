#pragma once
#include "Record.h"
#include "PexHeader.cpp"
#include "PexSections.cpp"

class PexData
{
    public:
    RecordHeader Header;
    StringTable stringTable;
    DebugInfo debugInfo;
    uint16_t userFlagCount = 0;
    vector<UserFlag> userFlags;
    uint16_t objectCount = 0;
    vector<Object> objects;

    void Load(const wstring& filename)
    {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open())
        {
            throw std::runtime_error("Open File Error");
        }

        PexBinaryReader reader(file);
        PexData parsed;

        try
        {
            parsed.ReadHeader(reader);
            parsed.ReadStringTable(reader);
            parsed.ReadDebugInfo(reader);
            parsed.ReadUserFlags(reader);
            parsed.ReadObjects(reader);
        }
        catch (const std::exception& exception)
        {
            throw std::runtime_error(
                "PEX parse failed at byte " + std::to_string(reader.Position()) + ": " + exception.what());
        }

        *this = std::move(parsed);
    }

    void ModifyStringTable(uint16_t Index, const std::string& Utf8Str)
    {
        if (Index >= stringTable.count)
        {
            throw std::out_of_range("StringTable index out of range: " + std::to_string(Index));
        }

        CheckedUInt16Length(Utf8Str.size(), "String table entry");

        size_t len = Utf8Str.size();
        std::vector<byte> newBytes(len);
        std::memcpy(newBytes.data(), Utf8Str.data(), len);

        stringTable.strings[Index] = std::move(newBytes);
    }

    void Save(const wstring& filename)
    {
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open())
        {
            throw std::runtime_error("Save File Error");
        }

        try
        {
            WriteHeader(file);
            WriteStringTable(file);
            WriteDebugInfo(file);
            WriteUserFlags(file);
            WriteObjects(file);
        }
        catch (const std::exception& e)
        {
            file.close();
            throw std::runtime_error(string("Save failed: ") + e.what());
        }

        file.close();
    }

    private:
    void ReadHeader(PexBinaryReader& reader)
    {
        Header.magic = ReadUInt32BE(reader);
        if (Header.magic != 0xFA57C0DE)
        {
            throw std::runtime_error("Invalid PEX file format (Magic number error)");
        }

        Header.majorVersion = ReadUInt8(reader);
        Header.minorVersion = ReadUInt8(reader);
        Header.gameId = ReadUInt16BE(reader);
        Header.compilationTime = ReadUInt64BE(reader);
        Header.sourceFileName = ReadWString(reader);
        Header.username = ReadWString(reader);
        Header.machinename = ReadWString(reader);
    }

    void ReadStringTable(PexBinaryReader& reader)
    {
        stringTable.count = ReadUInt16BE(reader);
        reader.ValidateCount(stringTable.count, 2, "String table count");
        stringTable.strings.resize(stringTable.count);

        for (uint16_t i = 0; i < stringTable.count; ++i)
            stringTable.strings[i] = ReadBytes(reader);
    }

    void ReadDebugInfo(PexBinaryReader& reader)
    {
        debugInfo.hasDebugInfo = ReadUInt8(reader);
        if (debugInfo.hasDebugInfo > 1)
            throw std::runtime_error("Debug info flag is invalid.");

        if (debugInfo.hasDebugInfo)
        {
            debugInfo.modificationTime = ReadUInt64BE(reader);
            debugInfo.functionCount = ReadUInt16BE(reader);
            reader.ValidateCount(debugInfo.functionCount, 9, "Debug function count");
            debugInfo.functions.resize(debugInfo.functionCount);

            for (uint16_t i = 0; i < debugInfo.functionCount; ++i)
                ReadDebugFunction(reader, debugInfo.functions[i]);
        }
    }

    void ReadDebugFunction(PexBinaryReader& reader, DebugFunction& func)
    {
        func.objectNameIndex = ReadUInt16BE(reader);
        func.stateNameIndex = ReadUInt16BE(reader);
        func.functionNameIndex = ReadUInt16BE(reader);
        func.functionType = ReadUInt8(reader);
        func.instructionCount = ReadUInt16BE(reader);
        reader.ValidateCount(func.instructionCount, 2, "Debug line-number count");
        func.lineNumbers.resize(func.instructionCount);

        for (uint16_t i = 0; i < func.instructionCount; ++i)
            func.lineNumbers[i] = ReadUInt16BE(reader);
    }

    void ReadUserFlags(PexBinaryReader& reader)
    {
        userFlagCount = ReadUInt16BE(reader);
        reader.ValidateCount(userFlagCount, 3, "User flag count");
        userFlags.resize(userFlagCount);

        for (uint16_t i = 0; i < userFlagCount; ++i)
        {
            userFlags[i].flagNameIndex = ReadUInt16BE(reader);
            userFlags[i].flagIndex = ReadUInt8(reader);
        }
    }

    void ReadObjects(PexBinaryReader& reader)
    {
        objectCount = ReadUInt16BE(reader);
        reader.ValidateCount(objectCount, 6, "Object count");
        objects.reserve(objectCount);

        for (uint16_t i = 0; i < objectCount; ++i)
        {
            const uint16_t nameIndex = ReadUInt16BE(reader);
            const uint32_t size = ReadUInt32BE(reader);

            // UESP: "size includes itself for some reason, hence size-4"
            // The `size` field itself occupies 4 bytes, so the actual size of the object data is `size - 4`!!
            if (size < 4)
                throw std::runtime_error("Object size field is invalid (less than 4).");

            const uint32_t dataSize = size - 4;
            if (dataSize > reader.Remaining())
                throw std::runtime_error("Object data size exceeds the remaining PEX input.");

            objects.emplace_back(nameIndex, size);
            const std::uint64_t objectDataStart = reader.Position();
            ReadObjectData(reader, objects.back().data);

            if (reader.Position() - objectDataStart != dataSize)
                throw std::runtime_error("Object size does not match the parsed object data.");
        }
    }

    void ReadObjectData(PexBinaryReader& reader, ObjectData& data)
    {
        data.parentClassName = ReadUInt16BE(reader);
        data.docString = ReadUInt16BE(reader);
        data.userFlags = ReadUInt32BE(reader);
        data.autoStateName = ReadUInt16BE(reader);

        // Variables
        data.numVariables = ReadUInt16BE(reader);
        reader.ValidateCount(data.numVariables, 9, "Object variable count");
        data.variables.resize(data.numVariables);
        for (uint16_t i = 0; i < data.numVariables; ++i)
            ReadVariable(reader, data.variables[i]);

        // Properties
        data.numProperties = ReadUInt16BE(reader);
        reader.ValidateCount(data.numProperties, 11, "Object property count");
        data.properties.resize(data.numProperties);
        for (uint16_t i = 0; i < data.numProperties; ++i)
            ReadProperty(reader, data.properties[i]);

        // States
        data.numStates = ReadUInt16BE(reader);
        reader.ValidateCount(data.numStates, 4, "Object state count");
        data.states.resize(data.numStates);
        for (uint16_t i = 0; i < data.numStates; ++i)
            ReadState(reader, data.states[i]);
    }

    void ReadVariable(PexBinaryReader& reader, Variable& var)
    {
        var.name = ReadUInt16BE(reader);
        var.typeName = ReadUInt16BE(reader);
        var.userFlags = ReadUInt32BE(reader);

        ReadVariableData(reader, var.data, true);
    }

    void ReadVariableData(PexBinaryReader& reader, VariableData& data, bool integer_unsigned = false)
    {
        const std::uint64_t position = reader.Position();
        data.type = ReadUInt8(reader);

        switch (data.type)
        {
        case 0: // null

            break;
        case 1: // identifier
        case 2: // string
            data.data = ReadUInt16BE(reader);
            break;
        case 3: // integer
            if (integer_unsigned)
            {
                const uint32_t unsignedValue = ReadUInt32BE(reader);
                data.data = static_cast<int32_t>(unsignedValue);
            }
            else
            {
                data.data = ReadInt32BE(reader);
            }
            break;
        case 4: // float
            data.data = ReadFloatBE(reader);
            break;
        case 5: // bool
            data.data = ReadUInt8(reader);
            break;
        default:
        {
            std::string error = "Unknown variable data type: " +
                std::to_string(static_cast<int>(data.type)) +
                " at file position: " + std::to_string(position);
            throw std::runtime_error(error);
        }
        }
    }

    void ReadProperty(PexBinaryReader& reader, Property& prop)
    {
        prop.name = ReadUInt16BE(reader);
        prop.type = ReadUInt16BE(reader);
        prop.docstring = ReadUInt16BE(reader);
        prop.userFlags = ReadUInt32BE(reader);
        prop.flags = ReadUInt8(reader);

        // autoVarName (if flags & 4)
        if (prop.flags & 4)
        {
            prop.autoVarName = ReadUInt16BE(reader);
        }

        // readHandler (if flags & 5 == 1)
        if ((prop.flags & 5) == 1)
        {
            ReadFunction(reader, prop.readHandler);
        }

        // writeHandler (if flags & 6 == 2)
        if ((prop.flags & 6) == 2)
        {
            ReadFunction(reader, prop.writeHandler);
        }
    }

    void ReadState(PexBinaryReader& reader, State& state)
    {
        state.name = ReadUInt16BE(reader);
        state.numFunctions = ReadUInt16BE(reader);

        reader.ValidateCount(state.numFunctions, 17, "State function count");
        state.functions.resize(state.numFunctions);

        for (uint16_t i = 0; i < state.numFunctions; ++i)
        {
            state.functions[i].functionName = ReadUInt16BE(reader);
            ReadFunction(reader, state.functions[i].function);
        }
    }

    void ReadFunction(PexBinaryReader& reader, Function& func)
    {
        func.returnType = ReadUInt16BE(reader);
        func.docString = ReadUInt16BE(reader);
        func.userFlags = ReadUInt32BE(reader);
        func.flags = ReadUInt8(reader);

        // Parameters
        func.numParams = ReadUInt16BE(reader);
        reader.ValidateCount(func.numParams, 4, "Function parameter count");
        func.params.resize(func.numParams);
        for (uint16_t i = 0; i < func.numParams; ++i)
        {
            func.params[i].name = ReadUInt16BE(reader);
            func.params[i].type = ReadUInt16BE(reader);
        }

        // Locals
        func.numLocals = ReadUInt16BE(reader);
        reader.ValidateCount(func.numLocals, 4, "Function local count");
        func.locals.resize(func.numLocals);
        for (uint16_t i = 0; i < func.numLocals; ++i)
        {
            func.locals[i].name = ReadUInt16BE(reader);
            func.locals[i].type = ReadUInt16BE(reader);
        }

        // Instructions
        func.numInstructions = ReadUInt16BE(reader);
        reader.ValidateCount(func.numInstructions, 1, "Function instruction count");
        func.instructions.resize(func.numInstructions);
        for (uint16_t i = 0; i < func.numInstructions; ++i)
            ReadInstruction(reader, func.instructions[i]);
    }

    void ReadInstruction(PexBinaryReader& reader, Instruction& instr)
    {
        const std::uint8_t opcode = ReadUInt8(reader);
        if (opcode > static_cast<std::uint8_t>(Opcode::array_rfindelement))
            throw std::runtime_error("Instruction opcode is invalid.");

        instr.op = static_cast<Opcode>(opcode);

        switch (instr.op)
        {
        case Opcode::nop:
            break;

        case Opcode::callmethod:
        {
            VariableData result, self, methodName, argCountData;
            reader.ValidateCount(4, 1, "Call method header arguments");
            ReadVariableData(reader, result);
            ReadVariableData(reader, self);
            ReadVariableData(reader, methodName);
            ReadVariableData(reader, argCountData);

            instr.arguments.push_back(result);
            instr.arguments.push_back(self);
            instr.arguments.push_back(methodName);
            instr.arguments.push_back(argCountData);

            const uint16_t argCount = ReadCallArgumentCount(reader, argCountData);

            for (uint16_t i = 0; i < argCount; ++i)
            {
                VariableData arg;
                ReadVariableData(reader, arg);
                instr.arguments.push_back(arg);
            }
            break;
        }

        case Opcode::callparent:
        {
            VariableData result, methodName, argCountData;
            reader.ValidateCount(3, 1, "Call parent header arguments");
            ReadVariableData(reader, result);
            ReadVariableData(reader, methodName);
            ReadVariableData(reader, argCountData);

            instr.arguments.push_back(result);
            instr.arguments.push_back(methodName);
            instr.arguments.push_back(argCountData);

            const uint16_t argCount = ReadCallArgumentCount(reader, argCountData);

            for (uint16_t i = 0; i < argCount; ++i)
            {
                VariableData arg;
                ReadVariableData(reader, arg);
                instr.arguments.push_back(arg);
            }
            break;
        }

        case Opcode::callstatic:
        {
            VariableData result, className, methodName, argCountData;
            reader.ValidateCount(4, 1, "Call static header arguments");
            ReadVariableData(reader, result);
            ReadVariableData(reader, className);
            ReadVariableData(reader, methodName);
            ReadVariableData(reader, argCountData);

            instr.arguments.push_back(result);
            instr.arguments.push_back(className);
            instr.arguments.push_back(methodName);
            instr.arguments.push_back(argCountData);

            const uint16_t argCount = ReadCallArgumentCount(reader, argCountData);

            for (uint16_t i = 0; i < argCount; ++i)
            {
                VariableData arg;
                ReadVariableData(reader, arg);
                instr.arguments.push_back(arg);
            }
            break;
        }

        default:
        {
            const std::size_t argCount = GetOpcodeArgumentCount(instr.op);
            reader.ValidateCount(argCount, 1, "Instruction argument count");
            instr.arguments.resize(argCount);

            for (std::size_t i = 0; i < argCount; ++i)
                ReadVariableData(reader, instr.arguments[i]);
            break;
        }
        }
    }

    uint16_t ReadCallArgumentCount(PexBinaryReader& reader, const VariableData& argumentCountData)
    {
        constexpr std::int32_t MaxCallArgumentCount = 4096;

        if (argumentCountData.type != 3)
            throw std::runtime_error("Call argument count is not encoded as an integer.");

        const std::int32_t count = std::get<std::int32_t>(argumentCountData.data);
        if (count < 0 || count > MaxCallArgumentCount)
            throw std::runtime_error("Call argument count exceeds the configured limit.");

        reader.ValidateCount(static_cast<std::size_t>(count), 1, "Call argument count");
        return static_cast<uint16_t>(count);
    }

    std::size_t GetOpcodeArgumentCount(Opcode op)
    {
        switch (op)
        {
        case Opcode::nop:
            return 0;
        case Opcode::iadd:
        case Opcode::fadd:
        case Opcode::isub:
        case Opcode::fsub:
        case Opcode::imul:
        case Opcode::fmul:
        case Opcode::idiv:
        case Opcode::fdiv:
        case Opcode::imod:
        case Opcode::cmp_eq:
        case Opcode::cmp_lt:
        case Opcode::cmp_le:
        case Opcode::cmp_gt:
        case Opcode::cmp_ge:
        case Opcode::strcat:
        case Opcode::propget:
        case Opcode::propset:
        case Opcode::array_getelement:
        case Opcode::array_setelement:
            return 3;
        case Opcode::not_:
        case Opcode::ineg:
        case Opcode::fneg:
        case Opcode::assign:
        case Opcode::cast:
        case Opcode::jmpt:
        case Opcode::jmpf:
        case Opcode::array_create:
        case Opcode::array_length:
            return 2;
        case Opcode::jmp:
        case Opcode::return_:
            return 1;
        case Opcode::array_findelement:
        case Opcode::array_rfindelement:
            return 4;
        default:
            throw std::runtime_error("Instruction opcode is invalid.");
        }
    }

    // Write functions remain the same...
    void WriteHeader(std::ofstream& f)
    {
        WriteUInt32BE(f, Header.magic);
        WriteUInt8(f, Header.majorVersion);
        WriteUInt8(f, Header.minorVersion);
        WriteUInt16BE(f, Header.gameId);
        WriteUInt64BE(f, Header.compilationTime);
        WriteWString(f, Header.sourceFileName);
        WriteWString(f, Header.username);
        WriteWString(f, Header.machinename);
    }


    void WriteStringTable(std::ofstream& f)
    {
        WriteUInt16BE(f, stringTable.count);

        if (stringTable.strings.size() < stringTable.count)
        {
            stringTable.strings.resize(stringTable.count);
        }

        for (uint16_t i = 0; i < stringTable.count; ++i)
        {
            const auto& bytes = stringTable.strings[i];
            WriteUInt16BE(f, CheckedUInt16Length(bytes.size(), "String table entry"));

            if (!bytes.empty())
            {
                f.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
            }
        }
    }

    void WriteDebugInfo(std::ofstream& f)
    {
        WriteUInt8(f, debugInfo.hasDebugInfo);

        if (debugInfo.hasDebugInfo)
        {
            WriteUInt64BE(f, debugInfo.modificationTime);
            WriteUInt16BE(f, debugInfo.functionCount);

            for (const auto& func : debugInfo.functions)
            {
                WriteDebugFunction(f, func);
            }
        }
    }

    void WriteDebugFunction(std::ofstream& f, const DebugFunction& func)
    {
        WriteUInt16BE(f, func.objectNameIndex);
        WriteUInt16BE(f, func.stateNameIndex);
        WriteUInt16BE(f, func.functionNameIndex);
        WriteUInt8(f, func.functionType);
        WriteUInt16BE(f, func.instructionCount);

        for (uint16_t lineNum : func.lineNumbers)
        {
            WriteUInt16BE(f, lineNum);
        }
    }

    void WriteUserFlags(std::ofstream& f)
    {
        WriteUInt16BE(f, userFlagCount);
        for (const auto& flag : userFlags)
        {
            WriteUInt16BE(f, flag.flagNameIndex);
            WriteUInt8(f, flag.flagIndex);
        }
    }

    void WriteObjects(std::ofstream& f)
    {
        WriteUInt16BE(f, objectCount);
        for (const auto& obj : objects)
        {
            WriteUInt16BE(f, obj.nameIndex);
            WriteUInt32BE(f, obj.size);
            WriteObjectData(f, obj.data);
        }
    }

    void WriteObjectData(std::ofstream& f, const ObjectData& data)
    {
        WriteUInt16BE(f, data.parentClassName);
        WriteUInt16BE(f, data.docString);
        WriteUInt32BE(f, data.userFlags);
        WriteUInt16BE(f, data.autoStateName);

        WriteUInt16BE(f, data.numVariables);
        for (const auto& var : data.variables)
        {
            WriteVariable(f, var);
        }

        WriteUInt16BE(f, data.numProperties);
        for (const auto& prop : data.properties)
        {
            WriteProperty(f, prop);
        }

        WriteUInt16BE(f, data.numStates);
        for (const auto& state : data.states)
        {
            WriteState(f, state);
        }
    }

    void WriteVariable(std::ofstream& f, const Variable& var)
    {
        WriteUInt16BE(f, var.name);
        WriteUInt16BE(f, var.typeName);
        WriteUInt32BE(f, var.userFlags);
        WriteVariableData(f, var.data, true);
    }

    void WriteVariableData(std::ofstream& f, const VariableData& data, bool integer_unsigned = false)
    {
        WriteUInt8(f, data.type);
        switch (data.type)
        {
        case 0: break;
        case 1: case 2: WriteUInt16BE(f, std::get<uint16_t>(data.data)); break;
        case 3:
            if (integer_unsigned)
                WriteUInt32BE(f, static_cast<uint32_t>(std::get<int32_t>(data.data)));
            else
                WriteInt32BE(f, std::get<int32_t>(data.data));
            break;
        case 4: WriteFloatBE(f, std::get<float>(data.data)); break;
        case 5: WriteUInt8(f, std::get<uint8_t>(data.data)); break;
        }
    }

    void WriteProperty(std::ofstream& f, const Property& prop)
    {
        WriteUInt16BE(f, prop.name);
        WriteUInt16BE(f, prop.type);
        WriteUInt16BE(f, prop.docstring);
        WriteUInt32BE(f, prop.userFlags);
        WriteUInt8(f, prop.flags);

        if (prop.flags & 4)  
        {
            WriteUInt16BE(f, prop.autoVarName);
        }

        if ((prop.flags & 5) == 1) 
        {
            WriteFunction(f, prop.readHandler);
        }

        if ((prop.flags & 6) == 2)  
        {
            WriteFunction(f, prop.writeHandler);
        }
    }

    void WriteState(std::ofstream& f, const State& state)
    {
        WriteUInt16BE(f, state.name);
        WriteUInt16BE(f, state.numFunctions);

        for (const auto& namedFunc : state.functions)
        {
            WriteUInt16BE(f, namedFunc.functionName);
            WriteFunction(f, namedFunc.function);
        }
    }

    void WriteFunction(std::ofstream& f, const Function& func)
    {
        WriteUInt16BE(f, func.returnType);
        WriteUInt16BE(f, func.docString);
        WriteUInt32BE(f, func.userFlags);
        WriteUInt8(f, func.flags);

        WriteUInt16BE(f, func.numParams);
        for (const auto& param : func.params)
        {
            WriteUInt16BE(f, param.name);
            WriteUInt16BE(f, param.type);
        }

        WriteUInt16BE(f, func.numLocals);
        for (const auto& local : func.locals)
        {
            WriteUInt16BE(f, local.name);
            WriteUInt16BE(f, local.type);
        }

        WriteUInt16BE(f, func.numInstructions);
        for (const auto& instr : func.instructions)
        {
            WriteInstruction(f, instr);
        }
    }

    void WriteInstruction(std::ofstream& f, const Instruction& instr)
    {
        WriteUInt8(f, static_cast<uint8_t>(instr.op));

        for (const auto& arg : instr.arguments)
        {
            WriteVariableData(f, arg);
        }
    }
};
