#pragma once

#include <optional>

namespace dto
{
struct ImageTokensDetails
{
    unsigned short image_tokens = 0;
    unsigned short text_tokens = 0;
};

struct ImageUsage
{
    unsigned short                    input_tokens = 0;      // >= 0
    unsigned short                    output_tokens = 0;     // >= 0
    unsigned short                    total_tokens = 0;      // >= 0
    ImageTokensDetails                input_tokens_details;  // {image_tokens, text_tokens}
    std::optional<ImageTokensDetails> output_tokens_details; // {image_tokens, text_tokens}
};
} // namespace dto