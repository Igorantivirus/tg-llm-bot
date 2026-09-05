#pragma once

#include <memory>

#include <openai/ChatsSettings/Types.hpp>

namespace core
{
struct Registration
{
    std::shared_ptr<bool> stop_;
    Registration(std::shared_ptr<bool> &stop)
    {
        stop = std::make_shared<bool>(false);
        stop_ = stop;
    }
    ~Registration()
    {
        *stop_ = true;
    }
    std::shared_ptr<bool> stop() const
    {
        return stop_;
    }
};
} // namespace core