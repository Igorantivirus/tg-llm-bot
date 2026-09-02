#pragma once

#include <dto/ChatCompletions/Request.hpp>
#include <dto/ChatCompletions/Response.hpp>
#include <dto/ModelsResponse.hpp>
#include <dto/Parser.hpp>
#include <net/HttpStream.hpp>
#include <utils/Types.hpp>

#include "ApiResponseGenerator.hpp"
#include <openai/Error.hpp>

namespace openai
{

class Api
{
public:
    Api(asio::any_io_executor ex, std::string host, std::string port, std::string token)
        : http_(ex),
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
        auto resp = res.value();
        if (!resp.body)
            co_return std::unexpected(Error::EmptyModels);

        co_return dto::deserialize<dto::ModelsResponse>(*resp.body);
    }

    utils::AsyncResult<ApiResponseGenerator> chatCompletions(dto::ChatCompletionsRequest dto)
    {
        net::BeastRequest req(http::verb::post, "/v1/chat/completions", 11);
        if (auto sdto = dto::serialize(dto); sdto)
            req.body() = sdto.value();
        else
            co_return std::unexpected(sdto.error());
        initRequestFields(req);

        auto res = co_await http_.request(host_, port_, std::move(req));
        if (!res)
            co_return std::unexpected(res.error());
        auto resp = res.value();
        if (resp.header.result_int() / 100 != 2)
            co_return std::unexpected(Error::FromServer);

        if (resp.body) // Тело сразу есть
        {
            auto resDto = dto::deserialize<dto::ChatCompletionsResponse>(*resp.body);
            if (!resDto)
                co_return std::unexpected(resDto.error());
            co_return ApiResponseGenerator(std::move(resDto.value()));
        }
        co_return ApiResponseGenerator{http_};
    }

private:
    net::HttpStream http_;

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