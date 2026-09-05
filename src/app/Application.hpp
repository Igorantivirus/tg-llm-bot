#pragma once

#include "handlers/QueryProcessor.hpp"
#include <boost/asio/io_context.hpp>
#include <boost/asio/thread_pool.hpp>

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
          presenter_(redirector_, proc_),

          proc_(io_.get_executor(), config.tcpSocketsCount, config.openAiUrl.host, config.openAiUrl.port),
          operator_(presenter_, proc_),

          data_(),
          permReadWriter_(data_, config.accessRightsFile),
          editor(data_),

          cmdProc_(operator_, editor, permReadWriter_, sender_),
          msgProc_(operator_, redirector_),
          queProc_(proc_, sender_),

          checker_(data_),

          reger_(io_.get_executor(), bot_, checker_, cmdProc_, msgProc_, queProc_),
          customizer_(bot_, reger_)
    {
        permReadWriter_.read();
        asio::co_spawn(io_.get_executor(), proc_.initModels(), asio::detached);
        proc_.settings().repo().setModel(1642467431, "aboba");
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

    TgBot::Bot                    bot_;
    transport::TgBotApiRedirector redirector_;
    transport::TgBotMessageSender sender_;
    transport::TgBotPresentation  presenter_;

    openai::ChatsProcessor proc_;
    core::Operator         operator_;

    permissions::Permissions data_;
    permissions::ReadWriter  permReadWriter_;
    permissions::Editor      editor;

    handlers::CommandsProcessor cmdProc_;
    handlers::MessagesProcessor msgProc_;
    handlers::QueryProcessor    queProc_;

    bot::PermissionChecker checker_;
    bot::EventRegistrator  reger_;
    bot::BotCustomizer     customizer_;

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
        auto guard = asio::make_work_guard(io_);
        io_.run();
    }
};
} // namespace app