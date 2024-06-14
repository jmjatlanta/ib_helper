#include "MockIBConnector.h"
#include "../src/util/SysLogger.h"
#include <stdexcept>
#include <exception>
#include <algorithm>

MockIBConnector::MockIBConnector(const std::string& hostname, int port, int clientId) 
    : nextRequestId(1), nextOrderId(1)
{
    logger = util::SysLogger::getInstance();
    this->SetDefaultAccount("ABC123");
}

MockIBConnector::~MockIBConnector() {}

uint32_t MockIBConnector::GetNextRequestId()
{
    return ++nextRequestId;
}

void doGetContractDetails(const Contract& contract, std::promise<std::vector<ContractDetails>>& promise)
{
    if ((contract.symbol == "MSFT" && contract.secType == "STK")
            || (contract.symbol == "ES" && contract.secType == "FUT"))
    {
        ContractDetails dets;
        dets.contract = contract;
        std::vector<ContractDetails> retval;
        retval.push_back(dets);
        promise.set_value(retval);
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

std::future<std::vector<ContractDetails>> MockIBConnector::GetContractDetails(const Contract& contract)
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
    for(auto pos : positions)
    {
        position(pos.account, pos.contract, pos.position, pos.avgCost);
    }
    positionEnd();
}

void MockIBConnector::RequestOpenOrders()
{
    for(auto o : openOrders)
    {
        openOrder(o.orderId, o.contract, o.order, o.orderState);
    }
    openOrderEnd();
}

void MockIBConnector::RequestAllOpenOrders()
{
    return RequestOpenOrders();
}

uint32_t MockIBConnector::RequestExecutionReports(const ExecutionFilter& filter)
{
    uint32_t id = GetNextRequestId();
    for(auto h : executionHandlers)
        h->OnExecDetailsEnd(id);
    return id;
}

void MockIBConnector::AddConnectionMonitor(ib_helper::IBConnectionMonitor* in)
{
    // add it to the list
    connectionMonitors.push_back(in);
    in->OnConnect(this);
}

uint32_t MockIBConnector::SubscribeToHistoricalData(const Contract& contract, ib_helper::HistoricalDataHandler* handler,
            const std::string& endDateTime, const std::string& timePeriod, const std::string& barSize, bool rth)
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

void MockIBConnector::SendBarWithTick(int barSubId, int tickSubId, int bidAskSubId, const Bar& bar, bool inPast)
{
    if (inPast)
    {
        SendBar(barSubId, bar, inPast);
    }
    else
    {
        // open
        Bar b(bar);
        b.high = b.open;
        b.low = b.open;
        b.close = b.open;
        SendBar(barSubId, b, inPast);
        SendBidAsk(tickSubId, b.close - 0.01, b.close + 0.01);
        SendTick(tickSubId, b.close);
        // low
        b.low = bar.low;
        b.close = b.low;
        SendBar(barSubId, b, false);
        SendBidAsk(tickSubId, b.close - 0.01, b.close - 0.01);
        SendTick(tickSubId, b.close);
        // high
        b.high = bar.high;
        b.close = b.high;
        SendBar(barSubId, b, false);
        SendBidAsk(tickSubId, b.close - 0.01, b.close - 0.01);
        SendTick(tickSubId, b.close);
        // close
        b.close = bar.high;
        SendBar(barSubId, b, false);
        SendBidAsk(tickSubId, b.close - 0.01, b.close - 0.01);
        SendTick(tickSubId, b.close);
    }
}

void MockIBConnector::SendHistoricalDataEnd(int subId)
{
    auto itr = historicalDataHandlers.find(subId);
    if (itr != historicalDataHandlers.end())
        (*itr).second->OnHistoricalDataEnd(subId, "", "");
}

void MockIBConnector::SendBar(int subId, const Bar& in, bool inPast)
{
    auto itr = historicalDataHandlers.find(subId);
    if (itr != historicalDataHandlers.end())
    {
        ib_helper::HistoricalDataHandler* handler = (*itr).second;
        if (inPast)
            handler->OnHistoricalData(subId, in);
        else
            handler->OnHistoricalDataUpdate(subId, in);
    }
}

uint32_t MockIBConnector::SubscribeToTickByTick(const Contract& contract, ib_helper::TickHandler* handler, 
        const std::string& tickType, int numberOfTicks, bool ignoreSize)
{
    uint32_t reqId = GetNextRequestId();
    tickHandlers[reqId] = handler;
    return reqId;
};
uint32_t MockIBConnector::SubscribeToMarketData(const Contract& contract, ib_helper::TickHandler* tickHandler, 
        const std::string& genericTickList, bool snapshot, bool regulatorySnapshot, 
        const TagValueListSPtr& mktDataOptions)
{
    uint32_t reqId = GetNextRequestId();
    tickHandlers[reqId] = tickHandler;
    return reqId;
}

void MockIBConnector::SendTick(int subId, double lastPrice)
{
    int tickType = 1;
    time_t time = 1;
    double size = 100;
    TickAttribLast tickAttribLast;
    std::string exchange;
    std::string specialConditions;
    // process any pending orders
    std::vector<MockOrder> processedOrders; // delayed status updates
    std::vector<MockOrder> submittedOrders; // delayed status updates
    {
        std::lock_guard lock(ordersMutex);
        for(auto& ord : orders)
        {
            if (ord.totalQuantity != ord.filledQuantity)
            {
                bool processed = false;
                if (ord.orderType == "MKT")
                {
                    if (processOrder(ord, lastPrice))
                    {
                        processed = true;
                        processedOrders.push_back(ord);
                    }
                }
                if (ord.orderType == "LMT")
                {
                    if ( (ord.action == "BUY" && ord.lmtPrice >= lastPrice)
                            || (ord.action == "SELL" && ord.lmtPrice <= lastPrice))
                    {
                        if(processOrder(ord, lastPrice))
                        {
                            processed = true;
                            processedOrders.push_back(ord);
                        }
                    }
                }
                if (ord.orderType == "STP")
                {
                    if( (ord.action == "BUY" && ord.auxPrice <= lastPrice)
                            || (ord.action == "SELL" && ord.auxPrice >= lastPrice))
                    {
                        if (processOrder(ord, lastPrice))
                        {
                            processed = true;
                            processedOrders.push_back(ord);
                        }
                    }
                }
                if (!processed && !ord.submitted)
                {
                    if (submitOrder(ord))
                        submittedOrders.push_back(ord);
                }
            }
        }
    }
    // update statuses (without lock)
    for(auto order : processedOrders)
    {
        orderStatus(order.orderId, order.status, order.filledQuantity,
                sub(order.totalQuantity, order.filledQuantity), lastPrice,
                order.orderId, 0, lastPrice, 123, "", 0.0);
    }
    for(auto order : submittedOrders)
    {
        orderStatus(order.orderId, order.status, order.filledQuantity,
                sub(order.totalQuantity, order.filledQuantity), 0.0,
                order.orderId, 0, 0.0, 123, "", 0.0);
    }
    // send tick to listeners
    auto itr = tickHandlers.find(subId);
    if (itr != tickHandlers.end())
    {
        ib_helper::TickHandler* handler = (*itr).second;
        handler->OnTickByTickAllLast(subId, tickType, time, lastPrice, size, tickAttribLast, exchange, specialConditions);
    }
}

void MockIBConnector::RequestAccountUpdates(bool subscribe, const std::string& account)
{
}

void MockIBConnector::SendBidAsk(uint32_t subscriptionId, double bid, double ask)
{
    auto itr = tickHandlers.find(subscriptionId);
    if (itr != tickHandlers.end())
    {
        ib_helper::TickHandler* handler = (*itr).second;
        time_t time = 1;
        TickAttribBidAsk tickAttribBidAsk;
        Decimal bidSize = doubleToDecimal(100.0);
        Decimal askSize = doubleToDecimal(100.0);
        handler->OnTickByTickBidAsk(subscriptionId, time, bid, ask, bidSize, askSize, tickAttribBidAsk);
    }
}

MockOrder& MockIBConnector::findOrderById(uint32_t orderId)
{
    // NOTE: non-locking
    auto itr = std::find_if(orders.begin(), orders.end(), [orderId](const MockOrder& curr){ return curr.orderId == orderId; });
    if (itr == orders.end())
        throw std::out_of_range(std::string("Order Id ") + std::to_string(orderId) + " not found.");
    return (*itr);
}

bool MockIBConnector::validateOrder(int orderId, const Contract& contract, const ::Order& order)
{
    // orderId
    if (orderId <= 0)
    {
        error(orderId, 390, "Supplied routed order ID is invalid.", "");
        return false;
    }
    if (decimalToDouble(order.totalQuantity) <= 0.0)
    {
        error(orderId, 160, "The size value cannot be zero.", "");
        return false;
    }
    if (order.orderType == "LMT" && order.lmtPrice <= 0.0)
    {
        error(orderId, 361, "Invalid trigger price", "");
        return false;
    }
    if (order.orderType == "STP" && order.auxPrice <= 0.0)
    {
        error(orderId, 361, "Invalid trigger price", "");
        return false;
    }
    return true;
}

void MockIBConnector::PlaceOrder(int orderId, const Contract& contract, const ::Order& order)
{
    if (orderRejectCode == 0)
    {
        // do some basic validation
        if (!validateOrder(orderId, contract, order))
            return;
        // are we adding or modifying?
        try
        {
            MockOrder copyOfOrder;
            {
                std::lock_guard lock(ordersMutex);
                MockOrder& mOrder = findOrderById(orderId);
                mOrder.update(order);
                copyOfOrder = mOrder;
            }
            double price = 0.0;
            if (copyOfOrder.orderType == "LMT")
                price = copyOfOrder.lmtPrice;
            if (copyOfOrder.orderType == "STP")
                price = copyOfOrder.auxPrice;
            logger->debug(clazz, "Order " + std::to_string(orderId) 
                    + " placed with type of " + copyOfOrder.orderType
                    + " and price of " + std::to_string(price)
                    + " and size of " + std::to_string(decimalToDouble(copyOfOrder.totalQuantity)));
            // NOTE: Valid statuses: PreSubmitted, Submitted, Filled, Cancelled
            orderStatus(orderId, copyOfOrder.status, order.filledQuantity, order.totalQuantity, 
                    0.0, orderId, 0, 0.0, 123, "", 0.0);
        } catch( const std::out_of_range& oor)
        {
            MockOrder copyOfOrder;
            {
                std::lock_guard lock(ordersMutex);
                orders.push_back(order);
                MockOrder& mOrder = orders[orders.size()-1];
                mOrder.contract = contract;
                mOrder.status = "PreSubmitted";
                mOrder.orderId = orderId;
                copyOfOrder = mOrder;
            }
            double price = 0.0;
            if (copyOfOrder.orderType == "LMT")
                price = copyOfOrder.lmtPrice;
            if (copyOfOrder.orderType == "STP")
                price = copyOfOrder.auxPrice;
            logger->debug(clazz, "Order " + std::to_string(orderId) 
                    + " placed with type of " + copyOfOrder.orderType
                    + " and price of " + std::to_string(price)
                    + " and size of " + std::to_string(decimalToDouble(copyOfOrder.totalQuantity)));
            // NOTE: Valid statuses: PreSubmitted, Submitted, Filled, Cancelled
            orderStatus(orderId, copyOfOrder.status, order.filledQuantity, order.totalQuantity, 
                    0.0, orderId, 0, 0.0, 123, "", 0.0);
        }
    }
    else
    {
        error(orderId, orderRejectCode, "Order processing error string", "");
    }
}

bool MockIBConnector::processOrder(MockOrder& order, double price)
{
    if (order.status == "PreSubmitted")
        return false;

    double fillQty = std::min( decimalToDouble(order.totalQuantity), maxOrderFillSize );
    order.filledQuantity = doubleToDecimal(fillQty);
    order.status = "Filled";
    return true;
}

bool MockIBConnector::submitOrder(MockOrder& order)
{
    if (order.status == "PreSubmitted")
    {
        order.submitted = true;
        order.status = "Submitted";
        return true;
    }
    return false;
}

void MockIBConnector::CancelOrder(int orderId, const std::string& time)
{
    // find order
    std::vector<MockOrder> matches;
    {
        std::lock_guard lock(ordersMutex);
        for(auto ord : orders)
        {
            if (ord.orderId == orderId)
            {
                matches.push_back(ord);
            }
        }
    }
    for(auto ord : matches)
    {
        //TODO: These are probably wrong codes. Find out what IB sends in these situations
        if (ord.status == "Cancelled")
        {
            error(orderId, 161, "Cannot cancel, already cancelled", "");
        }
        else if (ord.totalQuantity == ord.filledQuantity)
        {
            error(orderId, 161, "Cannot cancel, already cancelled", "");
        }
        orderStatus(orderId, "Cancelled", ord.filledQuantity, 
            sub(ord.totalQuantity, ord.filledQuantity),
            ord.auxPrice, ord.orderId, 0, ord.auxPrice, 123, "", 0.0);
        return;
    }
}
void MockIBConnector::SendOpenOrder(int orderId, const Contract& contract, const Order& order,
        const OrderState& orderState)
{
    openOrders.push_back( MockOpenOrder{orderId, contract, order, orderState} );
}

void MockIBConnector::SendPosition(const std::string& account, const Contract& contract, 
        Decimal pos, double avgCost)
{
    positions.push_back( MockPosition{ account, contract, pos, avgCost } );
}

void MockIBConnector::orderStatus( OrderId orderId, const std::string& status, Decimal filled,
	        Decimal remaining, double avgFillPrice, int permId, int parentId,
	        double lastFillPrice, int clientId, const std::string& whyHeld, double mktCapPrice)
{
    // to resolve locking issues, do each OnOrderStatus in a separate thread
    for(auto handler : orderHandlers)
    {
        if (handler != nullptr)
        {
            std::thread t([handler, orderId, status, filled, remaining, avgFillPrice, permId, parentId, 
                    lastFillPrice, clientId, whyHeld, mktCapPrice]() {
                    handler->OnOrderStatus(orderId, status, filled, remaining, avgFillPrice, permId, parentId, 
                        lastFillPrice, clientId, whyHeld, mktCapPrice);
            });
            t.detach();
        }
    }
}
