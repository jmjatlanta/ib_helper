#include "../src/ib_helper/IBConnector.hpp"
#include "gtest/gtest.h"
#include <thread>
#include <chrono>

TEST(IBConnectorTest, Connect)
{
    ib_helper::IBConnector connector{"127.0.0.1", 4002, 5};

    std::this_thread::sleep_for(std::chrono::seconds(1));

    EXPECT_TRUE(connector.IsConnected());
}
