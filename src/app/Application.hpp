#pragma once

#include "bot/BotCustomizer.hpp"
#include "bot/EventRegistrator.hpp"
#include "config/AppConfig.hpp"
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <optional>
#include <tgbot/Bot.h>
#include <thread>

#include <boost/asio.hpp>

#include <utils/MethodBinder.hpp>
#include <utils/Types.hpp>

#include "bot/PermissionChecker.hpp"
#include "middleware/CommandsProcessor.hpp"
#include "middleware/MessagesProcessor.hpp"
#include "middleware/Permissions/Permissions.hpp"
#include "middleware/TgBotMessageSender.hpp"
#include "middleware/TgBotPresentation.hpp"

namespace app
{
class Application
{
public:
    Application(config::AppConfig config)
        : io_(),
          pool_(config.threadCount),

          bot_(config.token),
          redirector_(pool_, bot_.getApi()),
          sender_(redirector_),
          presenter_(redirector_),

          proc_(io_.get_executor(), config.openAiUrl.host, config.openAiUrl.port),
          operator_(presenter_, proc_),

          data_(),
          permReadWriter_(data_, config.accessRightsFile),
          editor(data_),

          cmdProc_(operator_, editor, permReadWriter_, sender_),
          msgProc_(operator_, redirector_),

          checker_(data_),

          reger_(io_.get_executor(), bot_, checker_, cmdProc_, msgProc_),
          customizer_(bot_, reger_)

    {
        permReadWriter_.read();
        asio::co_spawn(io_.get_executor(), proc_.initModels(), asio::detached);
        proc_.settings().repo().setModel(1642467431, "igor-ai");
    }

    int run()
    {
        ioThread_.emplace(utils::buildMethod(&Application::ioMain, this));
        return botMain();
    }

private:
    asio::io_context            io_;
    std::optional<std::jthread> ioThread_;
    boost::asio::thread_pool    pool_;

    TgBot::Bot                     bot_;
    middleware::TgBotApiRedirector redirector_;
    middleware::TgBotMessageSender sender_;
    middleware::TgBotPresentation  presenter_;

    openai::ChatsProcessor proc_;
    core::Operator         operator_;

    middleware::Permissions          data_;
    middleware::PermissionReadWriter permReadWriter_;
    middleware::PermissionEditor     editor;
    middleware::CommandsProcessor    cmdProc_;
    middleware::MessagesProcessor    msgProc_;

    bot::PermissionChecker checker_;

    bot::EventRegistrator reger_;
    bot::BotCustomizer    customizer_;

private:
    int botMain()
    {
        try
        {
            customizer_.initHandlers();
            customizer_.run();
            return EXIT_SUCCESS;
        }
        catch (const std::exception &er)
        {
            // std::cout << "Error: " << er.what() << '\n';
            return EXIT_FAILURE;
        }
        catch (...)
        {
            // std::cout << "Enknown error\n";
            return EXIT_FAILURE;
        }
    }

    void ioMain(std::stop_token iot)
    {
        while (!iot.stop_requested())
        {
            io_.run();
        }
    }
};
} // namespace app