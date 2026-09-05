#pragma once

#include <unordered_map>

#include <openai/ChatsSettings/Types.hpp>

namespace core
{
struct Registration
{
    std::unordered_map<openai::ChatIdType, bool *> &stopsSignals;
    openai::ChatIdType                              chatId;
    bool                                            value = false;
    Registration(std::unordered_map<openai::ChatIdType, bool *> &stopsSignals, openai::ChatIdType chatId)
        : stopsSignals(stopsSignals), chatId(chatId)
    {
        stopsSignals[chatId] = &value;
    }
    ~Registration()
    {
        stopsSignals.erase(chatId);
    }
    const bool &stop() const
    {
        return *stopsSignals.find(chatId)->second;
    }
};
} // namespace core