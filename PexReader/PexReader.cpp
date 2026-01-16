#include <iostream>
#include "PexHelper.cpp"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifdef SSELexApi_EXPORTS
#define SSELex_API __declspec(dllexport)
#else
#define SSELex_API __declspec(dllimport)
#endif

extern "C"
{
    // Version info
    SSELex_API const char* C_GetVersion();
    SSELex_API int C_GetVersionLength();

    // PEX operations
    SSELex_API int C_ReadPex(const wchar_t* PexPath);
    SSELex_API int C_SavePex(const wchar_t* PexPath);
    SSELex_API void C_Close();

    // Header info
    SSELex_API const wchar_t* C_GetHeaderSourceFileName();
    SSELex_API const wchar_t* C_GetHeaderUsername();
    SSELex_API const wchar_t* C_GetHeaderMachineName();
    SSELex_API uint32_t C_GetHeaderMagic();
    SSELex_API uint8_t C_GetHeaderMajorVersion();
    SSELex_API uint8_t C_GetHeaderMinorVersion();
    SSELex_API uint16_t C_GetHeaderGameId();
    SSELex_API uint64_t C_GetHeaderCompilationTime();

    // String table
    SSELex_API uint16_t C_GetStringTableCount();
    SSELex_API int C_GetStringUtf8(uint16_t index, char* buffer, int bufferSize);
    SSELex_API int C_GetStringWide(uint16_t index, wchar_t* buffer, int bufferSize);

    // Debug info
    SSELex_API uint8_t C_HasDebugInfo();
    SSELex_API uint64_t C_GetDebugModificationTime();
    SSELex_API uint16_t C_GetDebugFunctionCount();
    SSELex_API int C_GetDebugFunctionInfo(uint16_t index, uint16_t* objectNameIndex,
        uint16_t* stateNameIndex, uint16_t* functionNameIndex,
        uint8_t* functionType, uint16_t** lineNumbers, int* lineCount);

    // User flags
    SSELex_API uint16_t C_GetUserFlagCount();
    SSELex_API int C_GetUserFlagInfo(uint16_t index, uint16_t* flagNameIndex, uint8_t* flagIndex);

    // Objects
    SSELex_API uint16_t C_GetObjectCount();
    SSELex_API int C_GetObjectInfo(uint16_t index, uint16_t* nameIndex, uint32_t* size);
    SSELex_API int C_GetObjectData(uint16_t objectIndex, uint16_t* parentClassName,
        uint16_t* docString, uint32_t* userFlags,
        uint16_t* autoStateName);

    // Variables
    SSELex_API uint16_t C_GetVariableCount(uint16_t objectIndex);
    SSELex_API int C_GetVariableInfo(uint16_t objectIndex, uint16_t varIndex,
        uint16_t* name, uint16_t* typeName,
        uint32_t* userFlags, uint8_t* dataType,
        void* dataValue);

    // Properties
    SSELex_API uint16_t C_GetPropertyCount(uint16_t objectIndex);
    SSELex_API int C_GetPropertyInfo(uint16_t objectIndex, uint16_t propIndex,
        uint16_t* name, uint16_t* type,
        uint16_t* docstring, uint32_t* userFlags,
        uint8_t* flags, uint16_t* autoVarName);

    // States
    SSELex_API uint16_t C_GetStateCount(uint16_t objectIndex);
    SSELex_API int C_GetStateInfo(uint16_t objectIndex, uint16_t stateIndex,
        uint16_t* name, uint16_t* numFunctions);

    // Functions
    SSELex_API int C_GetStateFunctionInfo(uint16_t objectIndex, uint16_t stateIndex,
        uint16_t funcIndex, uint16_t* functionName,
        uint16_t* returnType, uint16_t* docString,
        uint32_t* userFlags, uint8_t* flags,
        uint16_t* numParams, uint16_t* numLocals,
        uint16_t* numInstructions);

    // Instructions
    SSELex_API int C_GetInstructionInfo(uint16_t objectIndex, uint16_t stateIndex,
        uint16_t funcIndex, uint16_t instrIndex,
        uint8_t* opcode, uint16_t* argCount);

    SSELex_API int C_GetInstructionArgument(uint16_t objectIndex, uint16_t stateIndex,
        uint16_t funcIndex, uint16_t instrIndex,
        uint16_t argIndex, uint8_t* type,
        void* value);

    SSELex_API int C_GetFunctionParamInfo(uint16_t objectIndex, uint16_t stateIndex,
        uint16_t funcIndex, uint16_t paramIndex,
        uint16_t* name, uint16_t* type);

    SSELex_API uint16_t C_GetFunctionParamCount(uint16_t objectIndex, uint16_t stateIndex,
        uint16_t funcIndex);

    // Function locals
    SSELex_API int C_GetFunctionLocalInfo(uint16_t objectIndex, uint16_t stateIndex,
        uint16_t funcIndex, uint16_t localIndex,
        uint16_t* name, uint16_t* type);

    SSELex_API uint16_t C_GetFunctionLocalCount(uint16_t objectIndex, uint16_t stateIndex,
        uint16_t funcIndex);

    // Memory management
    SSELex_API void C_FreeBuffer(void* buffer);
}

static const std::string Version = "1.0.1";
static PexData* PexDataInstance = nullptr;
static std::wstring LastSetPath;

// Helper function to convert wstring to UTF-8 for C# interop
static std::string WStringToUTF8(const std::wstring& wstr)
{
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(),
        NULL, 0, NULL, NULL);
    std::string result(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(),
        &result[0], size_needed, NULL, NULL);
    return result;
}

// Helper function to convert UTF-8 to wstring
static std::wstring UTF8ToWString(const std::string& str)
{
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(),
        NULL, 0);
    std::wstring result(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(),
        &result[0], size_needed);
    return result;
}

const char* C_GetVersion()
{
    return Version.c_str();
}

int C_GetVersionLength()
{
    return static_cast<int>(Version.length());
}

void setConsoleToUTF8() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}

void Clear()
{
    delete PexDataInstance;
    PexDataInstance = nullptr;
    LastSetPath.clear();
}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        Clear();
        break;
    }
    return TRUE;
}

int C_ReadPex(const wchar_t* PexPath)
{
    Clear();

    try {
        PexDataInstance = new PexData();
        PexDataInstance->Load(PexPath);
        LastSetPath = PexPath;
        return 1; // Success
    }
    catch (const std::exception& e) {
        std::cerr << "Error loading PEX: " << e.what() << std::endl;
        Clear();
        return 0; // Failure
    }
}

int C_SavePex(const wchar_t* PexPath)
{
    if (!PexDataInstance) {
        return 0;
    }

    try {
        PexDataInstance->Save(PexPath);
        return 1;
    }
    catch (const std::exception& e) {
        std::cerr << "Error saving PEX: " << e.what() << std::endl;
        return 0;
    }
}

void C_Close()
{
    Clear();
}

// Header info functions
const wchar_t* C_GetHeaderSourceFileName()
{
    static std::wstring buffer;
    if (!PexDataInstance) return L"";
    buffer = PexDataInstance->Header.sourceFileName;
    return buffer.c_str();
}

const wchar_t* C_GetHeaderUsername()
{
    static std::wstring buffer;
    if (!PexDataInstance) return L"";
    buffer = PexDataInstance->Header.username;
    return buffer.c_str();
}

const wchar_t* C_GetHeaderMachineName()
{
    static std::wstring buffer;
    if (!PexDataInstance) return L"";
    buffer = PexDataInstance->Header.machinename;
    return buffer.c_str();
}

uint32_t C_GetHeaderMagic()
{
    return PexDataInstance ? PexDataInstance->Header.magic : 0;
}

uint8_t C_GetHeaderMajorVersion()
{
    return PexDataInstance ? PexDataInstance->Header.majorVersion : 0;
}

uint8_t C_GetHeaderMinorVersion()
{
    return PexDataInstance ? PexDataInstance->Header.minorVersion : 0;
}

uint16_t C_GetHeaderGameId()
{
    return PexDataInstance ? PexDataInstance->Header.gameId : 0;
}

uint64_t C_GetHeaderCompilationTime()
{
    return PexDataInstance ? PexDataInstance->Header.compilationTime : 0;
}

// String table functions
uint16_t C_GetStringTableCount()
{
    return PexDataInstance ? PexDataInstance->stringTable.count : 0;
}

int C_GetStringUtf8(uint16_t index, char* buffer, int bufferSize)
{
    if (!PexDataInstance || index >= PexDataInstance->stringTable.count) {
        return -1;
    }

    try {
        std::string str = PexDataInstance->stringTable.ToUtf8(index);
        int length = static_cast<int>(str.length());

        if (buffer && bufferSize > length) {
            std::memcpy(buffer, str.c_str(), length + 1);
        }

        return length;
    }
    catch (...) {
        return -1;
    }
}

int C_GetStringWide(uint16_t index, wchar_t* buffer, int bufferSize)
{
    if (!PexDataInstance || index >= PexDataInstance->stringTable.count) {
        return -1;
    }

    try {
        std::string utf8Str = PexDataInstance->stringTable.ToUtf8(index);
        std::wstring wideStr = UTF8ToWString(utf8Str);
        int length = static_cast<int>(wideStr.length());

        if (buffer && bufferSize > length) {
            std::memcpy(buffer, wideStr.c_str(), (length + 1) * sizeof(wchar_t));
        }

        return length;
    }
    catch (...) {
        return -1;
    }
}

// Debug info functions
uint8_t C_HasDebugInfo()
{
    return PexDataInstance ? PexDataInstance->debugInfo.hasDebugInfo : 0;
}

uint64_t C_GetDebugModificationTime()
{
    return PexDataInstance ? PexDataInstance->debugInfo.modificationTime : 0;
}

uint16_t C_GetDebugFunctionCount()
{
    return PexDataInstance ? PexDataInstance->debugInfo.functionCount : 0;
}

int C_GetDebugFunctionInfo(uint16_t index, uint16_t* objectNameIndex,
    uint16_t* stateNameIndex, uint16_t* functionNameIndex,
    uint8_t* functionType, uint16_t** lineNumbers, int* lineCount)
{
    if (!PexDataInstance || index >= PexDataInstance->debugInfo.functionCount) {
        return 0;
    }

    const auto& func = PexDataInstance->debugInfo.functions[index];

    if (objectNameIndex) *objectNameIndex = func.objectNameIndex;
    if (stateNameIndex) *stateNameIndex = func.stateNameIndex;
    if (functionNameIndex) *functionNameIndex = func.functionNameIndex;
    if (functionType) *functionType = func.functionType;

    if (lineNumbers && lineCount) {
        *lineCount = static_cast<int>(func.lineNumbers.size());
        *lineNumbers = new uint16_t[func.lineNumbers.size()];
        std::copy(func.lineNumbers.begin(), func.lineNumbers.end(), *lineNumbers);
    }

    return 1;
}

// User flags functions
uint16_t C_GetUserFlagCount()
{
    return PexDataInstance ? PexDataInstance->userFlagCount : 0;
}

int C_GetUserFlagInfo(uint16_t index, uint16_t* flagNameIndex, uint8_t* flagIndex)
{
    if (!PexDataInstance || index >= PexDataInstance->userFlagCount) {
        return 0;
    }

    const auto& flag = PexDataInstance->userFlags[index];

    if (flagNameIndex) *flagNameIndex = flag.flagNameIndex;
    if (flagIndex) *flagIndex = flag.flagIndex;

    return 1;
}

// Objects functions
uint16_t C_GetObjectCount()
{
    return PexDataInstance ? PexDataInstance->objectCount : 0;
}

int C_GetObjectInfo(uint16_t index, uint16_t* nameIndex, uint32_t* size)
{
    if (!PexDataInstance || index >= PexDataInstance->objectCount) {
        return 0;
    }

    const auto& obj = PexDataInstance->objects[index];

    if (nameIndex) *nameIndex = obj.nameIndex;
    if (size) *size = obj.size;

    return 1;
}

int C_GetObjectData(uint16_t objectIndex, uint16_t* parentClassName,
    uint16_t* docString, uint32_t* userFlags,
    uint16_t* autoStateName)
{
    if (!PexDataInstance || objectIndex >= PexDataInstance->objectCount) {
        return 0;
    }

    const auto& data = PexDataInstance->objects[objectIndex].data;

    if (parentClassName) *parentClassName = data.parentClassName;
    if (docString) *docString = data.docString;
    if (userFlags) *userFlags = data.userFlags;
    if (autoStateName) *autoStateName = data.autoStateName;

    return 1;
}

// Variables functions
uint16_t C_GetVariableCount(uint16_t objectIndex)
{
    if (!PexDataInstance || objectIndex >= PexDataInstance->objectCount) {
        return 0;
    }

    return PexDataInstance->objects[objectIndex].data.numVariables;
}

int C_GetVariableInfo(uint16_t objectIndex, uint16_t varIndex,
    uint16_t* name, uint16_t* typeName,
    uint32_t* userFlags, uint8_t* dataType,
    void* dataValue)
{
    if (!PexDataInstance || objectIndex >= PexDataInstance->objectCount) {
        return 0;
    }

    const auto& obj = PexDataInstance->objects[objectIndex];
    if (varIndex >= obj.data.numVariables) {
        return 0;
    }

    const auto& var = obj.data.variables[varIndex];

    if (name) *name = var.name;
    if (typeName) *typeName = var.typeName;
    if (userFlags) *userFlags = var.userFlags;
    if (dataType) *dataType = var.data.type;

    if (dataValue && dataType) {
        // Copy data value based on type
        switch (var.data.type) {
        case 0: // null
            // Nothing to copy
            break;
        case 1: // identifier
        case 2: // string
            *reinterpret_cast<uint16_t*>(dataValue) = std::get<uint16_t>(var.data.data);
            break;
        case 3: // integer
            *reinterpret_cast<int32_t*>(dataValue) = std::get<int32_t>(var.data.data);
            break;
        case 4: // float
            *reinterpret_cast<float*>(dataValue) = std::get<float>(var.data.data);
            break;
        case 5: // bool
            *reinterpret_cast<uint8_t*>(dataValue) = std::get<uint8_t>(var.data.data);
            break;
        }
    }

    return 1;
}

// Properties functions
uint16_t C_GetPropertyCount(uint16_t objectIndex)
{
    if (!PexDataInstance || objectIndex >= PexDataInstance->objectCount) {
        return 0;
    }

    return PexDataInstance->objects[objectIndex].data.numProperties;
}

int C_GetPropertyInfo(uint16_t objectIndex, uint16_t propIndex,
    uint16_t* name, uint16_t* type,
    uint16_t* docstring, uint32_t* userFlags,
    uint8_t* flags, uint16_t* autoVarName)
{
    if (!PexDataInstance || objectIndex >= PexDataInstance->objectCount) {
        return 0;
    }

    const auto& obj = PexDataInstance->objects[objectIndex];
    if (propIndex >= obj.data.numProperties) {
        return 0;
    }

    const auto& prop = obj.data.properties[propIndex];

    if (name) *name = prop.name;
    if (type) *type = prop.type;
    if (docstring) *docstring = prop.docstring;
    if (userFlags) *userFlags = prop.userFlags;
    if (flags) *flags = prop.flags;
    if (autoVarName) *autoVarName = prop.autoVarName;

    return 1;
}

// States functions
uint16_t C_GetStateCount(uint16_t objectIndex)
{
    if (!PexDataInstance || objectIndex >= PexDataInstance->objectCount) {
        return 0;
    }

    return PexDataInstance->objects[objectIndex].data.numStates;
}

int C_GetStateInfo(uint16_t objectIndex, uint16_t stateIndex,
    uint16_t* name, uint16_t* numFunctions)
{
    if (!PexDataInstance || objectIndex >= PexDataInstance->objectCount) {
        return 0;
    }

    const auto& obj = PexDataInstance->objects[objectIndex];
    if (stateIndex >= obj.data.numStates) {
        return 0;
    }

    const auto& state = obj.data.states[stateIndex];

    if (name) *name = state.name;
    if (numFunctions) *numFunctions = state.numFunctions;

    return 1;
}

// Functions functions
int C_GetStateFunctionInfo(uint16_t objectIndex, uint16_t stateIndex,
    uint16_t funcIndex, uint16_t* functionName,
    uint16_t* returnType, uint16_t* docString,
    uint32_t* userFlags, uint8_t* flags,
    uint16_t* numParams, uint16_t* numLocals,
    uint16_t* numInstructions)
{
    if (!PexDataInstance || objectIndex >= PexDataInstance->objectCount) {
        return 0;
    }

    const auto& obj = PexDataInstance->objects[objectIndex];
    if (stateIndex >= obj.data.numStates) {
        return 0;
    }

    const auto& state = obj.data.states[stateIndex];
    if (funcIndex >= state.numFunctions) {
        return 0;
    }

    const auto& namedFunc = state.functions[funcIndex];
    const auto& func = namedFunc.function;

    if (functionName) *functionName = namedFunc.functionName;
    if (returnType) *returnType = func.returnType;
    if (docString) *docString = func.docString;
    if (userFlags) *userFlags = func.userFlags;
    if (flags) *flags = func.flags;
    if (numParams) *numParams = func.numParams;
    if (numLocals) *numLocals = func.numLocals;
    if (numInstructions) *numInstructions = func.numInstructions;

    return 1;
}

// Instructions functions
int C_GetInstructionInfo(uint16_t objectIndex, uint16_t stateIndex,
    uint16_t funcIndex, uint16_t instrIndex,
    uint8_t* opcode, uint16_t* argCount)
{
    if (!PexDataInstance || objectIndex >= PexDataInstance->objectCount) {
        return 0;
    }

    const auto& obj = PexDataInstance->objects[objectIndex];
    if (stateIndex >= obj.data.numStates) {
        return 0;
    }

    const auto& state = obj.data.states[stateIndex];
    if (funcIndex >= state.numFunctions) {
        return 0;
    }

    const auto& namedFunc = state.functions[funcIndex];
    const auto& func = namedFunc.function;

    if (instrIndex >= func.numInstructions) {
        return 0;
    }

    const auto& instr = func.instructions[instrIndex];

    if (opcode) *opcode = static_cast<uint8_t>(instr.op);
    if (argCount) *argCount = static_cast<uint16_t>(instr.arguments.size());

    return 1;
}

int C_GetInstructionArgument(uint16_t objectIndex, uint16_t stateIndex,
    uint16_t funcIndex, uint16_t instrIndex,
    uint16_t argIndex, uint8_t* type,
    void* value)
{
    if (!PexDataInstance || objectIndex >= PexDataInstance->objectCount) {
        return 0;
    }

    const auto& obj = PexDataInstance->objects[objectIndex];
    if (stateIndex >= obj.data.numStates) {
        return 0;
    }

    const auto& state = obj.data.states[stateIndex];
    if (funcIndex >= state.numFunctions) {
        return 0;
    }

    const auto& namedFunc = state.functions[funcIndex];
    const auto& func = namedFunc.function;

    if (instrIndex >= func.numInstructions) {
        return 0;
    }

    const auto& instr = func.instructions[instrIndex];
    if (argIndex >= instr.arguments.size()) {
        return 0;
    }

    const auto& arg = instr.arguments[argIndex];

    if (type) *type = arg.type;

    if (value) {
        // Copy data value based on type
        switch (arg.type) {
        case 0: // null
            // Nothing to copy
            break;
        case 1: // identifier
        case 2: // string
            *reinterpret_cast<uint16_t*>(value) = std::get<uint16_t>(arg.data);
            break;
        case 3: // integer
            *reinterpret_cast<int32_t*>(value) = std::get<int32_t>(arg.data);
            break;
        case 4: // float
            *reinterpret_cast<float*>(value) = std::get<float>(arg.data);
            break;
        case 5: // bool
            *reinterpret_cast<uint8_t*>(value) = std::get<uint8_t>(arg.data);
            break;
        }
    }

    return 1;
}

// Function parameters functions
uint16_t C_GetFunctionParamCount(uint16_t objectIndex, uint16_t stateIndex,
    uint16_t funcIndex)
{
    if (!PexDataInstance || objectIndex >= PexDataInstance->objectCount) {
        return 0;
    }

    const auto& obj = PexDataInstance->objects[objectIndex];
    if (stateIndex >= obj.data.numStates) {
        return 0;
    }

    const auto& state = obj.data.states[stateIndex];
    if (funcIndex >= state.numFunctions) {
        return 0;
    }

    const auto& namedFunc = state.functions[funcIndex];
    return namedFunc.function.numParams;
}

int C_GetFunctionParamInfo(uint16_t objectIndex, uint16_t stateIndex,
    uint16_t funcIndex, uint16_t paramIndex,
    uint16_t* name, uint16_t* type)
{
    if (!PexDataInstance || objectIndex >= PexDataInstance->objectCount) {
        return 0;
    }

    const auto& obj = PexDataInstance->objects[objectIndex];
    if (stateIndex >= obj.data.numStates) {
        return 0;
    }

    const auto& state = obj.data.states[stateIndex];
    if (funcIndex >= state.numFunctions) {
        return 0;
    }

    const auto& namedFunc = state.functions[funcIndex];
    const auto& func = namedFunc.function;

    if (paramIndex >= func.numParams) {
        return 0;
    }

    const auto& param = func.params[paramIndex];

    if (name) *name = param.name;
    if (type) *type = param.type;

    return 1;
}

// Function locals functions
uint16_t C_GetFunctionLocalCount(uint16_t objectIndex, uint16_t stateIndex,
    uint16_t funcIndex)
{
    if (!PexDataInstance || objectIndex >= PexDataInstance->objectCount) {
        return 0;
    }

    const auto& obj = PexDataInstance->objects[objectIndex];
    if (stateIndex >= obj.data.numStates) {
        return 0;
    }

    const auto& state = obj.data.states[stateIndex];
    if (funcIndex >= state.numFunctions) {
        return 0;
    }

    const auto& namedFunc = state.functions[funcIndex];
    return namedFunc.function.numLocals;
}

int C_GetFunctionLocalInfo(uint16_t objectIndex, uint16_t stateIndex,
    uint16_t funcIndex, uint16_t localIndex,
    uint16_t* name, uint16_t* type)
{
    if (!PexDataInstance || objectIndex >= PexDataInstance->objectCount) {
        return 0;
    }

    const auto& obj = PexDataInstance->objects[objectIndex];
    if (stateIndex >= obj.data.numStates) {
        return 0;
    }

    const auto& state = obj.data.states[stateIndex];
    if (funcIndex >= state.numFunctions) {
        return 0;
    }

    const auto& namedFunc = state.functions[funcIndex];
    const auto& func = namedFunc.function;

    if (localIndex >= func.numLocals) {
        return 0;
    }

    const auto& local = func.locals[localIndex];

    if (name) *name = local.name;
    if (type) *type = local.type;

    return 1;
}

// Memory management
void C_FreeBuffer(void* buffer)
{
    if (buffer) {
        delete[] reinterpret_cast<uint8_t*>(buffer);
    }
}

// Main function for testing
int main()
{
    setConsoleToUTF8();

    PexData PexReader;
    PexReader.Load(TEXT("C:\\Users\\52508\\Desktop\\TestPex\\din_Config.pex"));

    std::cout << "Press Enter to exit...";
    std::cin.get();
    return 0;
}