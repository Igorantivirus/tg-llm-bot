#pragma once

#include <utils/ErrorGenerator.hpp>

namespace openai
{
enum class Error : std::uint8_t
{
    EmptyModels,
    InvalidResponse,
    NoSetModel,
    EmptyResponse,
    FromServer,
    ModerationStop,
    LengthLimit,
    ToolError,
    LegacyFinishReason,
    Unknown
};
constexpr const std::array<const char *, static_cast<std::size_t>(Error::Unknown) + 1> errorNames = {
    "Empty body in models response",
    "Invalid response from server",
    "The chat model is not installed",
    "Empty response from server",
    "Server return not 2xx return code",
    "The response generation has been stopped by moderation",
    "The communication generation limit has been exceeded",
    "Internal tool error",
    "Legacy finish reason was polled",
    "Unknown"};
} // namespace openai

UTILS_GENERATE_ERRORS(openai, 0x6a3f12d7b8e04d42ULL);