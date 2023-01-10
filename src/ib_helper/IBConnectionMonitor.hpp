#pragma once
#include <string>

class IBConnector;

class IBConnectionMonitor
{
    public:
    virtual void OnConnect(IBConnector* conn) = 0;
    virtual void OnDisconnect(IBConnector* conn) = 0;
    virtual void OnError(IBConnector* conn, const std::string& msg) = 0;
};

