#pragma once

#include <optional>
#include <string>

namespace dto
{
// ---------------------------------------------------------------------------
// Тело ошибки (одинаково для всех эндпоинтов OpenAI)
// HTTP 400/401/403/404/429/5xx
// ---------------------------------------------------------------------------
struct ApiErrorBody
{
    std::string                message; // человекочитаемое описание
    std::optional<std::string> type;    // "invalid_request_error" | "authentication_error" | "permission_error" | "not_found_error" | "rate_limit_error" | "api_error" | "overloaded_error"
    std::optional<std::string> param;   // имя поля, вызвавшего ошибку, напр. "size"; может быть null
    std::optional<std::string> code;    // машинный код, напр. "invalid_value"; часто null
};

struct ApiErrorResponse
{
    ApiErrorBody error; // обёртка: {"error": {...}}
};
} // namespace dto