#include "../src/ib_helper/IBConnector.hpp"
#include "../src/ib_helper/ContractBuilder.hpp"
#include "../src/util/SysLogger.h"

#include "gtest/gtest.h"

#include <thread>
#include <chrono>

bool isConnected(const ib_helper::IBConnector& conn)
{
    int counter = 0;
    while (true)
    {
        if (conn.IsConnected())
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return conn.IsConnected();
}

TEST(IBConnectorTest, Connect)
{
    ib_helper::IBConnector connector{"127.0.0.1", 4002, 5};

    std::this_thread::sleep_for(std::chrono::seconds(1));

    EXPECT_TRUE(isConnected(connector));
}

TEST(IBConnectorTest, Ticks)
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
        virtual void OnTickLast(int reqId, int tickType, long time, double price, Decimal size, 
                TickAttribLast tickAttribLast, const std::string& exchange, const std::string& specialConditions) override
        {
            logger->debug(clazz, "OnTickLast called");
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
    ib_helper::IBConnector conn{"127.0.0.1", 4002, 6};
    EXPECT_TRUE(isConnected(conn));
    MyTickHandler tickHandler{};
    ib_helper::ContractBuilder contractBuilder{&conn};
    Contract msft = contractBuilder.BuildStock("GOTU");
    uint32_t tickSubscriptionId = conn.SubscribeToMarketData(msft, &tickHandler, "225,233", false, false, nullptr);
    std::this_thread::sleep_for(std::chrono::seconds(3));
    conn.UnsubscribeFromMarketData(tickSubscriptionId);
    EXPECT_GT(tickHandler.methodsCalled, 0);
    tickHandler.methodsCalled = 0;
    // Last, AllLast, BidAsk
    tickSubscriptionId = conn.SubscribeToTickByTick(msft, &tickHandler, "BidAsk", 0, true);
    std::this_thread::sleep_for(std::chrono::seconds(15));
    uint32_t lastSubscriptionId = conn.SubscribeToTickByTick(msft, &tickHandler, "AllLast", 0, true);
    std::this_thread::sleep_for(std::chrono::seconds(30));
    conn.UnsubscribeFromTickByTick(tickSubscriptionId);
    conn.UnsubscribeFromTickByTick(lastSubscriptionId);
    EXPECT_GT(tickHandler.methodsCalled, 0);
}

TEST(IBConnectorTest, L2Exchanges)
{
    // mac: 192.168.50.194:7496
    // local: 127.0.0.1:4002
    ib_helper::IBConnector conn{"127.0.0.1", 4002, 6};
    EXPECT_TRUE(isConnected(conn));
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

TEST(IBConnectorTest, L2Book)
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
    ib_helper::IBConnector conn{"127.0.0.1", 4002, 6};
    EXPECT_TRUE(isConnected(conn));
    MyMarketDataHandler handler;
    ib_helper::ContractBuilder builder(&conn);
    Contract contract = builder.BuildStock("GOTU");
    auto subscriptionId = conn.SubscribeToMarketDepth(contract, &handler, 1);
    std::this_thread::sleep_for(std::chrono::seconds(10));
    conn.UnsubscribeFromMarketDepth(subscriptionId);
    EXPECT_GT(handler.depthCount + handler.l2Count, 0);
}

