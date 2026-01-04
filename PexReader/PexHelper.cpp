#pragma once
#include "Record.h"
#include "PexHeader.cpp"
#include "PexSections.cpp"
#include <iostream>
#include <sstream>

class PexData
{
public:
    RecordHeader Header;
    StringTable stringTable;
    DebugInfo debugInfo;
    uint16_t userFlagCount;
    vector<UserFlag> userFlags;
    uint16_t objectCount;
    vector<Object> objects;

    void Load(const string& filename)
    {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open())
        {
            throw std::runtime_error("Unable to open the file: " + filename);
        }

        try
        {
            std::cout << "Reading Header..." << std::endl;
            ReadHeader(file);
            std::cout << "Position after header: " << file.tellg() << std::endl;

            std::cout << "Reading StringTable..." << std::endl;
            ReadStringTable(file);
            std::cout << "Position after string table: " << file.tellg() << std::endl;
            std::cout << "String count: " << stringTable.count << std::endl;

            std::cout << "Reading DebugInfo..." << std::endl;
            ReadDebugInfo(file);
            std::cout << "Position after debug info: " << file.tellg() << std::endl;

            std::cout << "Reading UserFlags..." << std::endl;
            ReadUserFlags(file);
            std::cout << "Position after user flags: " << file.tellg() << std::endl;
            std::cout << "User flag count: " << userFlagCount << std::endl;

            std::cout << "Reading Objects..." << std::endl;
            ReadObjects(file);
            std::cout << "Position after objects: " << file.tellg() << std::endl;

            std::cout << "Load complete!" << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error at file position: " << file.tellg() << std::endl;
            file.close();
            throw std::runtime_error(string("Loading failed: ") + e.what());
        }

        file.close();
    }

    void Save(const string& filename)
    {
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open())
        {
            throw std::runtime_error("Unable to create file: " + filename);
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

    string GetPsc() 
    {
        std::ostringstream Script;

        if (objects.empty()) return "";

        uint16_t classNameIndex = objects[0].nameIndex;

        std::string className = stringTable.ToUtf8(classNameIndex);

        Script << "Scriptname " << className << " ";

        if (objects[0].data.parentClassName != 0xFFFF) 
        {
            std::string parentName = stringTable.ToUtf8(objects[0].data.parentClassName);
            Script << "extends " << parentName << " ";
        }
        Script << "\n";

        return Script.str();
    }

    private:
    void ReadHeader(std::ifstream& f)
    {
        Header.magic = ReadUInt32BE(f);
        if (Header.magic != 0xFA57C0DE)
        {
            throw std::runtime_error("Invalid PEX file format (Magic number error)");
        }

        Header.majorVersion = ReadUInt8(f);
        Header.minorVersion = ReadUInt8(f);
        Header.gameId = ReadUInt16BE(f);
        Header.compilationTime = ReadUInt64BE(f);
        Header.sourceFileName = ReadWString(f);
        Header.username = ReadWString(f);
        Header.machinename = ReadWString(f);
    }


    std::string toUtf8String(const std::wstring& wstr)
    {
#if __cplusplus >= 201103L
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        try {
            return converter.to_bytes(wstr);
        }
        catch (...) {
            std::string result;
            for (wchar_t wc : wstr) {
                if (wc < 128) {
                    result.push_back(static_cast<char>(wc));
                }
                else {
                    result.push_back('?');
                }
            }
            return result;
        }
#else
        std::string result;
        for (wchar_t wc : wstr) {
            if (wc < 128) {
                result.push_back(static_cast<char>(wc));
            }
            else {
                result.push_back('?');
            }
        }
        return result;
#endif
    }


    void ReadStringTable(std::ifstream& f)
    {
        stringTable.count = ReadUInt16BE(f);
        stringTable.strings.resize(stringTable.count);

        for (uint16_t i = 0; i < stringTable.count; ++i)
        {
            stringTable.strings[i] = ReadBytes(f);
            std::cout << "Utf8Str: " << stringTable.ToUtf8(i) << std::endl;
            //PrintHexAndText(stringTable.strings[i]);
        }
    }

    /*void ReadStringTable(std::ifstream& f)
    {
        stringTable.count = ReadUInt16BE(f);
        stringTable.strings.resize(stringTable.count);
        stringTable.strings_data.resize(stringTable.count);

        const size_t MAX_LENGTH = 1024;

        for (uint16_t i = 0; i < stringTable.count; ++i)
        {
            std::streampos posBeforeStringRead = f.tellg();

            std::vector<byte> buffer;
            byte currentByte;

            size_t bytesRead = 0;

            while (f.read(reinterpret_cast<char*>(&currentByte), 1))
            {
                if (bytesRead >= MAX_LENGTH)
                {
                    buffer.clear();
                    std::cout << "Exceeded MAX_LENGTH, clearing buffer." << std::endl;
                    break;
                }

                if (std::to_integer<unsigned char>(currentByte) == 0)
                {
                    break;
                }

                buffer.push_back(currentByte);
                ++bytesRead;
            }

            std::string strFromData;
            if (!buffer.empty())
            {
                size_t startIndex = 0;

                if (std::to_integer<unsigned char>(buffer[0]) == 0x00)
                {
                    startIndex = 1;
                }


                for (size_t j = startIndex; j < buffer.size(); ++j)
                {
                    strFromData.push_back(static_cast<char>(std::to_integer<unsigned char>(buffer[j]))); 
                }
            }


            if (strFromData.empty())
            {
                f.seekg(posBeforeStringRead, std::ios::beg);
                std::cout << "No valid UTF-8 data found, reverting to the first stone (original string)." << std::endl;

                stringTable.strings[i] = ReadWString(f);
                stringTable.strings_data[i].clear();
            }
            else
            {
                std::cout << "Valid UTF-8 data found, using the second stone." << std::endl;
                stringTable.strings_data[i] = buffer;
                std::cout << "UTF-8 Data: " << strFromData << std::endl;
            }
        }
    }*/

    void ReadDebugInfo(std::ifstream& f)
    {
        debugInfo.hasDebugInfo = ReadUInt8(f);
        std::cout << "  Has debug info: " << static_cast<int>(debugInfo.hasDebugInfo) << std::endl;

        if (debugInfo.hasDebugInfo)
        {
            debugInfo.modificationTime = ReadUInt64BE(f);
            debugInfo.functionCount = ReadUInt16BE(f);
            debugInfo.functions.resize(debugInfo.functionCount);

            for (uint16_t i = 0; i < debugInfo.functionCount; ++i)
            {
                ReadDebugFunction(f, debugInfo.functions[i]);
            }
        }
    }

    void ReadDebugFunction(std::ifstream& f, DebugFunction& func)
    {
        func.objectNameIndex = ReadUInt16BE(f);
        func.stateNameIndex = ReadUInt16BE(f);
        func.functionNameIndex = ReadUInt16BE(f);
        func.functionType = ReadUInt8(f);
        func.instructionCount = ReadUInt16BE(f);
        func.lineNumbers.resize(func.instructionCount);

        for (uint16_t i = 0; i < func.instructionCount; ++i)
        {
            func.lineNumbers[i] = ReadUInt16BE(f);
        }
    }

    void ReadUserFlags(std::ifstream& f)
    {
        userFlagCount = ReadUInt16BE(f);
        userFlags.resize(userFlagCount);

        for (uint16_t i = 0; i < userFlagCount; ++i)
        {
            userFlags[i].flagNameIndex = ReadUInt16BE(f);
            userFlags[i].flagIndex = ReadUInt8(f);
        }
    }

    void ReadObjects(std::ifstream& f)
    {
        objectCount = ReadUInt16BE(f);
        objects.reserve(objectCount);

        for (uint16_t i = 0; i < objectCount; ++i)
        {
            uint16_t nameIndex = ReadUInt16BE(f);
            uint32_t size = ReadUInt32BE(f);

            objects.emplace_back(nameIndex, size);
            ReadObjectData(f, objects.back().data);
        }
    }

    void ReadObjectData(std::ifstream& f, ObjectData& data)
    {
        data.parentClassName = ReadUInt16BE(f);
        data.docString = ReadUInt16BE(f);
        data.userFlags = ReadUInt32BE(f);
        data.autoStateName = ReadUInt16BE(f);

        // Variables
        data.numVariables = ReadUInt16BE(f);
        data.variables.resize(data.numVariables);
        for (uint16_t i = 0; i < data.numVariables; ++i)
        {
            ReadVariable(f, data.variables[i]);
        }

        // Properties
        data.numProperties = ReadUInt16BE(f);
        data.properties.resize(data.numProperties);
        for (uint16_t i = 0; i < data.numProperties; ++i)
        {
            ReadProperty(f, data.properties[i]);
        }

        // States
        data.numStates = ReadUInt16BE(f);
        data.states.resize(data.numStates);
        for (uint16_t i = 0; i < data.numStates; ++i)
        {
            ReadState(f, data.states[i]);
        }
    }

    void ReadVariable(std::ifstream& f, Variable& var)
    {
        var.name = ReadUInt16BE(f);
        var.typeName = ReadUInt16BE(f);
        var.userFlags = ReadUInt32BE(f);

        ReadVariableData(f, var.data, true);
    }

    void ReadVariableData(std::ifstream& f, VariableData& data, bool integer_unsigned = false)
    {
        std::streampos pos = f.tellg();
        data.type = ReadUInt8(f);

        switch (data.type)
        {
        case 0: // null

            break;
        case 1: // identifier
        case 2: // string
            data.data = ReadUInt16BE(f);
            break;
        case 3: // integer
            if (integer_unsigned)
            {
                uint32_t uint_val = ReadUInt32BE(f);
                data.data = static_cast<int32_t>(uint_val);
            }
            else
            {
                data.data = ReadInt32BE(f);
            }
            break;
        case 4: // float
            data.data = ReadFloatBE(f);
            break;
        case 5: // bool
            data.data = ReadUInt8(f);
            break;
        default:
        {
            std::string error = "Unknown variable data type: " +
                std::to_string(static_cast<int>(data.type)) +
                " at file position: " + std::to_string(pos);
            throw std::runtime_error(error);
        }
        }
    }

    void ReadProperty(std::ifstream& f, Property& prop)
    {
        prop.name = ReadUInt16BE(f);
        prop.type = ReadUInt16BE(f);
        prop.docstring = ReadUInt16BE(f);
        prop.userFlags = ReadUInt32BE(f);
        prop.flags = ReadUInt8(f);

        // autoVarName (if flags & 4)
        if (prop.flags & 4)
        {
            prop.autoVarName = ReadUInt16BE(f);
        }

        // readHandler (if flags & 5 == 1)
        if ((prop.flags & 5) == 1)
        {
            ReadFunction(f, prop.readHandler);
        }

        // writeHandler (if flags & 6 == 2)
        if ((prop.flags & 6) == 2)
        {
            ReadFunction(f, prop.writeHandler);
        }
    }

    void ReadState(std::ifstream& f, State& state)
    {
        state.name = ReadUInt16BE(f);
        state.numFunctions = ReadUInt16BE(f);

        state.functions.resize(state.numFunctions);

        for (uint16_t i = 0; i < state.numFunctions; ++i)
        {
            state.functions[i].functionName = ReadUInt16BE(f);
           
            ReadFunction(f, state.functions[i].function);
        }
    }

    void ReadFunction(std::ifstream& f, Function& func)
    {
        func.returnType = ReadUInt16BE(f);
        func.docString = ReadUInt16BE(f);
        func.userFlags = ReadUInt32BE(f);
        func.flags = ReadUInt8(f);

        // Parameters
        func.numParams = ReadUInt16BE(f);
        func.params.resize(func.numParams);
        for (uint16_t i = 0; i < func.numParams; ++i)
        {
            func.params[i].name = ReadUInt16BE(f);
            func.params[i].type = ReadUInt16BE(f);
        }

        // Locals
        func.numLocals = ReadUInt16BE(f);
        func.locals.resize(func.numLocals);
        for (uint16_t i = 0; i < func.numLocals; ++i)
        {
            func.locals[i].name = ReadUInt16BE(f);
            func.locals[i].type = ReadUInt16BE(f);
        }

        // Instructions
        func.numInstructions = ReadUInt16BE(f);
        func.instructions.resize(func.numInstructions);
        for (uint16_t i = 0; i < func.numInstructions; ++i)
        {
            ReadInstruction(f, func.instructions[i]);
        }
    }

    void ReadInstruction(std::ifstream& f, Instruction& instr)
    {
        instr.op = static_cast<Opcode>(ReadUInt8(f));

        switch (instr.op)
        {
        case Opcode::nop:
            break;

        case Opcode::callmethod:
        {
            VariableData result, self, methodName, argCountData;
            ReadVariableData(f, result);
            ReadVariableData(f, self);
            ReadVariableData(f, methodName);
            ReadVariableData(f, argCountData);

            instr.arguments.push_back(result);
            instr.arguments.push_back(self);
            instr.arguments.push_back(methodName);
            instr.arguments.push_back(argCountData);

            uint16_t argCount = 0;
            if (argCountData.type == 3)
            {
                argCount = static_cast<uint16_t>(std::get<int32_t>(argCountData.data));
            }

            for (uint16_t i = 0; i < argCount; ++i)
            {
                VariableData arg;
                ReadVariableData(f, arg);
                instr.arguments.push_back(arg);
            }
            break;
        }

        case Opcode::callparent:
        {
            VariableData result, methodName, argCountData;
            ReadVariableData(f, result);
            ReadVariableData(f, methodName);
            ReadVariableData(f, argCountData);

            instr.arguments.push_back(result);
            instr.arguments.push_back(methodName);
            instr.arguments.push_back(argCountData);

            uint16_t argCount = 0;
            if (argCountData.type == 3)
            {
                argCount = static_cast<uint16_t>(std::get<int32_t>(argCountData.data));
            }

            for (uint16_t i = 0; i < argCount; ++i)
            {
                VariableData arg;
                ReadVariableData(f, arg);
                instr.arguments.push_back(arg);
            }
            break;
        }

        case Opcode::callstatic:
        {
            VariableData result, className, methodName, argCountData;
            ReadVariableData(f, result);
            ReadVariableData(f, className);
            ReadVariableData(f, methodName);
            ReadVariableData(f, argCountData);

            instr.arguments.push_back(result);
            instr.arguments.push_back(className);
            instr.arguments.push_back(methodName);
            instr.arguments.push_back(argCountData);

            uint16_t argCount = 0;
            if (argCountData.type == 3)
            {
                argCount = static_cast<uint16_t>(std::get<int32_t>(argCountData.data));
            }

            for (uint16_t i = 0; i < argCount; ++i)
            {
                VariableData arg;
                ReadVariableData(f, arg);
                instr.arguments.push_back(arg);
            }
            break;
        }

        default:
        {
            int argCount = GetOpcodeArgumentCount(instr.op);
            instr.arguments.resize(argCount);

            for (int i = 0; i < argCount; ++i)
            {
                ReadVariableData(f, instr.arguments[i]);
            }
            break;
        }
        }
    }

    int GetOpcodeArgumentCount(Opcode op)
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
            return 0;
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

        for (uint16_t i = 0; i < stringTable.count; ++i)
        {
            if (i < stringTable.strings.size() && !stringTable.strings.empty())
            {
                WriteUInt16BE(f, static_cast<uint16_t>(stringTable.strings[i].size()));

                f.write(reinterpret_cast<const char*>(stringTable.strings[i].data()),
                    stringTable.strings[i].size());

                WriteUInt8(f, 0);
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
        case 0:
            break;
        case 1:
        case 2:
            WriteUInt16BE(f, std::get<uint16_t>(data.data));
            break;
        case 3:
            if (integer_unsigned)
            {
                WriteUInt32BE(f, static_cast<uint32_t>(std::get<int32_t>(data.data)));
            }
            else
            {
                WriteInt32BE(f, std::get<int32_t>(data.data));
            }
            break;
        case 4:
            WriteFloatBE(f, std::get<float>(data.data));
            break;
        case 5:
            WriteUInt8(f, std::get<uint8_t>(data.data));
            break;
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