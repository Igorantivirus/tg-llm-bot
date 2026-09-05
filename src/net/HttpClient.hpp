#pragma once

#include <memory>
#include <utils/Types.hpp>

#include "HttpResponse.hpp"
#include "TcpPool.hpp"

namespace net
{

class HttpClient
{
public:
    HttpClient(asio::any_io_executor ex, const std::size_t count)
        : pool_(std::make_shared<TcpPool>(ex, count))
    {
    }

    template <typename F = decltype(nullptr)>
    utils::AsyncResult<HttpResponse> request(const std::string_view host, const std::string_view port, BeastRequest req, HttpSettings setts = {}, F socketConfigurator = nullptr)
    {
        auto conn = pool_->pullConnection(pool_);
        if (!conn.valid())
            co_return std::unexpected(Error::NoFreeConnections);
        conn.buffer().max_size(setts.bufferSize);
        conn.parser()->body_limit(boost::none);
        conn.parser()->header_limit(setts.size.header);

        // connect
        if (auto resolveRes = co_await resolve(conn.stream().get_executor(), host, port, setts.timeout.resolve); resolveRes)
        {
            conn.setTimeOut(setts.timeout.connect);
            [[maybe_unused]] const auto [err, connectionRes] = co_await conn.stream().async_connect(resolveRes.value(), asio::as_tuple(asio::use_awaitable));
            if (err)
                co_return std::unexpected(Error::Connect);
            if constexpr (!std::is_null_pointer_v<F>) // Продвинутая настройка, если надо
            {
                static_assert(std::is_invocable_v<F, tcp::socket &>);
                socketConfigurator(conn.stream().socket());
            }
        }
        else
            co_return std::unexpected(Error::Resolve);
        // write request
        conn.setTimeOut(setts.timeout.request);
        [[maybe_unused]] const auto [errWrite, bytesWrite] = co_await http::async_write(conn.stream(), std::move(req), asio::as_tuple(asio::use_awaitable));
        if (errWrite)
            co_return std::unexpected(Error::WriteRequest);
        // read headers
        conn.setTimeOut(setts.timeout.header);
        [[maybe_unused]] const auto [errRead, bytesReads] = co_await http::async_read_header(conn.stream(), conn.buffer(), *conn.parser(), asio::as_tuple(asio::use_awaitable));
        if (errRead)
            co_return std::unexpected(Error::Busy);

        if (!conn.parser()->chunked()) // Чтение сразу и возврат
            co_return co_await readResult(conn, setts);

        co_return HttpResponse{
            .header = conn.parser()->get().base(), .body = HttpStreamReader{std::move(conn), std::move(setts)}
        }; // Дальше чтение чанково
    }

private:
    std::shared_ptr<TcpPool> pool_;

private:
    utils::AsyncResult<HttpResponse> readResult(TcpConnection &conn, const HttpSettings &setts)
    {
        http::response_parser<http::string_body> parser(std::move(*conn.parser()));
        parser.body_limit(setts.size.body);
        conn.setTimeOut(setts.timeout.body);
        [[maybe_unused]] const auto [errRead, bytesRead] = co_await http::async_read(conn.stream(), conn.buffer(), parser, asio::as_tuple(asio::use_awaitable));
        if (errRead)
            co_return std::unexpected(Error::ReadBody);
        auto res = parser.release();
        co_return HttpResponse{.header = std::move(res.base()), .body = std::move(res.body())};
    }

private:
    static utils::AsyncResult<asio::ip::basic_resolver_results<tcp>> resolve(asio::any_io_executor ex, const std::string_view host, const std::string_view port, const std::chrono::steady_clock::duration timeout)
    {
        tcp::resolver resolver(ex);
        const auto [err, resolve] = co_await resolver.async_resolve(host, port, asio::cancel_after(timeout, asio::as_tuple(asio::use_awaitable)));
        if (err)
            co_return std::unexpected(err);
        co_return resolve;
    }
};
} // namespace net