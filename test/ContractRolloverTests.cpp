#include "gtest/gtest.h"
#include "../src/ib_helper/ContractRolloverCalendar.hpp"

TEST(ContractRolloverTests, CurrentYYYYMM)
{
    time_t month = 60*60*24*28;

    ib_helper::ContractRolloverCalendar calendar;
    time_t now = 1672574400; // Jan 1, 2023
    std::string yyyymm = calendar.CurrentMonthYYYYMM("YM", now);
    EXPECT_EQ(yyyymm, "202303");
    now += month; // end of Jan
    yyyymm = calendar.CurrentMonthYYYYMM("YM", now);
    EXPECT_EQ(yyyymm, "202303");
    now += month; // end of Feb
    yyyymm = calendar.CurrentMonthYYYYMM("YM", now);
    EXPECT_EQ(yyyymm, "202303");
    now += month; // end of March
    yyyymm = calendar.CurrentMonthYYYYMM("YM", now);
    EXPECT_EQ(yyyymm, "202303");
    now += month; // end of April
    yyyymm = calendar.CurrentMonthYYYYMM("YM", now);
    EXPECT_NE(yyyymm, "202303");
}

TEST(ContractRolloverTests, SymbolWithSpaces)
{
    time_t month = 60*60*24*28;

    ib_helper::ContractRolloverCalendar calendar;
    time_t now = 1672574400; // Jan 1, 2023
    std::string symbol = "YM  MAR 03";
    std::string yyyymm = calendar.CurrentMonthYYYYMM(symbol, now);
    EXPECT_EQ(yyyymm, "202303");
    now += month; // end of Jan
    yyyymm = calendar.CurrentMonthYYYYMM(symbol, now);
    EXPECT_EQ(yyyymm, "202303");
    now += month; // end of Feb
    yyyymm = calendar.CurrentMonthYYYYMM(symbol, now);
    EXPECT_EQ(yyyymm, "202303");
    now += month; // end of March
    yyyymm = calendar.CurrentMonthYYYYMM(symbol, now);
    EXPECT_EQ(yyyymm, "202303");
    now += month; // end of April
    yyyymm = calendar.CurrentMonthYYYYMM(symbol, now);
    EXPECT_NE(yyyymm, "202303");
}

TEST(ContractRolloverTests, SymbolWithMonthYear)
{
    time_t month = 60*60*24*28;

    ib_helper::ContractRolloverCalendar calendar;
    time_t now = 1672574400; // Jan 1, 2023
    std::string symbol = "NQH3";
    std::string yyyymm = calendar.CurrentMonthYYYYMM(symbol, now);
    EXPECT_EQ(yyyymm, "202303");
    now += month; // end of Jan
    yyyymm = calendar.CurrentMonthYYYYMM(symbol, now);
    EXPECT_EQ(yyyymm, "202303");
    now += month; // end of Feb
    yyyymm = calendar.CurrentMonthYYYYMM(symbol, now);
    EXPECT_EQ(yyyymm, "202303");
    now += month; // end of March
    yyyymm = calendar.CurrentMonthYYYYMM(symbol, now);
    EXPECT_EQ(yyyymm, "202303");
    now += month; // end of April
    yyyymm = calendar.CurrentMonthYYYYMM(symbol, now);
    EXPECT_NE(yyyymm, "202303");
}

