#include "MockIBConnector.h"
#include "../src/util/SysLogger.h"
#include <stdexcept>
#include <exception>

MockIBConnector::MockIBConnector(const std::string& hostname, int port, int clientId) 
    : nextRequestId(1), nextOrderId(1)
{
}

MockIBConnector::~MockIBConnector() {}

uint32_t MockIBConnector::GetNextRequestId()
{
    return ++nextRequestId;
}

void doGetContractDetails(const Contract& contract, std::promise<ContractDetails>& promise)
{
    if ((contract.symbol == "MSFT" && contract.secType == "STK")
            || (contract.symbol == "ES" && contract.secType == "FUT"))
    {
        ContractDetails dets;
        dets.contract = contract;
        promise.set_value(dets);
    }
    else
    {
        try
        {
            promise.set_exception(std::make_exception_ptr( std::out_of_range("Invalid symbol: " + contract.symbol ))); 
        } catch (...)
        {
            // set_exception can throw
            util::SysLogger::getInstance()->error("set_exception threw");
        }
    }
}

std::future<ContractDetails> MockIBConnector::GetContractDetails(const Contract& contract)
{
    // we have some that are good and some that are bad
    uint32_t promiseId = GetNextRequestId();
    auto& promise = contractDetailsHandlers[promiseId];
    doGetContractDetails(contract, promise);
    return promise.get_future();
}

bool MockIBConnector::IsConnected() const { return true; }

void MockIBConnector::RequestPositions()
{
    positionEnd();
}

void MockIBConnector::RequestOpenOrders()
{
    openOrderEnd();
}

void MockIBConnector::AddConnectionMonitor(ib_helper::IBConnectionMonitor* in)
{
    // add it to the list
    connectionMonitors.push_back(in);
    in->OnConnect(this);
}

uint32_t MockIBConnector::SubscribeToHistoricalData(const Contract& contract, ib_helper::HistoricalDataHandler* handler,
            const std::string& timePeriod, const std::string& barSize)
{
    int id = GetNextRequestId();
    historicalDataHandlers[id] = handler;
    return id;
}

void MockIBConnector::UnsubscribeFromHistoricalData(uint32_t subId)
{
    // do nothing
}

void MockIBConnector::UnsubscribeFromTickByTick(uint32_t subId)
{
    // do nothing
}

void MockIBConnector::SendBar(int subId, const Bar& in, bool inPast)
{
    ib_helper::HistoricalDataHandler* handler = historicalDataHandlers[subId];
    if (inPast)
        handler->OnHistoricalData(subId, in);
    else
        handler->OnHistoricalDataUpdate(subId, in);
}

uint32_t MockIBConnector::SubscribeToTickByTick(const Contract& contract, ib_helper::TickHandler* handler, 
        const std::string& tickType, int numberOfTicks, bool ignoreSize)
{
    uint32_t reqId = GetNextRequestId();
    tickHandlers[reqId] = handler;
    return reqId;
};

void MockIBConnector::SendTick(int subId, double lastPrice)
{
    ib_helper::TickHandler* handler = tickHandlers[subId];
    int tickType = 1;
    time_t time = 1;
    double size = 100;
    TickAttribLast tickAttribLast;
    std::string exchange;
    std::string specialConditions;
    handler->OnTickByTickAllLast(subId, tickType, time, lastPrice, size, tickAttribLast, exchange, specialConditions);
}

