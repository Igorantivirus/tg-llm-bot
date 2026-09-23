#pragma once

#include "magic_enum/magic_enum.hpp"
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

enum class ImageFormat : std::uint8_t
{
    png,
    jpeg,
    webp,
    gif,
    tiff,
    bmp
};

inline std::string_view toStringFormat(const ImageFormat format)
{
    return magic_enum::enum_name(format);
}

inline std::string toStringMimeFormat(const ImageFormat format)
{
    std::string res;
    res.reserve(12);
    res = "image/";
    res += magic_enum::enum_name(format);
    return res;
}
inline std::optional<ImageFormat> toFormat(std::string_view sv)
{
    if(!sv.starts_with("image/"))
        return std::nullopt;
    return magic_enum::enum_cast<ImageFormat>(sv);
}

} // namespace dto