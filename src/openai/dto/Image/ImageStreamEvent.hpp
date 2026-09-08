#pragma once

#include <optional>
#include <string>

#include "ImageUsage.hpp"
#include "utils/Jsonser.hpp"

namespace dto
{

enum class ImageStreamEventType
{
    image_generation_partial_image,
    image_generation_completed,
    image_edit_partial_image,
    image_edit_completed
};
JSONSER_ENUM(ImageStreamEventType, image_generation_partial_image, "image_generation.partial_image")
JSONSER_ENUM(ImageStreamEventType, image_generation_completed, "image_generation.completed")
JSONSER_ENUM(ImageStreamEventType, image_edit_partial_image, "image_edit.partial_image")
JSONSER_ENUM(ImageStreamEventType, image_edit_completed, "image_edit.completed")

// ---------------------------------------------------------------------------
// SSE-события при stream == true
// ---------------------------------------------------------------------------
struct ImageStreamEvent
{
    ImageStreamEventType       type;
    std::optional<std::string> b64_json;            // кадр или финальная картинка
    std::optional<int>         partial_image_index; // 0..2; только у *.partial_image
    std::optional<ImageUsage>  usage;               // только у *.completed
};
} // namespace dto