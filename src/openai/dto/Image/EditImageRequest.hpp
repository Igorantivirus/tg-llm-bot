#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "ImageEnums.hpp"

namespace dto
{

enum class ImageInputFidelity : std::uint8_t
{
    hight,
    low
};

// ----------------------------------------------------------------------------
// POST /v1/images/edits
//
// В отличие от /v1/images/generations этот эндпоинт принимает только
// multipart/form-data: картинки уходят сырыми байтами отдельными частями тела.
// Поэтому структура не сериализуется через Jsonser — поля раскладывает по
// частям Api::imagesEdit. Запрет на сериализацию обеспечен наличием
// пользовательского конструктора: тип перестаёт быть агрегатом, и Jsonser
// сообщит об этом ошибкой вместо того, чтобы молча собрать неверный JSON.
// ----------------------------------------------------------------------------
/// @brief Картинка для multipart-части: байты вместе с форматом, чтобы
/// объявить корректные mime и filename, не разбирая сигнатуру файла.
struct ImageFile
{
    std::string            data;
    dto::ImageFormat format = dto::ImageFormat::png;
};

struct EditImageRequest
{
    EditImageRequest() noexcept
    {
    }

    std::vector<ImageFile> images; // required; part name "image" (или "image[]" для нескольких)
    std::string            prompt; // required; 1..32000 симв.

    std::optional<ImageFile>          mask;               // маска инпейнтинга; прозрачные области = заменяемые
    std::optional<ImageInputFidelity> input_fidelity;     //
    std::optional<std::string>        model;              //
    std::optional<unsigned short>     n;                  //
    std::optional<std::string>        size;               //
    std::optional<ImageQuality>       quality;            //
    std::optional<ImageBackground>    background;         //
    std::optional<ImageFormat>  output_format;      //
    std::optional<unsigned short>     output_compression; // 0..100; только для "jpeg" | "webp"
    std::optional<ImageModeration>    moderation;         //
    std::optional<bool>               stream;             //
    std::optional<unsigned short>     partial_images;     // 0..3; только при stream == true
    std::optional<std::string>        user;               //
};

} // namespace dto
