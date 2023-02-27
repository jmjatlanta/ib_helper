#pragma once
#include <string>

class TickHandler 
{
    public:
	virtual void OnTickPrice(int tickerId, int field, double price, TickAttrib attribs) = 0;
	virtual void OnTickSize(int tickerId, int field, Decimal size) = 0;
	virtual void OnTickOptionComputation(int tickerId, int field, int tickAttrib, double impliedVol, 
            double delta, double optPrice, double pvDividend, double gamma, double vega, double theta, 
            double undPrice) = 0;
	virtual void OnTickGeneric(int tickerId, int tickType, double value) = 0;
	virtual void OnTickString(int tickerId, int tickType, const std::string& value) = 0;
	virtual void OnTickEFP(int tickerId, int tickType, double basisPoints, const std::string& formattedBasisPoints, 
            double impliedFuture, int holdDays, const std::string& futureLastTradeDate, double dividendImpact, 
            double dividendsToLastTradeDate) = 0;
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
            const std::string& exchange, const std::string& specialConditions) = 0;
    virtual void OnTickBidAsk(int reqId, long time, double bidPrice, double askPrice, Decimal bidSize, Decimal askSize,
            TickAttribBidAsk tickAttribBidAsk) = 0;
    virtual void OnTickMidPoint(int reqId, long time, double midPoint) = 0;


}

