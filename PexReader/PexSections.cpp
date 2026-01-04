#pragma once
#include "Record.h"
#include "PexStringTable.cpp"

// Forward Declarations
struct DebugFunction;
struct ObjectData;
struct Variable;
struct VariableData;
struct Property;
struct State;


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

struct UserFlag
{
    uint16_t flagNameIndex;
    uint8_t flagIndex;
};

#pragma endregion

#pragma region VariableData
#pragma pack(push, 1)
struct VariableData
{
    uint8_t type;  // Type: 0 = null, 1 = identifier, 2 = string, 3 = integer, 4 = float, 5 = bool
    std::variant<uint16_t, int32_t, float, uint8_t> data;  // Data of different types

    VariableData() : type(0), data(uint16_t(0)) {}

    VariableData(uint8_t _type) : type(_type)
    {
        if (type == 0 || type == 1 || type == 2)
        {
            data = uint16_t(0);  //uint16_t for identifier and string
        }
        else if (type == 3)
        {
            data = int32_t(0);  //int32_t 
        }
        else if (type == 4)
        {
            data = float(0.0f);  //float 
        }
        else if (type == 5)
        {
            data = uint8_t(0);  //uint8_t 
        }
        else
        {
            data = uint16_t(0);  
        }
    }
};
#pragma pack(pop)
#pragma endregion

#pragma region Variable
struct Variable
{
    uint16_t name;
    uint16_t typeName;
    uint32_t userFlags;
    VariableData data;

    Variable() : name(0), typeName(0), userFlags(0), data() {}
};
#pragma endregion

#pragma region Opcode
enum class Opcode : uint8_t
{
    nop = 0x00,           // do nothing
    iadd = 0x01,          // add two integers
    fadd = 0x02,          // add two floats
    isub = 0x03,          // subtract two integers
    fsub = 0x04,          // subtract two floats
    imul = 0x05,          // multiply two integers
    fmul = 0x06,          // multiply two floats
    idiv = 0x07,          // divide two integers
    fdiv = 0x08,          // divide two floats
    imod = 0x09,          // remainder of two integers
    not_ = 0x0A,          // flip a bool, type conversion may occur
    ineg = 0x0B,          // negate an integer
    fneg = 0x0C,          // negate a float
    assign = 0x0D,        // store a variable
    cast = 0x0E,          // type conversion
    cmp_eq = 0x0F,        // set a bool to true if a == b
    cmp_lt = 0x10,        // set a bool to true if a < b
    cmp_le = 0x11,        // set a bool to true if a <= b
    cmp_gt = 0x12,        // set a bool to true if a > b
    cmp_ge = 0x13,        // set a bool to true if a >= b
    jmp = 0x14,           // relative unconditional branch
    jmpt = 0x15,          // relative conditional branch if a bool is true
    jmpf = 0x16,          // relative conditional branch if a bool is false
    callmethod = 0x17,    // call a method (NSS*)
    callparent = 0x18,    // call a parent method (NS*)
    callstatic = 0x19,    // call a static method (NNS*)
    return_ = 0x1A,       // return (A)
    strcat = 0x1B,        // concatenate two strings (SQQ)
    propget = 0x1C,       // retrieve an instance property (NSS)
    propset = 0x1D,       // set an instance property (NSA)
    array_create = 0x1E,  // create an array of the specified size (Su)
    array_length = 0x1F,  // get an array's length (SS)
    array_getelement = 0x20,  // get an element from an array (SSI)
    array_setelement = 0x21,  // set an element in an array to value (SIA)
    array_findelement = 0x22, // find an element in an array. The 4th arg is the startIndex, default = 0 (SSII)
    array_rfindelement = 0x23 // find an element in an array, starting from the end. The 4th arg is the startIndex, default = -1 (SSII)
};
#pragma endregion

#pragma region VariableType
struct VariableType
{
    uint16_t name;//Index(base 0) into string table
    uint16_t type;//Index(base 0) into string table
    
    VariableType() : name(0), type(0) {}
};
#pragma endregion

#pragma region Instruction
struct Instruction
{
    Opcode op;
    vector<VariableData> arguments; 

    Instruction() : op(Opcode::nop) {}
};
#pragma endregion

#pragma region Function
struct Function
{
    uint16_t returnType;
    uint16_t docString;
    uint32_t userFlags;
    uint8_t flags;
    uint16_t numParams;
    vector<VariableType> params;
    uint16_t numLocals;
    vector<VariableType> locals;
    uint16_t numInstructions;
    vector<Instruction> instructions;

    Function() : returnType(0), docString(0), userFlags(0), flags(0),
                 numParams(0), numLocals(0), numInstructions(0) {}
};
#pragma endregion

#pragma region Property
struct Property
{
    uint16_t name; //Index(base 0) into string table
    uint16_t type;  //Index(base 0) into string table
    uint16_t docstring; //Index(base 0) into string table
    uint32_t userFlags;     
    uint8_t flags; //bitfield: 1(bit 1) = read, 2(bit 2) = write, 4(bit 3) = autovar
    uint16_t autoVarName;//Index(base 0) into string table, present if (flags & 4) != 0
    Function readHandler;//present if (flags & 5) == 1
    Function writeHandler;//present if (flags & 6) == 2

    Property() : name(0), type(0), docstring(0), userFlags(0), flags(0), autoVarName(0) {}
};
#pragma endregion

#pragma region NamedFunction
struct NamedFunction
{
    uint16_t functionName; //Index(base 0) into string table
    Function function;

    NamedFunction() : functionName(0) {}
};
#pragma endregion

#pragma region State
struct State
{
    uint16_t name;      
    uint16_t numFunctions;
    vector<NamedFunction> functions;

    State() : name(0), numFunctions(0) {}
};
#pragma endregion

#pragma region ObjectData
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

    ObjectData() : parentClassName(0), docString(0), userFlags(0), autoStateName(0),
                   numVariables(0), numProperties(0), numStates(0) {}
};
#pragma endregion

#pragma region Object
struct Object
{
    uint16_t nameIndex;
    uint32_t size;
    ObjectData data; 

    Object() : nameIndex(0), size(0) {}

    Object(uint16_t _nameIndex, uint32_t _size)
        : nameIndex(_nameIndex), size(_size)
    {
    }
};
#pragma endregion

#pragma region RecordSections
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
#pragma endregion