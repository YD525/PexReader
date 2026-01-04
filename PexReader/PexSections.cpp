#pragma once
#include "Record.h"

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

//Debug Info 
struct DebugInfo
{
    uint8_t hasDebugInfo;//Flag, if zero then no debug info is present and the rest of the record is skipped
    uint64_t modificationTime;   
    uint16_t functionCount;      
    vector<DebugFunction> functions;

    DebugInfo() :hasDebugInfo(0), modificationTime(0), functionCount(0) {}

    DebugInfo(uint16_t _count) :hasDebugInfo(0), modificationTime(0), functionCount(_count)
    {
        functions.resize(functionCount);
    }
};

#pragma pack(push, 1)
struct UserFlag
{
    uint16_t flagNameIndex;  
    uint8_t flagIndex;       
};
#pragma pack(pop)

#pragma pack(push, 1)
struct VariableType
{
    uint16_t typeNameIndex;  
};
#pragma pack(pop)

#pragma pack(push, 1)
struct VariableData
{
    uint16_t nameIndex;     
    uint16_t typeNameIndex;  
    uint32_t userFlags;      
    uint32_t data;           
};
#pragma pack(pop)

#pragma pack(push, 1)
struct PropertyData
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

struct Object
{
    uint16_t nameIndex;          
    uint16_t parentNameIndex;    
    uint16_t ownerNameIndex;     
    uint16_t docStringIndex;     
    uint32_t userFlags;          
    uint16_t autoStateNameIndex; 


    uint16_t variableCount;      
    vector<VariableData> variables;


    uint16_t propertyCount;      
    vector<PropertyData> properties;


    uint16_t stateCount;         
    vector<State> states;


    uint16_t functionCount;      
    vector<Function> functions;
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