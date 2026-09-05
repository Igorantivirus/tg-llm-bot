#pragma once

#include <utils/ErrorGenerator.hpp>

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
    NoFreeConnections,
    Unknown
};
constexpr std::array<const char *, static_cast<std::size_t>(Error::Unknown) + 1> errorNames = {
    "Trying work with http stream while it is busy",
    "Resolve",
    "Connect",
    "Write request to server",
    "Read head",
    "Read full body",
    "Read next chunk",
    "Boost::Beast",
    "Invalid fragment in sse protocol",
    "No free tcp connections",
    "Unknown"};
} // namespace net

UTILS_GENERATE_ERRORS(net, 0x6a3f12d7b8e04d41ULL)