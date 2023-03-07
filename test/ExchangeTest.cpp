#include "../src/ib_helper/Exchange.hpp"

#include "gtest/gtest.h"

#include <thread>
#include <chrono>

TEST(ExchangeTest, TimeFormatA)
{
    ContractDetails contractDetails;
    contractDetails.timeZoneId = "America/New_York";
    // LiquidHours can be in 2 formats
    // 0090507:0700-1830,1830-2330;20090508:CLOSED // format A
    contractDetails.liquidHours = "20230307:0930-1600;20230308:CLOSED"; // format A
    Exchange exch(contractDetails);

    time_t today = 1678206727; // 2023-3-7 4:31:52GMT (11:31:52 EST)
    EXPECT_EQ(exch.premarketStart(today), 1678165200); // midnight, 2023-3-7
    EXPECT_EQ(exch.marketOpen(today), 1678199400); // 9:30AM EST (2:30pm GMT)
    EXPECT_EQ(exch.marketClose(today), 1678222800); // 4:00PM EST
}

TEST(ExchangeTest, TimeFormatB)
{
    ContractDetails contractDetails;
    contractDetails.timeZoneId = "America/New_York";
    // LiquidHours can be in 2 formats
    // 20180323:0400-20180323:2000;20180326:0400-20180326:2000 // Format B
    contractDetails.liquidHours = "20230307:0930-20230307:1600;20230308:0930-20320308:1600"; // format B
    Exchange exch(contractDetails);

    time_t today = 1678206727; // 2023-3-7 4:31:52GMT (11:31:52 EST)
    EXPECT_EQ(exch.premarketStart(today), 1678165200); // midnight, 2023-3-7
    EXPECT_EQ(exch.marketOpen(today), 1678199400); // 9:30AM EST
    EXPECT_EQ(exch.marketClose(today), 1678222800); // 4:00PM EST
}

/*
TEST(ExchangeTest, Chicago)
{
    ContractDetails contractDetails;
    contractDetails.timeZoneId = "America/Chicago";
    // LiquidHours can be in 2 formats
    // 20180323:0400-20180323:2000;20180326:0400-20180326:2000 // Format B
    contractDetails.liquidHours = "20230307:0930-20230307:1600;20230308:0930-20320308:1600"; // format B
    Exchange exch(contractDetails);

    time_t today = 1678206727; // 2023-3-7 4:31:52GMT (11:31:52 EST)
    EXPECT_EQ(exch.premarketStart(today), 1678168800); // midnight, 2023-3-7
    EXPECT_EQ(exch.marketOpen(today), 1678203000); // 9:30AM CST
    EXPECT_EQ(exch.marketClose(today), 1678226400); // 4:00PM CST
}

TEST(ExchangeTest, ChicagoEarly)
{
    ContractDetails contractDetails;
    contractDetails.timeZoneId = "America/Chicago";
    // LiquidHours can be in 2 formats
    // 20180323:0400-20180323:2000;20180326:0400-20180326:2000 // Format B
    contractDetails.liquidHours = "20230307:0830-20230307:1500;20230308:0830-20320308:1500"; // format B
    Exchange exch(contractDetails);

    time_t today = 1678206727; // 2023-3-7 4:31:52GMT (11:31:52 EST)
    EXPECT_EQ(exch.premarketStart(today), 1678168800); // midnight, 2023-3-7
    EXPECT_EQ(exch.marketOpen(today), 1678199400); // 8:30AM CST
    EXPECT_EQ(exch.marketClose(today), 1678222800); // 3:00PM CST
}
*/
