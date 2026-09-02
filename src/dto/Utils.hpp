#pragma once

#include "ChatCompletions/Choice.hpp"
#include "ChatCompletions/Response.hpp"

namespace dto
{

class Utils
{
public:
    static std::optional<std::string> findContent(const ChatCompletionsResponse &dto)
    {
        if (dto.choices.size() != 1)
            return std::nullopt;
        const dto::Choice          &choice = dto.choices[0];
        const dto::ResponseMessage *msg = choice.message ? &choice.message.value() : (choice.delta ? &choice.delta.value() : nullptr);
        if (!msg)
            return std::nullopt;
        return msg->content;
    }

private:
};
} // namespace dto