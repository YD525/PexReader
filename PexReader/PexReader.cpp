#include "PexHelper.cpp"
#include "LineNumberBuffer.h"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define PEX_READER_API __declspec(dllexport)

// ============================================================
//  Handle = PexData* cast to intptr_t
//  0 / nullptr indicates an invalid handle
// ============================================================

static const std::string Version = "1.0.1.5";

// ============================================================
//  Internal helpers
// ============================================================

// Convert a UTF-8 std::string to a wide std::wstring
static std::wstring UTF8ToWString(const std::string& Str)
{
    if (Str.empty()) return {};
    int Len = MultiByteToWideChar(CP_UTF8, 0, Str.c_str(), (int)Str.size(), nullptr, 0);
    std::wstring Result(Len, 0);
    MultiByteToWideChar(CP_UTF8, 0, Str.c_str(), (int)Str.size(), &Result[0], Len);
    return Result;
}

// Safely cast a handle back to a PexData pointer; returns nullptr on invalid input
static inline PexData* GetInst(intptr_t Handle)
{
    return reinterpret_cast<PexData*>(Handle);
}

// ============================================================
//  DLL entry point
// ============================================================

BOOL APIENTRY DllMain(HMODULE, DWORD, LPVOID)
{
    return TRUE;
}

// ============================================================
//  Exported function declarations
// ============================================================
extern "C"
{
    // Version
    PEX_READER_API const char* C_GetVersion();
    PEX_READER_API int            C_GetVersionLength();

    // Instance lifecycle - each file uses its own independent handle
    PEX_READER_API intptr_t       C_CreateInstance();
    PEX_READER_API void           C_DestroyInstance(intptr_t Handle);

    // PEX file operations
    PEX_READER_API int            C_ReadPex(intptr_t Handle, const wchar_t* PexPath);
    PEX_READER_API int            C_ModifyStringTable(intptr_t Handle, uint16_t Index, const char* Utf8Str);
    PEX_READER_API int            C_SavePex(intptr_t Handle, const wchar_t* PexPath);
    PEX_READER_API void           C_Close(intptr_t Handle);   // Resets the file state without destroying the instance

    // Header accessors
    PEX_READER_API const wchar_t* C_GetHeaderSourceFileName(intptr_t Handle);
    PEX_READER_API const wchar_t* C_GetHeaderUsername(intptr_t Handle);
    PEX_READER_API const wchar_t* C_GetHeaderMachineName(intptr_t Handle);
    PEX_READER_API uint32_t       C_GetHeaderMagic(intptr_t Handle);
    PEX_READER_API uint8_t        C_GetHeaderMajorVersion(intptr_t Handle);
    PEX_READER_API uint8_t        C_GetHeaderMinorVersion(intptr_t Handle);
    PEX_READER_API uint16_t       C_GetHeaderGameId(intptr_t Handle);
    PEX_READER_API uint64_t       C_GetHeaderCompilationTime(intptr_t Handle);

    // String table
    PEX_READER_API uint16_t       C_GetStringTableCount(intptr_t Handle);
    PEX_READER_API int            C_GetStringUtf8(intptr_t Handle, uint16_t Index, char* Buffer, int BufferSize);
    PEX_READER_API int            C_GetStringWide(intptr_t Handle, uint16_t Index, wchar_t* Buffer, int BufferSize);

    // Debug info
    PEX_READER_API uint8_t        C_HasDebugInfo(intptr_t Handle);
    PEX_READER_API uint64_t       C_GetDebugModificationTime(intptr_t Handle);
    PEX_READER_API uint16_t       C_GetDebugFunctionCount(intptr_t Handle);
    PEX_READER_API int            C_GetDebugFunctionInfo(intptr_t Handle, uint16_t Index,
        uint16_t* ObjectNameIndex, uint16_t* StateNameIndex,
        uint16_t* FunctionNameIndex, uint8_t* FunctionType,
        uint16_t** LineNumbers, int* LineCount);

    // User flags
    PEX_READER_API uint16_t       C_GetUserFlagCount(intptr_t Handle);
    PEX_READER_API int            C_GetUserFlagInfo(intptr_t Handle, uint16_t Index,
        uint16_t* FlagNameIndex, uint8_t* FlagIndex);

    // Objects
    PEX_READER_API uint16_t       C_GetObjectCount(intptr_t Handle);
    PEX_READER_API int            C_GetObjectInfo(intptr_t Handle, uint16_t Index,
        uint16_t* NameIndex, uint32_t* Size);
    PEX_READER_API int            C_GetObjectData(intptr_t Handle, uint16_t ObjectIndex,
        uint16_t* ParentClassName, uint16_t* DocString,
        uint32_t* UserFlags, uint16_t* AutoStateName);

    // Variables
    PEX_READER_API uint16_t       C_GetVariableCount(intptr_t Handle, uint16_t ObjectIndex);
    PEX_READER_API int            C_GetVariableInfo(intptr_t Handle, uint16_t ObjectIndex,
        uint16_t VarIndex, uint16_t* Name, uint16_t* TypeName,
        uint32_t* UserFlags, uint8_t* DataType, void* DataValue);

    // Properties
    PEX_READER_API uint16_t       C_GetPropertyCount(intptr_t Handle, uint16_t ObjectIndex);
    PEX_READER_API int            C_GetPropertyInfo(intptr_t Handle, uint16_t ObjectIndex,
        uint16_t PropIndex, uint16_t* Name, uint16_t* Type,
        uint16_t* Docstring, uint32_t* UserFlags,
        uint8_t* Flags, uint16_t* AutoVarName);

    // States
    PEX_READER_API uint16_t       C_GetStateCount(intptr_t Handle, uint16_t ObjectIndex);
    PEX_READER_API int            C_GetStateInfo(intptr_t Handle, uint16_t ObjectIndex,
        uint16_t StateIndex, uint16_t* Name, uint16_t* NumFunctions);

    // Functions
    PEX_READER_API int            C_GetStateFunctionInfo(intptr_t Handle,
        uint16_t ObjectIndex, uint16_t StateIndex, uint16_t FuncIndex,
        uint16_t* FunctionName, uint16_t* ReturnType, uint16_t* DocString,
        uint32_t* UserFlags, uint8_t* Flags,
        uint16_t* NumParams, uint16_t* NumLocals, uint16_t* NumInstructions);

    // Instructions
    PEX_READER_API int            C_GetInstructionInfo(intptr_t Handle,
        uint16_t ObjectIndex, uint16_t StateIndex,
        uint16_t FuncIndex, uint16_t InstrIndex,
        uint8_t* Opcode, uint16_t* ArgCount);
    PEX_READER_API int            C_GetInstructionArgument(intptr_t Handle,
        uint16_t ObjectIndex, uint16_t StateIndex,
        uint16_t FuncIndex, uint16_t InstrIndex, uint16_t ArgIndex,
        uint8_t* Type, void* Value);

    // Function parameters
    PEX_READER_API uint16_t       C_GetFunctionParamCount(intptr_t Handle,
        uint16_t ObjectIndex, uint16_t StateIndex, uint16_t FuncIndex);
    PEX_READER_API int            C_GetFunctionParamInfo(intptr_t Handle,
        uint16_t ObjectIndex, uint16_t StateIndex,
        uint16_t FuncIndex, uint16_t ParamIndex,
        uint16_t* Name, uint16_t* Type);

    // Function locals
    PEX_READER_API uint16_t       C_GetFunctionLocalCount(intptr_t Handle,
        uint16_t ObjectIndex, uint16_t StateIndex, uint16_t FuncIndex);
    PEX_READER_API int            C_GetFunctionLocalInfo(intptr_t Handle,
        uint16_t ObjectIndex, uint16_t StateIndex,
        uint16_t FuncIndex, uint16_t LocalIndex,
        uint16_t* Name, uint16_t* Type);

    // Memory management
    PEX_READER_API void           C_FreeBuffer(void* Buffer);
}

// ============================================================
//  Implementation
// ============================================================

const char* C_GetVersion() { return Version.c_str(); }
int         C_GetVersionLength() { return (int)Version.size(); }

// --------------------------------------------------------
//  Instance lifecycle
// --------------------------------------------------------

// Allocate a new PexData instance and return its handle.
// Returns 0 if allocation fails.
intptr_t C_CreateInstance()
{
    try
    {
        PexData* Inst = new PexData();
        return reinterpret_cast<intptr_t>(Inst);
    }
    catch (...)
    {
        return 0;
    }
}

// Free all memory associated with the given handle.
// The handle must not be used after this call.
void C_DestroyInstance(intptr_t Handle)
{
    PexData* Inst = GetInst(Handle);
    if (Inst) delete Inst;
}

// --------------------------------------------------------
//  PEX file operations
// --------------------------------------------------------

// Load a PEX file into the instance.
// Returns 1 on success, 0 on failure.
int C_ReadPex(intptr_t Handle, const wchar_t* PexPath)
{
    PexData* Inst = GetInst(Handle);
    if (!Inst || !PexPath) return 0;
    try
    {
        Inst->Load(PexPath);
        return 1;
    }
    catch (...)
    {
        return 0;
    }
}

// Replace the string at the given index in the string table.
// Utf8Str must be a null-terminated UTF-8 string.
// Returns 1 on success, 0 on failure.
int C_ModifyStringTable(intptr_t Handle, uint16_t Index, const char* Utf8Str)
{
    PexData* Inst = GetInst(Handle);
    if (!Inst || !Utf8Str) return 0;
    try
    {
        Inst->ModifyStringTable(Index, std::string(Utf8Str));
        return 1;
    }
    catch (...)
    {
        return 0;
    }
}

// Write the current PEX data to the given file path.
// Returns 1 on success, 0 on failure.
int C_SavePex(intptr_t Handle, const wchar_t* PexPath)
{
    PexData* Inst = GetInst(Handle);
    if (!Inst || !PexPath) return 0;
    try
    {
        Inst->Save(PexPath);
        return 1;
    }
    catch (...)
    {
        return 0;
    }
}

// Reset the file state inside the instance without freeing the instance itself.
// Call C_DestroyInstance to fully release memory.
void C_Close(intptr_t Handle)
{
    PexData* Inst = GetInst(Handle);
    if (Inst)
        *Inst = PexData{};
}

// --------------------------------------------------------
//  Header accessors
//  Note: string pointers reference a static buffer - copy immediately if needed.
// --------------------------------------------------------

const wchar_t* C_GetHeaderSourceFileName(intptr_t Handle)
{
    static std::wstring Buf;
    PexData* Inst = GetInst(Handle);
    if (!Inst) return L"";
    Buf = Inst->Header.sourceFileName;
    return Buf.c_str();
}

const wchar_t* C_GetHeaderUsername(intptr_t Handle)
{
    static std::wstring Buf;
    PexData* Inst = GetInst(Handle);
    if (!Inst) return L"";
    Buf = Inst->Header.username;
    return Buf.c_str();
}

const wchar_t* C_GetHeaderMachineName(intptr_t Handle)
{
    static std::wstring Buf;
    PexData* Inst = GetInst(Handle);
    if (!Inst) return L"";
    Buf = Inst->Header.machinename;
    return Buf.c_str();
}

uint32_t C_GetHeaderMagic(intptr_t Handle)
{
    PexData* Inst = GetInst(Handle);
    return Inst ? Inst->Header.magic : 0;
}

uint8_t C_GetHeaderMajorVersion(intptr_t Handle)
{
    PexData* Inst = GetInst(Handle);
    return Inst ? Inst->Header.majorVersion : 0;
}

uint8_t C_GetHeaderMinorVersion(intptr_t Handle)
{
    PexData* Inst = GetInst(Handle);
    return Inst ? Inst->Header.minorVersion : 0;
}

uint16_t C_GetHeaderGameId(intptr_t Handle)
{
    PexData* Inst = GetInst(Handle);
    return Inst ? Inst->Header.gameId : 0;
}

uint64_t C_GetHeaderCompilationTime(intptr_t Handle)
{
    PexData* Inst = GetInst(Handle);
    return Inst ? Inst->Header.compilationTime : 0;
}

// --------------------------------------------------------
//  String table
// --------------------------------------------------------

uint16_t C_GetStringTableCount(intptr_t Handle)
{
    PexData* Inst = GetInst(Handle);
    return Inst ? Inst->stringTable.count : 0;
}

// If Buffer is null, returns the required byte length without writing.
// Returns -1 on error.
int C_GetStringUtf8(intptr_t Handle, uint16_t Index, char* Buffer, int BufferSize)
{
    PexData* Inst = GetInst(Handle);
    if (!Inst || Index >= Inst->stringTable.count) return -1;
    try
    {
        std::string Str = Inst->stringTable.ToUtf8(Index);
        int Length = (int)Str.size();
        if (Buffer && BufferSize > Length)
            std::memcpy(Buffer, Str.c_str(), Length + 1);
        return Length;
    }
    catch (...) { return -1; }
}

// If Buffer is null, returns the required character count without writing.
// Returns -1 on error.
int C_GetStringWide(intptr_t Handle, uint16_t Index, wchar_t* Buffer, int BufferSize)
{
    PexData* Inst = GetInst(Handle);
    if (!Inst || Index >= Inst->stringTable.count) return -1;
    try
    {
        std::wstring Wide = UTF8ToWString(Inst->stringTable.ToUtf8(Index));
        int Length = (int)Wide.size();
        if (Buffer && BufferSize > Length)
            std::memcpy(Buffer, Wide.c_str(), (Length + 1) * sizeof(wchar_t));
        return Length;
    }
    catch (...) { return -1; }
}

// --------------------------------------------------------
//  Debug info
// --------------------------------------------------------

uint8_t C_HasDebugInfo(intptr_t Handle)
{
    PexData* Inst = GetInst(Handle);
    return Inst ? Inst->debugInfo.hasDebugInfo : 0;
}

uint64_t C_GetDebugModificationTime(intptr_t Handle)
{
    PexData* Inst = GetInst(Handle);
    return Inst ? Inst->debugInfo.modificationTime : 0;
}

uint16_t C_GetDebugFunctionCount(intptr_t Handle)
{
    PexData* Inst = GetInst(Handle);
    return Inst ? Inst->debugInfo.functionCount : 0;
}

// LineNumbers is heap-allocated by this function; the caller must free it via C_FreeBuffer.
// Returns 1 on success, 0 on failure.
int C_GetDebugFunctionInfo(intptr_t Handle, uint16_t Index,
    uint16_t* ObjectNameIndex, uint16_t* StateNameIndex,
    uint16_t* FunctionNameIndex, uint8_t* FunctionType,
    uint16_t** LineNumbers, int* LineCount)
{
    if (LineNumbers) *LineNumbers = nullptr;
    if (LineCount) *LineCount = 0;

    PexData* Inst = GetInst(Handle);
    if (!Inst || Index >= Inst->debugInfo.functionCount) return 0;

    const auto& Func = Inst->debugInfo.functions[Index];
    if (ObjectNameIndex)   *ObjectNameIndex = Func.objectNameIndex;
    if (StateNameIndex)    *StateNameIndex = Func.stateNameIndex;
    if (FunctionNameIndex) *FunctionNameIndex = Func.functionNameIndex;
    if (FunctionType)      *FunctionType = Func.functionType;

    if (LineNumbers && LineCount)
    {
        try
        {
            *LineNumbers = pex::interop::CopyLineNumbers(Func.lineNumbers);
            *LineCount = static_cast<int>(Func.lineNumbers.size());
        }
        catch (...)
        {
            *LineNumbers = nullptr;
            return 0;
        }
    }
    return 1;
}

// --------------------------------------------------------
//  User flags
// --------------------------------------------------------

uint16_t C_GetUserFlagCount(intptr_t Handle)
{
    PexData* Inst = GetInst(Handle);
    return Inst ? Inst->userFlagCount : 0;
}

// Returns 1 on success, 0 if the index is out of range.
int C_GetUserFlagInfo(intptr_t Handle, uint16_t Index,
    uint16_t* FlagNameIndex, uint8_t* FlagIndex)
{
    PexData* Inst = GetInst(Handle);
    if (!Inst || Index >= Inst->userFlagCount) return 0;
    const auto& Flag = Inst->userFlags[Index];
    if (FlagNameIndex) *FlagNameIndex = Flag.flagNameIndex;
    if (FlagIndex)     *FlagIndex = Flag.flagIndex;
    return 1;
}

// --------------------------------------------------------
//  Objects
// --------------------------------------------------------

uint16_t C_GetObjectCount(intptr_t Handle)
{
    PexData* Inst = GetInst(Handle);
    return Inst ? Inst->objectCount : 0;
}

// Returns 1 on success, 0 if the index is out of range.
int C_GetObjectInfo(intptr_t Handle, uint16_t Index,
    uint16_t* NameIndex, uint32_t* Size)
{
    PexData* Inst = GetInst(Handle);
    if (!Inst || Index >= Inst->objectCount) return 0;
    const auto& Obj = Inst->objects[Index];
    if (NameIndex) *NameIndex = Obj.nameIndex;
    if (Size)      *Size = Obj.size;
    return 1;
}

// Returns 1 on success, 0 if the index is out of range.
int C_GetObjectData(intptr_t Handle, uint16_t ObjectIndex,
    uint16_t* ParentClassName, uint16_t* DocString,
    uint32_t* UserFlags, uint16_t* AutoStateName)
{
    PexData* Inst = GetInst(Handle);
    if (!Inst || ObjectIndex >= Inst->objectCount) return 0;
    const auto& Data = Inst->objects[ObjectIndex].data;
    if (ParentClassName) *ParentClassName = Data.parentClassName;
    if (DocString)       *DocString = Data.docString;
    if (UserFlags)       *UserFlags = Data.userFlags;
    if (AutoStateName)   *AutoStateName = Data.autoStateName;
    return 1;
}

// --------------------------------------------------------
//  Variables
// --------------------------------------------------------

uint16_t C_GetVariableCount(intptr_t Handle, uint16_t ObjectIndex)
{
    PexData* Inst = GetInst(Handle);
    if (!Inst || ObjectIndex >= Inst->objectCount) return 0;
    return Inst->objects[ObjectIndex].data.numVariables;
}

// DataValue receives the raw bytes of the variable value; interpretation depends on DataType.
// Pass nullptr for DataValue if only type metadata is needed.
// Returns 1 on success, 0 on failure.
int C_GetVariableInfo(intptr_t Handle, uint16_t ObjectIndex, uint16_t VarIndex,
    uint16_t* Name, uint16_t* TypeName,
    uint32_t* UserFlags, uint8_t* DataType, void* DataValue)
{
    PexData* Inst = GetInst(Handle);
    if (!Inst || ObjectIndex >= Inst->objectCount) return 0;
    const auto& Obj = Inst->objects[ObjectIndex];
    if (VarIndex >= Obj.data.numVariables) return 0;
    const auto& Var = Obj.data.variables[VarIndex];
    if (Name)      *Name = Var.name;
    if (TypeName)  *TypeName = Var.typeName;
    if (UserFlags) *UserFlags = Var.userFlags;
    if (DataType)  *DataType = Var.data.type;
    if (DataValue)
    {
        switch (Var.data.type)
        {
        case 1: case 2: *reinterpret_cast<uint16_t*>(DataValue) = std::get<uint16_t>(Var.data.data); break;
        case 3:         *reinterpret_cast<int32_t*> (DataValue) = std::get<int32_t>(Var.data.data); break;
        case 4:         *reinterpret_cast<float*>   (DataValue) = std::get<float>(Var.data.data); break;
        case 5:         *reinterpret_cast<uint8_t*> (DataValue) = std::get<uint8_t>(Var.data.data); break;
        default: break;
        }
    }
    return 1;
}

// --------------------------------------------------------
//  Properties
// --------------------------------------------------------

uint16_t C_GetPropertyCount(intptr_t Handle, uint16_t ObjectIndex)
{
    PexData* Inst = GetInst(Handle);
    if (!Inst || ObjectIndex >= Inst->objectCount) return 0;
    return Inst->objects[ObjectIndex].data.numProperties;
}

// Returns 1 on success, 0 if any index is out of range.
int C_GetPropertyInfo(intptr_t Handle, uint16_t ObjectIndex, uint16_t PropIndex,
    uint16_t* Name, uint16_t* Type, uint16_t* Docstring,
    uint32_t* UserFlags, uint8_t* Flags, uint16_t* AutoVarName)
{
    PexData* Inst = GetInst(Handle);
    if (!Inst || ObjectIndex >= Inst->objectCount) return 0;
    const auto& Obj = Inst->objects[ObjectIndex];
    if (PropIndex >= Obj.data.numProperties) return 0;
    const auto& Prop = Obj.data.properties[PropIndex];
    if (Name)        *Name = Prop.name;
    if (Type)        *Type = Prop.type;
    if (Docstring)   *Docstring = Prop.docstring;
    if (UserFlags)   *UserFlags = Prop.userFlags;
    if (Flags)       *Flags = Prop.flags;
    if (AutoVarName) *AutoVarName = Prop.autoVarName;
    return 1;
}

// --------------------------------------------------------
//  States
// --------------------------------------------------------

uint16_t C_GetStateCount(intptr_t Handle, uint16_t ObjectIndex)
{
    PexData* Inst = GetInst(Handle);
    if (!Inst || ObjectIndex >= Inst->objectCount) return 0;
    return Inst->objects[ObjectIndex].data.numStates;
}

// Returns 1 on success, 0 if any index is out of range.
int C_GetStateInfo(intptr_t Handle, uint16_t ObjectIndex, uint16_t StateIndex,
    uint16_t* Name, uint16_t* NumFunctions)
{
    PexData* Inst = GetInst(Handle);
    if (!Inst || ObjectIndex >= Inst->objectCount) return 0;
    const auto& Obj = Inst->objects[ObjectIndex];
    if (StateIndex >= Obj.data.numStates) return 0;
    const auto& State = Obj.data.states[StateIndex];
    if (Name)         *Name = State.name;
    if (NumFunctions) *NumFunctions = State.numFunctions;
    return 1;
}

// --------------------------------------------------------
//  Functions
// --------------------------------------------------------

// Returns 1 on success, 0 if any index is out of range.
int C_GetStateFunctionInfo(intptr_t Handle,
    uint16_t ObjectIndex, uint16_t StateIndex, uint16_t FuncIndex,
    uint16_t* FunctionName, uint16_t* ReturnType, uint16_t* DocString,
    uint32_t* UserFlags, uint8_t* Flags,
    uint16_t* NumParams, uint16_t* NumLocals, uint16_t* NumInstructions)
{
    PexData* Inst = GetInst(Handle);
    if (!Inst || ObjectIndex >= Inst->objectCount) return 0;
    const auto& Obj = Inst->objects[ObjectIndex];
    if (StateIndex >= Obj.data.numStates) return 0;
    const auto& State = Obj.data.states[StateIndex];
    if (FuncIndex >= State.numFunctions) return 0;
    const auto& NamedFunc = State.functions[FuncIndex];
    const auto& Func = NamedFunc.function;
    if (FunctionName)    *FunctionName = NamedFunc.functionName;
    if (ReturnType)      *ReturnType = Func.returnType;
    if (DocString)       *DocString = Func.docString;
    if (UserFlags)       *UserFlags = Func.userFlags;
    if (Flags)           *Flags = Func.flags;
    if (NumParams)       *NumParams = Func.numParams;
    if (NumLocals)       *NumLocals = Func.numLocals;
    if (NumInstructions) *NumInstructions = Func.numInstructions;
    return 1;
}

// --------------------------------------------------------
//  Instructions
// --------------------------------------------------------

// Returns 1 on success, 0 if any index is out of range.
int C_GetInstructionInfo(intptr_t Handle,
    uint16_t ObjectIndex, uint16_t StateIndex,
    uint16_t FuncIndex, uint16_t InstrIndex,
    uint8_t* Opcode, uint16_t* ArgCount)
{
    PexData* Inst = GetInst(Handle);
    if (!Inst || ObjectIndex >= Inst->objectCount) return 0;
    const auto& Obj = Inst->objects[ObjectIndex];
    if (StateIndex >= Obj.data.numStates) return 0;
    const auto& State = Obj.data.states[StateIndex];
    if (FuncIndex >= State.numFunctions) return 0;
    const auto& Func = State.functions[FuncIndex].function;
    if (InstrIndex >= Func.numInstructions) return 0;
    const auto& Instr = Func.instructions[InstrIndex];
    if (Opcode)   *Opcode = static_cast<uint8_t>(Instr.op);
    if (ArgCount) *ArgCount = static_cast<uint16_t>(Instr.arguments.size());
    return 1;
}

// Value receives the raw bytes of the argument; interpretation depends on Type.
// Returns 1 on success, 0 if any index is out of range.
int C_GetInstructionArgument(intptr_t Handle,
    uint16_t ObjectIndex, uint16_t StateIndex,
    uint16_t FuncIndex, uint16_t InstrIndex, uint16_t ArgIndex,
    uint8_t* Type, void* Value)
{
    PexData* Inst = GetInst(Handle);
    if (!Inst || ObjectIndex >= Inst->objectCount) return 0;
    const auto& Obj = Inst->objects[ObjectIndex];
    if (StateIndex >= Obj.data.numStates) return 0;
    const auto& State = Obj.data.states[StateIndex];
    if (FuncIndex >= State.numFunctions) return 0;
    const auto& Func = State.functions[FuncIndex].function;
    if (InstrIndex >= Func.numInstructions) return 0;
    const auto& Instr = Func.instructions[InstrIndex];
    if (ArgIndex >= Instr.arguments.size()) return 0;
    const auto& Arg = Instr.arguments[ArgIndex];
    if (Type) *Type = Arg.type;
    if (Value)
    {
        switch (Arg.type)
        {
        case 1: case 2: *reinterpret_cast<uint16_t*>(Value) = std::get<uint16_t>(Arg.data); break;
        case 3:         *reinterpret_cast<int32_t*> (Value) = std::get<int32_t>(Arg.data); break;
        case 4:         *reinterpret_cast<float*>   (Value) = std::get<float>(Arg.data); break;
        case 5:         *reinterpret_cast<uint8_t*> (Value) = std::get<uint8_t>(Arg.data); break;
        default: break;
        }
    }
    return 1;
}

// --------------------------------------------------------
//  Function parameters
// --------------------------------------------------------

uint16_t C_GetFunctionParamCount(intptr_t Handle,
    uint16_t ObjectIndex, uint16_t StateIndex, uint16_t FuncIndex)
{
    PexData* Inst = GetInst(Handle);
    if (!Inst || ObjectIndex >= Inst->objectCount) return 0;
    const auto& Obj = Inst->objects[ObjectIndex];
    if (StateIndex >= Obj.data.numStates) return 0;
    const auto& State = Obj.data.states[StateIndex];
    if (FuncIndex >= State.numFunctions) return 0;
    return State.functions[FuncIndex].function.numParams;
}

// Returns 1 on success, 0 if any index is out of range.
int C_GetFunctionParamInfo(intptr_t Handle,
    uint16_t ObjectIndex, uint16_t StateIndex,
    uint16_t FuncIndex, uint16_t ParamIndex,
    uint16_t* Name, uint16_t* Type)
{
    PexData* Inst = GetInst(Handle);
    if (!Inst || ObjectIndex >= Inst->objectCount) return 0;
    const auto& Obj = Inst->objects[ObjectIndex];
    if (StateIndex >= Obj.data.numStates) return 0;
    const auto& State = Obj.data.states[StateIndex];
    if (FuncIndex >= State.numFunctions) return 0;
    const auto& Func = State.functions[FuncIndex].function;
    if (ParamIndex >= Func.numParams) return 0;
    const auto& Param = Func.params[ParamIndex];
    if (Name) *Name = Param.name;
    if (Type) *Type = Param.type;
    return 1;
}

// --------------------------------------------------------
//  Function locals
// --------------------------------------------------------

uint16_t C_GetFunctionLocalCount(intptr_t Handle,
    uint16_t ObjectIndex, uint16_t StateIndex, uint16_t FuncIndex)
{
    PexData* Inst = GetInst(Handle);
    if (!Inst || ObjectIndex >= Inst->objectCount) return 0;
    const auto& Obj = Inst->objects[ObjectIndex];
    if (StateIndex >= Obj.data.numStates) return 0;
    const auto& State = Obj.data.states[StateIndex];
    if (FuncIndex >= State.numFunctions) return 0;
    return State.functions[FuncIndex].function.numLocals;
}

// Returns 1 on success, 0 if any index is out of range.
int C_GetFunctionLocalInfo(intptr_t Handle,
    uint16_t ObjectIndex, uint16_t StateIndex,
    uint16_t FuncIndex, uint16_t LocalIndex,
    uint16_t* Name, uint16_t* Type)
{
    PexData* Inst = GetInst(Handle);
    if (!Inst || ObjectIndex >= Inst->objectCount) return 0;
    const auto& Obj = Inst->objects[ObjectIndex];
    if (StateIndex >= Obj.data.numStates) return 0;
    const auto& State = Obj.data.states[StateIndex];
    if (FuncIndex >= State.numFunctions) return 0;
    const auto& Func = State.functions[FuncIndex].function;
    if (LocalIndex >= Func.numLocals) return 0;
    const auto& Local = Func.locals[LocalIndex];
    if (Name) *Name = Local.name;
    if (Type) *Type = Local.type;
    return 1;
}

// --------------------------------------------------------
//  Memory management
// --------------------------------------------------------

// Free a line-number buffer returned by C_GetDebugFunctionInfo while preserving the original ABI.
void C_FreeBuffer(void* Buffer)
{
    pex::interop::FreeLineNumbers(static_cast<uint16_t*>(Buffer));
}
