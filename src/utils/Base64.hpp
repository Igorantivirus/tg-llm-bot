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

        std::string out;
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

private:
};
} // namespace utils