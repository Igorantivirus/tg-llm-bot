#pragma once

#include <expected>

#include <boost/asio/awaitable.hpp>
#include <boost/system/detail/error_code.hpp>

namespace asio = boost::asio;

namespace utils
{
using ErrorCode = boost::system::error_code;
template <typename Type>
using AsyncResult = asio::awaitable<std::expected<Type, ErrorCode>>;
template <typename Type>
using SyncResult = std::expected<Type, ErrorCode>;
constexpr SyncResult<void> empty = SyncResult<void>{};
} // namespace utils