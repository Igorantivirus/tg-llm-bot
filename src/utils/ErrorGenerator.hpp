#pragma once

#include <cstdint>

#include <boost/system/detail/error_category.hpp>
#include <boost/system/detail/error_code.hpp>
#include <boost/system/is_error_code_enum.hpp>

#include "ErrorNameHelper.hpp"
#include <array>

#ifndef UTILS_GENERATE_ERRORS
#define UTILS_GENERATE_ERRORS(NAMESPACE, CODE)                                    \
    namespace NAMESPACE                                                           \
    {                                                                             \
                                                                                  \
    class ErrorCategoryImpl final : public boost::system::error_category          \
    {                                                                             \
    public:                                                                       \
        ErrorCategoryImpl() noexcept                                              \
            : boost::system::error_category(CODE)                                 \
        {                                                                         \
        }                                                                         \
                                                                                  \
        const char *name() const noexcept override                                \
        {                                                                         \
            return #NAMESPACE;                                                    \
        }                                                                         \
                                                                                  \
        std::string message(int ev) const override                                \
        {                                                                         \
            return utils::getName(errorNames, ev);                                \
        }                                                                         \
                                                                                  \
        inline static const boost::system::error_category &getCategory() noexcept \
        {                                                                         \
            static const ErrorCategoryImpl instance;                              \
            return instance;                                                      \
        }                                                                         \
    };                                                                            \
                                                                                  \
    inline boost::system::error_code make_error_code(const Error e) noexcept      \
    {                                                                             \
        return {static_cast<int>(e), ErrorCategoryImpl::getCategory()};           \
    }                                                                             \
    }                                                                             \
                                                                                  \
    template <>                                                                   \
    struct boost::system::is_error_code_enum<NAMESPACE::Error> : std::true_type   \
    {                                                                             \
    };
#endif