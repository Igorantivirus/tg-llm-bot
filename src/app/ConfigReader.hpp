#pragma once

#include <expected>
#include <span>

#include <app/config/AppConfig.hpp>
#include <app/config/ConfigReader.hpp>

namespace app
{

class ConfigReader
{
public:
    static std::expected<config::AppConfig, std::string> read(std::span<char *> args)
    {
        if (args.size() != 2)
            return std::unexpected("There should be 2 arguments.");

        auto configOpt = config::read<config::AppConfig>(args[1]);
        if (!configOpt)
            return std::unexpected(configOpt.error());
        return configOpt.value();
    }
};

} // namespace app