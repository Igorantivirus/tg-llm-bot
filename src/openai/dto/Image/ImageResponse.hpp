#pragma once

#include <optional>
#include <string>
#include <vector>

#include "ImageOutputFormat.hpp"
#include "ImageUsage.hpp"

namespace dto
{

struct ImageData
{
    std::optional<std::string> b64_json;       // base64 без префикса "data:"; по умолчанию у GPT-image; у dall-e-* только если response_format == "b64_json"
    std::optional<std::string> url;            // только dall-e-2/dall-e-3 при response_format == "url"; живёт 60 минут; у GPT-image отсутствует
    std::optional<std::string> revised_prompt; // только dall-e-3: переписанный моделью промпт
};

struct ImageResponse
{
    std::int64_t created = 0; // unix timestamp, секунды; единственное поле, на присутствие которого можно рассчитывать

    std::optional<std::vector<ImageData>> data;          // список картинок; длина == n
    std::optional<std::string>            background;    // "transparent" | "opaque"
    std::optional<ImageOutputFormat>      output_format; //
    std::optional<std::string>            quality;       // "low" | "medium" | "high"
    std::optional<std::string>            size;          //
    std::optional<ImageUsage>             usage;         //
};

} // namespace dto