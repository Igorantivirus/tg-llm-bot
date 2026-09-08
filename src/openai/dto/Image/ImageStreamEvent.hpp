#pragma once

#include <optional>
#include <string>

#include "ImageUsage.hpp"

namespace dto
{
// ---------------------------------------------------------------------------
// SSE-события при stream == true
// ---------------------------------------------------------------------------
struct ImageStreamEvent
{
    std::string                type;                // "image_generation.partial_image" | "image_generation.completed" | "image_edit.partial_image" | "image_edit.completed"
    std::optional<std::string> b64_json;            // кадр или финальная картинка
    std::optional<int>         partial_image_index; // 0..2; только у *.partial_image
    std::optional<ImageUsage>  usage;               // только у *.completed
};
} // namespace dto