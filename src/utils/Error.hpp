#pragma once

#include "ErrorGenerator.hpp"

namespace utils
{
enum class Error : std::uint8_t
{
    EndOfStreamData,
    SizeNotDevidedBy4,
    TooLongDataSize,
    EncodeError,
    DecodeError,
    JsonserSerialize,
    JsonserDeserialize,
    NlohmannParsing,
    NlohmannBuild,
    Unknown
};
constexpr std::array<const char *, static_cast<std::size_t>(Error::Unknown) + 1> errorNames = {
    "End of data from stream",
    "Base 64 string size must be devided by 4",
    "Input too large for EVP_EncodeBlock",
    "EVP_EncodeBlock failed",
    "EVP_DecodeBlock failed",
    "Serialisation in jsonser",
    "Deserialisation in jsonser",
    "Json parsing",
    "Building json from structure",
    "Unknown"};
} // namespace utils

UTILS_GENERATE_ERRORS(utils, 0x6a3f12d7b8e04d40ULL)