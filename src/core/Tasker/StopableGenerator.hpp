#pragma once

#include <openai/messagegenerators/AssistantMessagesGenerator.hpp>
#include <utils/NoMovable.hpp>
#include <utils/StreamGenerator.hpp>

namespace core
{
class StopableGenerator : public utils::StreamGenerator<std::string>, public utils::NoMovable
{
public:
    StopableGenerator(openai::AssistantMessagesGenerator gen, const std::shared_ptr<bool> stop)
        : gen_(std::move(gen)), stop_(stop)
    {
    }

private:
    openai::AssistantMessagesGenerator gen_;
    const std::shared_ptr<bool>        stop_;

private:
    utils::AsyncResult<std::string> nextImpl() override
    {
        if (*stop_)
            co_return endOfStream;
        auto res = co_await gen_.next();
        co_return res;
    }
};
} // namespace core