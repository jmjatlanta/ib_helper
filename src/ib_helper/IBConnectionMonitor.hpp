#pragma once
#include <string>

namespace ib_helper
{

class IBConnector;

class IBConnectionMonitor
{
    public:
    virtual void OnConnect(IBConnector* conn) = 0;
    virtual void OnFullConnect(IBConnector* conn) = 0;
    virtual void OnDisconnect(IBConnector* conn) = 0;
    virtual void OnError(IBConnector* conn, const std::string& msg) = 0;
};

} // namespace ib_helper

