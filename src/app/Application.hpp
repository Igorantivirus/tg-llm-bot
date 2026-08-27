#pragma once

#include <cstdlib>
#include <optional>
#include <stop_token>
#include <thread>

#include <bot/Bot.hpp>
#include <config/AppConfig.hpp>
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
          chat_(io_.get_executor(), cnfg.openAiUrl.host, cnfg.openAiUrl.port),
          bot_(cnfg.token, chat_)
    {
    }

    int run()
    {
        ioThread_.emplace(utils::buildMethod(&Application::ioMain, this));

        chat_.initModels();

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
    openai::ChatsProcessor      chat_;
    bot::Bot                    bot_;

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