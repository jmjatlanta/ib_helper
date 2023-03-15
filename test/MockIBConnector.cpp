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
    // process any pending orders
    for(auto& ord : orders)
    {
        if (ord.totalQuantity != ord.filledQuantity)
        {
            if (ord.orderType == "LMT")
            {
                if ( (ord.action == "BUY" && ord.lmtPrice <= lastPrice)
                        || (ord.action == "SELL" && ord.lmtPrice >= lastPrice))
                    processOrder(ord, lastPrice);
            }
            if (ord.orderType == "STP")
            {
                if( (ord.action == "BUY" && ord.auxPrice >= lastPrice)
                        || (ord.action == "SELL" && ord.auxPrice <= lastPrice))
                    processOrder(ord, lastPrice);
            }
        }
    }
}

void MockIBConnector::RequestAccountUpdates(bool subscribe, const std::string& account)
{
}

void MockIBConnector::SendBidAsk(uint32_t subscriptionId, double bid, double ask)
{
    ib_helper::TickHandler* handler = tickHandlers[subscriptionId];
    time_t time = 1;
    TickAttribBidAsk tickAttribBidAsk;
    Decimal bidSize = doubleToDecimal(100.0);
    Decimal askSize = doubleToDecimal(100.0);
    handler->OnTickByTickBidAsk(subscriptionId, time, bid, ask, bidSize, askSize, tickAttribBidAsk);
}

void MockIBConnector::PlaceOrder(int orderId, const Contract& contract, const ::Order& order)
{
    if (orderRejectCode == 0)
    {
        orders.push_back(order);
        MockOrder& mOrder = orders[orders.size()-1];
        mOrder.contract = contract;
        mOrder.status = "PreSubmitted";
        double price = 0.0;
        if (mOrder.orderType == "LMT")
            price = mOrder.lmtPrice;
        if (mOrder.orderType == "STP")
            price = mOrder.auxPrice;
        // NOTE: Valid statuses: PreSubmitted, Submitted, Filled, Cancelled
        orderStatus(orderId, mOrder.status, order.filledQuantity, order.totalQuantity, 
                0.0, orderId, 0, 0.0, 123, "", 0.0);
        if (processOrdersImmediately && price != 0.0)
            processOrder(mOrder, price);
    }
    else
    {
        error(orderId, orderRejectCode, "Order processing error string", "");
    }
}

void MockIBConnector::processOrder(MockOrder& order, double price)
{
    order.filledQuantity = order.totalQuantity;
    order.status = "Filled";
    orderStatus(order.orderId, order.status, order.filledQuantity, 
            sub(order.totalQuantity, order.filledQuantity), price, 
            order.orderId, 0, price, 123, "", 0.0);
}

void MockIBConnector::CancelOrder(int orderId, const std::string& time)
{
    // find order
    for(auto ord : orders)
    {
        if (ord.orderId == orderId)
        {
            //TODO: These are probably wrong codes. Find out what IB sends in these situations
            if (ord.status == "Cancelled")
            {
                error(orderId, 161, "Cannot cancel, already cancelled", "");
            }
            if (ord.totalQuantity == ord.filledQuantity)
            {
                error(orderId, 161, "Cannot cancel, already cancelled", "");
            }
            orderStatus(orderId, "Cancelled", ord.filledQuantity, 
                sub(ord.totalQuantity, ord.filledQuantity),
                ord.auxPrice, ord.orderId, 0, ord.auxPrice, 123, "", 0.0);
            return;
        }
    }
}
