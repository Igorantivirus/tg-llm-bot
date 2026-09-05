#pragma once

#include <cstdint>

namespace dto
{

enum class Role : std::uint8_t
{
    system,
    developer, // Замена system
    user,
    assistant,
    tool,
    function, // Deprecated с 2023 г.
};

} // namespace dto
