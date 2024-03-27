#include <gtest/gtest.h>
#include "../src/util/DateUtil.hpp"
#include "bar.h"

TEST(dateutil, ny_am)
{
    std::time_t non_dst = 1708349400; // 2024-02-19 8:30:00 local Panama (DST not active in NY)
    EXPECT_EQ(to_930am_ny(non_dst), 1708353000);
    std::time_t premarket = 1710845708; // 2024-03-19 10:55:08 local Panama (DST active in NY)
    EXPECT_EQ(to_930am_ny(premarket), 1710855000);
}

TEST(dateutil, ny_midnight)
{
    std::time_t non_dst = 1708349400; // 2024-02-19 8:30:00 local Panama (DST not active in NY)
    // midnight UTC is 1708333500, add 5 hours (18k seconds) to get midnight NY 
    EXPECT_EQ(to_midnight_ny(non_dst), 1708318800);
    std::time_t premarket = 1710845708; // 2024-03-19 10:55:08 local Panama (DST active in NY)
    EXPECT_EQ(to_midnight_ny(premarket), 1710820800);
}

TEST(dateutil, ny_close)
{
    std::time_t non_dst = 1708349400; // 2024-02-19 8:30:00 local Panama (DST not active in NY)
    // midnight UTC is 1708333500, add 5 hours (18k seconds) to get midnight NY 
    EXPECT_EQ(to_4pm_ny(non_dst), 1708376400);
    std::time_t premarket = 1710845708; // 2024-03-19 10:55:08 local Panama (DST active in NY)
    EXPECT_EQ(to_4pm_ny(premarket), 1710878400);
    std::time_t late = 1710896400; // 2024-03-19 20:00:00 local Panama (but next day in UTC)
    EXPECT_EQ(to_4pm_ny(late), 1710878400);
}

TEST(dateutil, diff)
{
    std::time_t now = 1708349400; // 2024-02-19 8:30:00 local Panama (DST not active in NY)
    EXPECT_EQ(diff_with_ny(now), -18000); // 5 hours difference
    std::time_t premarket = 1710845708; // 2024-03-19 10:55:08 local Panama (DST active in NY)
    EXPECT_EQ(diff_with_ny(premarket), -14400); // 4 hours difference
}

TEST(dateutil, string_to_time_t)
{
    std::string input = "9:30";
    std::time_t now = 1708349400; // 2024-02-19 8:30:00 local Panama (DST not active in NY)
    EXPECT_EQ(to_time_t(input, now), 1708353000); // 2024-02-19 9:30:00 NY time (and Panama time)

    now = 1710845708; // 2024-03-19 10:55:08 local Panama (DST active in NY)
    EXPECT_EQ(to_time_t(input, now), 1710855000); // 2024-03-09 9:30::00 NY time (8:30AM Panama time)
}

TEST(dateutil, bars)
{
    // bars come in 2 times:
    // YYYYMMDD for daily bars
    // time_t for bars less than 1 day
    
    // type YYYYMMDD
    Bar daily;
    daily.time = "20240327";
    EXPECT_EQ( to_time_t(daily), 1711512000); // 2024-03-27 midnight NY

    Bar minute;
    minute.time = "1711512000";
    EXPECT_EQ( to_time_t(minute), 1711512000); // 2024-03-27 midnight NY
}
