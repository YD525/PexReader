#ifndef PEX_READER_API_H
#define PEX_READER_API_H

#include <stdint.h>
#include <wchar.h>

#if defined(_WIN32)
#if defined(PEX_READER_EXPORTS)
#define PEX_READER_API __declspec(dllexport)
#else
#define PEX_READER_API __declspec(dllimport)
#endif
#define PEX_READER_CALL __cdecl
#else
#define PEX_READER_API
#define PEX_READER_CALL
#endif

#if defined(__cplusplus)
#define PEX_READER_NOEXCEPT noexcept
extern "C" {
#else
#define PEX_READER_NOEXCEPT
#endif

/*
 * This value identifies the additive native contract described by this
 * header. It is independent from the product version returned by C_GetVersion.
 */
#define PEX_READER_ABI_VERSION 1u

/* Handles are pointer-sized opaque values created and destroyed by this DLL. */
typedef intptr_t PexReaderHandle;

/* Status values cross the ABI as signed 32-bit integers. */
typedef int32_t PexReaderStatus;
enum
{
    PEX_READER_STATUS_OK = 0,
    PEX_READER_STATUS_INVALID_ARGUMENT = 1,
    PEX_READER_STATUS_OUT_OF_RANGE = 2,
    PEX_READER_STATUS_BUFFER_TOO_SMALL = 3,
    PEX_READER_STATUS_IO_ERROR = 4,
    PEX_READER_STATUS_PARSE_ERROR = 5,
    PEX_READER_STATUS_OUT_OF_MEMORY = 6,
    PEX_READER_STATUS_INTERNAL_ERROR = 7
};

/*
 * Variable and instruction argument payloads use one of these representations,
 * selected by the separately returned PEX data type. The union is four bytes
 * with four-byte packing on supported Windows targets.
 */
#pragma pack(push, 4)
typedef union PexReaderValue
{
    uint16_t StringTableIndex;
    int32_t Integer;
    float Real;
    uint8_t Boolean;
} PexReaderValue;
#pragma pack(pop)

/*
 * Every entry point uses cdecl and catches C++ exceptions. C_ReadPex and
 * C_SavePex accept null-terminated Windows UTF-16 paths. C_ModifyStringTable
 * accepts null-terminated UTF-8. C_GetStringUtf8 measures byte capacity and
 * C_GetStringWide measures wchar_t capacity; both return the payload length
 * excluding the null terminator. A null buffer with zero capacity queries the
 * required length, and a writable buffer requires capacity greater than that
 * length.
 *
 * A handle is owned by the caller and must be destroyed exactly once with
 * C_DestroyInstance. C_Close resets its loaded data without ending its
 * lifetime. Header string pointers and the product version pointer are borrowed
 * and remain valid until the next call to the same function on the calling
 * thread. C_GetDebugFunctionInfo can allocate a line-number array; ownership is
 * transferred to the caller and the exact pointer must be released by
 * C_FreeBuffer in this module. Other output pointers are caller-owned.
 *
 * Required handles, paths, strings, and writable payload pointers must be
 * non-null. Metadata output pointers are optional unless ownership or sizing
 * requires a pair, as with lineNumbers and lineCount. C_DestroyInstance,
 * C_Close, and C_FreeBuffer accept null as a no-op.
 *
 * Status and error text are thread-local and describe the most recent ABI call
 * other than C_GetAbiVersion, C_GetLastStatus, or C_GetLastErrorUtf8 on the
 * calling thread. Error text is UTF-8 and remains valid until the next such
 * call. C_GetLastErrorUtf8 uses byte capacities and follows the same
 * length-query convention as other text buffers.
 */

PEX_READER_API uint32_t PEX_READER_CALL C_GetAbiVersion(void) PEX_READER_NOEXCEPT;
PEX_READER_API PexReaderStatus PEX_READER_CALL C_GetLastStatus(void) PEX_READER_NOEXCEPT;
PEX_READER_API int32_t PEX_READER_CALL C_GetLastErrorUtf8(
    uint8_t* buffer,
    int32_t bufferSize) PEX_READER_NOEXCEPT;

PEX_READER_API const char* PEX_READER_CALL C_GetVersion(void) PEX_READER_NOEXCEPT;
PEX_READER_API int32_t PEX_READER_CALL C_GetVersionLength(void) PEX_READER_NOEXCEPT;

PEX_READER_API PexReaderHandle PEX_READER_CALL C_CreateInstance(void) PEX_READER_NOEXCEPT;
PEX_READER_API void PEX_READER_CALL C_DestroyInstance(PexReaderHandle handle) PEX_READER_NOEXCEPT;
PEX_READER_API int32_t PEX_READER_CALL C_ReadPex(
    PexReaderHandle handle,
    const wchar_t* pexPath) PEX_READER_NOEXCEPT;
PEX_READER_API int32_t PEX_READER_CALL C_ModifyStringTable(
    PexReaderHandle handle,
    uint16_t index,
    const char* utf8String) PEX_READER_NOEXCEPT;
PEX_READER_API int32_t PEX_READER_CALL C_SavePex(
    PexReaderHandle handle,
    const wchar_t* pexPath) PEX_READER_NOEXCEPT;
PEX_READER_API void PEX_READER_CALL C_Close(PexReaderHandle handle) PEX_READER_NOEXCEPT;

PEX_READER_API const wchar_t* PEX_READER_CALL C_GetHeaderSourceFileName(
    PexReaderHandle handle) PEX_READER_NOEXCEPT;
PEX_READER_API const wchar_t* PEX_READER_CALL C_GetHeaderUsername(
    PexReaderHandle handle) PEX_READER_NOEXCEPT;
PEX_READER_API const wchar_t* PEX_READER_CALL C_GetHeaderMachineName(
    PexReaderHandle handle) PEX_READER_NOEXCEPT;
PEX_READER_API uint32_t PEX_READER_CALL C_GetHeaderMagic(PexReaderHandle handle) PEX_READER_NOEXCEPT;
PEX_READER_API uint8_t PEX_READER_CALL C_GetHeaderMajorVersion(PexReaderHandle handle) PEX_READER_NOEXCEPT;
PEX_READER_API uint8_t PEX_READER_CALL C_GetHeaderMinorVersion(PexReaderHandle handle) PEX_READER_NOEXCEPT;
PEX_READER_API uint16_t PEX_READER_CALL C_GetHeaderGameId(PexReaderHandle handle) PEX_READER_NOEXCEPT;
PEX_READER_API uint64_t PEX_READER_CALL C_GetHeaderCompilationTime(
    PexReaderHandle handle) PEX_READER_NOEXCEPT;

PEX_READER_API uint16_t PEX_READER_CALL C_GetStringTableCount(PexReaderHandle handle) PEX_READER_NOEXCEPT;
PEX_READER_API int32_t PEX_READER_CALL C_GetStringUtf8(
    PexReaderHandle handle,
    uint16_t index,
    char* buffer,
    int32_t bufferSize) PEX_READER_NOEXCEPT;
PEX_READER_API int32_t PEX_READER_CALL C_GetStringWide(
    PexReaderHandle handle,
    uint16_t index,
    wchar_t* buffer,
    int32_t bufferSize) PEX_READER_NOEXCEPT;

PEX_READER_API uint8_t PEX_READER_CALL C_HasDebugInfo(PexReaderHandle handle) PEX_READER_NOEXCEPT;
PEX_READER_API uint64_t PEX_READER_CALL C_GetDebugModificationTime(
    PexReaderHandle handle) PEX_READER_NOEXCEPT;
PEX_READER_API uint16_t PEX_READER_CALL C_GetDebugFunctionCount(
    PexReaderHandle handle) PEX_READER_NOEXCEPT;
PEX_READER_API int32_t PEX_READER_CALL C_GetDebugFunctionInfo(
    PexReaderHandle handle,
    uint16_t index,
    uint16_t* objectNameIndex,
    uint16_t* stateNameIndex,
    uint16_t* functionNameIndex,
    uint8_t* functionType,
    uint16_t** lineNumbers,
    int32_t* lineCount) PEX_READER_NOEXCEPT;

PEX_READER_API uint16_t PEX_READER_CALL C_GetUserFlagCount(PexReaderHandle handle) PEX_READER_NOEXCEPT;
PEX_READER_API int32_t PEX_READER_CALL C_GetUserFlagInfo(
    PexReaderHandle handle,
    uint16_t index,
    uint16_t* flagNameIndex,
    uint8_t* flagIndex) PEX_READER_NOEXCEPT;

PEX_READER_API uint16_t PEX_READER_CALL C_GetObjectCount(PexReaderHandle handle) PEX_READER_NOEXCEPT;
PEX_READER_API int32_t PEX_READER_CALL C_GetObjectInfo(
    PexReaderHandle handle,
    uint16_t index,
    uint16_t* nameIndex,
    uint32_t* size) PEX_READER_NOEXCEPT;
PEX_READER_API int32_t PEX_READER_CALL C_GetObjectData(
    PexReaderHandle handle,
    uint16_t objectIndex,
    uint16_t* parentClassName,
    uint16_t* docString,
    uint32_t* userFlags,
    uint16_t* autoStateName) PEX_READER_NOEXCEPT;

PEX_READER_API uint16_t PEX_READER_CALL C_GetVariableCount(
    PexReaderHandle handle,
    uint16_t objectIndex) PEX_READER_NOEXCEPT;
PEX_READER_API int32_t PEX_READER_CALL C_GetVariableInfo(
    PexReaderHandle handle,
    uint16_t objectIndex,
    uint16_t variableIndex,
    uint16_t* name,
    uint16_t* typeName,
    uint32_t* userFlags,
    uint8_t* dataType,
    PexReaderValue* dataValue) PEX_READER_NOEXCEPT;

PEX_READER_API uint16_t PEX_READER_CALL C_GetPropertyCount(
    PexReaderHandle handle,
    uint16_t objectIndex) PEX_READER_NOEXCEPT;
PEX_READER_API int32_t PEX_READER_CALL C_GetPropertyInfo(
    PexReaderHandle handle,
    uint16_t objectIndex,
    uint16_t propertyIndex,
    uint16_t* name,
    uint16_t* type,
    uint16_t* docString,
    uint32_t* userFlags,
    uint8_t* flags,
    uint16_t* autoVariableName) PEX_READER_NOEXCEPT;

PEX_READER_API uint16_t PEX_READER_CALL C_GetStateCount(
    PexReaderHandle handle,
    uint16_t objectIndex) PEX_READER_NOEXCEPT;
PEX_READER_API int32_t PEX_READER_CALL C_GetStateInfo(
    PexReaderHandle handle,
    uint16_t objectIndex,
    uint16_t stateIndex,
    uint16_t* name,
    uint16_t* functionCount) PEX_READER_NOEXCEPT;
PEX_READER_API int32_t PEX_READER_CALL C_GetStateFunctionInfo(
    PexReaderHandle handle,
    uint16_t objectIndex,
    uint16_t stateIndex,
    uint16_t functionIndex,
    uint16_t* functionName,
    uint16_t* returnType,
    uint16_t* docString,
    uint32_t* userFlags,
    uint8_t* flags,
    uint16_t* parameterCount,
    uint16_t* localCount,
    uint16_t* instructionCount) PEX_READER_NOEXCEPT;

PEX_READER_API int32_t PEX_READER_CALL C_GetInstructionInfo(
    PexReaderHandle handle,
    uint16_t objectIndex,
    uint16_t stateIndex,
    uint16_t functionIndex,
    uint16_t instructionIndex,
    uint8_t* opcode,
    uint16_t* argumentCount) PEX_READER_NOEXCEPT;
PEX_READER_API int32_t PEX_READER_CALL C_GetInstructionArgument(
    PexReaderHandle handle,
    uint16_t objectIndex,
    uint16_t stateIndex,
    uint16_t functionIndex,
    uint16_t instructionIndex,
    uint16_t argumentIndex,
    uint8_t* type,
    PexReaderValue* value) PEX_READER_NOEXCEPT;

PEX_READER_API uint16_t PEX_READER_CALL C_GetFunctionParamCount(
    PexReaderHandle handle,
    uint16_t objectIndex,
    uint16_t stateIndex,
    uint16_t functionIndex) PEX_READER_NOEXCEPT;
PEX_READER_API int32_t PEX_READER_CALL C_GetFunctionParamInfo(
    PexReaderHandle handle,
    uint16_t objectIndex,
    uint16_t stateIndex,
    uint16_t functionIndex,
    uint16_t parameterIndex,
    uint16_t* name,
    uint16_t* type) PEX_READER_NOEXCEPT;
PEX_READER_API uint16_t PEX_READER_CALL C_GetFunctionLocalCount(
    PexReaderHandle handle,
    uint16_t objectIndex,
    uint16_t stateIndex,
    uint16_t functionIndex) PEX_READER_NOEXCEPT;
PEX_READER_API int32_t PEX_READER_CALL C_GetFunctionLocalInfo(
    PexReaderHandle handle,
    uint16_t objectIndex,
    uint16_t stateIndex,
    uint16_t functionIndex,
    uint16_t localIndex,
    uint16_t* name,
    uint16_t* type) PEX_READER_NOEXCEPT;

PEX_READER_API void PEX_READER_CALL C_FreeBuffer(void* buffer) PEX_READER_NOEXCEPT;

#if defined(__cplusplus)
}
#endif

#undef PEX_READER_NOEXCEPT

#endif
