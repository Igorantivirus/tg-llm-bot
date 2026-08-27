#include "openai/ChatsProcessor.hpp"
#include "openai/MessageSender.hpp"
#include "openai/Types.hpp"
#include <boost/asio/io_context.hpp>
#include <cstdlib>
#include <iostream>

#include <app/Application.hpp>
#include <app/ConfigReader.hpp>
#include <stop_token>
#include <string>
#include <thread>

class TestMessageSender : public openai::MessageSender
{
public:
    MessageID sendMessage(const ChatID chatId, std::string message) override
    {
        std::cout << "sendMessage: " << message << '\n';
        return ++nextId;
    }
    void replaceMessage(const ChatID chatId, const MessageID msgId, std::string message) override
    {
        std::cout << "replaceMessage: " << message << '\n';
    }
    void sendError(const ChatID chatId, utils::ErrorCode err) override
    {
        std::cout << "sendError: " << err.message() << '\n';
    }
    void replaceMessageWithError(const ChatID chatId, const MessageID msgId, std::string message, utils::ErrorCode err) override
    {
        std::cout << "replaceMessageWithError: " << message << '\n';
        std::cout << "replaceMessageWithError: " << err.message() << '\n';
    }

private:
    MessageID nextId = 0;
};

int main(int argc, char **argv)
{
#ifdef _WIN32
    std::system("chcp 65001 > nul");
#endif

    asio::io_context io;

    openai::ChatsProcessor processor(io.get_executor(), "localhost", "9292");

    TestMessageSender sender;
    processor.setSender(sender);

    std::jthread th([&io](std::stop_token t)
    {
        while (!t.stop_requested())
            io.run();
    });

    processor.initModels();

    processor.setModelToChat(1, "igor-ai");

    std::string s;
    while (true)
    {
        std::getline(std::cin, s);
        if (s == "stop")
            break;
        else if (s == "model")
        {
            std::getline(std::cin, s);
            if(s.empty())
                s = "igor-ai";
            processor.setModelToChat(1, s);
            std::cout << "Установлена модель " << s << '\n';
        }
        else
            processor.sendMessage(1, std::move(s));
    }

    return 0;
}