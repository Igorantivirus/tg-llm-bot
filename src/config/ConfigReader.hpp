#pragma once

#include <exception>
#include <expected>
#include <fstream>
#include <string>

#include <nlohmann/json.hpp>

namespace config
{

template <class Config>
std::expected<Config, std::string> read(const char *fName)
{
    try
    {
        std::ifstream in(fName);
        if (!in.is_open())
            return std::unexpected("File not found.");
        nlohmann::json json;
        in >> json;
        in.close();
        return json.get<Config>();
    }
    catch (const std::exception &e)
    {
        return std::unexpected(e.what());
    }
    catch (...)
    {
        return std::unexpected("Unknown error.");
    }
}

} // namespace config