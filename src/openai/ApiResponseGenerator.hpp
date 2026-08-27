#pragma once

#include <optional>

#include <dto/ChatCompletionsResponse.hpp>
#include <dto/Parser.hpp>
#include <net/SseParser.hpp>
#include <utils/Types.hpp>

namespace openai
{
class Api;
class ApiResponseGenerator
{
    friend class Api;

private:
    ApiResponseGenerator(net::HttpStream &http)
        : parser_(http)
    {
    }

public:
    bool isValid()
    {
        return parser_.isValid();
    }

    utils::AsyncResult<std::optional<dto::ChatCompletionsResponse>> next()
    {
        if (!isValid())
            co_return std::nullopt;

        auto nextChunk = co_await parser_.next();
        if (!nextChunk)
            co_return std::unexpected(nextChunk.error());
        auto bodyOpt = nextChunk.value();
        if (!bodyOpt)
            co_return std::nullopt;

        std::string body = std::move(bodyOpt.value()); // Полностью следующий фрагмент

        auto parsed = dto::deserialize<dto::ChatCompletionsResponse>(body);
        if (!parsed)
            co_return std::unexpected(parsed.error());

        co_return parsed.value();
    }

private:
    net::SseParser parser_;
};
} // namespace openai