#pragma once

#include <cstdint>

#include <utils/Jsonser.hpp>

namespace dto
{
enum class ImageQuality : std::uint8_t
{
    auto_,
    hight,
    medium,
    low,
    hd,
    standart
};
JSONSER_ENUM(ImageQuality, auto_, "auto");

enum class ImageBackground : std::uint8_t
{
    transparent,
    opaque,
    auto_
};
JSONSER_ENUM(ImageBackground, auto_, "auto");

enum class ImageModeration : std::uint8_t
{
    auto_,
    low
};
JSONSER_ENUM(ImageModeration, auto_, "auto");

enum class ImageOutputFormat : std::uint8_t
{
    png,
    jpeg,
    webp
};
} // namespace dto