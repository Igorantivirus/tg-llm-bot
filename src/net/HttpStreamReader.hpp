#pragma once

#include <utils/MethodBinder.hpp>
#include <utils/StreamGenerator.hpp>

#include "Error.hpp"
#include "HttpSettings.hpp"
#include "TcpConnection.hpp"

namespace net
{
class HttpStreamReader : public utils::StreamGenerator<std::string>
{
public:
    HttpStreamReader(TcpConnection conn, HttpSettings setts)
        : conn_(std::move(conn)),
          setts_(std::move(setts))
    {
        initCallBacks();
    }
    HttpStreamReader(HttpStreamReader &&other)
        : conn_(std::move(other.conn_)),
          setts_(std::move(other.setts_)),
          chunk_(std::move(other.chunk_)),
          chunkCount_(std::move(other.chunkCount_)),
          chunkTotal_(std::move(other.chunkTotal_))
    {
        initCallBacks();
    }
    HttpStreamReader(const HttpStreamReader &other) = delete;

    HttpStreamReader &operator=(const HttpStreamReader &other) = delete;
    HttpStreamReader &operator=(HttpStreamReader &&other) = delete;

private:
    TcpConnection conn_;
    HttpSettings  setts_;

    std::string chunk_;
    std::size_t chunkCount_ = 0;
    std::size_t chunkTotal_ = 0;

    std::function<std::size_t(std::uint64_t, std::string_view, boost::beast::error_code &)> chunkCb_;
    std::function<void(std::uint64_t, std::string_view, boost::beast::error_code &)>        headerCb_;

private:
    bool isReady() const override
    {
        return conn_.valid();
    }

    utils::AsyncResult<std::string> nextImpl() override
    {
        while (!done())
        {
            conn_.setTimeOut(setts_.timeout.chunk);
            [[maybe_unused]] const auto [err, bytesRead] = co_await http::async_read_some(conn_.stream(), conn_.buffer(), *conn_.parser(), asio::as_tuple(asio::use_awaitable));
            if (err == http::error::end_of_chunk)
                co_return std::move(chunk_);
            if (err)
                co_return std::unexpected(Error::ReadChunk);
            if (conn_.parser()->is_done())
                co_return std::unexpected(endOfStream);
        }
        co_return std::unexpected(endOfStream);
    }

private:
    void initCallBacks()
    {
        chunkCb_ = utils::buildMethod(&HttpStreamReader::chunkCb, this);
        headerCb_ = utils::buildMethod(&HttpStreamReader::headerCb, this);

        conn_.parser()->on_chunk_body(chunkCb_);
        conn_.parser()->on_chunk_header(headerCb_);
    }

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
};
} // namespace net