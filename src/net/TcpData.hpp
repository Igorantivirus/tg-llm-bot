#pragma once

#include <memory>

#include "Types.hpp"

namespace net
{
struct TcpData
{
    using Ptr = std::shared_ptr<TcpData>;
    TcpData(asio::any_io_executor ex)
        : stream_(std::move(ex))
    {
        parser_.emplace();
    }
    std::optional<beast::tcp_stream>                       stream_;
    std::optional<http::response_parser<http::empty_body>> parser_;
    boost::beast::flat_buffer                              buffer_;
};
} // namespace net