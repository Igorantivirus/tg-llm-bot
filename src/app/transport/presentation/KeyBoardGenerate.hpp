#pragma once

#include <unordered_set>

#include <utils/Parser.hpp>

#include "Operation.hpp"
#include "app/Types.hpp"
#include <tgbot/Types.h>

namespace transport
{
class KeyBoardGenerate
{
public:
    static TgBot::InlineKeyboardMarkup::Ptr generateForModels(const std::unordered_set<std::string> &models, const app::ChatId chatId)
    {
        auto kb = std::make_shared<TgBot::InlineKeyboardMarkup>();

        Operation oper{.type = OperationType::SetMdl};

        for (const auto &model : models)
        {
            auto button = std::make_shared<TgBot::InlineKeyboardButton>();
            oper.data = model;
            button->text = model;
            if (auto res = utils::serialize(oper); res)
                button->callbackData = std::move(res.value());
            else
                continue;
            kb->inlineKeyboard.push_back({button});
        }
        return kb;
    };

private:
};
} // namespace transport