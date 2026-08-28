#pragma once

#include "bot/BotMessageSener.hpp"
#include <cstdlib>
#include <optional>
#include <stop_token>
#include <thread>

#include <bot/Bot.hpp>
#include <config/AppConfig.hpp>
#include <coords/CommandCoordinator.hpp>
#include <coords/MessageCoordinator.hpp>
#include <net/Types.hpp>
#include <openai/ChatsProcessor.hpp>
#include <utils/MethodBinder.hpp>

namespace app
{
class Application
{
public:
    Application(config::AppConfig cnfg)
        : io_(),
          cmdCoorder_(sender_, processor_),
          msgCoorder_(sender_, processor_),
          processor_(io_.get_executor(), cnfg.openAiUrl.host, cnfg.openAiUrl.port),
          bot_(cnfg.token, processor_, sender_, cmdCoorder_, msgCoorder_)
    {
    }

    int run()
    {
        ioThread_.emplace(utils::buildMethod(&Application::ioMain, this));

        processor_.initModels();

        try
        {
            bot_.run();
            return EXIT_SUCCESS;
        }
        catch (const std::exception &er)
        {
            std::cout << "Error: " << er.what() << '\n';
            return EXIT_FAILURE;
        }
        catch (...)
        {
            std::cout << "Enknown error\n";
            return EXIT_FAILURE;
        }
    }

private:
    std::optional<std::jthread> ioThread_;
    asio::io_context            io_;

    openai::ChatsProcessor     processor_;
    bot::BotMessageSener       sender_;
    coords::CommandCoordinator cmdCoorder_;
    coords::MessageCoordinator msgCoorder_;
    bot::Bot                   bot_;

private:
    void ioMain(std::stop_token iot)
    {
        while (!iot.stop_requested())
        {
            io_.run();
        }
    }
};
} // namespace app