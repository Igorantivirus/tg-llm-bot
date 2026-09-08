#pragma once

#include <unordered_set>

#include <utils/Parser.hpp>

#include "Operation.hpp"
#include "magic_enum/magic_enum.hpp"
#include "openai/dto/ChatCompletions/Request.hpp"
#include <tgbot/Types.h>

namespace transport
{
class KeyBoardGenerate
{
public:
    static TgBot::InlineKeyboardMarkup::Ptr generateForModels(const std::unordered_set<std::string> &models, std::string current)
    {
        auto kb = std::make_shared<TgBot::InlineKeyboardMarkup>();
        for (const auto &model : models)
            if (auto button = makeButton(Operation(OperationType::SetMdl, model), model == current ? "✅ " + model : model))
                kb->inlineKeyboard.push_back({button});
        if (auto button = makeButton(Operation(OperationType::Close), "Закрыть"))
            kb->inlineKeyboard.push_back({button});
        return kb;
    };
    static TgBot::InlineKeyboardMarkup::Ptr generateForEfforts(const std::unordered_set<dto::ReasoningEffort> &efforts, dto::ReasoningEffort current)
    {
        auto kb = std::make_shared<TgBot::InlineKeyboardMarkup>();
        for (const auto &effort : efforts)
        {
            std::string name(magic_enum::enum_name(effort));
            if (auto button = makeButton(Operation(OperationType::SetEfr, name), effort == current ? "✅ " + name : name))
                kb->inlineKeyboard.push_back({button});
        }
        if (auto button = makeButton(Operation(OperationType::Close), "Закрыть"))
            kb->inlineKeyboard.push_back({button});
        return kb;
    };

private:
    static TgBot::InlineKeyboardButton::Ptr makeButton(Operation oper, std::string text)
    {
        std::string cb;
        if (auto res = utils::serialize(oper); res)
            cb = std::move(res.value());
        else
            return nullptr;
        auto button = std::make_shared<TgBot::InlineKeyboardButton>();
        button->text = std::move(text);
        button->callbackData = std::move(cb);
        return button;
    }
};
} // namespace transport