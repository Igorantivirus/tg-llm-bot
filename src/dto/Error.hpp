#pragma once

#include <utils/ErrorGenerator.hpp>

namespace dto
{
enum class Error : std::uint8_t
{
    JsonserSerialize = 0,
    JsonserDeserialize,
    NlohmannParsing,
    NlohmannBuild,
    InvalidRequestedType,
    Unknown
};
constexpr std::array<const char *, static_cast<std::size_t>(Error::Unknown) + 1> errorNames = {
    "Serialisation in jsonser",
    "Deserialisation in jsonser",
    "Json parsing",
    "Building json from structure",
    "Invalid requested type in dto::MessageResponseValue",
    "Unknown"};
} // namespace dto

UTILS_GENERATE_ERRORS(dto, 0x6a3f12d7b8e04d42ULL)