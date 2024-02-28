#include <gtest/gtest.h>
#include "../src/ib_helper/SecurityType.hpp"
#include "../src/util/OptionSymbolParser.h"
#include "../src/util/DateUtil.hpp"

TEST(SecurityTypeTests, Basics)
{
    ib_helper::SecurityType::Type t = ib_helper::SecurityType::Type::UNKNOWN;
    EXPECT_EQ(ib_helper::SecurityType::to_string(t), "UNKNOWN");
    EXPECT_EQ(ib_helper::SecurityType::to_long_string(t), "Unknown");
    t = ib_helper::SecurityType::Type::FUT;
    EXPECT_EQ(ib_helper::SecurityType::to_string(t), "FUT");
    EXPECT_EQ(ib_helper::SecurityType::to_long_string(t), "Future");
    t = ib_helper::SecurityType::Type::STK;
    EXPECT_EQ(ib_helper::SecurityType::to_string(t), "STK");
    EXPECT_EQ(ib_helper::SecurityType::to_long_string(t), "Stock");
    t = ib_helper::SecurityType::Type::FOREX;
    EXPECT_EQ(ib_helper::SecurityType::to_string(t), "CASH");
    EXPECT_EQ(ib_helper::SecurityType::to_long_string(t), "Forex");
}

TEST(SecurityTypeTests, Options)
{
    OptionDetails a("AAAAAA240101C11111111");
    EXPECT_EQ(a.underlying, "AAAAAA");
    EXPECT_EQ(a.expiry, "240101");
    EXPECT_EQ(a.type, OptionDetails::Type::CALL);
    EXPECT_DOUBLE_EQ(a.strikePrice, 11111.111);
    EXPECT_EQ(a.to_string(), "AAAAAA240101C11111111");
    OptionDetails b("b     240201P00000001");
    EXPECT_EQ(b.underlying, "b");
    EXPECT_EQ(b.expiry, "240201");
    EXPECT_EQ(b.type, OptionDetails::Type::PUT);
    EXPECT_DOUBLE_EQ(b.strikePrice, 0.001);
    EXPECT_EQ(b.to_string(), "b     240201P00000001");
}

TEST(SecurityTypeTests, DateTimeUtils)
{

    std::time_t currTime = 1709062709; // 2024-2-27 19:38:29 GMT
    EXPECT_EQ(to_4pm_ny(currTime), 1709067600); // 2024-3-1 21:00:00 GMT
    EXPECT_EQ(to_next_friday(currTime), 1709339909);
}
