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
    SSELex_API const char* C_GetVersion();
    SSELex_API int C_GetVersionLength();
}

static const std::string Version = "1.0.0";

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
		break;
	}
	return TRUE;
}


int main()
{
    setConsoleToUTF8();

    PexData PexReader;
    PexReader.Load("C:\\Users\\52508\\Desktop\\TestPex\\din_Config.pex");

    std::cout << "Press Enter to exit...";
    std::cin.get();
    return 0;
}