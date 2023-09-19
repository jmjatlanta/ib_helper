#pragma once
#include <string>
#include "../src/ib_helper/IBConnector.hpp"

struct ib_options
{
    std::string host{"127.0.0.1"};
    int port = 4002;
    int connId = 1;
};


bool isConnected(const ib_helper::IBConnector& conn);
