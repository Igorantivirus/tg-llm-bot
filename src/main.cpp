#include <cstdlib>
#include <iostream>

#include <app/Application.hpp>
#include <app/ConfigReader.hpp>

int main(int argc, char **argv)
{
#ifdef _WIN32
    std::system("chcp 65001 > nul");
#endif
    auto res = app::ConfigReader::read(std::span<char *>(argv, argc));
    if (!res)
    {
        std::cerr << "Error: " << res.error() << '\n';
        return EXIT_FAILURE;
    }
    app::Application app(res.value());
    return app.run();
}