#pragma once

#include <unordered_map>
#include <unordered_set>

#include <openai/ChatsSettings/ChatsRepository.hpp>
#include <openai/Tools/Tool.hpp>
#include <openai/chatssettings/ChatHistory.hpp>

namespace openai
{
class ChatsProcessor;
class ChatsSettings
{
    friend class ChatsProcessor;

public:
    const std::unordered_map<std::string, Tool::Ptr> &tools() const
    {
        return tools_;
    }
    std::unordered_map<std::string, Tool::Ptr> &tools()
    {
        return tools_;
    }

    const std::unordered_set<std::string> &stops() const
    {
        return stops_;
    }
    std::unordered_set<std::string> &stops()
    {
        return stops_;
    }

    const ChatsRepository &repo() const
    {
        return repo_;
    }
    ChatsRepository &repo()
    {
        return repo_;
    }

    const std::unordered_set<std::string> models() const
    {
        return models_;
    }

    const bool stream() const
    {
        return stream_;
    }
    bool &stream()
    {
        return stream_;
    }

private:
    ChatsRepository                 repo_;
    std::unordered_set<std::string> models_;

    std::unordered_map<std::string, Tool::Ptr> tools_;
    std::unordered_set<std::string>            stops_;
    bool                                       stream_ = true;
};
} // namespace openai