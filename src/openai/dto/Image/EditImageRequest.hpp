#pragma once

#include <optional>
#include <string>
#include <vector>

#include "ImageOutputFormat.hpp"

namespace dto
{
// ---------------------------------------------------------------------------
// Ссылка на входное изображение (для images[] и mask)
// Ровно одно из двух полей должно быть задано.
// ---------------------------------------------------------------------------
struct ImageReference
{
    std::optional<std::string> file_id;   // id файла, загруженного через File API ("file-...")
    std::optional<std::string> image_url; // полный https URL ИЛИ data-URL: "data:image/png;base64,iVBOR..."; макс. длина строки 20971520
};

// ---------------------------------------------------------------------------
// POST /v1/images/edits   (JSON-вариант; есть также multipart с image[]=@file)
// ---------------------------------------------------------------------------
struct EditImageRequest
{
    std::vector<ImageReference> images; // required; 1..16 для GPT-image
    std::string                 prompt; // required; 1..32000 симв.

    std::optional<ImageReference>    mask;               // маска инпейнтинга; прозрачные области = заменяемые
    std::optional<std::string>       input_fidelity;     // "high" | "low"; насколько точно держаться исходника
    std::optional<std::string>       model;              //
    std::optional<unsigned short>    n;                  // 1..10
    std::optional<std::string>       size;               //
    std::optional<std::string>       quality;            // "auto" | "high" | "medium" | "low"
    std::optional<std::string>       background;         // "transparent" | "opaque" | "auto"
    std::optional<ImageOutputFormat> output_format;      // "png" | "jpeg" | "webp"
    std::optional<unsigned short>    output_compression; // 0..100; только для "jpeg" | "webp"
    std::optional<std::string>       moderation;         // "low" | "auto"
    std::optional<bool>              stream;             //
    std::optional<unsigned short>    partial_images;     // 0..3; только при stream == true
    std::optional<std::string>       user;               // id конечного пользователя
};

} // namespace dto