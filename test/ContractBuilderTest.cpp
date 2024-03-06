#include <gtest/gtest.h>
#include "../src/ib_helper/ContractBuilder.hpp"

TEST(contract_builder, DISABLED_unknown_exchange)
{
    // connect to IB
    ib_helper::IBConnector conn("localhost", 7497, 3);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    ib_helper::ContractBuilder contractBuilder(&conn);
    auto contract = contractBuilder.BuildStock("real");
    EXPECT_NE(contract.conId, 0);
}