#pragma once

#include <deque>

#include <memory>
#include <utils/MethodBinder.hpp>
#include <utils/NoMovable.hpp>

#include "TcpConnection.hpp"
#include "TcpReturner.hpp"

namespace net
{

class TcpPool : public utils::NoMovable, public TcpReturner
{
public:
    TcpPool(asio::any_io_executor ex, const std::size_t count)
        : ex_(ex)
    {
        for (std::size_t i = 0; i < count; ++i)
            datas_.push_back(std::make_shared<TcpData>(ex));
    }

    TcpConnection pullConnection(std::weak_ptr<TcpPool> me)
    {
        if (datas_.empty())
            return TcpConnection(nullptr, me);
        TcpData::Ptr data = std::move(datas_.back());
        datas_.pop_back();
        return TcpConnection(std::move(data), me);
    }

private:
    asio::any_io_executor    ex_;
    std::deque<TcpData::Ptr> datas_;

private:
    void returnPtr(TcpData::Ptr ptr) override
    {
        // Почистили, привели в порядок, можно и домой идти
        ptr->stream_.emplace(ex_);
        ptr->parser_.emplace();
        ptr->buffer_.clear();
        datas_.push_back(std::move(ptr));
    }
};
} // namespace net