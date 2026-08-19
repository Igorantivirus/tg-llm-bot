#pragma once

#include <chrono>
#include <cstddef>
#include <limits>

namespace net
{
struct HttpSettings
{
    struct TimeOut
    {
        constexpr static std::chrono::steady_clock::duration noLimit = std::chrono::steady_clock::duration::max();

        std::chrono::steady_clock::duration resolve = std::chrono::seconds(5);
        std::chrono::steady_clock::duration connect = std::chrono::seconds(10);
        std::chrono::steady_clock::duration request = std::chrono::seconds(30);
        std::chrono::steady_clock::duration header = std::chrono::seconds(60);
        std::chrono::steady_clock::duration chunk = std::chrono::seconds(160);
        std::chrono::steady_clock::duration body = std::chrono::seconds(120);
    };
    struct Size
    {
        constexpr static std::size_t noLimit = std::numeric_limits<std::size_t>::max();

        std::size_t header = 8192;
        std::size_t body = 8 * 1024 * 1024;
        std::size_t chunkOne = 16 * 1024;
        std::size_t chunkCount = noLimit;
        std::size_t chunkTotal = noLimit;
    };

    TimeOut     timeout;
    Size        size;
    std::size_t bufferSize = std::numeric_limits<std::size_t>::max();
};
} // namespace net