#include "ib_helper/IBConnector.hpp"
#include "ib_helper/ContractBuilder.hpp"
#include "../ta-lib/include/ta_libc.h"
#include "AccountManager.hpp"

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <thread>
#include <functional>

bool ctrl_c_pressed = false;
util::SysLogger* logger = nullptr;

void ctrlc_handler(int s) {
    ctrl_c_pressed = true;
}

class Book
{
    public:
    Book() {}
    void update(double bidPrice, Decimal bidSize, double askPrice, uint32_t askSize)
    {
        askSizes.emplace_back(askSize);
        askTotal += askSize;
    }

    uint32_t avg() { return askTotal / askSizes.size(); }

    /****
     * @brief calculate standard deviation of ask sizes
     * @returns the stdandard deviation of the last ask size
     */
    double stddev(uint32_t lastN)
    {
        double retVal = 1.0;
        if (lastN == 0 || lastN > askSizes.size())
            lastN = askSizes.size();
        if (lastN > 2)
        {
            // only interested in the last n prices 
            double* inReal = (double*) malloc( sizeof(double) * lastN );
            double* outReal = (double*) malloc( sizeof(double) * lastN );
            int startIdx = 0;
            if (askSizes.size() > lastN)
                startIdx = askSizes.size() - lastN;
            int endIdx = lastN;
            for(uint32_t i = startIdx; i < askSizes.size(); ++i)
            {
                inReal[i-startIdx] = (double)askSizes[i];
            }
            startIdx = 0;
            endIdx = lastN;
            int optInTimePeriod = endIdx + 1;
            int optInNbDev = 1.0;
            int outBegIdx = 0;
            int outNbElement = 0;
            TA_RetCode retCode = TA_STDDEV(startIdx, endIdx, inReal, optInTimePeriod, optInNbDev, 
                    &outBegIdx, &outNbElement, outReal);
            if (retCode == TA_RetCode::TA_SUCCESS)
            {
                retVal = outReal[0];
            }
            free(outReal);
            free(inReal);
        }
        return retVal;
    }
    private:
    std::vector<uint32_t> askSizes;
    uint64_t askTotal = 0;
};

class ScalpStrategy : public ib_helper::TickHandler
{
    public:
    ScalpStrategy(ib_helper::IBConnector* conn, Contract& contract) : ib(conn), contract(contract)
    {
        tickSubscriptionId = conn->SubscribeToTickByTick(contract, this, "Last", 0, false);
        bidAskSubscriptionId = conn->SubscribeToTickByTick(contract, this, "BidAsk", 0, false);
        acctMgr = (AccountManager*)conn->GetDefaultAccountHandler();
        if (acctMgr == nullptr)
            throw 1;
    }

    ~ScalpStrategy()
    {
        ib->UnsubscribeFromTickByTick(tickSubscriptionId);
        ib->UnsubscribeFromTickByTick(bidAskSubscriptionId);
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
        Position* p = getPosition(contract);
        if (p != nullptr 
                && decimalToDouble(p->expectedPos) > 0.0 
                && p->InSync())
        {
            ib_helper::Order initial = acctMgr->GetOrder(initialOrderId);
            if (tpOrderId == 0 && stopOrderId == 0)
            {
                // set up initial orders
                ib_helper::Order tp;
                tp.lmtPrice = initial.lmtPrice + 0.20;
                tp.orderType = "LMT";
                tp.action = "SELL";
                tp.transmit = true;
                tp.contract = contract;
                tp.totalQuantity = doubleToDecimal(100);
                tp.ocaGroup = contract.localSymbol + std::to_string(initialOrderId);
                tpOrderId = acctMgr->PlaceOrder(tp, false);
                ib_helper::Order stop;
                stop.auxPrice = initial.lmtPrice - 0.07;
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
                // manage position
                // stop at 0.94/0.01
                // then at 0.97/0.07
                // then at 0.07/0.12
                // then at 0.05 below current price
                ib_helper::Order stop = acctMgr->GetOrder(stopOrderId);
                // adjust existing orders
                if (price > initial.lmtPrice + 0.12
                        && stop.auxPrice < initial.lmtPrice + 0.06)
                {
                    stop.auxPrice = initial.lmtPrice + 0.06;
                    acctMgr->PlaceOrder(stop, false);
                    return;
                }
                if (price > initial.lmtPrice + 0.07
                        && stop.auxPrice < initial.lmtPrice - 0.04)
                {
                    stop.auxPrice = initial.lmtPrice - 0.04;
                    acctMgr->PlaceOrder(stop, false);
                    return;
                }
            }
        } // in sync and in a trade
        else
        {
            // why did we fail?
            if (p == nullptr)
            {
            }
            else
            {
                double expectedPos = decimalToDouble(p->expectedPos);
                if (expectedPos == 0.0)
                {
                    if (tpOrderId != 0)
                    {
                        // we need to reset
                        logger->debug("Strategy", "Resetting order stats");
                        tpOrderId = 0;
                        stopOrderId = 0;
                        initialOrderId = 0;
                        lastTriggerTime = 0;
                        lastTriggerPrice = 0;
                    }
                }
                else
                {
                    if (!p->InSync())
                        logger->debug("Strategy", "Position " 
                                + std::to_string(p->contract.conId) + " " 
                                + p->contract.localSymbol + " is not in sync: "
                                + std::to_string(expectedPos)
                                + " Actual: " + std::to_string(decimalToDouble(p->pos)));
                }
            }
        }
    }
    bool isMultipleOf(double in, int multiple)
    {
        return ((int)(in * 100)) % multiple == 0;
    }
    Position* getPosition(const Contract& contract)
    {
        return acctMgr->GetPosition(contract);
    }

    void placeInitialOrder(double limitPrice)
    {
        ib_helper::Order ord;
        ord.lmtPrice = limitPrice;
        ord.orderType = "LMT";
        ord.action = "BUY";
        ord.transmit = true;
        ord.contract = contract;
        ord.totalQuantity = doubleToDecimal(100);
        initialOrderId = acctMgr->PlaceOrder(ord, true);
    }

    virtual void OnTickByTickBidAsk(int reqId, time_t time, double bidPrice, double askPrice, Decimal bidSize,
            Decimal askSize, const TickAttribBidAsk& tickAttribBidAsk)
    {
        // have we already triggered?
        if (time - lastTriggerTime < 300 // 5 minutes
                && askPrice > lastTriggerPrice) // we have gone through the level
        {
            Position* p = getPosition(contract);
            if (p == nullptr || decimalToDouble(p->GetSize()) == 0.0)
            {
                // if we haven't already, we should place an order 1 tick above the triggerPrice
                placeInitialOrder(lastTriggerPrice + 0.01);
                // long 0.01 above, TP at 15
                // try to get in at 0.01, cancel if hits 0.07 without entry
            }
        }

        // convert Decimal to uint32_t
        uint32_t askSz = decimalToDouble(askSize);
        book.update( bidPrice, bidSize, askPrice, askSz);
        
        if( lastTriggerTime != time 
                && isMultipleOf(askPrice, 25) )
        {
            uint32_t neededSize = book.avg() + (book.stddev(500) * 10);
            if(neededSize < askSz)
            {
                lastTriggerTime = time;
                lastTriggerPrice = askPrice;
                logger->debug("Strategy", "Size triggered at " + std::to_string(lastTriggerPrice)
                        + " at " + std::to_string(lastTriggerTime));
            }
            /*
            else
            {
                logger->debug("Strategy", "Did not meet necessary size. Needed "
                        + std::to_string(neededSize) + " but had " + std::to_string(askSz));
            }
            */
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
    ib_helper::IBConnector* ib = nullptr;
    Contract contract;
    Book book;
    time_t lastTriggerTime = 0;
    double lastTriggerPrice = 0.0;
    AccountManager* acctMgr = nullptr;
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

