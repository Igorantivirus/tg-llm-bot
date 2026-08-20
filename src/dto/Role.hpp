#pragma once

#include <cstdint>

#include <utils/JsonSerialize.hpp>

namespace dto
{
enum class Role : std::uint8_t
{
    system,
    assistant,
    user
};
JSONSER_SIMPLE_SERIALIZATION_OUTLINE_WITH_DEFAULTS(Role);
} // namespace dto