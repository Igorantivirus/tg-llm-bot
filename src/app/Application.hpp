#pragma once

#include "store/ImageStore.hpp"
#include "utils/Types.hpp"
#include <app/handlers/QueryProcessor.hpp>
#include <app/tools/CreateImage.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/thread_pool.hpp>
#include <openai/Tools/Tool.hpp>

#include <app/config/AppConfig.hpp>

#include <app/transport/TgBotApiRedirector.hpp>
#include <app/transport/TgBotMessageSender.hpp>
#include <app/transport/TgBotPresentation.hpp>

#include <app/permissions/Editor.hpp>
#include <app/permissions/Permissions.hpp>
#include <app/permissions/ReadWriter.hpp>

#include <app/handlers/CommandsProcessor.hpp>
#include <app/handlers/MessagesProcessor.hpp>

#include <app/bot/BotCustomizer.hpp>
#include <app/bot/EventRegistrator.hpp>
#include <app/bot/PermissionChecker.hpp>
#include <exception>
#include <memory>

namespace app
{
class Application
{
public:
    Application(config::AppConfig config)
        : io_(),
          pool_(0),
          //   pool_(config.threadCount),

          bot_(config.token),
          redirector_(pool_, bot_.getApi()),
          sender_(redirector_),
          presenter_(sender_, config.locale),

          proc_(io_.get_executor(), std::move(config.defaultModel), config.defaultEffort, config.tcpSocketsCount, config.openAiUrl.host, config.openAiUrl.port),
          operator_(presenter_, proc_),

          data_(),
          permReadWriter_(data_, config.accessRightsFile),
          editor(data_),

          cmdProc_(sender_, operator_, config.locale, editor, permReadWriter_),
          msgProc_(sender_, operator_, imgStore_),
          queProc_(sender_, operator_, config.locale),

          checker_(data_),

          reger_(io_.get_executor(), bot_, checker_, cmdProc_, msgProc_, queProc_, config.locale),
          customizer_(bot_, reger_),
          cmnds_(std::move(config.commands))
    {
        permReadWriter_.read();
        asio::co_spawn(io_.get_executor(), proc_.initModels(), [](std::exception_ptr ex, utils::SyncResult<const std::unordered_set<std::string> *> res)
        {
            if (!res)
                std::cout << "Error init models: " << res.error().message() << '\n';
        });

        proc_.addTool(std::make_unique<tools::CreateImage>(proc_.getApi(), imgStore_, std::move(config.images)));
    }

    int run()
    {
        // ioThread_.emplace(utils::buildMethod(&Application::ioMain, this));
        return botMain();
    }

private:
    asio::io_context            io_;
    std::optional<std::jthread> ioThread_;
    boost::asio::thread_pool    pool_;

    TgBot::Bot                    bot_;
    transport::TgBotApiRedirector redirector_;
    transport::TgBotMessageSender sender_;
    transport::TgBotPresentation  presenter_;

    openai::ChatsProcessor proc_;
    core::Operator         operator_;

    permissions::Permissions data_;
    permissions::ReadWriter  permReadWriter_;
    permissions::Editor      editor;

    store::ImageStore imgStore_;

    handlers::CommandsProcessor cmdProc_;
    handlers::MessagesProcessor msgProc_;
    handlers::QueryProcessor    queProc_;

    bot::PermissionChecker checker_;
    bot::EventRegistrator  reger_;
    bot::BotCustomizer     customizer_;

    config::AllCommands cmnds_;

private:
    int botMain()
    {
        customizer_.initHandlers(cmnds_);

        int64_t lastUpdateId = 0;

        while (true)
        {
            // Один короткий опрос. timeout = 0 — вернуть управление сразу,
            // если обновлений нет.
            std::vector<TgBot::Update::Ptr> updates;
            try
            {
                updates = bot_.getApi().getUpdates(
                    lastUpdateId + 1, // offset
                    100,              // limit
                    0                 // timeout (0 = без ожидания)
                );
            }
            catch (const std::exception &e)
            {
                std::cerr << "getUpdates error: " << e.what() << '\n';
            }

            // Передаём каждое обновление в тот же обработчик,
            // который использует TgLongPoll.
            for (const auto &update : updates)
            {
                lastUpdateId = update->updateId;
                bot_.getEventHandler().handleUpdate(update);
            }

            // Теперь можно безопасно крутить io_context:
            // все колбэки уже поставлены в очередь.
            io_.restart(); // обязательно: после run() контекст остановлен
            io_.run();
        }

        return EXIT_SUCCESS;
    }
    // int botMain()
    // {
    //     try
    //     {
    //         customizer_.initHandlers(cmnds_);
    //         while(true)
    //         {
    //             std::cout << "startNext\n";
    //             customizer_.startNext();
    //             std::cout << "run\n";
    //             io_.run();
    //         }
    //         return EXIT_SUCCESS;
    //     }
    //     catch (const std::exception &er)
    //     {
    //         std::cout << "Error: " << er.what() << '\n';
    //         return EXIT_FAILURE;
    //     }
    //     catch (...)
    //     {
    //         std::cout << "Enknown error\n";
    //         return EXIT_FAILURE;
    //     }
    // }

    void ioMain(std::stop_token iot)
    {
        auto guard = asio::make_work_guard(io_);
        io_.run();
    }
};
} // namespace app