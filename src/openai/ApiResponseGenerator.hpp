#pragma once

#include <dto/ChatCompletionsResponse.hpp>
#include <dto/Parser.hpp>
#include <net/HttpStream.hpp>

#include "Error.hpp"
#include "utils/Types.hpp"

namespace openai
{
class Api;
class ApiResponseGenerator
{
    friend class Api;

private:
    ApiResponseGenerator(net::HttpStream &http)
        : httpPtr_(&http)
    {
    }

public:
    ~ApiResponseGenerator()
    {
        close();
    }

    bool isValid()
    {
        return httpPtr_;
    }

    utils::AsyncResult<std::optional<dto::ChatCompletionsResponse>> next()
    {
        if (!isValid())
            co_return std::nullopt;

        auto nextChunk = co_await httpPtr_->nextChunk();
        if (!nextChunk)
            co_return std::unexpected(nextChunk.error());
        auto bodyOpt = nextChunk.value();
        if (!bodyOpt)
            co_return std::nullopt;

        std::string body = std::move(bodyOpt.value());

        if (!body.starts_with("data: "))
        {
            close();
            co_return std::unexpected(Error::InvalidResponse);
        }
        if (body == "data: [DONE]")
        {
            close();
            co_return std::nullopt;
        }

        auto parsed = dto::deserialize<dto::ChatCompletionsResponse>(std::string_view(body.begin() + 6, body.end()));
        if (!parsed)
            co_return std::unexpected(parsed.error());

        co_return parsed.value();
    }

private:
    net::HttpStream *httpPtr_;

private:
    void close()
    {
        httpPtr_ = nullptr;
    }
};
} // namespace openai