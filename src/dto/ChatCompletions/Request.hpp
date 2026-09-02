#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "JsonSchema.hpp"
#include "Message.hpp"
#include "ResponseFormat.hpp"
#include "Tools.hpp"

namespace dto
{

struct StreamOptions
{
    std::optional<bool> include_usage; // Без include_usage: true в СТРИМИНГЕ USAGE НЕ ПРИХОДИТ ВООБЩЕ
    std::optional<bool> include_obfuscation;
};

struct LoraEntry
{
    std::int64_t id{};
    double       scale{};
};

struct AudioOutput
{
    std::string voice;  ///< "alloy", "echo", ...
    std::string format; ///< "wav" | "mp3" | "flac" | "opus" | "pcm16"
};

struct ChatCompletionsRequest
{
    // ========================================================================
    //  Основные параметры
    // ========================================================================

    std::string          model;
    std::vector<Message> messages;

    // ========================================================================
    //  OpenAI: сэмплирование и длина
    // ========================================================================

    /// Дефолт у OpenAI — 1.0; у llama.cpp — 0.8. Диапазон [0; 2].
    std::optional<double> temperature;

    /// Дефолт у OpenAI — 1.0; у llama.cpp — 0.95.
    std::optional<double> top_p;

    /// max_tokens помечен deprecated. Новые reasoning-модели требуют
    /// именно max_completion_tokens; llama.cpp понимает оба (мапит в n_predict).
    std::optional<std::int64_t> max_completion_tokens;
    std::optional<std::int64_t> max_tokens; ///< deprecated, для старых серверов

    /// Число вариантов ответа. В стриме даёт чанки с разными choices[].index —
    /// не смешивай варианты в одну строку при сборке.
    std::optional<std::int64_t> n;

    std::optional<double>       frequency_penalty; ///< default 0.0
    std::optional<double>       presence_penalty;  ///< default 0.0
    std::optional<std::int64_t> seed;

    /// stop — union: строка либо массив (у OpenAI до 4 элементов).
    std::optional<std::variant<std::string, std::vector<std::string>>> stop;

    // ========================================================================
    //  OpenAI: стриминг
    // ========================================================================

    std::optional<bool>          stream;
    std::optional<StreamOptions> stream_options;

    // ========================================================================
    //  OpenAI: инструменты и формат
    // ========================================================================

    /// tools отправляются при КАЖДОМ запросе. Это не состояние сессии,
    /// сервер их не запоминает: забудешь на втором круге — модель не сможет
    /// вызвать функцию снова.
    std::optional<std::vector<Tool>> tools;
    std::optional<ToolChoice>        tool_choice;
    std::optional<bool>              parallel_tool_calls;

    std::optional<ResponseFormat> response_format;

    // ========================================================================
    //  OpenAI: logprobs и прочее
    // ========================================================================

    std::optional<bool>         logprobs;
    std::optional<std::int64_t> top_logprobs;

    /// Объектная форма {"15043": 1.0}. llama.cpp нативно ждёт массив пар
    /// [[15043, 1.0]], но объект принимает ради совместимости — так что
    /// объектная форма работает у обоих.
    std::optional<std::unordered_map<std::string, double>> logit_bias;

    std::optional<std::string>              user;
    std::optional<std::string>              reasoning_effort; ///< "minimal"|"low"|"medium"|"high"; none у llama.cpp отключает thinking
    std::optional<std::vector<std::string>> modalities;       ///< ["text"] | ["text","audio"]
    std::optional<AudioOutput>              audio;
    std::optional<bool>                     store;
    std::optional<std::string>              service_tier;
    // std::optional<RawJson>                  metadata;

    // ========================================================================
    //  llama.cpp: сэмплеры
    // ========================================================================
    //
    //  Ни одного из этих полей нет в спецификации OpenAI. Настоящий
    //  api.openai.com ответит на них 400 либо молча проигнорирует.
    //  Гаси их при переключении на OpenAI (см. stripNonOpenAi() в Validate.hpp).
    //
    // ========================================================================

    std::optional<std::int64_t> top_k;       ///< default 40, 0 = disabled
    std::optional<double>       min_p;       ///< default 0.05, 0.0 = disabled
    std::optional<double>       top_n_sigma; ///< default -1.0 = disabled
    std::optional<double>       typical_p;   ///< default 1.0 = disabled

    std::optional<double>       repeat_penalty; ///< default 1.1
    std::optional<std::int64_t> repeat_last_n;  ///< default 64, 0 = off, -1 = ctx

    std::optional<double> xtc_probability; ///< default 0.0 = disabled
    std::optional<double> xtc_threshold;   ///< default 0.1 (>0.5 отключает XTC)

    std::optional<double> dynatemp_range;    ///< default 0.0 = disabled
    std::optional<double> dynatemp_exponent; ///< default 1.0

    std::optional<double>                   dry_multiplier;     ///< default 0.0 = disabled
    std::optional<double>                   dry_base;           ///< default 1.75
    std::optional<std::int64_t>             dry_allowed_length; ///< default 2
    std::optional<std::int64_t>             dry_penalty_last_n; ///< default -1
    std::optional<std::vector<std::string>> dry_sequence_breakers;

    /// 0 = disabled, 1 = Mirostat, 2 = Mirostat 2.0.
    /// При включении top_k / top_p / typical_p игнорируются.
    std::optional<std::int64_t> mirostat;
    std::optional<double>       mirostat_tau; ///< default 5.0
    std::optional<double>       mirostat_eta; ///< default 0.1

    /// Порядок применения сэмплеров.
    std::optional<std::vector<std::string>> samplers;
    std::optional<std::int64_t>             min_keep;

    // ========================================================================
    //  llama.cpp: ограничение генерации
    // ========================================================================

    /// GBNF-грамматика.
    std::optional<std::string> grammar;

    /// Альтернатива response_format.
    ///
    /// ВНИМАНИЕ: одновременно с grammar (или вместе с response_format) сервер
    /// отвечает «Either json_schema or grammar can be specified, but not both».
    /// validate() ловит это до отправки.
    std::optional<schema::Schema> json_schema;

    // ======================================================================== //
    //  llama.cpp: управление слотами и кэшем                                   //
    // ======================================================================== //

    std::optional<bool>         cache_prompt; ///< default true
    std::optional<std::int64_t> n_keep;
    std::optional<std::int64_t> n_cache_reuse;
    std::optional<bool>         ignore_eos;
    std::optional<std::int64_t> id_slot;          ///< default -1 = любой свободный
    std::optional<std::int64_t> t_max_predict_ms; ///< 0 = disabled
    std::optional<std::int64_t> n_probs;
    std::optional<bool>         post_sampling_probs;
    std::optional<bool>         return_tokens;
    std::optional<bool>         timings_per_token;
    std::optional<bool>         return_progress;
    std::optional<std::int64_t> sse_ping_interval; ///< -1 отключает пинги

    std::optional<std::vector<LoraEntry>>   lora;
    std::optional<std::vector<std::string>> response_fields;

    // ========================================================================
    //  llama.cpp: chat-специфика
    // ========================================================================

    /// Произвольные параметры jinja-шаблону, напр. {"enable_thinking": false}.
    // std::optional<RawJson> chat_template_kwargs;

    /// "none" | "deepseek" | "deepseek-legacy".
    /// none оставляет мысли неразобранными прямо в message.content;
    /// deepseek выносит их в message.reasoning_content.
    std::optional<std::string> reasoning_format;

    // ========================================================================

    /// Поля, которых нет в структуре: расширения конкретного бэкенда,
    /// экспериментальные флаги. Вливаются в корень объекта при сериализации.
    // std::optional<RawJson> extra;
};

} // namespace dto
