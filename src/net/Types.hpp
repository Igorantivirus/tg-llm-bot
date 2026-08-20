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

using BeastRequest = http::request<http::string_body>;
using BeastResponse = http::response<http::string_body>;

} // namespace net