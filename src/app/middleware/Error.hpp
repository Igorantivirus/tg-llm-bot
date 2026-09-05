#pragma once

#include <utils/ErrorGenerator.hpp>

namespace middleware
{
enum class Error : std::uint8_t
{
    TgApi = 0,
    TgSystem,
    TgUnknownStd,
    Unknown
};
constexpr std::array<const char *, static_cast<std::size_t>(Error::Unknown) + 1> errorNames = {
    "Tg Api return some exception",
    "System exception in tg api",
    "Unknown std exception in tg api",
    "Unknown"};
} // namespace middleware

UTILS_GENERATE_ERRORS(middleware, 0x6a3f12d7b8e04d44ULL)