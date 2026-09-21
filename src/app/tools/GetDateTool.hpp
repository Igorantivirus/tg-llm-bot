#pragma once

#include <chrono>

#include <openai/Tools/Tool.hpp>

namespace openai
{
class GetDateTool : public Tool
{
public:
    utils::AsyncResult<std::string> run(std::string args) override
    {
        co_return getCurrentDate();
    }
    std::string name() const override
    {
        return "get_current_date";
    }
    std::string description() const override
    {
        return "Получить текущую дату в формате <d.m.y>";
    }

private:
    static std::string getCurrentDate()
    {
        auto        now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);

        std::tm local_time{};
#ifdef _WIN32 // Windows: localtime_s
        localtime_s(&local_time, &now_time);
#else // Linux/Android/POSIX: localtime_r
        localtime_r(&now_time, &local_time);
#endif

        std::ostringstream oss;
        oss << std::put_time(&local_time, "%d.%m.%Y");
        return oss.str();
    }
};
} // namespace openai