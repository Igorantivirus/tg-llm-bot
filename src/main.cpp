#include "bot/Bot.hpp"
#include "config/AppConfig.hpp"
#include "config/ConfigReader.hpp"
#include "openai/ChatsProcessor.hpp"
#include <boost/asio.hpp>
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/error.hpp>
#include <boost/beast/http/impl/read.hpp>
#include <boost/beast/http/parser_fwd.hpp>
#include <boost/beast/http/string_body_fwd.hpp>
#include <config/ConfigJsonSerialization.hpp>
#include <cstdlib>
#include <dto/ChatCompletionsRequest.hpp>
#include <exception>
#include <expected>
#include <iostream>
#include <magic_enum/magic_enum.hpp>
#include <net/Types.hpp>
#include <openai/Api.hpp>
#include <stop_token>
#include <string>
#include <thread>
#include <utils/MethodBinder.hpp>

int main(int argc, char **argv)
{
#ifdef _WIN32
    std::system("chcp 65001 > nul");
#endif

    if (argc != 2)
        return EXIT_FAILURE;

    auto configOpt = config::read<config::AppConfig>(argv[1]);
    if (!configOpt)
    {
        std::cerr << "Error: " << configOpt.error() << '\n';
        return EXIT_FAILURE;
    }
    config::AppConfig cnfg = configOpt.value();

    asio::io_context io;

    std::jthread ioThread([&io](std::stop_token itoken)
    {
        while (!itoken.stop_requested())
        {
            io.run();
        }
    });

    openai::ChatsProcessor chats(io.get_executor(), cnfg.openAiUrl.host, cnfg.openAiUrl.port);

    chats.initModels();

    bot::Bot bt(cnfg.token, chats);

    try
    {
        bt.run();
    }
    catch (const std::exception &er)
    {
        std::cout << "Error: " << er.what() << '\n';
    }
    catch (...)
    {
        std::cout << "Enlnown error\n";
    }

    return EXIT_SUCCESS;
}
