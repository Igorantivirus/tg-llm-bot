#pragma once

#include <dto/ChatCompletionsResponse.hpp>
#include <dto/Parser.hpp>
#include <net/HttpStream.hpp>

#include "Error.hpp"

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
    // ApiResponseGenerator(const ApiResponseGenerator &) = delete;
    // ApiResponseGenerator(ApiResponseGenerator &) = default;
    // ApiResponseGenerator &operator=(const ApiResponseGenerator &) = delete;
    // ApiResponseGenerator &operator=(ApiResponseGenerator &&) = delete;
    ~ApiResponseGenerator()
    {
        close();
    }

    bool isValid()
    {
        return httpPtr_;
    }

    asio::awaitable<std::expected<dto::ChatCompletionsResponse, boost::system::error_code>> next()
    {
        if (!isValid())
            co_return std::unexpected(Error::Success);

        auto nextChunk = co_await httpPtr_->nextChunk();
        if (!nextChunk)
        {
            auto err = nextChunk.error();
            if (err == net::Error::Success)
                co_return std::unexpected(Error::Success);
            co_return std::unexpected(err);
        }

        std::string body = std::move(nextChunk.value());

        if (!body.starts_with("data: "))
        {
            close();
            co_return std::unexpected(Error::InvalidResponse);
        }
        if (body == "data: [DONE]")
        {
            close();
            co_return std::unexpected(Error::Success);
        }

        co_return dto::deserialize<dto::ChatCompletionsResponse>(std::string_view(body.begin() + 6, body.end()));
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