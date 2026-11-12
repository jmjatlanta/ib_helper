#include "../src/ib_helper/IbError.hpp"
#include <gtest/gtest.h>

/***
 * Tests for the TWS error classifier. None of these need a running TWS.
 */

using ib_helper::IbSeverity;
using ib_helper::classify;

TEST(IbErrorTest, FarmStatusIsInfo)
{
    EXPECT_EQ(classify(2104, "Market data farm connection is OK:usfarm"), IbSeverity::Info);
    EXPECT_EQ(classify(2106, "HMDS data farm connection is OK:ushmds"), IbSeverity::Info);
    EXPECT_EQ(classify(2158, "Sec-def data farm connection is OK:secdefnj"), IbSeverity::Info);
}

TEST(IbErrorTest, Other21xxAreWarnings)
{
    EXPECT_EQ(classify(2103, "Market data farm connection is broken:usfarm"), IbSeverity::Warning);
    EXPECT_EQ(classify(2109, "Order Event Warning: Attribute 'Outside Regular Trading Hours' is ignored"),
            IbSeverity::Warning);
    EXPECT_EQ(classify(2199, ""), IbSeverity::Warning);
}

TEST(IbErrorTest, ConfirmationsAndWarnings)
{
    EXPECT_EQ(classify(202, "Order Canceled - reason:"), IbSeverity::Info);
    EXPECT_EQ(classify(165, "Historical Market Data Service query message:"), IbSeverity::Info);
    EXPECT_EQ(classify(399, "Order Message: Warning: your order will not be placed at the exchange until..."),
            IbSeverity::Warning);
    EXPECT_EQ(classify(10167, "Requested market data is not subscribed. Displaying delayed market data."),
            IbSeverity::Warning);
    EXPECT_EQ(classify(10168, "Requested market data is not subscribed. Delayed market data is not enabled."),
            IbSeverity::Warning);
}

TEST(IbErrorTest, ConnectivityCodes)
{
    for(int code : {1100, 1101, 1102, 1300, 2110, 502, 504, 507, 509})
        EXPECT_EQ(classify(code, ""), IbSeverity::Connectivity) << "code " << code;
}

TEST(IbErrorTest, Code162DependsOnMessage)
{
    EXPECT_EQ(classify(162, "Historical Market Data Service error message:API historical data query cancelled: 1073741825"),
            IbSeverity::Info);
    EXPECT_EQ(classify(162, "Historical Market Data Service error message:Historical data request pacing violation"),
            IbSeverity::RequestFailed);
    EXPECT_EQ(classify(162, "Historical Market Data Service error message:HMDS query returned no data"),
            IbSeverity::RequestFailed);
}

TEST(IbErrorTest, OrderRejections)
{
    EXPECT_EQ(classify(201, "Order rejected - reason:"), IbSeverity::OrderRejected);
    EXPECT_EQ(classify(103, "Duplicate order id"), IbSeverity::OrderRejected);
    EXPECT_EQ(classify(110, "The price does not conform to the minimum price variation"), IbSeverity::OrderRejected);
    EXPECT_EQ(classify(10148, "OrderId 5 that needs to be cancelled can not be cancelled"), IbSeverity::OrderRejected);
}

TEST(IbErrorTest, RequestFailuresAndDefault)
{
    EXPECT_EQ(classify(200, "No security definition has been found for the request"), IbSeverity::RequestFailed);
    EXPECT_EQ(classify(354, "Requested market data is not subscribed"), IbSeverity::RequestFailed);
    EXPECT_EQ(classify(99999, "something new"), IbSeverity::RequestFailed);
    EXPECT_EQ(classify(503, "The TWS is out of date and must be upgraded"), IbSeverity::System);
}

TEST(IbErrorTest, IsErrorFollowsSeverity)
{
    ib_helper::IbError err;
    err.severity = IbSeverity::Info;
    EXPECT_FALSE(err.isError());
    err.severity = IbSeverity::Warning;
    EXPECT_FALSE(err.isError());
    err.severity = IbSeverity::Connectivity;
    EXPECT_FALSE(err.isError());
    err.severity = IbSeverity::RequestFailed;
    EXPECT_TRUE(err.isError());
    err.severity = IbSeverity::OrderRejected;
    EXPECT_TRUE(err.isError());
    err.severity = IbSeverity::System;
    EXPECT_TRUE(err.isError());
}
