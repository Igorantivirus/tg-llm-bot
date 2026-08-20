#pragma once

#include <array>
#include <cstddef>

namespace utils
{
template <std::size_t Size>
inline const char *getName(const std::array<const char *, Size> &names, int ev)
{
    const std::uint8_t ind = static_cast<std::uint8_t>(ev);
    if (ind >= names.size())
        return names[names.size() - 1];
    return names[ind];
}

} // namespace utils