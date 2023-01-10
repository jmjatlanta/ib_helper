#include "ib_helper/TickHandler.hpp"
#include "ib_helper/MarketDepthHandler.hpp"
#include "ib_helper/IBConnector.hpp"
#include <iostream>
#include <vector>
#include <functional>

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
    Book(IBConnector* ib, Contract contract) : ib(ib), contract(contract)
    {
        subscriptionId = ib->AddMarketDeptHandler(this, contract);
    }
    /***
     * MarketDepthHander methods
     */

	virtual void OnUpdateMktDepth(int tickerId, int position, int operation, int side, double price, Decimal size)
    {
    }
	virtual void OnUpdateMktDepthL2(int tickerId, int position, const std::string& marketMaker, int operation, 
            int side, double price, Decimal size, bool isSmartDepth)
    {
        // add
        // update
        // delete
    }
    private:
    IBConnector* ib = nullptr;
    uint32_t subscriptionId = 0;
    Contract contract;
    std::vector<BookRecord, std::greater<BookRecord> >  bids;
    std::vector<BookRecord, std::less<BookRecord> > asks;
};

class ScalpStrategy : public ib_helper::TickHandler
{
    double levelToWatch = 0.0; // will have a positive price if a large bidder is found
    public:
    ScalpStrategy()
    {
        // watch book, looking for a big bidder ( 3 stddev ) at a round number (x.00 x.50)
        // long 0.01 above, TP at 15
        // stop at 0.94/0.01
        // then at 0.97/0.07
        // then at 0.07/0.12
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
};

int main(int argc, char** argv)
{
    // connectivity, stock
    // start strategy
}

