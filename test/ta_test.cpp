/***
 * Tests for ta_lib functionality
 */
#include "gtest/gtest.h"
#include "../ta-lib/include/ta_libc.h"

#include <thread>
#include <chrono>

TEST(ta_test, stddevTest)
{
    double inReal[] = { 0.684808,  // 0
            0.613100, 
            -0.016941, 
            -0.519221, 
            2.147814, 
            -0.393357,  // 5
            0.123275, 
            0.707619, 
            2.157508,
            -0.138655, 
            -0.028071 }; // 10
    int startIdx = 0;
    int endIdx = 10;
    int optInTimePeriod = 10; // 2 to 100,000, in this case start calculating when you have 10 values (index 9)
    double optInNbDev = 1; // TA_REAL_MIN to TA_REAL_MAX
    int outBegIdx; // where it started providing results
    int outNbElement; // number of results in outReal
    double outReal[11];
    TA_RetCode retCode = TA_STDDEV(startIdx, endIdx, inReal, optInTimePeriod, optInNbDev, 
            &outBegIdx, &outNbElement, outReal);
    EXPECT_EQ( retCode, TA_RetCode::TA_SUCCESS);
    EXPECT_EQ( outBegIdx, 9);
    EXPECT_EQ( outNbElement, 2);
    for(int i = 0; i < 11; ++i)
    {
        std::cout << "In at " + std::to_string(i) + ": " + std::to_string(inReal[i]);
        if (i - outBegIdx >= 0)
            std::cout << " Value at " + std::to_string(i - outBegIdx) + ": " + std::to_string(outReal[i-outBegIdx]);
        std::cout << "\n";
    }
    EXPECT_EQ( outReal[0], 0.905163 );
    EXPECT_EQ( outReal[1], 0.918655 );
}

template<typename T>
std::vector<T> lastN(const std::vector<T>& in, int n)
{
    auto sz = in.size();
    const T* start = &(in.data())[sz-n];
    return std::vector<T>(start, start + n);
}

TEST(ta_test, vectorCleanTest)
{
    std::vector<double> entries;
    for(double i = 0; i < 100; i++)
    {
        entries.push_back(i);
    }
    // we want to build a vector with only 10
    auto sz = entries.size();
    EXPECT_EQ(100, entries.size());
    EXPECT_EQ(0, entries[0]);
    EXPECT_EQ(99, entries[99]);
    if (sz >= 10 * 10)
    {
        entries = lastN(entries, 10);
        EXPECT_EQ(10, entries.size());
        EXPECT_EQ(90, entries[0]);
        EXPECT_EQ(91, entries[1]);
        EXPECT_EQ(92, entries[2]);
        EXPECT_EQ(93, entries[3]);
        EXPECT_EQ(94, entries[4]);
        EXPECT_EQ(95, entries[5]);
        EXPECT_EQ(96, entries[6]);
        EXPECT_EQ(97, entries[7]);
        EXPECT_EQ(98, entries[8]);
        EXPECT_EQ(99, entries[9]);
    }
    else
    {
        FAIL();
    }
}

