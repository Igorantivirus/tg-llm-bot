#include "magic_enum/magic_enum.hpp"
#include "net/Types.hpp"
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
#include <openaiprocessor/OpenAiApi.hpp>
#include <openaiprocessor/dto/ChatCompletionsRequest.hpp>

#include <boost/asio.hpp>
#include <net/Types.hpp>
#include <string>
#include <type_traits>
#include <utils/MethodBinder.hpp>

struct Settings
{
    struct TimeOut
    {
        constexpr static std::chrono::steady_clock::duration noLimit = std::chrono::steady_clock::duration::max();

        std::chrono::steady_clock::duration resolve = std::chrono::seconds(5);
        std::chrono::steady_clock::duration connect = std::chrono::seconds(10);
        std::chrono::steady_clock::duration request = std::chrono::seconds(30);
        std::chrono::steady_clock::duration header = std::chrono::seconds(60);
        std::chrono::steady_clock::duration chunk = std::chrono::seconds(160);
        std::chrono::steady_clock::duration body = std::chrono::seconds(120);
    };
    struct Size
    {
        constexpr static std::size_t noLimit = std::numeric_limits<std::size_t>::max();

        std::size_t header = 8192;
        std::size_t body = 8 * 1024 * 1024;
        std::size_t chunkOne = 16 * 1024;
        std::size_t chunkCount = noLimit;
        std::size_t chunkTotal = noLimit;
    };

    TimeOut timeout;
    Size    size;
};

class HttpStream
{
public:
    struct Response
    {
        http::response_header<>    header;
        std::optional<std::string> body; // nullopt - chuncked, else - body (maybe empty)
    };

public:
    HttpStream(asio::any_io_executor ex, Settings setts = {})
        : stream_(std::move(ex)),
          setts_(std::move(setts)),
          chunkCb_(utils::buildMethod(&HttpStream::chunkCb, this)),
          headerCb_(utils::buildMethod(&HttpStream::headerCb, this))
    {
    }
    ~HttpStream()
    {
        close();
    }
    HttpStream(const HttpStream &) = delete;
    HttpStream(HttpStream &&) = delete;
    HttpStream &operator=(const HttpStream &) = delete;
    HttpStream &operator=(HttpStream &&) = delete;

    bool setSettings(Settings setts)
    {
        if (reading_)
            return false;
        setts_ = std::move(setts);
        return true;
    }

    template <typename F = decltype(nullptr)>
    asio::awaitable<std::expected<Response, net::ErrorResponse>> request(const std::string_view host, const std::string_view port, net::BeastRequest req, F socketConfigurator = nullptr)
    {
        close();

        // connect
        if (auto resolveRes = co_await resolve(stream_.get_executor(), host, port, setts_.timeout.resolve); resolveRes)
        {
            setTimeOut(setts_.timeout.connect);
            const auto [err, connectionRes] = co_await stream_.async_connect(resolveRes.value(), asio::as_tuple(asio::use_awaitable));
            if (err)
                co_return std::unexpected(net::ErrorResponse{err, net::Stage::Connect});
            if constexpr (!std::is_null_pointer_v<F>) // Продвинутая настройка, если надо
                socketConfigurator(stream_.socket());
        }
        else
            co_return std::unexpected(net::ErrorResponse{resolveRes.error(), net::Stage::Resolve});
        // write request
        setTimeOut(setts_.timeout.request);
        const auto [errWrite, bytesWrite] = co_await http::async_write(stream_, std::move(req), asio::as_tuple(asio::use_awaitable));
        if (errWrite)
            co_return std::unexpected(net::ErrorResponse{errWrite, net::Stage::Write});
        // read headers
        setTimeOut(setts_.timeout.header);
        const auto [errRead, bytesReads] = co_await http::async_read_header(stream_, buffer_, *parser_, asio::as_tuple(asio::use_awaitable));
        if (errRead)
            co_return std::unexpected(net::ErrorResponse{errRead, net::Stage::ReadHead});

        if (!parser_->chunked()) // Чтение сразу и возврат
            co_return co_await readResult();

        reading_ = true;
        co_return Response{.header = parser_->get().base(), .body = std::nullopt}; // Дальше чтение чанково
    }

    asio::awaitable<std::expected<std::optional<std::string>, net::ErrorResponse>> nextChunk()
    {
        while (reading_)
        {
            setTimeOut(setts_.timeout.chunk);
            const auto [err, bytesRead] = co_await http::async_read_some(stream_, buffer_, *parser_, asio::as_tuple(asio::use_awaitable));
            if (err == http::error::end_of_chunk)
                co_return std::move(chunk_);
            if (err)
            {
                close();
                co_return std::unexpected(net::ErrorResponse{err, net::Stage::ReadChunk});
            }
            if (parser_->is_done())
                close();
        }
        co_return std::nullopt;
    }

    void close()
    {
        reading_ = false;
        buffer_.clear();
        chunk_.clear();
        stream_.close();
        chunkCount_ = 0;
        chunkTotal_ = 0;

        resetParser();
    }

private:
    Settings setts_;

    bool reading_ = false;

    beast::tcp_stream                                             stream_;
    std::optional<http::response_parser<beast::http::empty_body>> parser_;
    boost::beast::flat_buffer                                     buffer_;

    std::string chunk_;
    std::size_t chunkCount_ = 0;
    std::size_t chunkTotal_ = 0;

    std::function<std::size_t(std::uint64_t, std::string_view, boost::beast::error_code &)> chunkCb_;
    std::function<void(std::uint64_t, std::string_view, boost::beast::error_code &)>        headerCb_;

private:
    std::size_t chunkCb(std::uint64_t remain, std::string_view body, boost::beast::error_code &ec)
    {
        ec = {};
        chunk_.append(body);
        if (remain == body.size())
            ec = http::error::end_of_chunk;
        return body.size();
    }
    void headerCb(std::uint64_t size, std::string_view extensions, boost::beast::error_code &ec)
    {
        chunk_.clear();
        if (chunkCount_ > setts_.size.chunkCount) // Число чанков
        {
            ec = boost::beast::http::error::body_limit;
            return;
        }
        if (size > setts_.size.chunkOne) // Размер одного чанка
        {
            ec = boost::beast::http::error::body_limit;
            return;
        }
        if (chunkTotal_ > setts_.size.chunkTotal) // Общая сумма чанков
        {
            ec = boost::beast::http::error::body_limit;
            return;
        }
        ec = {};
        ++chunkCount_;
        chunkTotal_ += size;
        chunk_.reserve(size);
    }

    void resetParser()
    {
        parser_.emplace();
        parser_->on_chunk_body(chunkCb_);
        parser_->on_chunk_header(headerCb_);

        parser_->body_limit(boost::none);
        parser_->header_limit(setts_.size.header);
    }

    asio::awaitable<std::expected<Response, net::ErrorResponse>> readResult()
    {
        http::response_parser<http::string_body> parser(std::move(*parser_));
        parser.body_limit(setts_.size.body);
        setTimeOut(setts_.timeout.body);
        const auto [errRead, bytesRead] = co_await http::async_read(stream_, buffer_, parser, asio::as_tuple(asio::use_awaitable));
        if (errRead)
            co_return std::unexpected(net::ErrorResponse{errRead, net::Stage::ReadBody});
        auto res = parser.release();
        co_return Response{.header = std::move(res.base()), .body = std::move(res.body())};
    }

    void setTimeOut(std::chrono::steady_clock::duration dur)
    {
        if (dur == Settings::TimeOut::noLimit)
            stream_.expires_never();
        else
            stream_.expires_after(dur);
    }

private:
    static asio::awaitable<std::expected<asio::ip::basic_resolver_results<tcp>, beast::error_code>> resolve(asio::any_io_executor ex, const std::string_view host, const std::string_view port, const std::chrono::steady_clock::duration timeout)
    {
        tcp::resolver resolver(ex);
        const auto [err, resolve] = co_await resolver.async_resolve(host, port, asio::cancel_after(timeout, asio::as_tuple(asio::use_awaitable)));
        if (err)
            co_return std::unexpected(err);
        co_return resolve;
    }
};

void requestStream(const std::string_view host, const std::string_view port, net::BeastRequest req)
{
    asio::io_context io;

    tcp::resolver resolver(io);

    boost::beast::error_code ec;

    const auto resolve = resolver.resolve(host, port, ec);
    if (ec)
        return;

    beast::tcp_stream stream(io);
    stream.connect(resolve, ec);
    if (ec)
        return;

    const auto bytesWrite = http::write(stream, std::move(req), ec);
    if (ec)
        return;

    boost::beast::flat_buffer buffer;

    boost::beast::http::response_parser<boost::beast::http::string_body> parser;

    auto callback1 = [](std::uint64_t remain, std::string_view body, boost::beast::error_code &ec)
    {
        ec = {};
        std::cout << "CHUNK_START|" << body << "|CHUNK_END\n";
        return body.size();
    };
    parser.on_chunk_body(callback1);
    auto callback2 = [](std::uint64_t size, std::string_view extensions, boost::beast::error_code &ec)
    {
        ec = {};
        //...
        std::cout << "-------------------HEADER------------------- " << size << " " << extensions << '\n';
    };
    parser.on_chunk_header(callback2);
    parser.body_limit(boost::none);

    // parser.on_chunk_body([&](std::uint64_t remain, std::string_view body, boost::beast::error_code &ec)
    // {
    //     // accumulator.append(body.data(), body.size());
    //     // extractSseEvents(accumulator); // твоя логика склейки
    //     // return body.size(); // сколько байт ты забрал
    // });

    const auto bytesReads = http::read_header(stream, buffer, parser, ec);
    if (ec)
        return;

    if (!parser.chunked())
    {
        std::cout << "not chunked\n";
        return;
    }

    std::cout << "-------------------START-----------\n\n";

    while (!parser.is_done())
    {
        http::read_some(stream, buffer, parser, ec);
        if (ec == http::error::end_of_chunk)
            ec = {}; // штатное: чанк закончился
        if (ec)
            break;

        // http::read_some(stream, buffer, parser, ec);
        // if (ec)
        // {
        //     std::cout << "END\n";
        //     std::cout << ec << '\n';
        //     break;
        // }
        // else
        //     std::cout << "Next line:\n|"
        //               << parser.get().body() << "|\nend line\n";
        // // buffer.clear();
    }
    auto res = parser.release();
    std::cout << res << '\n';
}

constexpr bool isNum(const std::string &s)
{
    int res;
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), res);
    return ec == std::errc{} && ptr == s.data() + s.size();
}

void initRequestFields(net::BeastRequest &req, const std::string fullHost)
{
    req.set(http::field::host, fullHost);
    req.set(http::field::content_type, "application/json");
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.prepare_payload();
}

void chatCompletions(dto::ChatCompletionsRequest dto, const std::string fullHost)
{
    net::BeastRequest req(http::verb::post, "/v1/chat/completions", 11);
    if (std::optional<std::string> sdto = dto::serialize(dto); sdto)
    {
        std::cout << *sdto << '\n';
        req.body() = *sdto;
    }
    else
        return;
    initRequestFields(req, fullHost);

    requestStream("localhost", "9292", std::move(req));
}

// auto body_cb = [&](std::uint64_t remain, std::string_view body, beast::error_code &ec)
// {
//     if (remain == body.size())
//     {
//         ec = http::error::end_of_chunk; // сигнал наверх
//         return 0;
//     }
//     ec = {};
//     chunk.append(body);
//     return body.size();
// };

asio::awaitable<void> foo(const std::string_view host, const std::string_view port)
{
    dto::ChatCompletionsRequest dto;
    dto.model = "igor-ai";
    dto.messages.push_back(dto::Message{.role = "system", .content = "Ответь кратко"});
    dto.messages.push_back(dto::Message{.role = "user", .content = "Привет, как дела?"});
    dto.stream = false;

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
    if(!res)
    {
        std::cout << "Error: " << res.error().error << ". Stage: "<< magic_enum::enum_name(res.error().stage) << '\n';
        co_return;
    }
    auto value = res.value();
    if(value.body)
        std::cout << "Onbe elem: " << value.header << '\n' << value.body.value() << '\n';
    else
    {
        for(auto next = co_await stream.nextChunk(); next && next.value(); next = co_await stream.nextChunk())
        {
            std::cout << "Next chunk: " << next.value().value() << '\n';
        }
    }

    // auto res = co_await stream.request(host, port, std::move(req));
    // if (!res)
    // {
    //     std::cout << "Error " << res.error().error << '\n';
    //     co_return;
    // }
    // for (auto next = co_await stream.nextChunk(); next && next; next = co_await stream.nextChunk())
    // {
    //     std::cout << "Next chunk: " << next.value() << '\n';
    // }

    // dto::ChatCompletionsRequest dto;
    // dto.model = "saiga-nemo-12b";
    // dto.messages.push_back(dto::Message{.role = "system", .content = "Ответь с нижними подчёркиваниями"});
    // dto.messages.push_back(dto::Message{.role = "user", .content = "Привет, как дела?"});

    // OpenAiApi api("localhost", "9292");

    // std::optional<dto::ModelsResponse> res = co_await api.models();

    // if (!res)
    // {
    //     std::cout << "error models" << '\n';
    //     co_return;
    // }
    // for (const auto &i : res.value().data)
    //     std::cout << i.id << '\n';

    // std::string resp = co_await api.chatCompletions(std::move(dto));
    // std::cout << resp << '\n';
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

// io.run();

// std::cout << openAi.modelsLoaded() << '\n';

// if (argc != 2)
// {
//     std::cerr << "First argument must be path to json config of application.";
//     return EXIT_FAILURE;
// }
// auto config = config::read<config::AppConfig>(argv[1]);
// if (!config)
// {
//     std::cerr << "Read config error: " << config.error();
//     return EXIT_FAILURE;
// }

// Bot bot(std::move(config.value()));
// bot.run();

// #include <tgbot/HttpClient.h>
// #include <tgbot/TgLongPoll.h>

// constexpr const char *BOT_TOKEN = "";

// #include <cstdlib>
// #include <exception>
// #include <iostream>
// #include <memory>
// #include <string>

// #include <tgbot/tgbot.h>

// int main()
// {
// #ifdef _WIN32
//     std::system("chcp 65001 > nul");
// #endif
//     const auto token = std::string(BOT_TOKEN);
//     // std::cout << "Token: " << token << std::endl;

//     // getDefaulf

//     TgBot::Bot bot(token);
//     bot.getEvents().onCommand("start", [&bot](std::shared_ptr<TgBot::Message> message)
//     {
//         bot.getApi().sendMessage(message->chat->id, "Hi!");
//     });
//     bot.getEvents().onAnyMessage([&bot](std::shared_ptr<TgBot::Message> message)
//     {
//         const auto text = message->text.value_or("");
//         std::cout << "User wrote " << text << std::endl;
//         if (text.starts_with("/start"))
//         {
//             return;
//         }
//         bot.getApi().sendMessage(message->chat->id, "Your message is: " + text);
//     });

//     const auto handleError = [](const std::exception &error)
//     {
//         std::cout << "error: " << error.what() << std::endl;
//     };

//     TgBot::TgLongPoll longPoll(bot);
//     try
//     {
//         while (true)
//         {
//             longPoll.start();
//         }
//     }
//     catch (...)
//     {

//     }

//     // try
//     // {
//     //     std::cout << "Bot username: " << bot.getApi().getMe()->username.value_or("") << std::endl;
//     //     bot.getApi().deleteWebhook();
//     //     while (true)
//     //     {

//     //         TgBot::TgLongPoll longPoll(bot, 100, 10);
//     //         longPoll.start();
//     //         std::cout << "end\n";
//     //     }
//     // }
//     // catch (const std::exception &error)
//     // {
//     //     handleError(error);
//     //     return EXIT_FAILURE;
//     // }

//     return EXIT_SUCCESS;
// }

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