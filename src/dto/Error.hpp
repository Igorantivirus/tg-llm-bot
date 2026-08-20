#pragma once

#include <array>
#include <boost/system/detail/error_category.hpp>
#include <boost/system/detail/error_code.hpp>
#include <boost/system/is_error_code_enum.hpp>
#include <cstdint>
#include <utils/ErrorNameHelper.hpp>

namespace dto
{
enum class Error : std::uint8_t
{
    Success = 0,
    JsonserSerialize,
    JsonserDeserialize,
    NlohmannParsing,
    NlohmannBuild,
    Unknown
};

class ErrorCategoryImpl final : public boost::system::error_category
{
public:
    ErrorCategoryImpl() noexcept
        : boost::system::error_category(0x6a3f12d7b8e04d40ULL)
    {
    }

    const char *name() const noexcept override
    {
        return "dto";
    }

    std::string message(int ev) const override
    {
        return utils::getName(names, ev);
    }

    inline static const boost::system::error_category &getCategory() noexcept
    {
        static const ErrorCategoryImpl instance;
        return instance;
    }

private:
    constexpr static const std::array<const char *, static_cast<std::size_t>(Error::Unknown) + 1> names = {
        "Success",
        "Serialisation in jsonser",
        "Deserialisation in jsonser",
        "Json parsing",
        "Building json from structure",
        "Unknown"};
};

inline boost::system::error_code make_error_code(const Error e) noexcept
{
    return {static_cast<int>(e), ErrorCategoryImpl::getCategory()};
}

} // namespace dto

template <>
struct boost::system::is_error_code_enum<dto::Error> : std::true_type
{
};