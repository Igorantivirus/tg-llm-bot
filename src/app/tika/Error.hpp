#pragma once

#include <utils/ErrorGenerator.hpp>

namespace tika
{
enum class Error : std::uint8_t
{
    TikaError,
    ZipOpen,
    Unknown
};
constexpr const std::array<const char *, static_cast<std::size_t>(Error::Unknown) + 1> errorNames = {
    "Tika return not 200 code",
    "Error of open zip file",
    "Unknown"};
} // namespace openai

UTILS_GENERATE_ERRORS(tika, 0x6a3f12d7b8e04d47ULL);