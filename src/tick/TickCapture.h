#pragma once
#include <fstream>
#include "Contract.h"
#include "IBConnector.hpp"

#pragma pack(1)
class AllLast
{
    public:
    time_t time;
    double price;
    uint64_t size;

    friend std::string to_string(const AllLast&);
};
#pragma pack(8)

std::string to_string(const AllLast&);
/***
 * capture ticks into a tick database
*/
class TickCapture : public ib_helper::TickHandler
{
    public:
    TickCapture(const Contract& contract, std::shared_ptr<ib_helper::IBConnector> conn, std::ofstream& out);

    void OnTickPrice(int tickerId, int field, double price, TickAttrib attribs) override {}
    void OnTickSize(int tickerId, int field, Decimal size) override {}
    void OnTickOptionComputation(int tickerId, int field, int tickAttrib, double impliedVol, double delta, 
            double optPrice, double pvDividend, double gamma, double vega, double theta, double undPrice) override {}
    void OnTickGeneric(int tickerId, int tickType, double value) override {}
    void OnTickString(int tickerId, int tickType, const std::string& value) override {}
    void OnTickEFP(int tickerId, int tickType, double basisPoints, const std::string& formattedBasisPoints, 
            double impliedFuture, int holdDays, const std::string& futureLastTradeDate, double dividendImpact, 
            double dividendsToLastTradeDate) override {}
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
    void OnTickLast(int reqId, int tickType, long time, double price, Decimal size, TickAttribLast tickAttribLast,
            const std::string& exchange, const std::string&  specialConditions) override {}
    void OnTickByTickAllLast(int reqId, int tickType, time_t time, double price, Decimal size,
            const TickAttribLast& tickAttribLast, const std::string& exchange, 
            const std::string& specialConditions) override;
    void OnTickByTickBidAsk(int reqId, time_t time, double bidPrice, double askPrice, Decimal bidSize,
            Decimal askSize, const TickAttribBidAsk& tickAttribBidAsk) override {}
    void OnTickByTickMidPoint(int reqId, time_t time, double midPoint) override {}

private:
    Contract contract;
    std::shared_ptr<ib_helper::IBConnector> conn;
    int reqId = 0;
    std::ofstream& out;
};

class TickReader
{
    public:
    TickReader(std::ifstream& in) : in(in) {}

    AllLast read();

    private:
    std::ifstream& in;
};
