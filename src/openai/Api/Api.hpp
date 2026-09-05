#pragma once

#include <net/HttpClient.hpp>
#include <utils/Parser.hpp>
#include <utils/Types.hpp>

#include <openai/Error.hpp>
#include <openai/api/ApiResponseGenerator.hpp>
#include <openai/dto/ChatCompletions/Request.hpp>
#include <openai/dto/ChatCompletions/Response.hpp>
#include <openai/dto/ModelsResponse.hpp>

namespace openai
{

class Api
{
public:
    Api(asio::any_io_executor ex, const std::size_t tcpConnsCount, std::string host, std::string port, std::string token)
        : http_(ex, tcpConnsCount),
          fullHost_(isNum(port) ? host + ':' + port : host),
          host_(std::move(host)),
          port_(std::move(port)),
          authorization_("Bearer " + token),
          token_(std::move(token))
    {
    }

    utils::AsyncResult<dto::ModelsResponse> models()
    {
        net::BeastRequest req(http::verb::get, "/v1/models", 11);
        initRequestFields(req);

        auto res = co_await http_.request(host_, port_, std::move(req));
        if (!res)
            co_return std::unexpected(res.error());
        auto resp = std::move(res.value());
        if (resp.isStreaming())
            co_return std::unexpected(Error::EmptyModels);

        co_return utils::deserialize<dto::ModelsResponse>(resp.stringBody());
    }

    utils::AsyncResult<ApiResponseGenerator> chatCompletions(dto::ChatCompletionsRequest dto)
    {
        net::BeastRequest req(http::verb::post, "/v1/chat/completions", 11);
        if (auto sdto = utils::serialize(dto); sdto)
            req.body() = sdto.value();
        else
            co_return std::unexpected(sdto.error());
        initRequestFields(req);

        auto res = co_await http_.request(host_, port_, std::move(req));
        if (!res)
            co_return std::unexpected(res.error());
        auto resp = std::move(res.value());
        if (resp.header.result_int() / 100 != 2)
            co_return std::unexpected(Error::FromServer);

        if (!resp.isStreaming()) // Тело сразу есть
        {
            auto resDto = utils::deserialize<dto::ChatCompletionsResponse>(resp.stringBody());
            if (!resDto)
                co_return std::unexpected(resDto.error());
            co_return ApiResponseGenerator(std::move(resDto.value()));
        }
        co_return ApiResponseGenerator{std::move(resp.streamBody())};
    }

private:
    net::HttpClient http_;

    const std::string fullHost_;
    const std::string host_;
    const std::string port_;
    const std::string authorization_;
    const std::string token_;

private:
    constexpr static bool isNum(const std::string &s)
    {
        int res;
        auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), res);
        return ec == std::errc{} && ptr == s.data() + s.size();
    }

private:
    void initRequestFields(net::BeastRequest &req) const
    {
        req.set(http::field::host, fullHost_);
        req.set(http::field::content_type, "application/json");
        req.set(http::field::authorization, authorization_);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.prepare_payload();
    }
};

} // namespace openai