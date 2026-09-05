#pragma once

#include "TcpData.hpp"

namespace net
{
class TcpReturner
{
public:
    virtual ~TcpReturner() = default;
    virtual void returnPtr(TcpData::Ptr) = 0;
};
} // namespace net