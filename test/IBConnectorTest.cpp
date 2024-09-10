#include <gtest/gtest.h>
#include "../src/ib_helper/IBConnector.hpp"
#include "../src/ib_helper/ContractBuilder.hpp"
#include "../src/util/SysLogger.h"
#include "test_helpers.h"

#include "MockIBConnector.h"

#include <thread>
#include <chrono>

TEST(IBConnectorTest, Connect)
{
    class MyConnectionMonitor : public ib_helper::IBConnectionMonitor
    {
        public:
        void OnConnect(ib_helper::IBConnector* in) override
        {
            std::cout << "OnConnect called" << std::endl;
        }
        void OnFullConnect(ib_helper::IBConnector* conn) override
        {
            std::cout << "OnFullConnect called" << std::endl;
        }
        void OnDisconnect(ib_helper::IBConnector* conn) override
        {
            std::cout << "OnDisconnect called" << std::endl;
        }
        void OnError(ib_helper::IBConnector* conn, int id, int code, const std::string& msg, const std::string& json) override
        {
            std::cout << "OnError called" << std::endl;
        }
    };
    ib_options ops;
    MyConnectionMonitor connMon;
    ib_helper::IBConnector connector{ops.host, ops.port, ops.connId, &connMon};

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    ASSERT_TRUE(isConnected(connector));
    // check managedAccounts
    EXPECT_GT(connector.GetDefaultAccount().size(), 0);
}

TEST(IBConnectorTest, DISABLED_Ticks)
{
    class MyTickHandler : public ib_helper::TickHandler
    {
        public:
        int methodsCalled = 0;
        util::SysLogger* logger = util::SysLogger::getInstance();
        const std::string clazz = "IBConnectorTest";

        virtual void OnTickPrice(int tickerId, int field, double price, TickAttrib attribs) override
        {
            logger->debug(clazz, "OnTickPrice called");
            ++methodsCalled;
        }
        virtual void OnTickSize(int tickerId, int field, Decimal size) override
        {
            logger->debug(clazz, "OnTickSize called");
            ++methodsCalled;
        }
        virtual void OnTickOptionComputation(int tickerId, int field, int tickAttrib, double impliedVol, double delta,
                double optPrice, double pvDividend, double gamma, double vega, double thead, double undPrice) override
        {
            logger->debug(clazz, "OnTickOptionComputation called");
            ++methodsCalled;
        }
        virtual void OnTickGeneric(int tickerId, int tickType, double value) override
        {
            logger->debug(clazz, "OnTickGeneric called");
            ++methodsCalled;
        }
        virtual void OnTickString(int tickerId, int tickType, const std::string& value) override
        {
            logger->debug(clazz, "OnTickString called");
            ++methodsCalled;
        }
        virtual void OnTickEFP(int tickerId, int tickType, double basisPoints, const std::string& formattedBasisPoints,
                double impliedFuture, int holdDays, const std::string& futureLastTradeDate, double dividendImpact, 
                double dividendsToLastTradeDate) override
        {
            logger->debug(clazz, "OnTickEFP called");
            ++methodsCalled;
        }
        virtual void OnTickByTickAllLast(int reqId, int tickType, time_t time, double price, Decimal size,
            const TickAttribLast& tickAttribLast, const std::string& exchange, 
            const std::string& specialConditions) override
        {
            std::string msg = "OnTickByTickAllLast type: " + std::to_string(tickType)
                + " time: " + std::to_string(time)
                + " price: " + std::to_string(price)
                + " size: " + decimalStringToDisplay(size)
                + " cond: " + specialConditions;
            logger->debug(clazz, msg);
            ++methodsCalled;
        }
        virtual void OnTickByTickBidAsk(int reqId, time_t time, double bidPrice, double askPrice, Decimal bidSize,
            Decimal askSize, const TickAttribBidAsk& tickAttribBidAsk) override
        {
            std::string msg = "OnTickByTickBidAsk at time " + std::to_string(time)
                + " Bid: " + std::to_string(bidPrice)
                + " Ask: " + std::to_string(askPrice)
                + " Bid Size: " + decimalStringToDisplay(bidSize)
                + " Ask Size: " + decimalStringToDisplay(askSize);
            logger->debug(clazz, msg);
            ++methodsCalled;
        }
        virtual void OnTickByTickMidPoint(int reqId, time_t time, double midPoint) override
        {
            logger->debug(clazz, "OnTickByTickMidPoint called");
            ++methodsCalled;
        }

    };
    
    // mac: 192.168.50.194:7496
    // local: 127.0.0.1:4002
    ib_options ops;
    ib_helper::IBConnector conn{ops.host, ops.port, ops.connId + 1};
    ASSERT_TRUE(isConnected(conn));
    MyTickHandler tickHandler;
    ib_helper::ContractBuilder contractBuilder{&conn};
    ContractDetails msft = contractBuilder.BuildStock("GOTU");
    uint32_t tickSubscriptionId = conn.SubscribeToMarketData(msft.contract, &tickHandler, "225,233", false, false, nullptr);
    std::this_thread::sleep_for(std::chrono::seconds(3));
    conn.UnsubscribeFromMarketData(tickSubscriptionId);
    EXPECT_GT(tickHandler.methodsCalled, 0);
    tickHandler.methodsCalled = 0;
    // Last, AllLast, BidAsk
    tickSubscriptionId = conn.SubscribeToTickByTick(msft.contract, &tickHandler, "BidAsk", 0, true);
    std::this_thread::sleep_for(std::chrono::seconds(15));
    uint32_t lastSubscriptionId = conn.SubscribeToTickByTick(msft.contract, &tickHandler, "AllLast", 0, true);
    std::this_thread::sleep_for(std::chrono::seconds(30));
    conn.UnsubscribeFromTickByTick(tickSubscriptionId);
    conn.UnsubscribeFromTickByTick(lastSubscriptionId);
    EXPECT_GT(tickHandler.methodsCalled, 0);
}

TEST(IBConnectorTest, L2Exchanges)
{
    // mac: 192.168.50.194:7496
    // local: 127.0.0.1:4002
    ib_options ops;
    ib_helper::IBConnector conn{ops.host, ops.port, ops.connId + 2};
    ASSERT_TRUE(isConnected(conn));
    std::future<std::vector<DepthMktDataDescription> > fut = conn.RequestMktDepthExchanges();
    auto vec = fut.get();
    int linesPrinted = 0;
    for(auto itr = vec.begin(); itr != vec.end(); ++itr)
    {
        ++linesPrinted;
        auto desc = *itr;
        std::cout << "Exchange: " << desc.exchange << " SecType: " << desc.secType 
                << " Listing Exchange: " << desc.listingExch
                << " Svc Data Type: " << desc.serviceDataType
                << " Agg Group: " << std::to_string(desc.aggGroup)
                << "\n";
    }
    EXPECT_GT(linesPrinted, 0);
}

TEST(IBConnectorTest, ContractExchanges)
{
    // mac: 192.168.50.194:7496
    // local: 127.0.0.1:4002
    ib_options ops;
    ib_helper::IBConnector conn{ops.host, ops.port, ops.connId + 2};
    ASSERT_TRUE(isConnected(conn));
    Contract contract;
    contract.symbol = "ES";
    contract.secType = "FUT";
    contract.currency = "USD";
    contract.exchange = "SMART";
    std::future<std::vector<ContractDetails>> fut1= conn.GetContractDetails(contract);
    try
    {
        auto dets = fut1.get();
        FAIL();
    } catch(const std::out_of_range& oor)
    {
        std::string msg = oor.what();
        EXPECT_EQ(msg, "Symbol not found");
    }
    contract.exchange = "CME";
    fut1 = conn.GetContractDetails(contract);
    try
    {
        auto dets = fut1.get();
        ASSERT_GT(dets.size(), 0);
        EXPECT_EQ(dets[0].contract.symbol, "ES");
    } catch(const std::out_of_range& oor)
    {
        FAIL();
    }
}

TEST(IBConnectorTest, FutureContractBuilderES)
{
    ib_options ops;
    ib_helper::IBConnector conn{ops.host, ops.port, ops.connId + 3};
    ASSERT_TRUE(isConnected(conn));
    ib_helper::ContractBuilder contractBuilder(&conn);
    try
    {
        auto es = contractBuilder.BuildFuture("ES");
        auto vec = contractBuilder.GetDetails(es.contract);
        ASSERT_GT(vec.size(), 0);
        auto details = vec[0];
        EXPECT_EQ(details.contract.symbol, "ES");
        EXPECT_EQ(details.contract.localSymbol, "ESU3"); 
    } catch(const std::out_of_range& oor) {
        FAIL();
    }
}

TEST(IBConnectorTest, FutureContractBuilderNG)
{
    ib_options ops;
    ib_helper::IBConnector conn{ops.host, ops.port, ops.connId + 4};
    ASSERT_TRUE(isConnected(conn));
    ib_helper::ContractBuilder contractBuilder(&conn);
    try
    {
        auto ng = contractBuilder.BuildFuture("NG");
        auto vec = contractBuilder.GetDetails(ng.contract);
        ASSERT_GT(vec.size(), 0);
        auto details = vec[0];
        EXPECT_EQ(details.contract.symbol, "NG");
        EXPECT_EQ(details.contract.localSymbol, "NGJ3"); 
    } catch(const std::out_of_range& oor) {
        FAIL();
    }
}

TEST(IBConnectorTest, FutureContractBuilderYM)
{
    ib_options ops;
    ib_helper::IBConnector conn{ops.host, ops.port, ops.connId + 5};
    ASSERT_TRUE(isConnected(conn));
    ib_helper::ContractBuilder contractBuilder(&conn);
    try
    {
        auto ym = contractBuilder.BuildFuture("YM");
        auto vec = contractBuilder.GetDetails(ym.contract);
        ASSERT_GT(vec.size(), 1);
        auto details = vec[0];
        EXPECT_EQ(details.contract.symbol, "YM");
        EXPECT_EQ(details.contract.localSymbol, "YM   JUN 23"); 
    } catch(const std::out_of_range& oor) {
        FAIL();
    }
}

TEST(IBConnectorTest, OptionChainParameters)
{
    ib_options ops;
    ib_helper::IBConnector conn{ops.host, ops.port, ops.connId + 6};
    ASSERT_TRUE(isConnected(conn));
    ib_helper::ContractBuilder contractBuilder(&conn);
    try
    {
        // make sure passing a localSymbol gets parsed correctly
        auto opt = contractBuilder.BuildOption("AAPL");
        auto params = contractBuilder.GetOptionParameters(opt);
        ASSERT_GT(params.size(), 0);
        for(auto& p : params)
        {
            std::cout << "Param: Exchange: " << p.exchange
                << " Expiries:\n";
            for(auto& e : p.expirations)
            {
                std::cout << " " << e << "\n";
            }
            std::cout << "Strikes:\n";
            for(auto& s : p.strikes)
            {
                std::cout << " " << s << "\n";
            }
            std::cout << "\n";
        }
    } catch(const std::out_of_range& oor) {
        FAIL();
    }
}

TEST(IBConnectorTest, OptionContractBuilderAAPL)
{
    ib_options ops;
    ib_helper::IBConnector conn{ops.host, ops.port, ops.connId + 6};
    ASSERT_TRUE(isConnected(conn));
    ib_helper::ContractBuilder contractBuilder(&conn);
    try
    {
        // make sure passing a localSymbol gets parsed correctly
        auto und = contractBuilder.BuildOption("AAPL  240621C00010000");
        ASSERT_EQ(und.localSymbol, "AAPL  240621C00010000");
        ASSERT_EQ(und.symbol, "AAPL");
        ASSERT_EQ(und.strike, 10.0);
        ASSERT_EQ(und.lastTradeDateOrContractMonth, "20240621");
        // now attempt to get the whole chain
        und = contractBuilder.BuildOption("AAPL");
        auto whole_chain = contractBuilder.GetDetails(und);
        size_t whole_chain_size = whole_chain.size();
        ASSERT_GT(whole_chain_size, 1);
        // now just get 1 strike
        und.strike = whole_chain[0].contract.strike;
        auto one_strike = contractBuilder.GetDetails(und);
        size_t one_strike_size = one_strike.size();
        EXPECT_GT(one_strike_size, 0);
        EXPECT_LT(one_strike_size, whole_chain_size);
        // all strikes for an expiry
        und.strike = 0.0;
        und.lastTradeDateOrContractMonth = whole_chain[0].contract.lastTradeDateOrContractMonth;
        auto one_expiry = contractBuilder.GetDetails(und);
        size_t one_expiry_size = one_expiry.size();
        EXPECT_LT(one_expiry_size, whole_chain_size);
        EXPECT_GT(one_expiry_size, 0);
        ContractDetails dets = whole_chain[0];
        std::cout << "Example option last trade date: " + whole_chain[0].contract.lastTradeDateOrContractMonth + "\n";
        std::cout << "Example option symbol         : " + whole_chain[0].contract.localSymbol + "\n";
        std::cout << "Example option price          : " + std::to_string(whole_chain[0].contract.strike) + "\n";

    } catch(const std::out_of_range& oor) {
        FAIL();
    }
}

TEST(IBConnectorTest, DISABLED_L2Book)
{
    class MyMarketDataHandler : public ib_helper::MarketDepthHandler
    {
        public:
        int depthCount = 0;
        int l2Count = 0;
        util::SysLogger* logger = util::SysLogger::getInstance();
        const std::string clazz = "IBConnectorTest";

	    virtual void OnUpdateMktDepth(int tickerId, int position, int operation, int side, double price, Decimal size) override
        {
            logger->debug(clazz, "OnUpdateMktDepth");
            ++depthCount;
        }
	    virtual void OnUpdateMktDepthL2(int tickerId, int position, const std::string&  marketMaker, int operation, int side, 
                double price, Decimal size, bool isSmartDepth) override
        {
            std::string message = "L2," + std::to_string(position)
                    + "," + marketMaker
                    + "," + std::to_string(operation)
                    + "," + std::to_string(side)
                    + "," + std::to_string(price)
                    + "," + decimalStringToDisplay(size)
                    + "," + (isSmartDepth?"true":"false");
            logger->debug(clazz, message);
            ++l2Count;
        }
    };
    // mac: 192.168.50.194:7496
    // local: 127.0.0.1:4002
    ib_options ops;
    ib_helper::IBConnector conn{ops.host, ops.port, ops.connId + 3};
    ASSERT_TRUE(isConnected(conn));
    MyMarketDataHandler handler;
    ib_helper::ContractBuilder builder(&conn);
    ContractDetails contractDets = builder.BuildStock("GOTU");
    auto subscriptionId = conn.SubscribeToMarketDepth(contractDets.contract, &handler, 1);
    std::this_thread::sleep_for(std::chrono::seconds(10));
    conn.UnsubscribeFromMarketDepth(subscriptionId);
    EXPECT_GT(handler.depthCount + handler.l2Count, 0);
}

TEST(IBConnectorTest, MockGetContractDetails)
{
    MockIBConnector conn("", 0, 0);
    Contract contract;
    contract.symbol = "XYZABC";
    auto fut = conn.GetContractDetails(contract);
    try
    {

        std::vector<ContractDetails> dets = fut.get();
        FAIL();
    } 
    catch(const std::out_of_range& oor)
    {
        // this is what is supposed to happen
        std::string msg = oor.what();
        EXPECT_EQ(msg, "Invalid symbol: XYZABC");
    }
    catch(...)
    {
        FAIL();
    }
}

