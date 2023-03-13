#include "HistoricalBarFeeder.h"
#include <gtest/gtest.h>

TEST(HistoricalBarFeederTest, WriteRead)
{
    {
        HistoricalBarFileWriter writer("temp.csv");
        Bar b{"1", 11.0, 9.0, 10.0, 9.5, doubleToDecimal(0.0), doubleToDecimal(100.0), 1};
        writer.Write(b);
        b.time = "2";
        b.open = b.close;
        b.close = 10.0;
        writer.Write(b);
    }
    // now read it back
    HistoricalBarFileReader feeder("temp.csv");
    Bar b = feeder.NextBar();
    EXPECT_EQ(b.time, "1");
    EXPECT_EQ(b.high, 11.0);
    EXPECT_EQ(b.low, 9.0);
    EXPECT_EQ(b.open, 10.0);
    EXPECT_EQ(b.close, 9.5);
    EXPECT_EQ(decimalToDouble(b.wap), 0.0);
    EXPECT_EQ(decimalToDouble(b.volume), 100.0);
    EXPECT_EQ(b.count, 1);
    try
    {
    b = feeder.NextBar();
        EXPECT_EQ(b.time, "2");
        EXPECT_EQ(b.high, 11.0);
        EXPECT_EQ(b.low, 9.0);
        EXPECT_EQ(b.open, 9.5);
        EXPECT_EQ(b.close, 10.0);
        EXPECT_EQ(decimalToDouble(b.wap), 0.0);
        EXPECT_EQ(decimalToDouble(b.volume), 100.0);
        EXPECT_EQ(b.count, 1);
    } catch(...) {
        FAIL();
    }

}
