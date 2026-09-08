#pragma once

#include <limits>
#include <string>

#include <openssl/evp.h>

#include "Error.hpp"
#include "Types.hpp"

namespace utils
{
class Base64
{
public:
    static SyncResult<std::string> encode(const std::string &input)
    {
        if (input.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
            return std::unexpected(Error::TooLongDataSize);

        const std::size_t out_len = 4 * ((input.size() + 2) / 3); // без NUL
        std::string       out;
        out.resize(out_len + 1); // +1 для '\0'

        const int written = EVP_EncodeBlock(
            reinterpret_cast<unsigned char *>(out.data()),
            reinterpret_cast<const unsigned char *>(input.data()),
            static_cast<int>(input.size()));

        if (written < 0)
            return std::unexpected(Error::EncodeError);

        out.resize(static_cast<std::size_t>(written));
        return out;
    }
    static SyncResult<std::string> decode(const std::string &input)
    {
        if (input.empty())
            return std::string{};
        if (input.size() % 4 != 0)
            return std::unexpected(Error::SizeNotDevidedBy4);
        if (input.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
            return std::unexpected(Error::TooLongDataSize);

        const std::size_t out_len = 3 * input.size() / 4;
        std::string       out;
        out.resize(out_len + 1);

        const int written = EVP_DecodeBlock(
            reinterpret_cast<unsigned char *>(out.data()),
            reinterpret_cast<const unsigned char *>(input.data()),
            static_cast<int>(input.size()));

        if (written < 0)
            return std::unexpected(Error::DecodeError);

        std::size_t result_size = static_cast<std::size_t>(written);
        for (std::size_t i = 1; i <= 2; i++)
            if (input[input.size() - i] == '=')
                result_size--;

        out.resize(result_size);
        return out;
    }

private:
};
} // namespace utils