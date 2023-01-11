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
    int optInTimePeriod = 10; // 2 to 100,000
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
