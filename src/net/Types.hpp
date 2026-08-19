#pragma once

#include <boost/asio.hpp>
#include <boost/asio/experimental/coro.hpp>
#include <boost/beast.hpp>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
using tcp = asio::ip::tcp;

namespace net
{

enum class Stage
{
    None,
    Busy,
    Resolve,
    Connect,
    Write,
    ReadHead,
    ReadBody,
    ReadChunk,
};

using BeastRequest = http::request<http::string_body>;
using BeastResponse = http::response<http::string_body>;

struct ErrorResponse
{
    beast::error_code error;
    Stage             stage;
};

} // namespace net