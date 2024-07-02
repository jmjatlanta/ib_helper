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

class MockExchange : public Exchange
{
    public:
    MockExchange(const ContractDetails& contractDetails) : Exchange(contractDetails) {}

    void setStartTime(const std::string& in) { Exchange::setStartTime(in); }
    void setStopTime(const std::string& in) { Exchange::setStopTime(in); }
    std::chrono::time_point<std::chrono::system_clock> midnightAtExchange(time_t today) { return Exchange::midnightAtExchange(today); }

};

TEST(ExchangeTest, MidnightAtExchange)
{
    time_t today = 1719892494; // 2024-7-1 21:54:54 NY
    time_t midnightNY = 1719806400; // 2024-7-1 00:00:00 NY
    ContractDetails contractDetails;
    contractDetails.liquidHours = "20230307:0830-20230307:1500;20230308:0830-20320308:1500"; // format B
    contractDetails.timeZoneId = "America/New_York";
    MockExchange exch(contractDetails);

    exch.setStartTime("09:30");
    exch.setStopTime("24:00");
    EXPECT_EQ(exch.midnightAtExchange(today), std::chrono::system_clock::from_time_t(midnightNY));
                               //
}
TEST(ExchangeTest, ManualOpenClose)
{
    ContractDetails contractDetails;
    contractDetails.timeZoneId = "America/New_York";
    // LiquidHours can be in 2 formats
    contractDetails.liquidHours = "20230307:0830-20230307:1500;20230308:0830-20320308:1500"; // format B
    MockExchange exch(contractDetails);
    exch.setStartTime("07:00");
    exch.setStopTime("11:00");

    time_t today = 1678206727; // 2023-3-7 4:31:52GMT (11:31:52 EST)
    EXPECT_FALSE(exch.isWithinRange(today)); // 11:31am
    exch.setStopTime("12:00");
    EXPECT_TRUE(exch.isWithinRange(today)); // 11:31am

    today = 1719892494; // 2024-7-1 21:54:54 NY
    exch.setStartTime("09:30");
    exch.setStopTime("24:00");
    EXPECT_TRUE(exch.isWithinRange(today));
}

ContractDetails buildStockContractDetails(const std::string& ticker)
{
    ContractDetails contractDetails;
    contractDetails.contract.symbol = ticker;
    contractDetails.contract.localSymbol = ticker;
    contractDetails.validExchanges = "SMART,AMEX,NYSE,CBOE,PHLX,ISE,CHX,ARCA,ISLAND,DRCTEDGE,BEX,BATS,EDGEA,BYX,IEX,EDGX,FOXRIVER,PEARL,NYSENAT,LTSE,MEMX,TPLUS1,IBEOS,OVERNIGHT,PSX";
    contractDetails.tradingHours = "20240314:0400-20240314:2000;20240315:0400-20240315:2000;20240316:CLOSED;20240317:CLOSED;20240318:0400-20240318:2000;20240319:0400-20240319:2000";
    contractDetails.liquidHours = "20240314:0930-20240314:1600;20240315:0930-20240315:1600;20240316:CLOSED;20240317:CLOSED;20240318:0930-20240318:1600;20240319:0930-20240319:1600";
    contractDetails.timeZoneId = "US/Eastern";
    return contractDetails;
}

TEST(ExchangeTest, PreviousTradingDay)
{
    std::time_t currentTime = 1710422529; // 2024-3-14 13:22:09 GMT (8:23 Panama, 9:23 NY)
    std::time_t laterTime = 1710423363; // 2024-3-14 13:36:03 GMT (8:36 Panama, 9:36 NY)
    ContractDetails contractDetails = buildStockContractDetails("REAL");
    Exchange exchange(contractDetails);
    EXPECT_FALSE( exchange.isWithinRange(currentTime) );
    EXPECT_EQ( exchange.premarketStart(currentTime), 1710388800); // 2024-03-14 04:00:00 (Midnight NY)
    EXPECT_EQ( exchange.marketOpen(currentTime), 1710423000); // 2024-03-14 13:30:00 (9:30AM NY)
    EXPECT_EQ( exchange.marketClose(currentTime), 1710446400); // 2024-03-14 20:00:00 (4PM NY)
    EXPECT_TRUE( exchange.isWithinRange(laterTime) );
}
