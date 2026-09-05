#pragma once

#include <openai/MessagesGenerator/AssistantMessagesGenerator.hpp>
#include <utils/StreamGenerator.hpp>

namespace core
{
class StopableGenerator : public utils::StreamGenerator<std::string>
{
public:
    StopableGenerator(openai::AssistantMessagesGenerator gen, const bool &stop)
        : gen_(std::move(gen)), stop_(stop)
    {
    }

private:
    openai::AssistantMessagesGenerator gen_;
    const bool                        &stop_;

private:
    utils::AsyncResult<std::string> nextImpl() override
    {
        if (stop_)
            co_return endOfStream;
        co_return co_await gen_.next();
    }
};
} // namespace core