#pragma once

#include <boost/system/detail/error_code.hpp>
#include <expected>
#include <optional>

#include <utils/MethodBinder.hpp>
#include <utils/Types.hpp>

#include "BusyGuard.hpp"
#include "Error.hpp"
#include "HttpSettings.hpp"
#include "Types.hpp"

namespace net
{

class HttpStream
{
public:
    struct Response
    {
        http::response_header<>    header;
        std::optional<std::string> body; // nullopt - chuncked, else - body (maybe empty)
    };

public:
#pragma region initialize + destruct

    HttpStream(asio::any_io_executor ex, HttpSettings setts = {})
        : stream_(std::move(ex)),
          setts_(std::move(setts)),
          chunkCb_(utils::buildMethod(&HttpStream::chunkCb, this)),
          headerCb_(utils::buildMethod(&HttpStream::headerCb, this))
    {
    }
    ~HttpStream()
    {
        stream_.close();
        buffer_.clear();
        if (parser_)
            parser_.reset();
    }
    HttpStream(const HttpStream &) = delete;
    HttpStream(HttpStream &&) = delete;
    HttpStream &operator=(const HttpStream &) = delete;
    HttpStream &operator=(HttpStream &&) = delete;

#pragma endregion

#pragma region sync

    bool setSettings(HttpSettings setts)
    {
        if (busy_)
            return false;
        setts_ = std::move(setts);
        return true;
    }

    void close()
    {
        if (busy_)
            stream_.close();
        else
            fullReset();
    }

#pragma endregion

#pragma region async

    template <typename F = decltype(nullptr)>
    utils::AsyncResult<Response> request(const std::string_view host, const std::string_view port, BeastRequest req, F socketConfigurator = nullptr)
    {
        if (busy_)
            co_return std::unexpected(Error::Busy);
        details::BusyGuard bg(busy_);
        fullReset();

        // connect
        if (auto resolveRes = co_await resolve(stream_.get_executor(), host, port, setts_.timeout.resolve); resolveRes)
        {
            setTimeOut(setts_.timeout.connect);
            [[maybe_unused]] const auto [err, connectionRes] = co_await stream_.async_connect(resolveRes.value(), asio::as_tuple(asio::use_awaitable));
            if (err)
                co_return std::unexpected(Error::Connect);
            if constexpr (!std::is_null_pointer_v<F>) // Продвинутая настройка, если надо
            {
                static_assert(std::is_invocable_v<F, tcp::socket &>);
                socketConfigurator(stream_.socket());
            }
        }
        else
            co_return std::unexpected(Error::Resolve);
        // write request
        setTimeOut(setts_.timeout.request);
        [[maybe_unused]] const auto [errWrite, bytesWrite] = co_await http::async_write(stream_, std::move(req), asio::as_tuple(asio::use_awaitable));
        if (errWrite)
            co_return std::unexpected(Error::WriteRequest);
        // read headers
        setTimeOut(setts_.timeout.header);
        [[maybe_unused]] const auto [errRead, bytesReads] = co_await http::async_read_header(stream_, buffer_, *parser_, asio::as_tuple(asio::use_awaitable));
        if (errRead)
            co_return std::unexpected(Error::Busy);

        if (!parser_->chunked()) // Чтение сразу и возврат
            co_return co_await readResult();

        reading_ = true;
        co_return Response{.header = parser_->get().base(), .body = std::nullopt}; // Дальше чтение чанково
    }

    utils::AsyncResult<std::optional<std::string>> nextChunk()
    {
        if (busy_)
            co_return std::unexpected(Error::Busy);
        details::BusyGuard bg(busy_);
        while (reading_)
        {
            setTimeOut(setts_.timeout.chunk);
            [[maybe_unused]] const auto [err, bytesRead] = co_await http::async_read_some(stream_, buffer_, *parser_, asio::as_tuple(asio::use_awaitable));
            if (err == http::error::end_of_chunk)
                co_return std::move(chunk_);
            if (err)
            {
                fullReset();
                co_return std::unexpected(Error::ReadChunk);
            }
            if (parser_->is_done())
                fullReset();
        }
        co_return std::nullopt;
    }

#pragma endregion

private:
    beast::tcp_stream                                             stream_;
    std::optional<http::response_parser<beast::http::empty_body>> parser_;
    boost::beast::flat_buffer                                     buffer_;

    HttpSettings setts_;

    bool reading_ = false;
    bool busy_ = false;

    std::string chunk_;
    std::size_t chunkCount_ = 0;
    std::size_t chunkTotal_ = 0;

    std::function<std::size_t(std::uint64_t, std::string_view, boost::beast::error_code &)> chunkCb_;
    std::function<void(std::uint64_t, std::string_view, boost::beast::error_code &)>        headerCb_;

private:
#pragma region private

    std::size_t chunkCb(std::uint64_t remain, std::string_view body, boost::beast::error_code &ec)
    {
        ec = {};
        chunk_.append(body);
        if (remain == body.size())
            ec = http::error::end_of_chunk;
        return body.size();
    }
    void headerCb(std::uint64_t size, [[maybe_unused]] std::string_view extensions, boost::beast::error_code &ec)
    {
        chunk_.clear();
        if (chunkCount_ >= setts_.size.chunkCount) // Число чанков
        {
            ec = boost::beast::http::error::body_limit;
            return;
        }
        if (chunkTotal_ >= setts_.size.chunkTotal) // Общая сумма чанков
        {
            ec = boost::beast::http::error::body_limit;
            return;
        }
        if (size > setts_.size.chunkOne) // Размер одного чанка
        {
            ec = boost::beast::http::error::body_limit;
            return;
        }
        ec = {};
        ++chunkCount_;
        chunkTotal_ += size;
        chunk_.reserve(static_cast<std::size_t>(size));
    }

    void fullReset()
    {
        reading_ = false;
        chunkCount_ = 0;
        chunkTotal_ = 0;
        chunk_.clear();

        buffer_.clear();
        buffer_.max_size(setts_.bufferSize);

        stream_.close();

        resetParser();
    }

    void resetParser()
    {
        parser_.emplace();
        parser_->on_chunk_body(chunkCb_);
        parser_->on_chunk_header(headerCb_);

        parser_->body_limit(boost::none);
        parser_->header_limit(setts_.size.header);
    }

    utils::AsyncResult<Response> readResult()
    {
        http::response_parser<http::string_body> parser(std::move(*parser_));
        parser.body_limit(setts_.size.body);
        setTimeOut(setts_.timeout.body);
        [[maybe_unused]] const auto [errRead, bytesRead] = co_await http::async_read(stream_, buffer_, parser, asio::as_tuple(asio::use_awaitable));
        if (errRead)
            co_return std::unexpected(Error::ReadBody);
        auto res = parser.release();
        co_return Response{.header = std::move(res.base()), .body = std::move(res.body())};
    }

    void setTimeOut(std::chrono::steady_clock::duration dur)
    {
        if (dur == HttpSettings::TimeOut::noLimit)
            stream_.expires_never();
        else
            stream_.expires_after(dur);
    }

#pragma endregion

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