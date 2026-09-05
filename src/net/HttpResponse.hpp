#pragma once

#include <variant>

#include "HttpStreamReader.hpp"

namespace net
{
struct HttpResponse
{
    http::response_header<>                     header;
    std::variant<std::string, HttpStreamReader> body;

    bool isStreaming() const
    {
        return body.index() == 1;
    }

    std::string &stringBody()
    {
        if (!isStreaming())
            return std::get<std::string>(body);
        throw std::logic_error("Response not contain std::string.");
    }
    HttpStreamReader &streamBody()
    {
        if (isStreaming())
            return std::get<HttpStreamReader>(body);
        throw std::logic_error("Response not contain stream.");
    }
};
} // namespace net