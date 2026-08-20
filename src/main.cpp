#include "dto/Role.hpp"
#include "openai/ChatsProcessor.hpp"
#include "openai/MessageSender.hpp"
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
#include <dto/ChatCompletionsRequest.hpp>
#include <expected>
#include <iostream>
#include <magic_enum/magic_enum.hpp>
#include <net/Types.hpp>
#include <openai/Api.hpp>
#include <string>
#include <utils/MethodBinder.hpp>

// void initRequestFields(net::BeastRequest &req, const std::string fullHost)
// {
//     req.set(http::field::host, fullHost);
//     req.set(http::field::content_type, "application/json");
//     req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
//     req.prepare_payload();
// }

// asio::awaitable<void> foo(const std::string_view host, const std::string_view port)
// {
//     dto::ChatCompletionsRequest dto;
//     dto.model = "igor-ai";
//     dto.messages.push_back(dto::Message{.role = "system", .content = "Ответь кратко"});
//     dto.messages.push_back(dto::Message{.role = "user", .content = "Привет, как дела?"});
//     dto.stream = true;

//     net::BeastRequest req(http::verb::post, "/v1/chat/completions", 11);
//     if (std::optional<std::string> sdto = dto::serialize(dto); sdto)
//     {
//         std::cout << "resuest: " << *sdto << '\n';
//         req.body() = *sdto;
//     }
//     else
//         co_return;
//     std::string fullHost;
//     fullHost.append(host);
//     fullHost.push_back(':');
//     fullHost.append(port);

//     initRequestFields(req, fullHost);

//     HttpStream stream(co_await boost::asio::this_coro::executor);

//     auto res = co_await stream.request(host, port, std::move(req));
//     if (!res)
//     {
//         std::cout << "Error: " << res.error().error << ". Stage: " << magic_enum::enum_name(res.error().stage) << '\n';
//         co_return;
//     }
//     auto value = res.value();
//     if (value.body)
//         std::cout << "Onbe elem: " << value.header << '\n'
//                   << value.body.value() << '\n';
//     else
//     {
//         for (auto next = co_await stream.nextChunk(); next && next.value(); next = co_await stream.nextChunk())
//         {
//             std::cout << "Next chunk: " << next.value().value() << '\n';
//         }
//     }
// }

class SonsoleSender : public openai::MessageSender
{
public:
    MessageID sendMessage(const ChatID chatId, std::string message)
    {
        std::cout << "sendMessage: " << message << "\n\n";
        return 0;
    }
    void addToMessage(const ChatID chatId, const MessageID msgId, std::string message)
    {
        std::cout << "addToMessage: " << message << "\n\n";
    }
    void replaceMessage(const ChatID chatId, const MessageID msgId, std::string message)
    {
        std::cout << "replaceMessage: " << message << "\n\n";
    }

    MessageID sendError(const ChatID chatId, Error ec)
    {
        std::cout << "sendError: " << toString(ec) << "\n\n";
        return 0;
    }
    void addToError(const ChatID chatId, const MessageID msgId, Error ec)
    {
        std::cout << "addToError: " << toString(ec) << "\n\n";
    }
    void replaceError(const ChatID chatId, const MessageID msgId, Error ec)
    {
        std::cout << "replaceError: " << toString(ec) << "\n\n";
    }
};

asio::awaitable<void> test1(const std::string_view host, const std::string_view port)
{
    SonsoleSender          sender;
    openai::ChatsProcessor chats(co_await boost::asio::this_coro::executor, sender, std::string(host), std::string(port));

    if (auto res = co_await chats.initModels(); !res)
        std::cout << res << '\n';

    chats.setModel(0, "igor-ai");

    if(auto res = co_await chats.sendMessage(0, "привет, как дела?");!res)
        std::cout << "Error: " << res << '\n';

    co_return;
}

asio::awaitable<void> foo(const std::string_view host, const std::string_view port)
{
    openai::Api api(co_await boost::asio::this_coro::executor, std::string(host), std::string(port));

    dto::ChatCompletionsRequest dto;
    dto.model = "igor-ai";
    dto.messages.push_back(dto::Message{.role = dto::Role::system, .content = "Ответь кратко"});
    dto.messages.push_back(dto::Message{.role = dto::Role::user, .content = "Привет, как дела?"});
    dto.stream = true;

    auto res = co_await api.chatCompletions(std::move(dto));
    if (!res)
    {
        std::cout << "Error\n";
        co_return;
    }
    auto gen = std::get<1>(res.value());
    for (auto next = co_await gen.next(); next; next = co_await gen.next())
    {
        std::cout << "Next value: " << *dto::serialize(*next) << "\n\n";
    }
}

int main(int argc, char **argv)
{
#ifdef _WIN32
    std::system("chcp 65001 > nul");
#endif

    // dto::ChatCompletionsRequest dto;
    // dto.model = "igor-ai";
    // dto.messages.push_back(dto::Message{.role = "system", .content = "Ответь кратко"});
    // dto.messages.push_back(dto::Message{.role = "user", .content = "Привет, как дела?"});
    // dto.stream = true;

    // chatCompletions(std::move(dto), "localhost:9292");

    asio::io_context io;

    asio::co_spawn(io, test1("100.104.60.3", "9292"), [](std::exception_ptr ptr)
    {
    });

    io.run();

    return EXIT_SUCCESS;
}