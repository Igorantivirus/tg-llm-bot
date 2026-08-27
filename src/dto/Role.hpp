#pragma once

#include <cstdint>

namespace dto
{
enum class Role : std::uint8_t
{
    system,
    assistant,
    user
};
} // namespace dto