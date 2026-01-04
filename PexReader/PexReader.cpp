#include <iostream>
#include "PexHelper.cpp"

int main()
{
    PexData PexReader;
    PexReader.Load("C:\\Users\\52508\\Desktop\\TestPex\\din_Config.pex");

    std::cout << "Press Enter to exit...";
    std::cin.get();
    return 0;
}