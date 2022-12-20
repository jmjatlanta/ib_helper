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
            const std::string& specialConditions)
        {
            logger->debug(clazz, "OnTickByTickAllLast called");
            ++methodsCalled;
        }
        virtual void OnTickByTickBidAsk(int reqId, time_t time, double bidPrice, double askPrice, Decimal bidSize,
            Decimal askSize, const TickAttribBidAsk& tickAttribBidAsk)
        {
            logger->debug(clazz, "OnTickByTickBidAsk called");
            ++methodsCalled;
        }
        virtual void OnTickByTickMidPoint(int reqId, time_t time, double midPoint)
        {
            logger->debug(clazz, "OnTickByTickMidPoint called");
            ++methodsCalled;
        }

    };
    
    ib_helper::IBConnector conn{"127.0.0.1", 4002, 6};
    EXPECT_TRUE(isConnected(conn));
    MyTickHandler tickHandler{};
    ib_helper::ContractBuilder contractBuilder{&conn};
    Contract msft = contractBuilder.BuildStock("MSFT");
    uint32_t tickSubscriptionId = conn.SubscribeToMarketData(msft, &tickHandler, "225,233", false, false, nullptr);
    std::this_thread::sleep_for(std::chrono::seconds(3));
    conn.UnsubscribeFromMarketData(tickSubscriptionId);
    EXPECT_GT(tickHandler.methodsCalled, 0);
    tickHandler.methodsCalled = 0;
    tickSubscriptionId = conn.SubscribeToTickByTick(msft, &tickHandler, "Last", 0, true);
    std::this_thread::sleep_for(std::chrono::seconds(3));
    conn.UnsubscribeFromTickByTick(tickSubscriptionId);
    EXPECT_GT(tickHandler.methodsCalled, 0);
}

