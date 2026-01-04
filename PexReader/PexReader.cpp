#include <iostream>
#include "PexHelper.cpp"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void setConsoleToUTF8() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
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