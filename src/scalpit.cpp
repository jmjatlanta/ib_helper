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

void ctrlc_handler(int s) {
    ctrl_c_pressed = true;
}

class BookRecord
{
    public:
    BookRecord(uint32_t pos, const std::string& marketMaker, double price, Decimal size) 
        : position(pos), marketMaker(marketMaker), price(price), size(size) {}
    uint32_t position;
    std::string marketMaker;
    double price;
    Decimal size;
    bool operator>(const BookRecord& in) const { return position > in.position; }
    bool operator<(const BookRecord& in) const { return position < in.position; }
    bool operator==(const BookRecord& in) const { return position == in.position; }
};

class Book : public ib_helper::MarketDepthHandler
{
    public:
    Book(ib_helper::IBConnector* ib, Contract contract) : ib(ib), contract(contract)
    {
        subscriptionId = ib->SubscribeToMarketDepth(contract, this, 3);
    }
    /***
     * MarketDepthHander methods
     */

	virtual void OnUpdateMktDepth(int tickerId, int position, int operation, int side, double price, Decimal size)
    {
    }

    void AddToVec(std::vector<BookRecord>* vec, int position, const std::string& marketMaker, double price, 
            Decimal size, bool isSmartDepth) 
    {
        // sanity checks
        if (position > vec->size())
            vec->emplace_back( BookRecord(position, marketMaker, price, size));
        else
            vec->insert(vec->begin() + position, BookRecord(position, marketMaker, price, size) );
    }
    
    void UpdateVec(std::vector<BookRecord>* vec, int position, const std::string& marketMaker, double price,
            Decimal size, bool isSmartDepth)
    {
        // get record
        BookRecord& rec = (*vec)[position];
        rec.marketMaker = marketMaker;
        rec.price = price;
        rec.size = size;
    }

    void DeleteFromVec(std::vector<BookRecord>* vec, int position)
    {
        vec->erase(vec->begin() + position);
    }

	virtual void OnUpdateMktDepthL2(int tickerId, int position, const std::string& marketMaker, int operation, 
            int side, double price, Decimal size, bool isSmartDepth)
    {
        std::vector<BookRecord>* vec = &bids;
        if (side == TickType::ASK)
            vec = &asks;
        switch(operation)
        {
            case 0: // ADD 
                AddToVec(vec, position, marketMaker, price, size, isSmartDepth); 
                break;
            case 1: // UPDATE
                UpdateVec(vec, position, marketMaker, price, size, isSmartDepth);
                break;
            case 2: // REMOVE 
                DeleteFromVec(vec, position);
                break;
        }
    }
    private:
    ib_helper::IBConnector* ib = nullptr;
    uint32_t subscriptionId = 0;
    Contract contract;
    std::vector<BookRecord> bids;
    std::vector<BookRecord> asks;
};

class ScalpStrategy : public ib_helper::TickHandler
{
    public:
    ScalpStrategy(ib_helper::IBConnector* conn, Contract& contract) : ib(conn), contract(contract), book(conn, contract)
    {
        // watch book, looking for a big bidder ( 3 stddev ) at a round number (x.00 x.50)
        // long 0.01 above, TP at 15
        // stop at 0.94/0.01
        // then at 0.97/0.07
        // then at 0.07/0.12
        // try to get in at 0.01, cancel if hits 0.07 without entry
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
    }
    virtual void OnTickByTickBidAsk(int reqId, time_t time, double bidPrice, double askPrice, Decimal bidSize,
            Decimal askSize, const TickAttribBidAsk& tickAttribBidAsk)
    {
    }
    virtual void OnTickByTickMidPoint(int reqId, time_t time, double midPoint)
    {
    }

    double levelToWatch = 0.0; // will have a positive price if a large bidder is found
    ib_helper::IBConnector* ib = nullptr;
    Contract contract;
    Book book;
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

    ib_helper::IBConnector conn(host, strtol(port.c_str(), nullptr, 10), strtol(clientId.c_str(), nullptr, 10));
    AccountManager accountManager(&conn, accountNumber);
    conn.AddAccountHandler(&accountManager);
    ib_helper::ContractBuilder contractBuilder(&conn);
    Contract contract = contractBuilder.BuildStock(ticker);
    ScalpStrategy strategy(&conn, contract);

    while(!ctrl_c_pressed)
        std::this_thread::sleep_for(std::chrono::seconds(1));
}

