#include "../src/ib_helper/SecurityType.hpp"
#include <gtest/gtest.h>

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

