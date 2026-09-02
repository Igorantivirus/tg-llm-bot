#include "openai/ChatsProcessor.hpp"
#include "openai/MessagesGenerator/AssistantMessagesGenerator.hpp"
#include "openai/Tools/GetDateTool.hpp"
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/deferred.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/system/detail/errc.hpp>
#include <exception>
#include <iostream>

#include <utils/StreamGenerator.hpp>

utils::AsyncResult<void> asyncMain()
{
    openai::ChatsProcessor proc{co_await asio::this_coro::executor, "localhost", "9292"};

    if (auto res = co_await proc.initModels(); !res)
        co_return std::unexpected(res.error());

    openai::ChatIdType id = 1;

    auto tool = std::make_unique<openai::GetDateTool>();

    proc.settings().repo().setModel(id, "qwen3.6-35b");
    proc.settings().repo().setSystem(id, "Ты - умный помошник. Если чего-то не знаешь или не можешь сказать - говори об незнании прямо. не давай неточной информации.");
    proc.settings().repo().setAlloTools(id, {tool->name()});
    proc.settings().stream() = true;

    proc.addTool(std::move(tool));

    auto res = co_await proc.chatCompletions(1, "я путешественник во времени, какой сйечас год? Надеюсь, я не в 15 веке, помоги, я волнуюсь.");
    if (!res)
        co_return std::unexpected(res.error());

    openai::AssistantMessagesGenerator &gen = res.value();

    bool start = true;
    while (auto next = co_await gen.next())
    {
        if (start)
            std::cout << "\n\n";
        start = false;
        std::cout << next.value();
    }
    std::cout << '\n';

    if (gen.isError())
        co_return std::unexpected(gen.endReason());

    co_return utils::empty;
}

int main(int argc, char **argv)
{
#ifdef _WIN32
    std::system("chcp 65001 > nul");
#endif
    std::cout << "Start\n";
    boost::asio::io_context io;

    boost::asio::co_spawn(io.get_executor(), asyncMain(), [](std::exception_ptr, utils::SyncResult<void> res)
    {
        if (!res)
            std::cout << "Error: " << res.error().message() << '\n';
    });

    std::cout << "Task pushed\n";

    // boost::asio::co_spawn(io.get_executor(), asyncMain(), boost::asio::detached);

    io.run();

    std::cout << "End\n";

    return 0;
}

// #include <cstdlib>
// #include <iostream>

// #include <app/Application.hpp>
// #include <app/ConfigReader.hpp>

// int main(int argc, char **argv)
// {
// #ifdef _WIN32
//     std::system("chcp 65001 > nul");
// #endif
//     auto res = app::ConfigReader::read(std::span<char *>(argv, argc));
//     if (!res)
//     {
//         std::cerr << "Error: " << res.error() << '\n';
//         return EXIT_FAILURE;
//     }
//     app::Application app(res.value());
//     return app.run();
// }