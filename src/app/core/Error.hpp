#pragma once

#include <utils/ErrorGenerator.hpp>

namespace core
{
enum class Error : std::uint8_t
{
    ModelOutOfRange,
    Unknown
};
constexpr std::array<const char *, static_cast<std::size_t>(Error::Unknown) + 1> errorNames = {
    "Model out of models list",
    "Unknown"};
} // namespace core

UTILS_GENERATE_ERRORS(core, 0x6a3f12d7b8e04d45ULL)