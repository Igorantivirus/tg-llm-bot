#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "Choice.hpp"
#include "Usage.hpp"

namespace dto
{

struct ChatCompletionsResponse
{
    std::vector<Choice>        choices;
    std::int64_t               created{}; // Unix time
    std::string                id;
    std::string                model;
    std::string                object; // "chat.completion" (полный ответ) | "chat.completion.chunk" (чанк стрима).
    std::optional<std::string> system_fingerprint;
    std::optional<Usage>       usage;
    std::optional<Timings>     timings; // Расширение llama.cpp

    // std::optional<RawJson> unknown_fields;
};

} // namespace dto
