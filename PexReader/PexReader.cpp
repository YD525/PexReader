#include "PexHelper.cpp"
#include "LineNumberBuffer.h"

#define PEX_READER_EXPORTS
#include "PexReaderApi.h"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <algorithm>
#include <cstring>
#include <new>
#include <stdexcept>

// ============================================================
//  Handle = PexData* cast to intptr_t
//  0 / nullptr indicates an invalid handle
// ============================================================

static const std::string Version = "1.0.1.6";

// ============================================================
//  Internal helpers
// ============================================================

// Convert a UTF-8 std::string to a wide std::wstring
static std::wstring UTF8ToWString(const std::string& Str)
{
    if (Str.empty()) return {};
    const int Len = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        Str.data(),
        static_cast<int>(Str.size()),
        nullptr,
        0);
    if (Len <= 0)
        throw std::invalid_argument("The PEX string is not valid UTF-8.");

    std::wstring Result(Len, 0);
    const int Converted = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        Str.data(),
        static_cast<int>(Str.size()),
        &Result[0],
        Len);
    if (Converted != Len)
        throw std::invalid_argument("The PEX string could not be converted to UTF-16.");
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

namespace pex
{
namespace interop
{
namespace implementation
{

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
    Inst->ModifyStringTable(Index, std::string(Utf8Str));
    return 1;
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
    static thread_local std::wstring Buf;
    PexData* Inst = GetInst(Handle);
    if (!Inst) return L"";
    Buf = Inst->Header.sourceFileName;
    return Buf.c_str();
}

const wchar_t* C_GetHeaderUsername(intptr_t Handle)
{
    static thread_local std::wstring Buf;
    PexData* Inst = GetInst(Handle);
    if (!Inst) return L"";
    Buf = Inst->Header.username;
    return Buf.c_str();
}

const wchar_t* C_GetHeaderMachineName(intptr_t Handle)
{
    static thread_local std::wstring Buf;
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
    std::string Str = Inst->stringTable.ToUtf8(Index);
    int Length = (int)Str.size();
    if (Buffer && BufferSize > Length)
        std::memcpy(Buffer, Str.c_str(), Length + 1);
    return Length;
}

// If Buffer is null, returns the required character count without writing.
// Returns -1 on error.
int C_GetStringWide(intptr_t Handle, uint16_t Index, wchar_t* Buffer, int BufferSize)
{
    PexData* Inst = GetInst(Handle);
    if (!Inst || Index >= Inst->stringTable.count) return -1;
    std::wstring Wide = UTF8ToWString(Inst->stringTable.ToUtf8(Index));
    int Length = (int)Wide.size();
    if (Buffer && BufferSize > Length)
        std::memcpy(Buffer, Wide.c_str(), (Length + 1) * sizeof(wchar_t));
    return Length;
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
        *LineNumbers = ::pex::interop::CopyLineNumbers(Func.lineNumbers);
        *LineCount = static_cast<int>(Func.lineNumbers.size());
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
    ::pex::interop::FreeLineNumbers(static_cast<uint16_t*>(Buffer));
}

}
}
}

namespace
{
    struct AbiErrorState
    {
        PexReaderStatus Status = PEX_READER_STATUS_OK;
        char Message[256]{};
    };

    thread_local AbiErrorState LastAbiError;

    void ClearAbiError() noexcept
    {
        LastAbiError.Status = PEX_READER_STATUS_OK;
        LastAbiError.Message[0] = '\0';
    }

    void SetAbiError(PexReaderStatus status, const char* message) noexcept
    {
        LastAbiError.Status = status;
        const std::size_t length = (std::min)(std::strlen(message), sizeof(LastAbiError.Message) - 1);
        std::memcpy(LastAbiError.Message, message, length);
        LastAbiError.Message[length] = '\0';
    }

    void CaptureAbiException() noexcept
    {
        try
        {
            throw;
        }
        catch (const std::bad_alloc&)
        {
            SetAbiError(PEX_READER_STATUS_OUT_OF_MEMORY, "PexReader could not allocate required memory.");
        }
        catch (const std::invalid_argument&)
        {
            SetAbiError(PEX_READER_STATUS_INVALID_ARGUMENT, "PexReader rejected an invalid argument.");
        }
        catch (const std::out_of_range&)
        {
            SetAbiError(PEX_READER_STATUS_OUT_OF_RANGE, "PexReader rejected an out-of-range value.");
        }
        catch (const std::ios_base::failure&)
        {
            SetAbiError(PEX_READER_STATUS_IO_ERROR, "PexReader encountered an input or output error.");
        }
        catch (const std::exception&)
        {
            SetAbiError(PEX_READER_STATUS_INTERNAL_ERROR, "PexReader encountered an internal error.");
        }
        catch (...)
        {
            SetAbiError(PEX_READER_STATUS_INTERNAL_ERROR, "PexReader encountered an unknown internal error.");
        }
    }

    template<typename TResult, typename TAction>
    TResult InvokeAbi(TResult failureValue, TAction&& action) noexcept
    {
        try
        {
            ClearAbiError();
            return action();
        }
        catch (...)
        {
            CaptureAbiException();
            return failureValue;
        }
    }

    template<typename TAction>
    void InvokeAbi(TAction&& action) noexcept
    {
        try
        {
            ClearAbiError();
            action();
        }
        catch (...)
        {
            CaptureAbiException();
        }
    }

    bool ValidateHandle(PexReaderHandle handle) noexcept
    {
        if (handle != 0)
            return true;

        SetAbiError(PEX_READER_STATUS_INVALID_ARGUMENT, "The PexReader handle is null.");
        return false;
    }
}

uint32_t PEX_READER_CALL C_GetAbiVersion(void) noexcept
{
    return PEX_READER_ABI_VERSION;
}

PexReaderStatus PEX_READER_CALL C_GetLastStatus(void) noexcept
{
    return LastAbiError.Status;
}

int32_t PEX_READER_CALL C_GetLastErrorUtf8(uint8_t* buffer, int32_t bufferSize) noexcept
{
    const std::size_t length = std::strlen(LastAbiError.Message);
    if (length > static_cast<std::size_t>(INT32_MAX))
        return -1;
    if (buffer != nullptr && bufferSize > static_cast<int32_t>(length))
        std::memcpy(buffer, LastAbiError.Message, length + 1);
    return static_cast<int32_t>(length);
}

#define PEX_ABI_RETURN(returnType, name, failureValue, parameters, arguments) \
    returnType PEX_READER_CALL name parameters noexcept \
    { \
        return InvokeAbi<returnType>(failureValue, [&]() -> returnType \
        { \
            return pex::interop::implementation::name arguments; \
        }); \
    }

#define PEX_ABI_HANDLE_RETURN(returnType, name, failureValue, parameters, arguments) \
    returnType PEX_READER_CALL name parameters noexcept \
    { \
        return InvokeAbi<returnType>(failureValue, [&]() -> returnType \
        { \
            if (!ValidateHandle(handle)) \
                return failureValue; \
            return pex::interop::implementation::name arguments; \
        }); \
    }

#define PEX_ABI_VOID(name, parameters, arguments) \
    void PEX_READER_CALL name parameters noexcept \
    { \
        InvokeAbi([&]() { pex::interop::implementation::name arguments; }); \
    }

#define PEX_ABI_HANDLE_VOID(name, parameters, arguments) \
    void PEX_READER_CALL name parameters noexcept \
    { \
        InvokeAbi([&]() \
        { \
            if (handle != 0) \
                pex::interop::implementation::name arguments; \
        }); \
    }

PEX_ABI_RETURN(const char*, C_GetVersion, nullptr, (void), ())
PEX_ABI_RETURN(int32_t, C_GetVersionLength, -1, (void), ())

PexReaderHandle PEX_READER_CALL C_CreateInstance(void) noexcept
{
    return InvokeAbi<PexReaderHandle>(0, []()
    {
        const PexReaderHandle handle = pex::interop::implementation::C_CreateInstance();
        if (handle == 0)
            SetAbiError(PEX_READER_STATUS_OUT_OF_MEMORY, "PexReader could not create an instance.");
        return handle;
    });
}

PEX_ABI_VOID(C_DestroyInstance, (PexReaderHandle handle), (handle))

int32_t PEX_READER_CALL C_ReadPex(PexReaderHandle handle, const wchar_t* pexPath) noexcept
{
    return InvokeAbi<int32_t>(0, [&]()
    {
        if (!ValidateHandle(handle) || pexPath == nullptr)
        {
            if (pexPath == nullptr)
                SetAbiError(PEX_READER_STATUS_INVALID_ARGUMENT, "The PEX input path is null.");
            return 0;
        }

        const int32_t result = pex::interop::implementation::C_ReadPex(handle, pexPath);
        if (result == 0)
            SetAbiError(PEX_READER_STATUS_PARSE_ERROR, "The PEX file could not be opened or parsed.");
        return result;
    });
}

int32_t PEX_READER_CALL C_ModifyStringTable(
    PexReaderHandle handle,
    uint16_t index,
    const char* utf8String) noexcept
{
    return InvokeAbi<int32_t>(0, [&]()
    {
        if (!ValidateHandle(handle) || utf8String == nullptr)
        {
            if (utf8String == nullptr)
                SetAbiError(PEX_READER_STATUS_INVALID_ARGUMENT, "The replacement UTF-8 string is null.");
            return 0;
        }

        const int32_t result = pex::interop::implementation::C_ModifyStringTable(handle, index, utf8String);
        if (result == 0)
            SetAbiError(PEX_READER_STATUS_OUT_OF_RANGE, "The string table index is out of range.");
        return result;
    });
}

int32_t PEX_READER_CALL C_SavePex(PexReaderHandle handle, const wchar_t* pexPath) noexcept
{
    return InvokeAbi<int32_t>(0, [&]()
    {
        if (!ValidateHandle(handle) || pexPath == nullptr)
        {
            if (pexPath == nullptr)
                SetAbiError(PEX_READER_STATUS_INVALID_ARGUMENT, "The PEX output path is null.");
            return 0;
        }

        const int32_t result = pex::interop::implementation::C_SavePex(handle, pexPath);
        if (result == 0)
            SetAbiError(PEX_READER_STATUS_IO_ERROR, "The PEX file could not be saved.");
        return result;
    });
}

PEX_ABI_HANDLE_VOID(C_Close, (PexReaderHandle handle), (handle))
PEX_ABI_HANDLE_RETURN(const wchar_t*, C_GetHeaderSourceFileName, L"", (PexReaderHandle handle), (handle))
PEX_ABI_HANDLE_RETURN(const wchar_t*, C_GetHeaderUsername, L"", (PexReaderHandle handle), (handle))
PEX_ABI_HANDLE_RETURN(const wchar_t*, C_GetHeaderMachineName, L"", (PexReaderHandle handle), (handle))
PEX_ABI_HANDLE_RETURN(uint32_t, C_GetHeaderMagic, 0, (PexReaderHandle handle), (handle))
PEX_ABI_HANDLE_RETURN(uint8_t, C_GetHeaderMajorVersion, 0, (PexReaderHandle handle), (handle))
PEX_ABI_HANDLE_RETURN(uint8_t, C_GetHeaderMinorVersion, 0, (PexReaderHandle handle), (handle))
PEX_ABI_HANDLE_RETURN(uint16_t, C_GetHeaderGameId, 0, (PexReaderHandle handle), (handle))
PEX_ABI_HANDLE_RETURN(uint64_t, C_GetHeaderCompilationTime, 0, (PexReaderHandle handle), (handle))
PEX_ABI_HANDLE_RETURN(uint16_t, C_GetStringTableCount, 0, (PexReaderHandle handle), (handle))

int32_t PEX_READER_CALL C_GetStringUtf8(
    PexReaderHandle handle,
    uint16_t index,
    char* buffer,
    int32_t bufferSize) noexcept
{
    return InvokeAbi<int32_t>(-1, [&]()
    {
        if (!ValidateHandle(handle) || bufferSize < 0 || (buffer == nullptr && bufferSize != 0))
        {
            SetAbiError(PEX_READER_STATUS_INVALID_ARGUMENT, "The UTF-8 string request is invalid.");
            return -1;
        }
        const int32_t length = pex::interop::implementation::C_GetStringUtf8(handle, index, buffer, bufferSize);
        if (length < 0)
            SetAbiError(PEX_READER_STATUS_OUT_OF_RANGE, "The string table index is out of range.");
        else if (buffer != nullptr && bufferSize <= length)
            SetAbiError(PEX_READER_STATUS_BUFFER_TOO_SMALL, "The UTF-8 output buffer is too small.");
        return length;
    });
}

int32_t PEX_READER_CALL C_GetStringWide(
    PexReaderHandle handle,
    uint16_t index,
    wchar_t* buffer,
    int32_t bufferSize) noexcept
{
    return InvokeAbi<int32_t>(-1, [&]()
    {
        if (!ValidateHandle(handle) || bufferSize < 0 || (buffer == nullptr && bufferSize != 0))
        {
            SetAbiError(PEX_READER_STATUS_INVALID_ARGUMENT, "The UTF-16 string request is invalid.");
            return -1;
        }
        const int32_t length = pex::interop::implementation::C_GetStringWide(handle, index, buffer, bufferSize);
        if (length < 0)
            SetAbiError(PEX_READER_STATUS_OUT_OF_RANGE, "The string table index is out of range.");
        else if (buffer != nullptr && bufferSize <= length)
            SetAbiError(PEX_READER_STATUS_BUFFER_TOO_SMALL, "The UTF-16 output buffer is too small.");
        return length;
    });
}

PEX_ABI_HANDLE_RETURN(uint8_t, C_HasDebugInfo, 0, (PexReaderHandle handle), (handle))
PEX_ABI_HANDLE_RETURN(uint64_t, C_GetDebugModificationTime, 0, (PexReaderHandle handle), (handle))
PEX_ABI_HANDLE_RETURN(uint16_t, C_GetDebugFunctionCount, 0, (PexReaderHandle handle), (handle))
PEX_ABI_HANDLE_RETURN(int32_t, C_GetDebugFunctionInfo, 0,
    (PexReaderHandle handle, uint16_t index, uint16_t* objectNameIndex, uint16_t* stateNameIndex,
        uint16_t* functionNameIndex, uint8_t* functionType, uint16_t** lineNumbers, int32_t* lineCount),
    (handle, index, objectNameIndex, stateNameIndex, functionNameIndex, functionType, lineNumbers, lineCount))
PEX_ABI_HANDLE_RETURN(uint16_t, C_GetUserFlagCount, 0, (PexReaderHandle handle), (handle))
PEX_ABI_HANDLE_RETURN(int32_t, C_GetUserFlagInfo, 0,
    (PexReaderHandle handle, uint16_t index, uint16_t* flagNameIndex, uint8_t* flagIndex),
    (handle, index, flagNameIndex, flagIndex))
PEX_ABI_HANDLE_RETURN(uint16_t, C_GetObjectCount, 0, (PexReaderHandle handle), (handle))
PEX_ABI_HANDLE_RETURN(int32_t, C_GetObjectInfo, 0,
    (PexReaderHandle handle, uint16_t index, uint16_t* nameIndex, uint32_t* size),
    (handle, index, nameIndex, size))
PEX_ABI_HANDLE_RETURN(int32_t, C_GetObjectData, 0,
    (PexReaderHandle handle, uint16_t objectIndex, uint16_t* parentClassName, uint16_t* docString,
        uint32_t* userFlags, uint16_t* autoStateName),
    (handle, objectIndex, parentClassName, docString, userFlags, autoStateName))
PEX_ABI_HANDLE_RETURN(uint16_t, C_GetVariableCount, 0,
    (PexReaderHandle handle, uint16_t objectIndex), (handle, objectIndex))
PEX_ABI_HANDLE_RETURN(int32_t, C_GetVariableInfo, 0,
    (PexReaderHandle handle, uint16_t objectIndex, uint16_t variableIndex, uint16_t* name,
        uint16_t* typeName, uint32_t* userFlags, uint8_t* dataType, PexReaderValue* dataValue),
    (handle, objectIndex, variableIndex, name, typeName, userFlags, dataType, dataValue))
PEX_ABI_HANDLE_RETURN(uint16_t, C_GetPropertyCount, 0,
    (PexReaderHandle handle, uint16_t objectIndex), (handle, objectIndex))
PEX_ABI_HANDLE_RETURN(int32_t, C_GetPropertyInfo, 0,
    (PexReaderHandle handle, uint16_t objectIndex, uint16_t propertyIndex, uint16_t* name,
        uint16_t* type, uint16_t* docString, uint32_t* userFlags, uint8_t* flags, uint16_t* autoVariableName),
    (handle, objectIndex, propertyIndex, name, type, docString, userFlags, flags, autoVariableName))
PEX_ABI_HANDLE_RETURN(uint16_t, C_GetStateCount, 0,
    (PexReaderHandle handle, uint16_t objectIndex), (handle, objectIndex))
PEX_ABI_HANDLE_RETURN(int32_t, C_GetStateInfo, 0,
    (PexReaderHandle handle, uint16_t objectIndex, uint16_t stateIndex, uint16_t* name,
        uint16_t* functionCount),
    (handle, objectIndex, stateIndex, name, functionCount))
PEX_ABI_HANDLE_RETURN(int32_t, C_GetStateFunctionInfo, 0,
    (PexReaderHandle handle, uint16_t objectIndex, uint16_t stateIndex, uint16_t functionIndex,
        uint16_t* functionName, uint16_t* returnType, uint16_t* docString, uint32_t* userFlags,
        uint8_t* flags, uint16_t* parameterCount, uint16_t* localCount, uint16_t* instructionCount),
    (handle, objectIndex, stateIndex, functionIndex, functionName, returnType, docString, userFlags,
        flags, parameterCount, localCount, instructionCount))
PEX_ABI_HANDLE_RETURN(int32_t, C_GetInstructionInfo, 0,
    (PexReaderHandle handle, uint16_t objectIndex, uint16_t stateIndex, uint16_t functionIndex,
        uint16_t instructionIndex, uint8_t* opcode, uint16_t* argumentCount),
    (handle, objectIndex, stateIndex, functionIndex, instructionIndex, opcode, argumentCount))
PEX_ABI_HANDLE_RETURN(int32_t, C_GetInstructionArgument, 0,
    (PexReaderHandle handle, uint16_t objectIndex, uint16_t stateIndex, uint16_t functionIndex,
        uint16_t instructionIndex, uint16_t argumentIndex, uint8_t* type, PexReaderValue* value),
    (handle, objectIndex, stateIndex, functionIndex, instructionIndex, argumentIndex, type, value))
PEX_ABI_HANDLE_RETURN(uint16_t, C_GetFunctionParamCount, 0,
    (PexReaderHandle handle, uint16_t objectIndex, uint16_t stateIndex, uint16_t functionIndex),
    (handle, objectIndex, stateIndex, functionIndex))
PEX_ABI_HANDLE_RETURN(int32_t, C_GetFunctionParamInfo, 0,
    (PexReaderHandle handle, uint16_t objectIndex, uint16_t stateIndex, uint16_t functionIndex,
        uint16_t parameterIndex, uint16_t* name, uint16_t* type),
    (handle, objectIndex, stateIndex, functionIndex, parameterIndex, name, type))
PEX_ABI_HANDLE_RETURN(uint16_t, C_GetFunctionLocalCount, 0,
    (PexReaderHandle handle, uint16_t objectIndex, uint16_t stateIndex, uint16_t functionIndex),
    (handle, objectIndex, stateIndex, functionIndex))
PEX_ABI_HANDLE_RETURN(int32_t, C_GetFunctionLocalInfo, 0,
    (PexReaderHandle handle, uint16_t objectIndex, uint16_t stateIndex, uint16_t functionIndex,
        uint16_t localIndex, uint16_t* name, uint16_t* type),
    (handle, objectIndex, stateIndex, functionIndex, localIndex, name, type))
PEX_ABI_VOID(C_FreeBuffer, (void* buffer), (buffer))

#undef PEX_ABI_RETURN
#undef PEX_ABI_HANDLE_RETURN
#undef PEX_ABI_VOID
#undef PEX_ABI_HANDLE_VOID
