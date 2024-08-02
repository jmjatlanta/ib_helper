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
    virtual void OnError(IBConnector* conn, int id, int code, const std::string& msg, const std::string& json) = 0;
};

} // namespace ib_helper

