#pragma once

#include <openai/chatssettings/ChatHistory.hpp>

namespace openai
{
/// @brief Контекст вызова инструмента: откуда его позвали и с какими настройками.
///
/// Настройки чата даются только на чтение — инструмент выбирает по ним поведение
/// (например, модель для картинок), но менять их не его дело. Новые сведения
/// добавляются полем сюда, без правки сигнатуры run() у всех инструментов.
struct ToolContext
{
    ChatIdType         chatId = 0;
    const ChatHistory *history = nullptr; // nullptr, если настройки чата недоступны
};
} // namespace openai