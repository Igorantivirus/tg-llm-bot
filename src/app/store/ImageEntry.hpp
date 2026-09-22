#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include <openai/dto/Image/ImageEnums.hpp>

namespace store
{

/// @brief Картинка в хранилище вместе со всем, что нужно знать о ней позже.
///
/// Формат нужен, чтобы объявить корректный mime при отправке картинки на
/// редактирование, размер — чтобы унаследовать геометрию оригинала, когда
/// модель не задала своё соотношение сторон. И то, и другое известно из
/// источника: images API сообщает их в ответе, Telegram — в PhotoSize.
struct ImageEntry
{
    std::string                data;
    dto::ImageOutputFormat     format = dto::ImageOutputFormat::png;
    std::optional<std::string> size; // "ШИРИНАxВЫСОТА", как его понимает images API

    static std::string makeSize(const std::int32_t width, const std::int32_t height)
    {
        return std::to_string(width) + 'x' + std::to_string(height);
    }
};

} // namespace store
