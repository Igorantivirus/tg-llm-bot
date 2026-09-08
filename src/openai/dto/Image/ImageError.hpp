#pragma once

#include <optional>
#include <string>

namespace dto
{
// ---------------------------------------------------------------------------
// Тело ошибки (одинаково для всех эндпоинтов OpenAI)
// HTTP 400/401/403/404/429/5xx
// ---------------------------------------------------------------------------

enum class ImageErrorType
{
    invalid_request_error,
    authentication_error,
    permission_error,
    not_found_error,
    rate_limit_error,
    api_error,
    overloaded_error
};

struct ImageApiErrorBody
{
    std::string                   message;
    std::optional<ImageErrorType> type;
    std::optional<std::string>    param; // имя поля, вызвавшего ошибку, напр. "size"; может быть null
    std::optional<std::string>    code;  // машинный код, напр. "invalid_value"; часто null
};

struct ImageApiErrorResponse
{
    ImageApiErrorBody error; // обёртка: {"error": {...}}
};
} // namespace dto