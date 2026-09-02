#pragma once

#include <cstdint>

#include <dto/ChatCompletions/Message.hpp>

namespace openai
{
using ChatIdType = std::int64_t;
using DialogFragment = std::vector<dto::Message>;
} // namespace openai