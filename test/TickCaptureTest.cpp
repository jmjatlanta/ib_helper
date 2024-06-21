#include <gtest/gtest.h>
#include <fstream>
#include "TickCapture.h"
#include "ContractBuilder.hpp"

TEST(TickCapture, CaptureSome)
{
    std::shared_ptr<ib_helper::IBConnector> conn = std::make_shared<ib_helper::IBConnector>( "localhost", 7497, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    ASSERT_TRUE(conn->IsConnected());

    ib_helper::ContractBuilder builder{conn.get()};
    Contract fut = builder.Build("FUT", "ES");
    std::ofstream out{"tick.out", std::ios_base::binary};
    TickCapture tc{fut, conn, out};
    // note: this test runs indefinitely
    std::this_thread::sleep_for(std::chrono::hours(24));
}

TEST(TickCapture, ReadSome)
{
    // now try to read it back in
    std::ifstream in{"tick.out", std::ios_base::binary};
    TickReader reader(in);

    while(!in.eof())
    {
        AllLast curr = reader.read();
        std::cout << to_string(curr) << "\n";
    }
}

