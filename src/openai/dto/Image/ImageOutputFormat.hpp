#pragma once

#include <cstdint>

namespace dto
{
enum class ImageOutputFormat : std::uint8_t
{
    png,
    jpeg,
    webp
};
}