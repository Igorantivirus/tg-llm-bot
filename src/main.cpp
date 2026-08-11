#include <tgbot/HttpClient.h>
#include <tgbot/TgLongPoll.h>
constexpr const char *BOT_TOKEN = "";

#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <string>

#include <tgbot/tgbot.h>

int main()
{
#ifdef _WIN32
    std::system("chcp 65001 > nul");
#endif
    const auto token = std::string(BOT_TOKEN);
    // std::cout << "Token: " << token << std::endl;

    // getDefaulf

    TgBot::Bot bot(token);
    bot.getEvents().onCommand("start", [&bot](std::shared_ptr<TgBot::Message> message)
    {
        bot.getApi().sendMessage(message->chat->id, "Hi!");
    });
    bot.getEvents().onAnyMessage([&bot](std::shared_ptr<TgBot::Message> message)
    {
        const auto text = message->text.value_or("");
        std::cout << "User wrote " << text << std::endl;
        if (text.starts_with("/start"))
        {
            return;
        }
        bot.getApi().sendMessage(message->chat->id, "Your message is: " + text);
    });

    const auto handleError = [](const std::exception &error)
    {
        std::cout << "error: " << error.what() << std::endl;
    };

    TgBot::TgLongPoll longPoll(bot);
    try
    {
        while (true)
        {
            longPoll.start();
        }
    }
    catch (...)
    {

    }

    // try
    // {
    //     std::cout << "Bot username: " << bot.getApi().getMe()->username.value_or("") << std::endl;
    //     bot.getApi().deleteWebhook();
    //     while (true)
    //     {

    //         TgBot::TgLongPoll longPoll(bot, 100, 10);
    //         longPoll.start();
    //         std::cout << "end\n";
    //     }
    // }
    // catch (const std::exception &error)
    // {
    //     handleError(error);
    //     return EXIT_FAILURE;
    // }

    return EXIT_SUCCESS;
}

// #include <boost/asio/awaitable.hpp>
// #include <chrono>
// #include <cstdlib>
// #include <expected>
// #include <iostream>

// #include <boost/asio.hpp>
// #include <boost/asio/ip/tcp.hpp>
// #include <boost/beast.hpp>

// namespace asio = boost::asio;
// namespace beast = boost::beast;
// namespace http = beast::http;
// using tcp = asio::ip::tcp;

// enum class Stage
// {
//     Resolve,
//     Connect,
//     Write,
//     Read
// };

// using BeastRequest = http::request<http::string_body>;
// using BeastResponse = http::response<http::string_body>;

// struct ErrorResponse
// {
//     beast::error_code error;
//     Stage stage;
// };

// asio::awaitable<std::expected<boost::asio::ip::basic_resolver_results<boost::asio::ip::tcp>, beast::error_code>> resolve(const std::string_view host, const std::string_view port)
// {
//     boost::asio::ip::tcp::resolver resolver(co_await asio::this_coro::executor);
//     const auto [err, resolve] = co_await resolver.async_resolve(host, port, asio::as_tuple(asio::use_awaitable));
//     if (err)
//         co_return std::unexpected(err);
//     co_return resolve;
// }

// asio::awaitable<std::expected<BeastResponse, ErrorResponse>> request(const std::string_view host, const std::string_view port, BeastRequest req)
// {
//     beast::tcp_stream stream(co_await asio::this_coro::executor);

//     if (auto resolveRes = co_await resolve(host, port); resolveRes)
//     {
//         const auto [err, connectionRes] = co_await stream.async_connect(resolveRes.value(), asio::as_tuple(asio::use_awaitable));
//         if (err)
//             co_return std::unexpected(ErrorResponse{err, Stage::Connect});
//     }
//     else
//         co_return std::unexpected(ErrorResponse{resolveRes.error(), Stage::Resolve});

//     const auto [errWrite, bytesWrite] = co_await http::async_write(stream, std::move(req), asio::as_tuple(asio::use_awaitable));
//     if (errWrite)
//         co_return std::unexpected(ErrorResponse{errWrite, Stage::Write});

//     boost::beast::flat_buffer buffer;
//     http::response<http::string_body> res;

//     const auto [errRead, bytesRead] = co_await http::async_read(stream, buffer, res, asio::as_tuple(asio::use_awaitable));
//     if (errRead)
//         co_return std::unexpected(ErrorResponse{errRead, Stage::Write});

//     co_return res;
// }

// asio::awaitable<void> makeRequest()
// {
//     //localhost:9292/v1/models
//     std::string host = "localhost";
//     std::string port = "9292";
//     std::string target = "/v1/models";
//     std::string body = "";

//     BeastRequest req(http::verb::get, target, 11);
//     req.set(http::field::host, host);
//     req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

//     auto start = std::chrono::steady_clock::now();

//     auto resp = co_await request(host, port, std::move(req));

//     auto end = std::chrono::steady_clock::now();

//     std::cout << "Time: " << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / 1000. / 1000. << " miliseconds" << '\n';

//     if (!resp)
//         std::cout << resp.error().error << '\n';
//     else
//         std::cout << resp.value().body() << '\n';

//     co_return;
// }

// #include <tgbot/tgbot.h>

// int main()
// {
// #if _WIN32
//     std::system("chcp 1251 > nul");
// #endif

//     TgBot

//     // asio::io_context io;

//     // asio::co_spawn(io, makeRequest, [](std::exception_ptr e)
//     // {
//     // });

//     // io.run();

//     return 0;
// }