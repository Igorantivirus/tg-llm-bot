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
        nlohmann::json json;
        std::ifstream(fName) >> json;
        return json.get<Config>();
    }
    catch (const std::exception &e)
    {
        return std::unexpected(e.what());
    }
    catch (...)
    {
        return std::unexpected(std::string("Unknown error."));
    }
}

} // namespace config