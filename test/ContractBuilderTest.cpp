#include <gtest/gtest.h>
#include "../src/ib_helper/ContractBuilder.hpp"
#include <vector>
#include <string>

struct ib_host
{
    std::string url = "localhost";
    int port = 7496;
};

static ib_host ibhost;

std::string to_string(const ContractDetails& in)
{
    std::stringstream ss;
    ss << "Contract: " << in.contract.symbol
            << "\n Local symbol: " << in.contract.localSymbol
            << "\n Exchanges: " << in.validExchanges
            << "\n Trading Hours: " << in.tradingHours
            << "\n Liquid Hours: " << in.liquidHours
            << "\n Time Zone: " << in.timeZoneId
            << "\n";
    return ss.str();
}

TEST(contract_builder, DISABLED_unknown_exchange)
{
    // connect to IB
    ib_helper::IBConnector conn(ibhost.url, ibhost.port, 3);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    ib_helper::ContractBuilder contractBuilder(&conn);
    auto contractDets = contractBuilder.BuildStock("real");
    EXPECT_NE(contractDets.contract.conId, 0);
    auto contractDetailsCollection = contractBuilder.GetDetails(contractDets.contract);
    for(auto contractDetails : contractDetailsCollection)
        std::cout << ::to_string(contractDetails);
}

TEST(contract_builder, DISABLED_zc)
{
    ib_helper::IBConnector conn(ibhost.url, ibhost.port, 7);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    ASSERT_TRUE( conn.IsConnected() );
    ib_helper::ContractBuilder contractBuilder(&conn);
    auto dets = contractBuilder.BuildFuture("ZC");
    EXPECT_NE(dets.contract.conId, 0);
    EXPECT_EQ(dets.minTick, 0.25);
}

TEST(contract_builder, DISABLED_zs)
{
    ib_helper::IBConnector conn(ibhost.url, ibhost.port, 7);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    ASSERT_TRUE( conn.IsConnected() );
    ib_helper::ContractBuilder contractBuilder(&conn);
    auto dets = contractBuilder.BuildFuture("ZS");
    EXPECT_NE(dets.contract.conId, 0);
    EXPECT_EQ(dets.minTick, 0.25);
}

TEST(contract_builder, DISABLED_rty)
{
    ib_helper::IBConnector conn(ibhost.url, ibhost.port, 3);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    ASSERT_TRUE( conn.IsConnected() );
    ib_helper::ContractBuilder contractBuilder(&conn);
    auto dets = contractBuilder.BuildFuture("RTY");
    EXPECT_NE(dets.contract.conId, 0);
}

TEST(contract_builder, DISABLED_mnq)
{
    ib_helper::IBConnector conn(ibhost.url, ibhost.port, 3);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    ASSERT_TRUE( conn.IsConnected() );
    ib_helper::ContractBuilder contractBuilder(&conn);
    auto dets = contractBuilder.BuildFuture("MNQ");
    EXPECT_NE(dets.contract.conId, 0);
}

TEST(contract_builder, trading_times)
{
    ContractDetails contractDetails;
    // 20090507:0700-1830,1830-2330;20090508:CLOSED
    // 20180323:0400-20180323:2000;20180326:0400-20180326:2000
    contractDetails.tradingHours = "20090507:0700-1830,1830-2330;20090508:CLOSED";
    std::vector<std::string> hours = ib_helper::tokenize(contractDetails.tradingHours, ";");
    ASSERT_EQ(hours.size(), 2);
    EXPECT_EQ(ib_helper::get_trading_open(hours[0]), std::chrono::system_clock::from_time_t (1241697600));
    EXPECT_EQ(ib_helper::get_trading_close(hours[0]), std::chrono::system_clock::from_time_t(1241739000));
    contractDetails.tradingHours = "20180323:0400-20180323:2000;20180326:0400-20180326:2000";
    hours = ib_helper::tokenize(contractDetails.tradingHours, ";");
    ASSERT_EQ(hours.size(), 2);
    EXPECT_EQ(ib_helper::get_trading_open(hours[0]), std::chrono::system_clock::from_time_t (1521795600));
    EXPECT_EQ(ib_helper::get_trading_close(hours[0]), std::chrono::system_clock::from_time_t(1521853200));
}

TEST(contract_builder, num_candles)
{
    ContractDetails contractDetails;
    contractDetails.tradingHours = "20090507:0700-1830,1830-2330;20090508:CLOSED";
    EXPECT_EQ(candles_per_day(contractDetails, ib_helper::BarSize::ONE_HOUR), 12);
    contractDetails.tradingHours = "20180323:0400-20180323:2000;20180326:0400-20180326:2000";
    EXPECT_EQ(candles_per_day(contractDetails, ib_helper::BarSize::ONE_HOUR), 16);
}

TEST(contract_builder, more_days)
{
    // connect to IB
    ib_helper::IBConnector conn(ibhost.url, ibhost.port, 3);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    ib_helper::ContractBuilder contractBuilder(&conn);
    auto contractDets = contractBuilder.BuildStock("MSFT");
    EXPECT_NE(contractDets.contract.conId, 0);
    auto contractDetailsCollection = contractBuilder.GetDetails(contractDets.contract);
    for(auto contractDetails : contractDetailsCollection)
    {
        EXPECT_EQ(candles_per_day(contractDetails, ib_helper::BarSize::ONE_HOUR), 16);
        //std::cout << ::to_string(contractDetails);
    }
    auto vec = ib_helper::tokenize(contractDets.tradingHours, ";");
    EXPECT_EQ(vec.size(), 5);
}
