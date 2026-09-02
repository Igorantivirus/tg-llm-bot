#pragma once

#include "AdditionalsToMessage.hpp"
#include "MessagesGenerator/AssistantMessagesGenerator.hpp"

namespace openai
{

class ChatsProcessor
{
public:
    ChatsProcessor(asio::any_io_executor ex, std::string host, std::string port, std::string apiToken = {})
        : api_(ex, std::move(host), std::move(port), apiToken)
    {
    }

    const ChatsSettings &settings() const
    {
        return setts_;
    }
    ChatsSettings &settings()
    {
        return setts_;
    }

    void addTool(Tool::Ptr tool)
    {
        setts_.tools_[tool->name()] = std::move(tool);
    }

    utils::AsyncResult<const std::unordered_set<std::string> *> initModels()
    {
        auto modelsDto = co_await api_.models();
        if (!modelsDto)
            co_return std::unexpected(modelsDto.error());
        dto::ModelsResponse modelsRes = std::move(modelsDto.value());

        setts_.models_ = modelsRes.data | std::views::transform([](const dto::Model &model) -> std::string
        {
            return model.id;
        }) | std::ranges::to<std::unordered_set<std::string>>();

        co_return &setts_.models_;
    }

    utils::AsyncResult<AssistantMessagesGenerator> chatCompletions(const ChatIdType chatId, std::string msg, AdditionalsToMessage additionals = {})
    {
        dto::Message startmsg = constructStartMessage(std::move(msg), std::move(additionals));
        co_return AssistantMessagesGenerator(api_, chatId, std::move(startmsg), setts_);
    }

private:
    Api api_;

    ChatsSettings setts_;

private:
    static dto::Message constructStartMessage(std::string msg, AdditionalsToMessage additionals)
    {
        dto::Message res;
        res.role = dto::Role::user;
        if (additionals.empty())
            res.content = std::move(msg);
        else
        {
            std::vector<dto::ContentPart> parts;
            parts.reserve(additionals.sumOfDataParts() + 1);
            parts.emplace_back(dto::TextPart{.text = std::move(msg)});
            appendParts(parts, std::move(additionals.imagesB64), &dto::ImagePart::image_url);
            appendParts(parts, std::move(additionals.audiosB64), &dto::AudioPart::input_audio);
            appendParts(parts, std::move(additionals.filesB64), &dto::FilePart::file);
        }

        return res;
    }

    template <typename Container, typename PartType, typename Member>
    static void appendParts(std::vector<dto::ContentPart> &parts, Container &&container, Member PartType::*member)
    {
        for (auto &elem : std::forward<Container>(container))
        {
            PartType part;
            part.*member = std::move(elem);
            parts.emplace_back(std::move(part));
        }
    }
};

} // namespace openai