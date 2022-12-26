#include "ib_helper/IBConnector.hpp"
#include "util/SysLogger.h"
#include "ib_helper/ContractBuilder.hpp"

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

class MyMarketDepthHandler : public ib_helper::MarketDepthHandler
{
    private:
    util::SysLogger* logger = util::SysLogger::getInstance();
    const std::string clazz = "MyMarketDepthHandler";

    public:
	virtual void OnUpdateMktDepth(int tickerId, int position, int operation, int side, double price, Decimal size) override
    {
        // shouldn't be called
    }
	virtual void OnUpdateMktDepthL2(int tickerId, int position, const std::string& marketMaker, int operation, 
            int side, double price, Decimal size, bool isSmartDepth) override
    {
        std::string msg = std::string("L,") + std::to_string(tickerId)
            + "," + std::to_string(position)
            + ",\"" + marketMaker
            + "\"," + std::to_string(operation)
            + "," + std::to_string(side)
            + "," + std::to_string(price)
            + "," + std::to_string(size)
            + "," + (isSmartDepth ? "TRUE" : "FALSE");
        logger->debug(clazz, msg);
    }
};

class MyTickHandler : public ib_helper::TickHandler
{
        public:
        int methodsCalled = 0;
        util::SysLogger* logger = util::SysLogger::getInstance();
        const std::string clazz = "MyTickHandler";

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
            // write out a tick record
            std::string msg = "T," + std::to_string(tickType)
                + "," + std::to_string(time)
                + "," + std::to_string(price)
                + "," + decimalStringToDisplay(size)
                + ",\"" + specialConditions
                + "\"";
            logger->debug(clazz, msg);
            ++methodsCalled;
        }
        virtual void OnTickByTickBidAsk(int reqId, time_t time, double bidPrice, double askPrice, Decimal bidSize,
            Decimal askSize, const TickAttribBidAsk& tickAttribBidAsk) override
        {
            // write out a bid/ask record
            std::string msg = "B," + std::to_string(time)
                + "," + std::to_string(bidPrice)
                + "," + std::to_string(askPrice)
                + "," + decimalStringToDisplay(bidSize)
                + "," + decimalStringToDisplay(askSize);
            logger->debug(clazz, msg);
            ++methodsCalled;
        }
        virtual void OnTickByTickMidPoint(int reqId, time_t time, double midPoint) override
        {
            logger->debug(clazz, "OnTickByTickMidPoint called");
            ++methodsCalled;
        }

};

bool ctrl_c_pressed = false;

void ctrlc_handler(int s) {
    ctrl_c_pressed = true;
}
    
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

int main(int argc, char** argv)
{
    if (argc < 4)
    {
        std::cerr << "Syntax: " << argv[0] << " host port clientid ticker\n";
        exit(1);
    }
    std::string host = argv[1];
    std::string port = argv[2];
    std::string clientId = argv[3];
    std::string ticker = argv[4];

    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = ctrlc_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, nullptr);

    // mac: 192.168.50.194:7496
    // local: 127.0.0.1:4002
    ib_helper::IBConnector conn{host.c_str(), std::stoi(port.c_str()), std::stoi(clientId.c_str())};
    if (!isConnected(conn))
    {
        std::cerr << "Unable to connect to IB\n";
        exit(1);
    }
    MyTickHandler tickHandler{};
    MyMarketDepthHandler marketDepthHandler{};

    ib_helper::ContractBuilder contractBuilder{&conn};
    Contract stock = contractBuilder.BuildStock(ticker);
    // Last, AllLast, BidAsk
    uint32_t tickSubscriptionId = conn.SubscribeToTickByTick(stock, &tickHandler, "BidAsk", 0, true);
    uint32_t lastSubscriptionId = conn.SubscribeToTickByTick(stock, &tickHandler, "AllLast", 0, true);
    uint32_t marketDepthSubscriptionId = conn.SubscribeToMarketDepth(stock, &marketDepthHandler, 3);
    while (!ctrl_c_pressed)
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::cerr << "\nExiting\n";
    conn.UnsubscribeFromTickByTick(tickSubscriptionId);
    conn.UnsubscribeFromTickByTick(lastSubscriptionId);
    conn.UnsubscribeFromMarketData(marketDepthSubscriptionId);
}

