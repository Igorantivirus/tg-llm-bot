#pragma once

#include "ErrorGenerator.hpp"

namespace utils
{
enum class Error : std::uint8_t
{
    EndOfStreamData,
    TooLongDataSize,
    EncodeError,
    Unknown
};
constexpr std::array<const char *, static_cast<std::size_t>(Error::Unknown) + 1> errorNames = {
    "End of data from stream",
    "Input too large for EVP_EncodeBlock",
    "EVP_EncodeBlock failed",
    "Unknown"};
} // namespace utils

UTILS_GENERATE_ERRORS(utils, 0x6a3f12d7b8e04d40ULL)