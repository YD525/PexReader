#pragma once
#include "Record.h"

// Forward Declarations
struct DebugFunction;
struct Variable;
struct Property;
struct State;
struct Instruction;
struct Function;
struct UserFlag;
struct Parameter;
struct LocalVariable;


#pragma region String Table 
//String Table 
struct StringTable
{
    uint16_t count;
    vector<wstring> strings;

    StringTable() : count(0) {}

    StringTable(uint16_t _count) : count(_count)
    {
        strings.resize(count);
    }
};
#pragma endregion
#pragma region DebugInfo 
//Debug Info 
struct DebugInfo
{
    uint8_t hasDebugInfo;//Flag, if zero then no debug info is present and the rest of the record is skipped
    uint64_t modificationTime; //time_t
    uint16_t functionCount;
    vector<DebugFunction> functions;

    DebugInfo() :hasDebugInfo(0), modificationTime(0), functionCount(0) {}

    DebugInfo(uint16_t _count) :hasDebugInfo(0), modificationTime(0), functionCount(_count)
    {
        functions.resize(functionCount);
    }
};
#pragma endregion
#pragma region DebugFunction
//Debug Function
struct DebugFunction
{
    uint16_t objectNameIndex;
    uint16_t stateNameIndex;
    uint16_t functionNameIndex;
    uint8_t functionType;
    uint16_t instructionCount;
    vector<uint16_t> lineNumbers;

    DebugFunction() : objectNameIndex(0), stateNameIndex(0), functionNameIndex(0),
        functionType(0), instructionCount(0) {
    }

    DebugFunction(uint16_t _instructionCount)
        : objectNameIndex(0), stateNameIndex(0), functionNameIndex(0),
        functionType(0), instructionCount(_instructionCount)
    {
        lineNumbers.resize(instructionCount);
    }
};
#pragma endregion
#pragma region User Flag
//UserFlag
#pragma pack(push, 1)
struct UserFlag
{
    uint16_t flagNameIndex;
    uint8_t flagIndex;
};
#pragma pack(pop)
#pragma endregion
#pragma region Object&Object Data
//Object Object Data
struct Object
{
    uint16_t nameIndex;
    uint32_t size;

    ObjectData data; 

    Object(uint16_t _nameIndex, uint32_t _size)
        : nameIndex(_nameIndex), size(_size)
    {
    }
};
struct ObjectData
{
    uint16_t parentClassName;
    uint16_t docString;
    uint32_t userFlags;
    uint16_t autoStateName;
    uint16_t numVariables;
    std::vector<Variable> variables;
    uint16_t numProperties;
    std::vector<Property> properties;
    uint16_t numStates;
    std::vector<State> states;
};

#pragma pack(push, 1)
struct Variable
{
    uint16_t name;
    uint16_t typeName;
    uint32_t userFlags;
    VariableData data;
};
struct VariableData
{
    uint8_t type;  // Type: 0 = null, 1 = identifier, 2 = string, 3 = integer, 4 = float, 5 = bool
    std::variant<uint16_t, int32_t, float, uint8_t, std::string> data;  // Data of different types

    VariableData(uint8_t _type) : type(_type)
    {
        if (type == 0 || type == 1 || type == 2) {
            data = uint16_t(0);  //uint16_t
        }
        else if (type == 3) {
            data = int32_t(0);  //int32_t
        }
        else if (type == 4) {
            data = float(0.0f);  //float
        }
        else if (type == 5) {
            data = uint8_t(0);  //bool (uint8_t)
        }
    }

    VariableData(uint8_t _type, const std::string& str) : type(_type)
    {
        if (type == 2) {
            data = str;
        }
        else {
            *this = VariableData(_type);
        }
    }
};
#pragma pack(pop)

#pragma endregion



#pragma pack(push, 1)
struct Property
{
    uint16_t nameIndex;      
    uint8_t type;            
    uint32_t docStringIndex; 
    uint32_t userFlags;     
    uint32_t flags;          
};
#pragma pack(pop)

struct State
{
    uint16_t nameIndex;      
    uint16_t functionCount; 
    vector<uint16_t> functionIndices; 
};

#pragma pack(push, 1)
struct Parameter
{
    uint16_t nameIndex;      
    uint16_t typeNameIndex;  
};
#pragma pack(pop)

#pragma pack(push, 1)
struct LocalVariable
{
    uint16_t nameIndex;      
    uint16_t typeNameIndex;  
};
#pragma pack(pop)

struct Instruction
{
    uint8_t opcode;         
    vector<uint8_t> args;    
};

struct Function
{
    uint16_t returnTypeIndex;    
    uint16_t docStringIndex;     
    uint32_t userFlags;          
    uint8_t flags;               

    uint16_t paramCount;        
    vector<Parameter> params;

    uint16_t localCount;         
    vector<LocalVariable> locals;

    uint16_t instructionCount;  
    vector<Instruction> instructions;
};



struct RecordSections
{
    StringTable stringTable;
    DebugInfo debugInfo;

    uint16_t userFlagCount;
    vector<UserFlag> userFlags;

    uint16_t objectCount;
    vector<Object> objects;

    RecordSections() : userFlagCount(0), objectCount(0) {}
};