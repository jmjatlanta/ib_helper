#include <gtest/gtest.h>
#include "../src/ib_helper/ContractBuilder.hpp"

struct ib_host
{
    std::string url = "localhost";
    int port = 7497;
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
