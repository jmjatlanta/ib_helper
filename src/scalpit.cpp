#include "ib_helper/IBConnector.hpp"
#include "ib_helper/ContractBuilder.hpp"
#include "../ta-lib/include/ta_libc.h"
#include "AccountManager.hpp"

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <functional>

bool ctrl_c_pressed = false;
util::SysLogger* logger = nullptr;

void ctrlc_handler(int s) {
    ctrl_c_pressed = true;
}

std::string getDate() 
{
    std::time_t currTime = std::time(0);
    std::tm* now = std::localtime(&currTime);
    std::stringstream ss;
    ss << (now->tm_year + 1900) 
        << std::setw(2) << std::setfill('0') << now->tm_mon + 1 
        << std::setw(2) << std::setfill('0') << now->tm_mday;
    return ss.str();
}

class TickFileWriter
{
    public:
    TickFileWriter(const Contract& contract)
    {
        stream.open(contract.localSymbol + getDate() + ".tic", std::ios_base::app);
    }

    ~TickFileWriter()
    {
        stream.close();
    }

    void LogAllLast(int reqId, int tickType, time_t time, double price, Decimal size,
        const TickAttribLast& tickAttribLast, const std::string& exchange, 
        const std::string& specialConditions)
    {
        // write out a tick record
        std::string msg = "T," + std::to_string(tickType)
            + "," + std::to_string(time)
            + "," + std::to_string(price)
            + "," + decimalStringToDisplay(size)
            + ",\"" + specialConditions
            + "\"";
        stream << msg << "\n";
    }
    
    void LogBidAsk(int reqId, time_t time, double bidPrice, double askPrice, Decimal bidSize,
        Decimal askSize, const TickAttribBidAsk& tickAttribBidAsk)
    {
        // write out a bid/ask record
        std::string msg = "B," + std::to_string(time)
            + "," + std::to_string(bidPrice)
            + "," + std::to_string(askPrice)
            + "," + decimalStringToDisplay(bidSize)
            + "," + decimalStringToDisplay(askSize);
        stream << msg << "\n";
    }
    std::ofstream stream;
};

class Book
{
    public:
    Book() {}

    /***
     * Update totals
     * @param bidPrice unused
     * @param bidSize unused
     * @param askPrice unused
     * @param askSize the ask size
     */
    void update(double bidPrice, Decimal bidSize, double askPrice, uint32_t askSize)
    {
        askSizes.emplace_back(askSize);
        askTotal += askSize; // slight risk of overflow here
        ++askEntries;
    }

    /***
     * @brief compute the average size since the beginning of time
     * @return the average size
     */
    uint32_t avg() 
    { 
        return askTotal / askEntries;  // div0 protected, not called except after update
    }

    /****
     * @brief calculate standard deviation of ask sizes
     * @note called on some ticks
     * @param lastN the last N values to consider (2 to 100,000)
     * @returns the stdandard deviation of the last ask size
     */
    double stddev(uint32_t lastN)
    {
        double outReal = 0.0;
        // if we have grown too big, chop askSizes down to size to save memory
        auto sz = askSizes.size();
        if (sz > lastN * 10)
        {
            askSizes = shrinkVector(askSizes, lastN);
        }
        sz = askSizes.size();
        int endIdx = std::min((int)lastN-1, (int)sz-1);
        if (endIdx > 2)
        {
            // only interested in the last n prices 
            int startIdx = std::max((int)(endIdx - lastN), 0);
            int optInTimePeriod = endIdx + 1;
            int optInNbDev = 1.0;
            int outBegIdx = 0;
            int outNbElement = 0;
            // NOTE: Not a normal distribution, high positive skew
            if ( TA_RetCode::TA_SUCCESS == TA_STDDEV(startIdx, endIdx, askSizes.data(), optInTimePeriod, optInNbDev, 
                    &outBegIdx, &outNbElement, &outReal))
            {
                outReal = 0.0;
            }
        }
        return outReal;
    }

    private:
    /****
     * @brief shrink a vector
     * @note no bounds checking
     * @param in the vector
     * @param n the new size
     * @return the new vector, with the last (n) elements in it
     */
    template<typename T>
    std::vector<T> shrinkVector(const std::vector<T>& in, int n)
    {
        auto sz = in.size();
        const T* start = &(in.data())[sz-n];
        return std::vector<T>(start, start + n);
    }

    private:
    std::vector<double> askSizes; // May need to prevent this array from needlesslly getting too large
    uint64_t askTotal = 0; // May need to prevent integer overflow
    uint32_t askEntries = 0;
};

class ScalpStrategy : public ib_helper::TickHandler, ib_helper::OrderHandler
{
    public:
    ScalpStrategy(ib_helper::IBConnector* conn, Contract& contract) 
        : ib(conn), contract(contract), tickFileWriter(contract), clazz("ScalpStrategy")
    {
        tickSubscriptionId = conn->SubscribeToTickByTick(contract, this, "Last", 0, false);
        bidAskSubscriptionId = conn->SubscribeToTickByTick(contract, this, "BidAsk", 0, false);
        acctMgr = (AccountManager*)conn->GetDefaultAccountHandler();
        if (acctMgr == nullptr)
            throw 1;
        acctMgr->AddOrderHandler(this);
    }

    ~ScalpStrategy()
    {
        ib->UnsubscribeFromTickByTick(tickSubscriptionId);
        ib->UnsubscribeFromTickByTick(bidAskSubscriptionId);
    }

    /**
     * OrderHandler methods
     */
    virtual void OnOpenOrder(int orderId, Contract contract, Order order, OrderState orderState) override {}
    virtual void OnOpenOrderEnd() override {}
    virtual void OnOrderBound(long orderId, int apiClientId, int apiOrderId) override {}
    virtual void OnOrderStatus(int orderId, const std::string& status, Decimal filled, Decimal remaining,
            double avgFillPrice, int permId, int parentId, double lastFillPrice, int clientId,
            const std::string& whyHeld, double mktCapPrice) override
    {
        ib_helper::Order order = acctMgr->GetOrder(orderId);
        // if we are working with the initial order, we may need to add the tp and stop orders
        if (orderId == initialOrderId 
                && tpOrderId == 0 
                && stopOrderId == 0
                && order.status == ib_helper::OrderStatus::FILLED)
        {
            // set up initial orders
            ib_helper::Order tp;
            tp.filledQuantity = doubleToDecimal(0.0);
            tp.lmtPrice = order.lmtPrice + 0.18;
            tp.orderType = "LMT";
            tp.action = "SELL";
            tp.transmit = true;
            tp.contract = contract;
            tp.totalQuantity = doubleToDecimal(100);
            tp.ocaGroup = contract.localSymbol + std::to_string(initialOrderId);
            tpOrderId = acctMgr->PlaceOrder(tp, false);
            ib_helper::Order stop;
            stop.filledQuantity = doubleToDecimal(0.0);
            stop.auxPrice = order.lmtPrice - 0.07;
            stop.orderType = "STOP";
            stop.action = "SELL";
            stop.transmit = true;
            stop.contract = contract;
            stop.totalQuantity = doubleToDecimal(100);
            stop.ocaGroup = contract.localSymbol + std::to_string(initialOrderId);
            stopOrderId = acctMgr->PlaceOrder(stop, false);
        }
        else
        {
            if ( ((orderId == tpOrderId 
                        || orderId == stopOrderId) 
                        && order.status == ib_helper::OrderStatus::FILLED)
                    || (orderId == initialOrderId
                        && order.status == ib_helper::OrderStatus::CANCELLED))
            {
                // if we are done, reset everything
                logger->debug(clazz, "Resetting order stats");
                tpOrderId = 0;
                stopOrderId = 0;
                initialOrderId = 0;
                lastTriggerTime = 0;
                lastTriggerPrice = 0;
            }
        }
    }

    /***
     * TickHandler methods
     */

	virtual void OnTickPrice(int tickerId, int field, double price, TickAttrib attribs)
    {
    }
	virtual void OnTickSize(int tickerId, int field, Decimal size)
    {
    }
	virtual void OnTickOptionComputation(int tickerId, int field, int tickAttrib, double impliedVol, double delta, 
            double optPrice, double pvDividend, double gamma, double vega, double theta, double undPrice)
    {
    }
	virtual void OnTickGeneric(int tickerId, int tickType, double value)
    {
    }
	virtual void OnTickString(int tickerId, int tickType, const std::string& value)
    {
    }
	virtual void OnTickEFP(int tickerId, int tickType, double basisPoints, const std::string& formattedBasisPoints, 
            double impliedFuture, int holdDays, const std::string& futureLastTradeDate, double dividendImpact, 
            double dividendsToLastTradeDate)
    {
    }
	/***
	 * Used with reqTickByTick
	 * @param reqId
	 * @param tickType
	 * @param time
	 * @param price
	 * @param size
	 * @param tickAttribLast
	 * @param exchange
	 * @param specialConditions
	 */
    virtual void OnTickLast(int reqId, int tickType, long time, double price, Decimal size, TickAttribLast tickAttribLast,
            const std::string& exchange, const std::string&  specialConditions)
    {
    }
    virtual void OnTickByTickAllLast(int reqId, int tickType, time_t time, double price, Decimal size,
            const TickAttribLast& tickAttribLast, const std::string& exchange, 
            const std::string& specialConditions)
    {
        tickFileWriter.LogAllLast(reqId, tickType, time, price, size, tickAttribLast, exchange, specialConditions);
        if (initialOrderId != 0)
        {
            ib_helper::Order initial = acctMgr->GetOrder(initialOrderId);
            // if it has been a long time and the order is still not filled, cancel
            if (initial.status == ib_helper::OrderStatus::SUBMITTED
                    && std::time(0) - initialOrderTime > 60) // 1 min
                acctMgr->CancelOrder(initial);
            if (tpOrderId != 0 && stopOrderId != 0)
            {
                // manage position
                ib_helper::Order stop = acctMgr->GetOrder(stopOrderId);
                double newAux = stop.auxPrice;
                // adjust existing orders
                // at 0.19, move to 0.17
                if (price > initial.lmtPrice + 0.18
                        && newAux < initial.lmtPrice + 0.14)
                    newAux = initial.lmtPrice + 14;
                // at 0.13, move to 0.07
                if (price > initial.lmtPrice + 0.12
                        && newAux < initial.lmtPrice + 0.06)
                    newAux = initial.lmtPrice + 0.06;
                // at 0.08 move to 0.98
                if (price > initial.lmtPrice + 0.07
                        && stop.auxPrice < initial.lmtPrice - 0.03)
                    newAux = initial.lmtPrice - 0.03;
                if (newAux > stop.auxPrice)
                {
                    stop.auxPrice = newAux;
                    acctMgr->PlaceOrder(stop, false);
                }
            }
        } // in sync and in a trade
    }
    bool isMultipleOf(double in, int multiple)
    {
        return ((int)(in * 100)) % multiple == 0;
    }

    void placeInitialOrder(double limitPrice)
    {
        ib_helper::Order ord;
        ord.filledQuantity = doubleToDecimal(0.0);
        ord.lmtPrice = limitPrice;
        ord.orderType = "LMT";
        ord.action = "BUY";
        ord.transmit = true;
        ord.contract = contract;
        ord.totalQuantity = doubleToDecimal(100);
        initialOrderTime = std::time(0);
        initialOrderId = acctMgr->PlaceOrder(ord, true);
    }

    virtual void OnTickByTickBidAsk(int reqId, time_t time, double bidPrice, double askPrice, Decimal bidSize,
            Decimal askSize, const TickAttribBidAsk& tickAttribBidAsk)
    {
        tickFileWriter.LogBidAsk(reqId, time, bidPrice, askPrice, bidSize, askSize, tickAttribBidAsk);
        // have we already triggered?
        if (time - lastTriggerTime < 300 // 5 minutes
                && askPrice > lastTriggerPrice // we have gone through the level
                && initialOrderId == 0) // we have not already placed an order
        {
            placeInitialOrder(lastTriggerPrice + 0.01);
        }

        // convert Decimal to uint32_t
        uint32_t askSz = decimalToDouble(askSize);
        bool isMultiple = isMultipleOf(askPrice, 50);
        if (isMultiple)
            book.update( bidPrice, bidSize, askPrice, askSz);
        
        // calculate if we have triggered
        if( lastTriggerTime != time 
                && isMultiple )
        {
            uint32_t sdev = book.stddev(1000);
            uint32_t avg = book.avg();
            if(askSz > avg + sdev)
            {
                lastTriggerTime = time;
                lastTriggerPrice = askPrice;
                logger->debug(clazz, "Size triggered at " + std::to_string(askSz)
                        + " time " + std::to_string(lastTriggerTime)
                        + " avg " + std::to_string(avg)
                        + " sdev " + std::to_string(sdev));
            }
        }
    }
    virtual void OnTickByTickMidPoint(int reqId, time_t time, double midPoint)
    {
    }


    int tickSubscriptionId = 0;
    int bidAskSubscriptionId = 0;
    uint32_t initialOrderId = 0;
    uint32_t tpOrderId = 0;
    uint32_t stopOrderId = 0;
    uint64_t initialOrderTime = 0;
    ib_helper::IBConnector* ib = nullptr;
    Contract contract;
    Book book;
    time_t lastTriggerTime = 0;
    double lastTriggerPrice = 0.0;
    AccountManager* acctMgr = nullptr;
    const std::string clazz;
    TickFileWriter tickFileWriter;
};

int main(int argc, char** argv)
{
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = ctrlc_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, nullptr);

    if (argc < 5)
    {
        std::cerr << "Syntax: " << argv[0] << " host port clientId account# ticker\n";
        exit(1);
    }
    std::string host = argv[1];
    std::string port = argv[2];
    std::string clientId = argv[3];
    std::string accountNumber = argv[4];
    std::string ticker = argv[5];

    logger = util::SysLogger::getInstance();
    std::string filename = ticker + getDate() + ".log";
    std::ofstream outStream(filename, std::ios_base::app);
    logger->set_stream(outStream);
    ib_helper::IBConnector conn(host, strtol(port.c_str(), nullptr, 10), strtol(clientId.c_str(), nullptr, 10));
    AccountManager accountManager(&conn, accountNumber);
    conn.AddAccountHandler(&accountManager);
    conn.AddOrderHandler(&accountManager);
    ib_helper::ContractBuilder contractBuilder(&conn);
    Contract contract = contractBuilder.BuildStock(ticker);
    ScalpStrategy strategy(&conn, contract);

    while(!ctrl_c_pressed)
        std::this_thread::sleep_for(std::chrono::seconds(1));
}

