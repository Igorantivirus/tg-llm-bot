#pragma once

#include <cstdint>
#include <optional>

namespace dto
{

struct PromptTokensDetails
{
    std::optional<std::int64_t> cached_tokens;
    std::optional<std::int64_t> audio_tokens;
};

struct CompletionTokensDetails
{
    std::optional<std::int64_t> reasoning_tokens;
    std::optional<std::int64_t> audio_tokens;
    std::optional<std::int64_t> accepted_prediction_tokens;
    std::optional<std::int64_t> rejected_prediction_tokens;
};

struct Usage
{
    std::int64_t prompt_tokens{};
    std::int64_t completion_tokens{};
    std::int64_t total_tokens{};

    std::optional<PromptTokensDetails>     prompt_tokens_details;
    std::optional<CompletionTokensDetails> completion_tokens_details;
};

// ============================================================================
//  Расширения llama.cpp
// ============================================================================

struct Timings
{
    std::optional<std::int64_t> cache_n;

    std::optional<std::int64_t> prompt_n;
    std::optional<double>       prompt_ms;
    std::optional<double>       prompt_per_token_ms;
    std::optional<double>       prompt_per_second;

    std::optional<std::int64_t> predicted_n;
    std::optional<double>       predicted_ms;
    std::optional<double>       predicted_per_token_ms;
    std::optional<double>       predicted_per_second;
};

struct PromptProgress
{
    std::int64_t total{};
    std::int64_t cache{};
    std::int64_t processed{};
    std::int64_t time_ms{};
};

} // namespace dto
