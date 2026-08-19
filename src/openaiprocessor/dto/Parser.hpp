#pragma once

#include "utils/JsonSerialize.hpp"
#include <iostream>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

namespace dto
{

template <typename Dto>
std::optional<Dto> deserialize(const std::string &str)
{
    try
    {
        Dto res;

        nlohmann::json j = nlohmann::json::parse(str);
        j.get_to(res);
        return res;
    }
    catch (const utils::jsonser::Error &e)
    {
        std::cerr << "Error of parse string \"" << str << "\": " << e.what() << '\n';
        return std::nullopt;
    }
    catch (const nlohmann::json::exception &e)
    {
        std::cerr << "Nlohmann error of parse string \"" << str << "\": " << e.what() << '\n';
        return std::nullopt;
    }
    catch (...)
    {
        std::cerr << "Unknown error of parse string \"" << str << "\"\n";
        return std::nullopt;
    }
}

template <typename Dto>
std::optional<std::string> serialize(const Dto &dto)
{
    try
    {
        return nlohmann::json(dto).dump();
    }
    catch (const utils::jsonser::Error &e)
    {
        std::cerr << "Error of serialise: \"" << e.what() << '\n';
        return std::nullopt;
    }
    catch (const nlohmann::json::exception &e)
    {
        std::cerr << "Nlohmann error of serialise" << e.what() << '\n';
        return std::nullopt;
    }
    catch (...)
    {
        std::cerr << "Unknown error of serialise\n";
        return std::nullopt;
    }
}

} // namespace dto