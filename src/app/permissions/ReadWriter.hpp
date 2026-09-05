#pragma once

#include <exception>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include <nlohmann/json.hpp>
#include <utils/Jsonser.hpp>

#include <app/permissions/Permissions.hpp>

namespace permissions
{
class ReadWriter
{
public:
public:
    ReadWriter(Permissions &data, std::string fileName)
        : data_(data), fileName_(std::move(fileName))
    {
    }

    void save()
    {
        try
        {
            nlohmann::json j;
            jsonser::Serialize::toJson(j, data_.data_);
            std::ofstream out(fileName_);
            if (out.is_open())
                out << j;
            else
                throw std::logic_error("Invalid file name");
        }
        catch (const std::exception &ex)
        {
            std::cerr << "Error: " << ex.what() << '\n';
        }
        catch (...)
        {
            std::cerr << "Unknown error\n";
        }
    }

    void read()
    {
        Data data;
        try
        {
            nlohmann::json j;

            std::ifstream in(fileName_);
            if (in.is_open())
                in >> j;
            else
                throw std::logic_error("Invalid file name");
            jsonser::Deserialize::fromJson(j, data);
        }
        catch (const std::exception &ex)
        {
            std::cerr << "Error: " << ex.what() << '\n';
            return;
        }
        catch (...)
        {
            std::cerr << "Unknown error\n";
            return;
        }
        data_.data_ = std::move(data);
    }

private:
    Permissions &data_;
    std::string  fileName_;
};
} // namespace permissions