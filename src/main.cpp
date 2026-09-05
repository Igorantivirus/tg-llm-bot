// #include <iostream>
// #include <memory>
// #include <string>
// #include <tgbot/tgbot.h>
// #include <variant>

// using namespace TgBot;

// int main()
// {
//     #ifdef _WIN32
//         std::system("chcp 65001 > nul");
//     #endif
//     Bot bot("6928361979:AAGxp4--4K31HkNawImer5dBu_6BzNDgm1c");
//     int counter = 0;

//     // Собираем inline-клавиатуру
//     auto makeKeyboard = []()
//     {
//         auto kb = std::make_shared<InlineKeyboardMarkup>();

//         auto plus = std::make_shared<InlineKeyboardButton>();
//         plus->text = "+1";
//         plus->callbackData = "inc"; // это придёт в query->data

//         auto minus = std::make_shared<InlineKeyboardButton>();
//         minus->text = "-1";
//         minus->callbackData = "dec";

//         auto reset = std::make_shared<InlineKeyboardButton>();
//         reset->text = "Сброс";
//         reset->callbackData = "reset";

//         kb->inlineKeyboard.push_back({plus, minus}); // первая строка
//         kb->inlineKeyboard.push_back({reset});       // вторая строка
//         return kb;
//     };

//     // /start — отправляем сообщение с кнопками
//     bot.getEvents().onCommand("start", [&](Message::Ptr msg)
//     {
//         bot.getApi().sendMessage(msg->chat->id,
//                                  "Счётчик: " + std::to_string(counter),
//                                  nullptr, nullptr, makeKeyboard());
//     });

//     // Нажатие на любую inline-кнопку попадает сюда
//     bot.getEvents().onCallbackQuery([&](CallbackQuery::Ptr query)
//     {
//         // В 1.12 query->message — variant<Message, InaccessibleMessage>
//         Message::Ptr msg;
//         if (query->message)
//         {
//             if (auto *p = std::get_if<Message::Ptr>(&query->message->value))
//             {
//                 msg = *p;
//             }
//         }
//         if (!msg)
//         {
//             bot.getApi().answerCallbackQuery(query->id, "Сообщение недоступно", true);
//             return;
//         }

//         std::string data = query->data.value_or("");

//         // ===== ТВОЙ КОД ПО НАЖАТИЮ =====
//         if (data == "inc")
//             ++counter;
//         else if (data == "dec")
//             --counter;
//         else if (data == "reset")
//             counter = 0;

//         std::cout << "Пользователь " << query->from->id
//                   << " нажал: " << data
//                   << ", counter = " << counter << std::endl;
//         // ===============================

//         // Обновляем текст на месте, кнопки передаём заново, чтобы они не пропали
//         try
//         {
//             bot.getApi().editMessageText("Счётчик: " + std::to_string(counter),
//                                          msg->chat->id, msg->messageId,
//                                          "", "", nullptr, makeKeyboard());
//         }
//         catch (TgException &e)
//         {
//             // Telegram ругается, если текст не изменился (например, два раза "Сброс")
//             std::cerr << "editMessageText: " << e.what() << std::endl;
//         }

//         // Обязательно: убираем "часики" на кнопке
//         bot.getApi().answerCallbackQuery(query->id, "Нажато: " + data);
//     });

//     std::cout << "Бот запущен: "
//               << bot.getApi().getMe()->username.value_or("?") << std::endl;

//     TgLongPoll longPoll(bot);
//     while (true)
//     {
//         try
//         {
//             longPoll.start();
//         }
//         catch (std::exception &e)
//         {
//             std::cerr << "Ошибка: " << e.what() << std::endl;
//         }
//     }
// }

// #include "openai/ChatsProcessor.hpp"
// #include "openai/MessagesGenerator/AssistantMessagesGenerator.hpp"
// #include "openai/Tools/GetDateTool.hpp"
// #include "openai/Tools/Tool.hpp"
// #include <boost/asio/co_spawn.hpp>
// #include <boost/asio/deferred.hpp>
// #include <boost/asio/detached.hpp>
// #include <boost/asio/io_context.hpp>
// #include <boost/asio/this_coro.hpp>
// #include <boost/asio/use_awaitable.hpp>
// #include <boost/system/detail/errc.hpp>
// #include <exception>
// #include <iostream>

// #include <utils/StreamGenerator.hpp>

// class GetCountryTool : public openai::Tool
// {
// public:
//     utils::AsyncResult<std::string> run(std::string args) override
//     {
//         co_return "Бразилия";
//     }

//     std::string name() const override
//     {
//         return "gen_currebnt_country";
//     }
//     std::string description() const override
//     {
//         return "Получить страну расположения пользователя";
//     }
// };

// utils::AsyncResult<void> asyncMain()
// {
//     openai::ChatsProcessor proc{co_await asio::this_coro::executor, "localhost", "9292"};

//     if (auto res = co_await proc.initModels(); !res)
//         co_return std::unexpected(res.error());

//     openai::ChatIdType id = 1;

//     auto tool1 = std::make_unique<GetCountryTool>();
//     auto tool2 = std::make_unique<openai::GetDateTool>();

//     proc.settings().repo().setModel(id, "qwen3.6-35b");
//     proc.settings().repo().setSystem(id, "Ты - умный помошник. Если чего-то не знаешь или не можешь сказать - говори об незнании прямо. не давай неточной информации.");
//     proc.settings().repo().setAlloTools(id, {tool1->name(), tool2->name()});
//     proc.settings().stream() = true;

//     proc.addTool(std::move(tool1));
//     proc.addTool(std::move(tool2));

//     auto res = co_await proc.chatCompletions(1, "я путешественник во времени, какой сейчас год? и в какой стране я вообще нахожусь? Надеюсь, я не в 15 веке в европпе. . . помоги понять где и когда я, я волнуюсь.");
//     if (!res)
//         co_return std::unexpected(res.error());

//     openai::AssistantMessagesGenerator &gen = res.value();

//     bool start = true;
//     while (auto next = co_await gen.next())
//     {
//         if (start)
//             std::cout << "\n\n";
//         start = false;
//         std::cout << next.value();
//     }
//     std::cout << '\n';

//     if (gen.isError())
//         co_return std::unexpected(gen.endReason());

//     co_return utils::empty;
// }

// int main(int argc, char **argv)
// {
// #ifdef _WIN32
//     std::system("chcp 65001 > nul");
// #endif
//     std::cout << "Start\n";
//     boost::asio::io_context io;

//     boost::asio::co_spawn(io.get_executor(), asyncMain(), [](std::exception_ptr, utils::SyncResult<void> res)
//     {
//         if (!res)
//             std::cout << "Error: " << res.error().message() << '\n';
//     });

//     std::cout << "Task pushed\n";

//     // boost::asio::co_spawn(io.get_executor(), asyncMain(), boost::asio::detached);

//     io.run();

//     std::cout << "End\n";

//     return 0;
// }
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include <cstdlib>
#include <iostream>

#include <app/Application.hpp>
#include <app/ConfigReader.hpp>

int main(int argc, char **argv)
{
#ifdef _WIN32
    std::system("chcp 65001 > nul");
#endif
    auto res = app::ConfigReader::read(std::span<char *>(argv, argc));
    if (!res)
    {
        std::cerr << "Error: " << res.error() << '\n';
        return EXIT_FAILURE;
    }
    app::Application app(res.value());
    return app.run();
}