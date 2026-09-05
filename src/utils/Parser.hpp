#pragma once

#include <exception>
#include <iostream>

#include <nlohmann/json.hpp>

#include <utils/Jsonser.hpp>
#include <utils/Types.hpp>

#include "Error.hpp"

namespace utils
{

template <typename Dto>
utils::SyncResult<Dto> deserialize(const std::string_view str)
{
    try
    {
        Dto            res;
        nlohmann::json j = nlohmann::json::parse(str);
        jsonser::Deserialize::fromJson(j, res);
        return res;
    }
    catch (const nlohmann::json::exception &e)
    {
        std::cerr << "Deserialize error: " << e.what() << " with string: " << str << '\n';
        return std::unexpected(Error::NlohmannBuild);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Deserialize error: " << e.what() << " with string: " << str << '\n';
        return std::unexpected(Error::JsonserDeserialize);
    }
    catch (...)
    {
        std::cerr << "Deserialize unknown error with string: " << str << '\n';
        return std::unexpected(Error::Unknown);
    }
}

template <typename Dto>
utils::SyncResult<std::string> serialize(const Dto &dto)
{
    try
    {
        nlohmann::json j;
        jsonser::Serialize::toJson(j, dto);
        return j.dump();
    }
    catch (const nlohmann::json::exception &e)
    {
        std::cerr << "Serialize error: " << e.what() << " with type: " << typeid(dto).name() << '\n';
        return std::unexpected(Error::NlohmannParsing);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Serialize error: " << e.what() << " with type: " << typeid(dto).name() << '\n';
        return std::unexpected(Error::JsonserSerialize);
    }
    catch (...)
    {
        std::cerr << "Serialize unknown error with type: " << typeid(dto).name() << '\n';
        return std::unexpected(Error::Unknown);
    }
}

} // namespace dto