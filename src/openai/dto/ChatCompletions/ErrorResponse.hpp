#pragma once

#include <string>

namespace dto
{

struct ErrorResponse
{
    std::string error;
    std::string src;
};

} // namespace dto
