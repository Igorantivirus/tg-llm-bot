#pragma once

#include <array>
#include <boost/system/detail/error_category.hpp>
#include <boost/system/detail/error_code.hpp>
#include <boost/system/is_error_code_enum.hpp>
#include <cstdint>
#include <utils/ErrorNameHelper.hpp>

namespace net
{
enum class Error : std::uint8_t
{
    Busy,
    Resolve,
    Connect,
    WriteRequest,
    ReadHead,
    ReadBody,
    ReadChunk,
    Beast,
    InvalidSseFragment,
    Unknown
};

class ErrorCategoryImpl final : public boost::system::error_category
{
public:
    ErrorCategoryImpl() noexcept
        : boost::system::error_category(0x6a3f12d7b8e04d41ULL)
    {
    }

    const char *name() const noexcept override
    {
        return "net";
    }

    std::string message(int ev) const override
    {
        return utils::getName(names, ev);
    }

    inline static const boost::system::error_category &getCategory() noexcept
    {
        static const ErrorCategoryImpl instance;
        return instance;
    }

private:
    constexpr static const std::array<const char *, static_cast<std::size_t>(Error::Unknown) + 1> names = {
        "Trying work with http stream while it is busy",
        "Resolve",
        "Connect",
        "Write request to server",
        "Read head",
        "Read full body",
        "Read next chunk",
        "Boost::Beast",
        "Invalid fragment in sse protocol",
        "Unknown"};
};

inline boost::system::error_code make_error_code(const Error e) noexcept
{
    return {static_cast<int>(e), ErrorCategoryImpl::getCategory()};
}

} // namespace net

template <>
struct boost::system::is_error_code_enum<net::Error> : std::true_type
{
};