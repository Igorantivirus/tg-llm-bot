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
#include <chrono>
#include <expected>
#include <functional>
#include <limits>
#include <magic_enum/magic_enum.hpp>
#include <net/Types.hpp>
#include <openaiprocessor/OpenAiApi.hpp>
#include <openaiprocessor/dto/ChatCompletionsRequest.hpp>
#include <string>
#include <type_traits>
#include <utils/MethodBinder.hpp>





void initRequestFields(net::BeastRequest &req, const std::string fullHost)
{
    req.set(http::field::host, fullHost);
    req.set(http::field::content_type, "application/json");
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.prepare_payload();
}

asio::awaitable<void> foo(const std::string_view host, const std::string_view port)
{
    dto::ChatCompletionsRequest dto;
    dto.model = "igor-ai";
    dto.messages.push_back(dto::Message{.role = "system", .content = "Ответь кратко"});
    dto.messages.push_back(dto::Message{.role = "user", .content = "Привет, как дела?"});
    dto.stream = true;

    net::BeastRequest req(http::verb::post, "/v1/chat/completions", 11);
    if (std::optional<std::string> sdto = dto::serialize(dto); sdto)
    {
        std::cout << "resuest: " << *sdto << '\n';
        req.body() = *sdto;
    }
    else
        co_return;
    std::string fullHost;
    fullHost.append(host);
    fullHost.push_back(':');
    fullHost.append(port);

    initRequestFields(req, fullHost);

    HttpStream stream(co_await boost::asio::this_coro::executor);

    auto res = co_await stream.request(host, port, std::move(req));
    if (!res)
    {
        std::cout << "Error: " << res.error().error << ". Stage: " << magic_enum::enum_name(res.error().stage) << '\n';
        co_return;
    }
    auto value = res.value();
    if (value.body)
        std::cout << "Onbe elem: " << value.header << '\n'
                  << value.body.value() << '\n';
    else
    {
        for (auto next = co_await stream.nextChunk(); next && next.value(); next = co_await stream.nextChunk())
        {
            std::cout << "Next chunk: " << next.value().value() << '\n';
        }
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

    asio::co_spawn(io, foo("100.104.60.3", "9292"), [](std::exception_ptr ptr)
    {
    });

    io.run();

    return EXIT_SUCCESS;
}