#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "ImageEnums.hpp"

namespace dto
{

struct ImageReference
{
    std::optional<std::string> file_id;   // id файла, загруженного через File API ("file-...")
    std::optional<std::string> image_url; // полный https URL ИЛИ data-URL: "data:image/png;base64,iVBOR..."; макс. длина строки 20971520
};

enum class ImageInputFidelity : std::uint8_t
{
    hight,
    low
};

// ----------------------------------------------------------------------------
// POST /v1/images/edits   (JSON-вариант; есть также multipart с image[]=@file)
// ----------------------------------------------------------------------------
struct EditImageRequest
{
    std::vector<ImageReference> images; // required; 1..16 для GPT-image
    std::string                 prompt; // required; 1..32000 симв.

    std::optional<ImageReference>     mask;               // маска инпейнтинга; прозрачные области = заменяемые
    std::optional<ImageInputFidelity> input_fidelity;     //
    std::optional<std::string>        model;              //
    std::optional<unsigned short>     n;                  //
    std::optional<std::string>        size;               //
    std::optional<ImageQuality>       quality;            //
    std::optional<ImageBackground>    background;         //
    std::optional<ImageOutputFormat>  output_format;      //
    std::optional<unsigned short>     output_compression; // 0..100; только для "jpeg" | "webp"
    std::optional<ImageModeration>    moderation;         //
    std::optional<bool>               stream;             //
    std::optional<unsigned short>     partial_images;     // 0..3; только при stream == true
    std::optional<std::string>        user;               //
};

} // namespace dto