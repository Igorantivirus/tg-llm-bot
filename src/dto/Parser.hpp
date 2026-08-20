#pragma once

#include <expected>
#include <string>

#include <boost/system/detail/error_code.hpp>
#include <nlohmann/json.hpp>

#include <utils/JsonSerialize.hpp>

#include "Error.hpp"

namespace dto
{

template <typename Dto>
std::expected<Dto, boost::system::error_code> deserialize(const std::string_view str)
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
        return std::unexpected(Error::JsonserDeserialize);
    }
    catch (const nlohmann::json::exception &e)
    {
        return std::unexpected(Error::NlohmannBuild);
    }
    catch (...)
    {
        return std::unexpected(Error::Unknown);
    }
}

template <typename Dto>
std::expected<std::string, boost::system::error_code> serialize(const Dto &dto)
{
    try
    {
        return nlohmann::json(dto).dump();
    }
    catch (const utils::jsonser::Error &e)
    {
        return std::unexpected(Error::JsonserSerialize);
    }
    catch (const nlohmann::json::exception &e)
    {
        return std::unexpected(Error::NlohmannParsing);
    }
    catch (...)
    {
        return std::unexpected(Error::Unknown);
    }
}

} // namespace dto