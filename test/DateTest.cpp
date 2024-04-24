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

TEST(dateutil, cleanTime)
{
    EXPECT_TRUE(cleanTime(std::string{}, true).empty());
    EXPECT_TRUE( cleanTime(std::string{}, false).empty() );
    EXPECT_EQ( cleanTime("0:0", false), "0:00");
    EXPECT_EQ( cleanTime("0:00", false), "0:00");
    EXPECT_EQ( to_12hr_format("0:00").first, "12:00");
    EXPECT_EQ( to_12hr_format("0:00").second, false);
    EXPECT_EQ( cleanTime("0:00", true), "12:00");
    EXPECT_EQ( cleanTime("0:01", false), "0:01");
    EXPECT_EQ( cleanTime("0:01", true), "12:01");
    EXPECT_EQ( to_12hr_format("0:01").first, "12:01");
    EXPECT_EQ( to_12hr_format("0:01").second, false);
    EXPECT_EQ( cleanTime("1:01", false), "1:01");
    EXPECT_EQ( to_12hr_format("1:01").first, "1:01");
    EXPECT_EQ( to_12hr_format("1:01").second, false);
    EXPECT_EQ( cleanTime("1:01", true), "13:01");
    EXPECT_EQ( to_12hr_format("13:01").first, "1:01");
    EXPECT_EQ( to_12hr_format("13:01").second, true);
    EXPECT_EQ( cleanTime("12:00", false), "0:00");
    EXPECT_EQ( to_12hr_format("12:00").first, "12:00");
    EXPECT_EQ( to_12hr_format("12:00").second, true);
    EXPECT_EQ( cleanTime("12:01", false), "0:01");
    EXPECT_EQ( to_12hr_format("12:01").first, "12:01");
    EXPECT_EQ( to_12hr_format("12:01").second, true);
    EXPECT_EQ( cleanTime("12:00", true), "12:00");
    EXPECT_EQ( cleanTime("12:01", true), "12:01");
    EXPECT_EQ( cleanTime("13:00", true), "13:00");
    EXPECT_EQ( cleanTime("13:00", false), "13:00");
    EXPECT_EQ( to_12hr_format("13:01").first, "1:01");
    EXPECT_EQ( to_12hr_format("13:01").second, true);
}
