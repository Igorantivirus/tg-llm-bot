#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include <utils/Jsonser.hpp>

#include "ImageEnums.hpp"

namespace dto
{

enum class ImageResponseFormat : std::uint8_t
{
    url,
    b64_json
};
enum class ImageStyle : std::uint8_t
{
    vivid,
    natural
};

// ---------------------------------------------------------------------------
// POST /v1/images/generations   (Content-Type: application/json)
// ---------------------------------------------------------------------------
struct GenerateImageRequest
{
    std::string prompt; // required; <=32000 симв.

    std::optional<std::string>         model;
    std::optional<unsigned short>      n;
    std::optional<std::string>         size;
    std::optional<ImageQuality>        quality;
    std::optional<ImageBackground>     background;
    std::optional<ImageOutputFormat>   output_format;
    std::optional<unsigned>            output_compression; // 0..100; только GPT-image с output_format "jpeg" или "webp"; default 100
    std::optional<ImageResponseFormat> response_format;
    std::optional<ImageStyle>          style;
    std::optional<std::string>         moderation;
    std::optional<bool>                stream;
    std::optional<unsigned short>      partial_images; // 0..3; имеет смысл только при stream == true
    std::optional<std::string>         user;
};

} // namespace dto
