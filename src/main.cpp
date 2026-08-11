#include <cstdlib>
#include <iostream>

int main()
{
#ifdef _WIN32
    std:system("chcp 65001 > nul");
#endif


    std::cout << "Привет, Мир!\n";

    return 0;
}