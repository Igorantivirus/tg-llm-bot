#pragma once

#include <memory>

#include "HttpSettings.hpp"
#include "TcpData.hpp"
#include "TcpReturner.hpp"

namespace net
{
class TcpConnection
{
public:
    TcpConnection(TcpData::Ptr data, std::weak_ptr<TcpReturner> returner)
        : data_(std::move(data)), returner_(std::move(returner))
    {
    }
    TcpConnection(TcpConnection &&other)
        : data_(std::move(other.data_)),
          returner_(std::move(other.returner_))
    {
        other.data_ = nullptr;
    }
    TcpConnection(const TcpConnection &other) = delete;
    ~TcpConnection()
    {
        if (auto ptr = returner_.lock(); ptr && data_)
            ptr->returnPtr(std::move(data_));
    }
    TcpConnection &operator=(const TcpConnection &other) = delete;
    TcpConnection &operator=(TcpConnection &&other) = delete;

    bool valid() const
    {
        return static_cast<bool>(data_);
    }

    beast::tcp_stream &stream()
    {
        return data_->stream_.value();
    }
    std::optional<http::response_parser<http::empty_body>> &parser()
    {
        return data_->parser_;
    }

    boost::beast::flat_buffer &buffer()
    {
        return data_->buffer_;
    }

    void setTimeOut(std::chrono::steady_clock::duration dur)
    {
        if (dur == HttpSettings::TimeOut::noLimit)
            stream().expires_never();
        else
            stream().expires_after(dur);
    }

private:
    TcpData::Ptr               data_;
    std::weak_ptr<TcpReturner> returner_;
};
} // namespace net