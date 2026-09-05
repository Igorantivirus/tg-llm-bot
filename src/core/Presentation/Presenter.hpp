#pragma once

#include <unordered_set>

#include <utils/StreamGenerator.hpp>

#include "InfoType.hpp"
#include "OperationInfo.hpp"

namespace core
{
class Presenter
{
public:
    virtual ~Presenter() = default;

    virtual asio::awaitable<void> presentMessage(OperationInfo::Ptr, utils::StreamGenerator<std::string> &) = 0;
    virtual asio::awaitable<void> presentInfo(OperationInfo::Ptr, const InfoType) = 0;
    virtual asio::awaitable<void> presentError(OperationInfo::Ptr, const utils::ErrorCode) = 0;

    virtual asio::awaitable<void> presentModels(OperationInfo::Ptr, std::unordered_set<std::string>) = 0;
    virtual asio::awaitable<void> presentModel(OperationInfo::Ptr, std::string) = 0;
    virtual asio::awaitable<void> presentSystem(OperationInfo::Ptr, std::string) = 0;
};
} // namespace core