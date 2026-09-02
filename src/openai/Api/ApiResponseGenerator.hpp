#pragma once

#include <dto/ChatCompletions/Response.hpp>
#include <dto/Parser.hpp>
#include <net/SseStream.hpp>
#include <utils/StreamGenerator.hpp>
#include <utils/Types.hpp>

namespace openai
{
class Api;

class ApiResponseGenerator : public utils::StreamGenerator<dto::ChatCompletionsResponse>
{
    friend class Api;

private:
    ApiResponseGenerator(net::HttpStream &http)
        : parser_(http),
          mainResult_(std::nullopt)
    {
    }

    ApiResponseGenerator(dto::ChatCompletionsResponse dto)
        : parser_(std::nullopt),
          mainResult_(dto)
    {
    }

private:
    net::SseParser                              parser_;
    std::optional<dto::ChatCompletionsResponse> mainResult_;

private:
    bool isReady() const override
    {
        return !parser_.done() || mainResult_;
    }
    utils::AsyncResult<dto::ChatCompletionsResponse> nextImpl() override
    {
        if (mainResult_)
            co_return takeResult(); // Результат вернётся, а sse stream будет не валидным
        auto nextChunk = co_await parser_.next();
        if (!nextChunk)
            co_return std::unexpected(nextChunk.error());
        std::string body = std::move(nextChunk.value()); // Полностью следующий фрагмент
        auto        parsed = dto::deserialize<dto::ChatCompletionsResponse>(body);
        if (!parsed)
            co_return std::unexpected(parsed.error());
        co_return parsed.value();
    }
    dto::ChatCompletionsResponse takeResult()
    {
        dto::ChatCompletionsResponse res = mainResult_.value();
        mainResult_ = std::nullopt;
        return res;
    }
};
} // namespace openai