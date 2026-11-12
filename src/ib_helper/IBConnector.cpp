#include "IBConnector.hpp"
#include "../util/Logger.h"
#include "ScannerSubscription.h"
#include <algorithm>

std::string to_string(EClient::ConnState in)
{
    switch(in)
    {
        case EClient::ConnState::CS_CONNECTED:
            return "CS_CONNECTED";
        case EClient::ConnState::CS_CONNECTING:
            return "CS_CONNECTING";
        case EClient::ConnState::CS_DISCONNECTED:
            return "CS_DISCONNECTED";
        case EClient::ConnState::CS_REDIRECT:
            return "CS_REDIRECT";
    }
    return "UNKNOWN";
}

namespace ib_helper {

Logger* logger = nullptr;
std::string logCategory("IBConnector");


std::string to_string(IBConnector::ConnectionStatus in)
{
    switch(in)
    {
        case IBConnector::ConnectionStatus::NOT_STARTED:
            return "NOT_STARTED";
        case IBConnector::ConnectionStatus::ATTEMPTING_CONNECTION:
            return "ATTEMPTING_CONNECTION";
        case IBConnector::ConnectionStatus::PARTIALLY_CONNECTED:
            return "PARTIALLY_CONNECTED";
        case IBConnector::ConnectionStatus::FULLY_CONNECTED:
            return "FULLY_CONNECTED";
        case IBConnector::ConnectionStatus::ATTEMPTING_SHUTDOWN:
            return "ATTEMPTING_SHUTDOWN";
        case IBConnector::ConnectionStatus::SHUTDOWN:
            return "SHUTDOWN";
        default:
            break;
    }
    return "UNKNOWN";
}

IBConnector::IBConnector() : currentConnectionStatus(ConnectionStatus::NOT_STARTED)
{
    logger = Logger::getInstance();
}

IBConnector::IBConnector(const std::string& hostname, int port, int clientId, IBConnectionMonitor* connMonitor) 
        : hostname(hostname), port(port), clientId(clientId), 
        currentConnectionStatus(ConnectionStatus::NOT_STARTED)
{
    logger = Logger::getInstance();
    AddConnectionMonitor(connMonitor);
    connect();
}

bool IBConnector::connect()
{
    if (!IsConnected() 
            && currentConnectionStatus != ConnectionStatus::ATTEMPTING_CONNECTION 
            && currentConnectionStatus != ConnectionStatus::PARTIALLY_CONNECTED)
    {
        currentConnectionStatus = ConnectionStatus::ATTEMPTING_CONNECTION;
        //logger->debug("IBConnector", "connect: Attempting to connect to IB host " + hostname + ":" + std::to_string(port));
        osSignal = new EReaderOSSignal(1000); // timeout (1000 == 1 sec)
        ibClient = new EClientSocket(this, osSignal);
        if (!ibClient->eConnect(hostname.c_str(), port, clientId, false))
        {
            logger->debug("IBConnector", "connect: eConnect failed for IB host " + hostname + ":" + std::to_string(port));
            // clean up the mess
            try
            {
                // stop listener thread
                currentConnectionStatus = ConnectionStatus::ATTEMPTING_SHUTDOWN;
                if (ibClient != nullptr)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    delete ibClient;
                    ibClient = nullptr;
                }
                if (osSignal != nullptr)
                {
                    delete osSignal;
                    osSignal = nullptr;
                }
            } 
            catch(...)
            {
                logger->error("IBConnector", "connect() threw an exception");
            }
            currentConnectionStatus = ConnectionStatus::NOT_STARTED;
            return false;
        }
        if (currentConnectionStatus != ConnectionStatus::FULLY_CONNECTED)
            currentConnectionStatus = ConnectionStatus::PARTIALLY_CONNECTED;
        if (listenerThread != nullptr && listenerThread->joinable())
            listenerThread->join();
        listenerThread = std::make_shared<std::thread>(&IBConnector::processMessages, this);
        return true;
    }
    return IsConnected();
}

bool IBConnector::disconnect()
{
    logger->debug("IBConnector", "disconnect: Attempting to disconnect from IB host " + hostname + ":" + std::to_string(port));
    currentConnectionStatus = ConnectionStatus::ATTEMPTING_SHUTDOWN;
    // there should be no more EReader messages after the thread shuts down (EReader is destroyed
    if (listenerThread != nullptr && listenerThread->joinable())
        listenerThread->join();
    if (ibClient != nullptr)
    {
        //ibClient->eDisconnect(); <-- called from ereader in dtor
        // give some time to inform other threads
        //std::this_thread::sleep_for(std::chrono::milliseconds(500));
        delete ibClient;
        ibClient = nullptr;
    }
    if (osSignal != nullptr)
    {
        delete osSignal;
        osSignal = nullptr;
    }
    currentConnectionStatus = ConnectionStatus::SHUTDOWN;
    // lock the mutex
    std::scoped_lock lock(connectionMonitorsMutex);
    std::for_each(connectionMonitors.begin(), connectionMonitors.end(), [this](IBConnectionMonitor* curr)
                  { curr->OnDisconnect(this); });
    logger->debug("IBConnector", "disconnect completed");
    return true;
}

IBConnector::~IBConnector() {
    disconnect();
}
bool IBConnector::IsConnected() const { return currentConnectionStatus == ConnectionStatus::FULLY_CONNECTED; }
bool IBConnector::IsConnecting() const { return currentConnectionStatus == ConnectionStatus::ATTEMPTING_CONNECTION; }

std::future<std::vector<SecurityDefinitionOptionParameter>> IBConnector::GetOptionParameters(const Contract& contract)
{
    if (ibClient == nullptr)
    {
        std::promise<std::vector<SecurityDefinitionOptionParameter>> p;
        p.set_exception(std::exception_ptr{});
        return p.get_future();
    }
    int promiseId = GetNextRequestId();
    auto& promise = securityDefinitionHandlers[promiseId];
    auto& vec = securityDefinitionData[promiseId]; // put the vector in there
    //logger->debug("IBConnector", "GetOptionParameters: about to request details for " + contract.symbol + " id: " + std::to_string(promiseId) );
    ibClient->reqSecDefOptParams(promiseId, contract.symbol, "", contract.secType, contract.conId);
    //logger->debug("IBConnector", "GetContractDetails: request made for " + contract.symbol);
    return promise.get_future();
}
std::future<std::vector<ContractDetails>> IBConnector::GetContractDetails(const Contract& contract)
{
    if (ibClient == nullptr)
    {
        std::promise<std::vector<ContractDetails>> p;
        p.set_exception(std::exception_ptr{});
        return p.get_future();
    }
    int promiseId = GetNextRequestId();
    std::scoped_lock lock(contractDetailsHandlersMutex, contractDetailsDataMutex);
    auto& promise = contractDetailsHandlers[promiseId];
    auto& vec = contractDetailsData[promiseId]; // put the vector in there
    //logger->debug("IBConnector", "GetContractDetails: about to request details for " + contract.symbol + " id: " + std::to_string(promiseId) );
    ibClient->reqContractDetails(promiseId, contract);
    //logger->debug("IBConnector", "GetContractDetails: request made for " + contract.symbol);
    return promise.get_future();
}

void IBConnector::RequestPositions()
{
    if (ibClient != nullptr)
        ibClient->reqPositions();
}

int IBConnector::RequestExecutionReports(const ExecutionFilter& filter)
{
    int reqId = GetNextRequestId();
    if (ibClient != nullptr)
        ibClient->reqExecutions(reqId, filter);
    return reqId;
}

void IBConnector::RequestAccountUpdates(bool subscribe, const std::string& account)
{
    if (ibClient != nullptr)
        ibClient->reqAccountUpdates(subscribe, account);
}

void IBConnector::RequestOpenOrders()
{
    if (ibClient != nullptr)
        ibClient->reqOpenOrders();
}

void IBConnector::RequestAllOpenOrders()
{
    if (ibClient != nullptr)
        ibClient->reqAllOpenOrders();
}

void IBConnector::AddAccountHandler(AccountHandler* in)
{
    std::scoped_lock lock(accountHandlersMutex);
    accountHandlers.push_back(in);
}

void IBConnector::RemoveAccountHandler(AccountHandler* in)
{
    std::scoped_lock lock(accountHandlersMutex);
    std::erase(accountHandlers, in);
}

void IBConnector::AddScannerHandler(ScannerHandler* in)
{
    std::scoped_lock lock(scannerHandlerMutex);
    scannerHandlers.push_back(in);
}

void IBConnector::RemoveScannerHandler(ScannerHandler* in)
{
    std::scoped_lock lock(scannerHandlerMutex);
    std::erase(scannerHandlers, in);
}

void IBConnector::RequestScannerParameters()
{
    ibClient->reqScannerParameters();
}

int IBConnector::RequestScannerSubscription( ScannerSubscription scannerSubscription, 
            TagValueListSPtr scannerSubscriptionOptions, TagValueListSPtr scannerSubscriptionFilterOptions)
{
    if (ibClient == nullptr)
        return -1;
    int nextReq = GetNextRequestId();
    {
        std::scoped_lock lock(scannerHandlerMutex);
        scannerRequestIds.insert(nextReq);
    }
    ibClient->reqScannerSubscription(nextReq, scannerSubscription, scannerSubscriptionOptions, scannerSubscriptionFilterOptions);
    return nextReq;
}

void IBConnector::CancelScannerSubscription(int reqId)
{
    //logger->debug("IBConnector", "CancelScannerSubscription called with reqId of " + std::to_string(reqId));
    ibClient->cancelScannerSubscription(reqId);
}

void IBConnector::AddErrorHandler(ErrorHandler* in)
{
    if (in == nullptr)
        return;
    std::scoped_lock lock(errorHandlersMutex);
    if (std::find(errorHandlers.begin(), errorHandlers.end(), in) == errorHandlers.end())
        errorHandlers.push_back(in);
}

void IBConnector::RemoveErrorHandler(ErrorHandler* in)
{
    std::scoped_lock lock(errorHandlersMutex);
    std::erase(errorHandlers, in);
}

void IBConnector::AddOrderHandler(OrderHandler* in)
{
    std::scoped_lock lock(orderHandlersMutex);
    for(auto* handler : orderHandlers)
        if (handler == in)
            return;
    orderHandlers.push_back(in);
}

void IBConnector::RemoveOrderHandler(OrderHandler* in)
{
    std::scoped_lock lock(orderHandlersMutex);
    std::erase(orderHandlers, in);
}

void IBConnector::AddExecutionHandler(ExecutionHandler* in)
{
    std::scoped_lock lock(executionHandlersMutex);
    for(auto* handler : executionHandlers)
        if (handler == in)
            return;
    executionHandlers.push_back(in);
}

void IBConnector::RemoveExecutionHandler(ExecutionHandler* in)
{
    std::scoped_lock lock(executionHandlersMutex);
    executionHandlers.erase(std::remove(executionHandlers.begin(), executionHandlers.end(), in), executionHandlers.end());
}

int IBConnector::GetNextOrderId()
{
    int id = ++nextOrderId;
    if (!IsOrderId(id) && logger != nullptr)
        logger->error(logCategory, "GetNextOrderId: order id " + std::to_string(id)
                + " is outside the order id range and may collide with request ids");
    return id;
}

int IBConnector::GetNextRequestId()
{
    int current = nextRequestId.load();
    int next;
    do
    {
        next = (current >= INT_MAX || current < REQUEST_ID_BASE) ? REQUEST_ID_BASE : current + 1;
    } while (!nextRequestId.compare_exchange_weak(current, next));
    return next;
}

AccountHandler* IBConnector::GetDefaultAccountHandler()
{ 
    std::scoped_lock lock(accountHandlersMutex);
    if (accountHandlers.size() == 0) 
        return nullptr; 
    return accountHandlers[0]; 
}

std::vector<std::string> IBConnector::GetAccounts() const
{
    return accounts;
}

void IBConnector::RemoveConnectionMonitor(IBConnectionMonitor* in)
{
    std::scoped_lock lock(connectionMonitorsMutex);
    for(auto itr = connectionMonitors.begin(); itr != connectionMonitors.end(); ++itr)
    {
        if ( (*itr) == in)
        {
            connectionMonitors.erase(itr);
            break;
        }
    }
}

void IBConnector::AddConnectionMonitor(IBConnectionMonitor* in)
{
    if (in != nullptr)
    {
        std::scoped_lock lock(connectionMonitorsMutex);
        auto itr = std::find(connectionMonitors.begin(), connectionMonitors.end(), in);
        if (itr == connectionMonitors.end())
            connectionMonitors.push_back(in);
    }
}

void IBConnector::CancelOrder(int orderId, const OrderCancel& obj)
{
    if (ibClient != nullptr)
        ibClient->cancelOrder(orderId, obj);
}

void IBConnector::PlaceOrder(int orderId, const Contract& contract, const ::Order& ord)
{
    if (ibClient != nullptr)
        ibClient->placeOrder(orderId, contract, ord);
}

static bool ConnectionShuttingDown(IBConnector::ConnectionStatus in)
{
    return in == IBConnector::ConnectionStatus::ATTEMPTING_SHUTDOWN || in == IBConnector::ConnectionStatus::SHUTDOWN;
}

void IBConnector::processMessages() {
    try
    {
        reader = new EReader(ibClient, osSignal);
        reader->start();
        while (!ConnectionShuttingDown(currentConnectionStatus)) 
        {
            osSignal->waitForSignal();
            if (!ConnectionShuttingDown(currentConnectionStatus))
            {
                //logger->debug("IBConnector", "processMessages: processing");
                reader->processMsgs();
            }
            //logger->debug("IBConnector", "processMessages: waiting");
        }
        logger->debug("IBConnector", "processMessages: shutting down reader");
    } 
    catch(const std::exception& ex)
    {
        logger->error("IBConnector", "processMessages: Standard exception: " + std::string(ex.what()));
    }
    catch(...)
    {
        logger->error("IBConnector", "processMessages: Exception thrown");
    }
    if (reader != nullptr)
        delete reader;
    reader = nullptr;
    logger->debug("IBConnector", "processMessages loop complete");
}

/***
 * Subscribe to tick data. What is returned is dependent on genericTickList
 * @param contract the contract
 * @param tickHandler who will receive the data
 * @param genericTickList the types of ticks desired
 * @param snapshot true to only get a snapshot
 * @param regulatorySnapshot true to only get a regulatory snapshot
 * @param mktDataOptions not currently used by us
 */
int IBConnector::SubscribeToMarketData(const Contract& contract, TickHandler* tickHandler, 
        const std::string& genericTickList, bool snapshot, bool regulatorySnapshot, 
        const TagValueListSPtr& mktDataOptions)
{
    if (ibClient == nullptr)
        return 0;

    int reqId = GetNextRequestId();
    {
        std::scoped_lock lock(tickHandlersMutex);
        tickHandlers[reqId] = tickHandler;
    }
    ibClient->reqMktData(reqId, contract, genericTickList, snapshot, regulatorySnapshot, mktDataOptions);
    return reqId;
};

void IBConnector::UnsubscribeFromMarketData(int reqId) 
{ 
    //logger->debug("IBConnector", "UnsubscribFromMarketData called with reqId of " + std::to_string(reqId));
    {
        std::scoped_lock lock(tickHandlersMutex);
        tickHandlers[reqId] = nullptr;
    }
    if (ibClient != nullptr)
        ibClient->cancelMktData(reqId); 
}

int IBConnector::SubscribeToTickByTick(const Contract& contract, TickHandler* handler, const std::string& tickType, 
            int numberOfTicks, bool ignoreSize)
{
    int reqId = GetNextRequestId();
    {
        std::scoped_lock lock(tickHandlersMutex);
        tickHandlers[reqId] = handler;
    }
    if (ibClient != nullptr)
        ibClient->reqTickByTickData(reqId, contract, tickType, numberOfTicks, ignoreSize);
    return reqId;
};

void IBConnector::UnsubscribeFromTickByTick(int reqId) 
{ 
    //logger->debug("IBConnector", "UnsubscribFromTickByTick called with reqId of " + std::to_string(reqId));
    if (ibClient != nullptr)
        ibClient->cancelTickByTickData(reqId); 
    {
        std::scoped_lock lock(tickHandlersMutex);
        tickHandlers[reqId] = nullptr;
    }
}

int IBConnector::SubscribeToMarketDepth(const Contract& contract, MarketDepthHandler* depthHandler, uint32_t numLines)
{
    int reqId = GetNextRequestId();
    {
        std::scoped_lock lock(marketDepthHandlersMutex);
        marketDepthHandlers[reqId] = depthHandler;
    }
    if (ibClient != nullptr)
    {
        TagValueListSPtr mktDepthOptions;
        ibClient->reqMktDepth(reqId, contract, numLines, true, mktDepthOptions);
    }
    return reqId;
}

void IBConnector::UnsubscribeFromMarketDepth(int subscriptionId)
{
    //logger->debug("IBConnector", "UnsubscribFromMarketDepth called with subscriptionId of " + std::to_string(subscriptionId));
    {
        std::scoped_lock lock(marketDepthHandlersMutex);
        marketDepthHandlers[subscriptionId] = nullptr;
    }
    if (ibClient != nullptr)
        ibClient->cancelMktDepth(subscriptionId, true);
}

std::future<std::vector<DepthMktDataDescription> > IBConnector::RequestMktDepthExchanges()
{
    std::scoped_lock lock(mktDepthExchangesPromisesMutex);
    auto& promise = mktDepthExchangesPromises.emplace_back( );
    if (ibClient != nullptr)
        ibClient->reqMktDepthExchanges();
    return promise.get_future();
}

void IBConnector::UnsubscribeFromHistoricalData(int historicalSubscriptionId)
{
    //logger->debug("IBConnector", "UnsubscribFromHistoricalData called with subscriptionId of " + std::to_string(historicalSubscriptionId));
    if (ibClient != nullptr)
        ibClient->cancelHistoricalData(historicalSubscriptionId);
    {
        std::scoped_lock lock(historicalDataHandlersMutex);
        historicalDataHandlers[historicalSubscriptionId] = nullptr;
    }
    //logger->debug("IBConnector", "UnsubscribeFromHistoricalData: " + std::to_string(historicalSubscriptionId));
}

int IBConnector::SubscribeToGroupEvents(DisplayGroupHandler* in, int groupId)
{
    std::scoped_lock lock(displayGroupHandlersMutex);
    auto id = GetNextRequestId();
    displayGroupHandlers.push_back(DisplayGroupCombination{in, groupId, id});
    ibClient->subscribeToGroupEvents(id, groupId);
    return id;
}

void IBConnector::UnsubscribeFromGroupEvents(int reqId)
{
    //logger->debug("IBConnector", "UnsubscribFromGropuEvents called with subscriptionId of " + std::to_string(reqId));
    std::scoped_lock lock(displayGroupHandlersMutex);
    std::erase_if(displayGroupHandlers, [reqId](const DisplayGroupCombination& curr)
                  { return curr.reqId == reqId; });
}

int IBConnector::SubscribeToHistoricalData(const Contract& contract, HistoricalDataHandler* handler,
        const std::string& endDateTime, const std::string& timePeriod, const std::string& barSize, bool rth)
{
    if (ibClient == nullptr)
        return 0;
    std::string whatToShow = "TRADES";
    if (contract.secType == "CASH")
        whatToShow = "MIDPOINT";
    int id = GetNextRequestId();
    {
        std::scoped_lock lock(historicalDataHandlersMutex);
        historicalDataHandlers[id] = handler;
    }
    //logger->debug("IBConnector", "SubscribeToHistoricalData: subscribing with id " + std::to_string(id)
    //        + " for " + contract.symbol
    //        + " endDateTime: " + endDateTime
    //        + " timePeriod: " + timePeriod
    //        + " barSize: " + barSize
    //        + " rth: " + (rth ? "true" : "false"));
    ibClient->reqHistoricalData(id, contract, endDateTime, timePeriod, barSize, whatToShow, rth, 2, endDateTime.empty(), TagValueListSPtr());
    return id;
}

void IBConnector::tickPrice( TickerId tickerId, TickType field, double price, const TickAttrib& attrib)
{
    // For a list of TickTypes, see https://interactivebrokers.github.io/tws-api/tick_types.html
    std::scoped_lock lock(tickHandlersMutex);
    auto itr = tickHandlers.find(tickerId);
    if (itr != tickHandlers.end() && (*itr).second != nullptr)
        (*itr).second->OnTickPrice(tickerId, field, price, attrib);
    else
    {
        std::string str;
        std::for_each(tickHandlers.begin(), tickHandlers.end(),
                [&str](const const std::pair<const int, TickHandler*>& curr)
                { if (curr.second != nullptr) str += ", " + std::to_string(curr.first); });
        logger->debug("IBConnector", "tickPrice: No subscribers for id " + std::to_string(tickerId)
                + ". Valid subscriber ids: " + str);
    }
}
void IBConnector::tickSize(TickerId tickerId, TickType field, Decimal size)
{
    try 
    {
        std::scoped_lock lock(tickHandlersMutex);
        TickHandler* handler = tickHandlers.at(tickerId);
        if (handler != nullptr)
            handler->OnTickSize(tickerId, field, size);
    } catch (const std::out_of_range& oor) {}
}
void IBConnector::tickOptionComputation( TickerId tickerId, TickType tickType, int tickAttrib, double impliedVol,
            double delta, double optPrice, double pvDividend, double gamma, double vega, double theta, double undPrice)
{
    try 
    {
        std::scoped_lock lock(tickHandlersMutex);
        TickHandler* handler = tickHandlers.at(tickerId);
        if (handler != nullptr)
            handler->OnTickOptionComputation(tickerId, tickType, tickAttrib, impliedVol, delta, optPrice, pvDividend,
                    gamma, vega, theta, undPrice);
    } catch (const std::out_of_range& oor) {}
}
void IBConnector::tickGeneric(TickerId tickerId, TickType tickType, double value)
{
    try 
    {
        std::scoped_lock lock(tickHandlersMutex);
        TickHandler* handler = tickHandlers.at(tickerId);
        if (handler != nullptr)
            handler->OnTickGeneric(tickerId, tickType, value);
    } catch (const std::out_of_range& oor) {}
}
void IBConnector::tickString(TickerId tickerId, TickType tickType, const std::string& value)
{
    try 
    {
        std::scoped_lock lock(tickHandlersMutex);
        TickHandler* handler = tickHandlers.at(tickerId);
        if (handler != nullptr)
            handler->OnTickString(tickerId, tickType, value);
    } catch (const std::out_of_range& oor) {}
}
void IBConnector::tickEFP(TickerId tickerId, TickType tickType, double basisPoints,
            const std::string& formattedBasisPoints, double totalDividends, int holdDays, 
            const std::string& futureLastTradeDate, double dividendImpact, double dividendsToLastTradeDate)
{
    try 
    {
        std::scoped_lock lock(tickHandlersMutex);
        TickHandler* handler = tickHandlers.at(tickerId);
        if (handler != nullptr)
            handler->OnTickEFP(tickerId, tickType, basisPoints, formattedBasisPoints, totalDividends, holdDays, 
                futureLastTradeDate, dividendImpact, dividendsToLastTradeDate);
    } catch (const std::out_of_range& oor) {}
}
void IBConnector::tickByTickAllLast(int reqId, int tickType, time_t time, double price, Decimal size,
            const TickAttribLast& tickAttribLast, const std::string& exchange, 
            const std::string& specialConditions)
{
    try 
    {
        std::scoped_lock lock(tickHandlersMutex);
        TickHandler* handler = tickHandlers.at(reqId);
        if(handler != nullptr)
            handler->OnTickByTickAllLast(reqId, tickType, time, price, size, tickAttribLast, exchange, specialConditions);
    } catch (const std::out_of_range& oor) {}
}
void IBConnector::tickByTickBidAsk(int reqId, time_t time, double bidPrice, double askPrice, Decimal bidSize,
            Decimal askSize, const TickAttribBidAsk& tickAttribBidAsk)
{
    try 
    {
        std::scoped_lock lock(tickHandlersMutex);
        TickHandler* handler = tickHandlers.at(reqId);
        if (handler != nullptr)
            handler->OnTickByTickBidAsk(reqId, time, bidPrice, askPrice, bidSize, askSize, tickAttribBidAsk);
    } catch (const std::out_of_range& oor) {}
}
void IBConnector::tickByTickMidPoint(int reqId, time_t time, double midPoint)
{
    try 
    {
        std::scoped_lock lock(tickHandlersMutex);
        TickHandler* handler = tickHandlers.at(reqId);
        if (handler != nullptr)
            handler->OnTickByTickMidPoint(reqId, time, midPoint);
    } catch (const std::out_of_range& oor) {}
}
void IBConnector::orderBound(long long orderId, int apiClientId, int apiOrderId)
{
    std::scoped_lock lock(orderHandlersMutex);
    for(auto handler : orderHandlers)
    {
        if (handler != nullptr)
            handler->OnOrderBound(orderId, apiClientId, apiOrderId);
    }
}
void IBConnector::orderStatus( OrderId orderId, const std::string& status, Decimal filled,
            Decimal remaining, double avgFillPrice, long long permId, int parentId,
            double lastFillPrice, int clientId, const std::string& whyHeld, double mktCapPrice)
{
    std::scoped_lock lock(orderHandlersMutex);
    for(auto handler : orderHandlers)
    {
        if (handler != nullptr)
            handler->OnOrderStatus(orderId, status, filled, remaining, avgFillPrice, permId, parentId, 
                lastFillPrice, clientId, whyHeld, mktCapPrice);
    }
}
void IBConnector::openOrder( OrderId orderId, const Contract& contract, const ::Order& order, const OrderState& orderState)
{
    std::scoped_lock lock(orderHandlersMutex);
    for(auto handler : orderHandlers)
    {
        if (handler != nullptr)
            handler->OnOpenOrder(orderId, contract, order, orderState);
    }
}
void IBConnector::openOrderEnd()
{
    std::scoped_lock lock(orderHandlersMutex);
    for(auto handler : orderHandlers)
    {
        if (handler != nullptr)
            handler->OnOpenOrderEnd();
    }
}
void IBConnector::winError( const std::string& str, int lastError)
{
    logger->error(logCategory, str);
}
void IBConnector::connectionClosed()
{
    // we cannot lock this, as the connection monitors need to unsubscribe themselves
    //std::scoped_lock lock(connectionMonitorsMutex);
    if (currentConnectionStatus != ConnectionStatus::PARTIALLY_CONNECTED)
    {
        logger->debug(logCategory, "connectionClosed called");
        currentConnectionStatus = ConnectionStatus::SHUTDOWN;
        for(auto cm : connectionMonitors)
            cm->OnDisconnect(this);
    }
    else
        logger->debug(logCategory, "connectionClosed called, but ignoring");
}
void IBConnector::updateAccountValue(const std::string& key, const std::string& val,
            const std::string& currency, const std::string& accountName)
{
    std::scoped_lock lock(accountHandlersMutex);
    for(auto h : accountHandlers)
        h->OnAccountValueUpdate(key, val, currency, accountName);
}
void IBConnector::updatePortfolio( const Contract& contract, Decimal position, double marketPrice, double marketValue,
            double averageCost, double unrealizedPNL, double realizedPNL, const std::string& accountName)
{
    std::scoped_lock lock(accountHandlersMutex);
    for(auto h : accountHandlers)
        h->OnPortfolioUpdate(contract, position, marketPrice, marketValue, averageCost, unrealizedPNL, realizedPNL, accountName);
}
void IBConnector::updateAccountTime(const std::string& timeStamp)
{
    std::scoped_lock lock(accountHandlersMutex);
    for(auto h : accountHandlers)
        h->OnUpdateAccountTime(timeStamp);
}
void IBConnector::accountDownloadEnd(const std::string& accountName)
{
    std::scoped_lock lock(accountHandlersMutex);
    for(auto h : accountHandlers)
        h->OnAccountDownloadEnd(accountName);
}
void IBConnector::nextValidId( OrderId orderId)
{
    nextOrderId = orderId;
    //logger->debug("IBConnector", "nextValidId: setting fullyConnected to true");
    if (currentConnectionStatus == ConnectionStatus::PARTIALLY_CONNECTED)
        currentConnectionStatus = ConnectionStatus::FULLY_CONNECTED;
    for(auto monitor : connectionMonitors)
        monitor->OnFullConnect(this);
}
void IBConnector::contractDetails( int reqId, const ContractDetails& contractDetails)
{
    //logger->debug("IBConnector", "contractDetails: response for id " + std::to_string(reqId) 
    //        + " and symbol " + contractDetails.contract.symbol);
    std::scoped_lock lock(contractDetailsDataMutex);
    auto itr = contractDetailsData.find(reqId);
    if (itr != contractDetailsData.end())
    {
        auto& vec = (*itr).second;
        vec.push_back(contractDetails);
    }
    else
    {
        logger->error("IBConnector", "contractDetails: id " + std::to_string(reqId) + " not found in collection");
    }
}
void IBConnector::bondContractDetails( int reqId, const ContractDetails& contractDetails){}
void IBConnector::contractDetailsEnd( int reqId)
{
    //logger->debug("IBConnector", "contractDetailsEnd: reqId: " + std::to_string(reqId) );
    std::scoped_lock lock(contractDetailsHandlersMutex, contractDetailsDataMutex);
    auto itr = contractDetailsHandlers.find(reqId);
    auto dataItr = contractDetailsData.find(reqId);
    std::vector<ContractDetails> vec;
    if (dataItr != contractDetailsData.end())
        vec = (*dataItr).second;
    if (itr != contractDetailsHandlers.end())
    {
        auto& promise = (*itr).second;
        promise.set_value(vec);
        contractDetailsHandlers.erase(itr);
        contractDetailsData.erase(dataItr);
        //logger->debug("IBConnector", "contractDetails: id " + std::to_string(reqId) + " promise value set.");
    }
    else
    {
        logger->error("IBConnector", "contractDetails: id " + std::to_string(reqId) + " not found in collection");
    }
}
void IBConnector::execDetails( int reqId, const Contract& contract, const Execution& execution)
{
    std::scoped_lock lock(executionHandlersMutex);
    for(auto h : executionHandlers)
        h->OnExecDetails(reqId, contract, execution);
}
void IBConnector::execDetailsEnd( int reqId)
{
    std::scoped_lock lock(executionHandlersMutex);
    for(auto h : executionHandlers)
        h->OnExecDetailsEnd(reqId);
}
void IBConnector::execDetailsProtoBuf(const protobuf::ExecutionDetails& executionDetailsProto) {}
void IBConnector::execDetailsEndProtoBuf(const protobuf::ExecutionDetailsEnd& executionDetailsEndProto) {}
void IBConnector::orderStatusProtoBuf(const protobuf::OrderStatus& orderStatusProto) {}
void IBConnector::openOrderProtoBuf(const protobuf::OpenOrder& openOrderProto) {}
void IBConnector::openOrdersEndProtoBuf(const protobuf::OpenOrdersEnd& openOrderEndProto) {}
void IBConnector::errorProtoBuf(const protobuf::ErrorMessage& errorProto) {}

IbSeverity IBConnector::ClassifyError(int errorCode, const std::string& errorString) const
{
    return classify(errorCode, errorString);
}

void IBConnector::error(int id, time_t time, int errorCode, const std::string& errorString, 
            const std::string& advancedOrderRejectJson)
{
    const IbError err{id, errorCode, errorString, advancedOrderRejectJson, time,
            ClassifyError(errorCode, errorString)};
    std::string msg = std::string(to_string(err.severity))
        + " id: " + std::to_string(id) 
        + " Code: " + std::to_string(errorCode) 
        + ": " + errorString 
        + ". JSON: " + advancedOrderRejectJson
        + " current status: " + to_string(currentConnectionStatus);
    switch(err.severity)
    {
        case IbSeverity::Info:
            logger->debug(logCategory, msg);
            break;
        case IbSeverity::Warning:
        case IbSeverity::Connectivity:
            logger->warning(logCategory, msg);
            break;
        default:
            logger->error(logCategory, msg);
            break;
    }
    // Route the message to whoever owns the id. Order ids and request ids are in
    // separate ranges (see REQUEST_ID_BASE), so an id can only match one kind of owner.
    if (RouteConnectivityError(err))
        return;
    if (IsOrderId(id) && RouteOrderError(err))
        return;
    if (IsRequestId(id) && RouteRequestError(err))
        return;
    RouteUnmatchedError(err);
}

bool IBConnector::RouteConnectivityError(const IbError& err)
{
    switch(err.code)
    {
        case 502: // couldn't connect to TWS
        case 509: // error reading socket
        case 1100: // connectivity between TWS and IB lost
        case 1101: // connectivity restored, data lost
        case 1102: // connectivity restored, data maintained
        case 1300: // socket port reset
            break;
        default:
            return false;
    }
    std::vector<IBConnectionMonitor*> monitors;
    {
        std::scoped_lock lock(connectionMonitorsMutex);
        monitors = connectionMonitors;
    }
    for(auto* monitor : monitors)
    {
        monitor->OnError(this, err.id, err.code, err.message, err.advancedRejectJson);
        if (err.code == 1100)
            monitor->OnIbConnectivity(this, false, false);
        else if (err.code == 1101)
            monitor->OnIbConnectivity(this, true, true);
        else if (err.code == 1102)
            monitor->OnIbConnectivity(this, true, false);
    }
    if (err.code == 502 || err.code == 509)
    {
        // if this is the "can't connect to IB" error on login, do a shutdown to get out of connection loop
        if (currentConnectionStatus == ConnectionStatus::FULLY_CONNECTED)
        {
            // they're legit
            currentConnectionStatus = ConnectionStatus::ATTEMPTING_SHUTDOWN;
            disconnect();
        }
        else if (currentConnectionStatus == ConnectionStatus::PARTIALLY_CONNECTED)
        {
            // we can't call disconnect as we may be on the same thread
            cleanUpPartialConnection();
        }
    }
    // with no monitors, let the catch-all handlers see it
    return !monitors.empty();
}

bool IBConnector::RouteOrderError(const IbError& err)
{
    std::vector<OrderHandler*> handlers;
    {
        std::scoped_lock lock(orderHandlersMutex);
        handlers = orderHandlers;
    }
    for(auto* handler : handlers)
        handler->OnOrderError(err.id, err);
    return !handlers.empty();
}

bool IBConnector::RouteRequestError(const IbError& err)
{
    // Handler pointers are copied under their lock and called without it,
    // so a handler may unsubscribe from inside OnError.
    const int id = err.id;
    {
        // scanner subscription
        bool isScanner = false;
        bool ended = false;
        std::vector<ScannerHandler*> handlers;
        {
            std::scoped_lock lock(scannerHandlerMutex);
            isScanner = scannerRequestIds.contains(id);
            if (isScanner)
            {
                // TWS ends a scanner subscription with 162 (e.g. after a cancel) or 165
                ended = err.code == 162 || err.code == 165;
                if (ended)
                    scannerRequestIds.erase(id);
                handlers = scannerHandlers;
            }
        }
        if (isScanner)
        {
            for(auto* h : handlers)
            {
                h->OnError(this, id, err);
                if (ended)
                    h->OnScannerSubscriptionEnd(this, id);
            }
            return true;
        }
    }
    {
        // market data or tick-by-tick subscription
        TickHandler* handler = nullptr;
        {
            std::scoped_lock lock(tickHandlersMutex);
            auto itr = tickHandlers.find(id);
            if (itr != tickHandlers.end())
                handler = itr->second;
        }
        if (handler != nullptr)
        {
            handler->OnError(id, err);
            return true;
        }
    }
    {
        // market depth subscription
        MarketDepthHandler* handler = nullptr;
        {
            std::scoped_lock lock(marketDepthHandlersMutex);
            auto itr = marketDepthHandlers.find(id);
            if (itr != marketDepthHandlers.end())
                handler = itr->second;
        }
        if (handler != nullptr)
        {
            handler->OnError(id, err);
            return true;
        }
    }
    {
        // historical data request
        HistoricalDataHandler* handler = nullptr;
        {
            std::scoped_lock lock(historicalDataHandlersMutex);
            auto itr = historicalDataHandlers.find(id);
            if (itr != historicalDataHandlers.end())
                handler = itr->second;
        }
        if (handler != nullptr)
        {
            handler->OnError(id, err);
            if (err.code == 165) // historical data service query message: the request is over
                handler->OnHistoricalDataEnd(id, "", "");
            return true;
        }
    }
    if (!err.isError())
        return false; // warnings for a pending future have no owner to tell
    {
        // GetContractDetails future
        std::promise<std::vector<ContractDetails>> promise;
        bool found = false;
        {
            std::scoped_lock lock(contractDetailsHandlersMutex, contractDetailsDataMutex);
            auto itr = contractDetailsHandlers.find(id);
            if (itr != contractDetailsHandlers.end())
            {
                promise = std::move(itr->second);
                contractDetailsHandlers.erase(itr);
                contractDetailsData.erase(id);
                found = true;
            }
        }
        if (found)
        {
            if (err.code == 200) // no security definition found. Kept as out_of_range for existing callers
                promise.set_exception(std::make_exception_ptr(std::out_of_range("Symbol not found")));
            else
                promise.set_exception(std::make_exception_ptr(IbException(err)));
            return true;
        }
    }
    {
        // GetOptionParameters future
        auto itr = securityDefinitionHandlers.find(id);
        if (itr != securityDefinitionHandlers.end())
        {
            auto promise = std::move(itr->second);
            securityDefinitionHandlers.erase(itr);
            securityDefinitionData.erase(id);
            promise.set_exception(std::make_exception_ptr(IbException(err)));
            return true;
        }
    }
    return false;
}

void IBConnector::RouteUnmatchedError(const IbError& err)
{
    std::vector<ErrorHandler*> handlers;
    {
        std::scoped_lock lock(errorHandlersMutex);
        handlers = errorHandlers;
    }
    for(auto* handler : handlers)
        handler->OnError(this, err);
    if (err.code == 2109) // Outside Regular Trading Hours warning, never sent to AccountHandlers
        return;
    std::scoped_lock lock(accountHandlersMutex);
    for(auto handler : accountHandlers)
    {
        if (handler != nullptr)
            handler->OnError(err.id, err.code, err.message, err.advancedRejectJson);
    }
}
void IBConnector::updateMktDepth(TickerId reqId, int position, int operation, int side, double price, Decimal size)
{
    try 
    {
        std::scoped_lock lock(marketDepthHandlersMutex);
        MarketDepthHandler* handler = marketDepthHandlers.at(reqId);
        if (handler != nullptr)
            handler->OnUpdateMktDepth(reqId, position, operation, side, price, size);
    } catch (const std::out_of_range& oor) {}
}
void IBConnector::updateMktDepthL2(TickerId reqId, int position, const std::string& marketMaker, int operation,
            int side, double price, Decimal size, bool isSmartDepth)
{
    try 
    {
        std::scoped_lock lock(marketDepthHandlersMutex);
        MarketDepthHandler* handler = marketDepthHandlers.at(reqId);
        if (handler != nullptr)
            handler->OnUpdateMktDepthL2(reqId, position, marketMaker, operation, side, price, size, isSmartDepth);
    } catch (const std::out_of_range& oor) {}
}
void IBConnector::updateNewsBulletin(int msgId, int msgType, const std::string& newsMessage, 
            const std::string& originExch){}
std::vector<std::string> splitCSV(const std::string& in)
{
    std::vector<std::string> retval;

    std::string leftover = in;
    int pos = leftover.find(",");
    while (pos != std::string::npos)
    {
        retval.push_back( leftover.substr(0, pos) );
        leftover = leftover.substr(pos + 1);
        pos = leftover.find(",");
    }
    // put in the last one
    if (leftover.size() > 0)
        retval.push_back(leftover);
    return retval;
}

void IBConnector::managedAccounts( const std::string& accountsList)
{
    // split by comma
    accounts = splitCSV(accountsList);
    if (accounts.size() == 1)
        defaultAccount = accounts[0];
    if (accounts.size() > 1)
        defaultAccount = accounts[1];
}
void IBConnector::receiveFA(faDataType pFaDataType, const std::string& cxml){}
void IBConnector::historicalData(TickerId reqId, const Bar& bar)
{
    //logger->debug("IBConnector", "historicalData called for id " + std::to_string(reqId));
    std::scoped_lock lock(historicalDataHandlersMutex);
    auto handler = historicalDataHandlers[reqId];
    if (handler != nullptr)
        handler->OnHistoricalData(reqId, bar);
    else
        logger->debug("IBConnector", "historicalData called for id " + std::to_string(reqId) + " but no subscribers");
}
void IBConnector::historicalDataEnd(int reqId, const std::string& startDateStr, const std::string& endDateStr)
{
    //logger->debug("IBConnector", "historicalDataEnd called for id " + std::to_string(reqId));
    std::scoped_lock lock(historicalDataHandlersMutex);
    auto handler = historicalDataHandlers[reqId];
    if (handler != nullptr)
        handler->OnHistoricalDataEnd(reqId, startDateStr, endDateStr);
    else
        logger->debug("IBConnector", "historicalDataEnd called for id " + std::to_string(reqId) + " but no subscribers");
}

void IBConnector::scannerParameters(const std::string& xml)
{
    std::scoped_lock lock(scannerHandlerMutex);
    for(auto handler : scannerHandlers)
        handler->OnScannerParameters(this, xml);
}

void IBConnector::scannerData(int reqId, int rank, const ContractDetails& contractDetails, const std::string& distance,
            const std::string& benchmark, const std::string& projection, const std::string& legsStr)
{
    std::scoped_lock lock(scannerHandlerMutex);
    for(auto handler : scannerHandlers)
        handler->OnScannerData(this, reqId, rank, contractDetails, distance, benchmark, projection, legsStr);
}
void IBConnector::scannerDataEnd(int reqId)
{
    std::scoped_lock lock(scannerHandlerMutex);
    for(auto handler : scannerHandlers)
        handler->OnScannerDataEnd(this, reqId);
}
void IBConnector::realtimeBar(TickerId reqId, long time, double open, double high, double low, double close,
            Decimal volume, Decimal wap, int count){}
void IBConnector::currentTime(long time){}
void IBConnector::currentTimeInMillis(time_t timeInMillis) {}
void IBConnector::fundamentalData(TickerId reqId, const std::string& data){}
void IBConnector::deltaNeutralValidation(int reqId, const DeltaNeutralContract& deltaNeutralContract){}
void IBConnector::tickSnapshotEnd( int reqId){}
void IBConnector::marketDataType( TickerId reqId, int marketDataType){}
void IBConnector::commissionAndFeesReport( const CommissionAndFeesReport& commissionReport)
{
    std::scoped_lock lock(executionHandlersMutex);
    for(auto h : executionHandlers)
    {
        h->OnCommissionReport(commissionReport);
    }
}
void IBConnector::position( const std::string& account, const Contract& contract, Decimal position, 
            double avgCost)
{
    std::scoped_lock lock(accountHandlersMutex);
    for(auto handler : accountHandlers)
    {
        if (handler != nullptr)
            handler->OnPosition(account, contract, position, avgCost);
    }
}
void IBConnector::positionEnd()
{
    std::scoped_lock lock(accountHandlersMutex);
    for(auto handler : accountHandlers)
    {
        handler->OnPositionEnd();
    }
}
void IBConnector::accountSummary( int reqId, const std::string& account, const std::string& tag, 
            const std::string& value, const std::string& curency){}
void IBConnector::accountSummaryEnd( int reqId){}
void IBConnector::verifyMessageAPI( const std::string& apiData){}
void IBConnector::verifyCompleted( bool isSuccessful, const std::string& errorText){}
void IBConnector::displayGroupList( int reqId, const std::string& groups){}
void IBConnector::displayGroupUpdated( int reqId, const std::string& contractInfo)
{
    std::scoped_lock lock(displayGroupHandlersMutex);
    //logger->debug("IBConnector", "displayGroupUpdated: reqId: " + std::to_string(reqId) + " Contract info: " + contractInfo);
    std::for_each(displayGroupHandlers.begin(), displayGroupHandlers.end(), [reqId, contractInfo](const DisplayGroupCombination& curr)
            { if (curr.reqId == reqId) curr.handler->OnDisplayGroupUpdated(reqId, contractInfo); });
}
void IBConnector::verifyAndAuthMessageAPI( const std::string& apiData, const std::string& xyzChallange){}
void IBConnector::verifyAndAuthCompleted( bool isSuccessful, const std::string& errorText){}
/***
 * @brief IB has acknowledged our connect request
 */
void IBConnector::connectAck()
{
    std::scoped_lock lock(connectionMonitorsMutex);
    for(auto cm : connectionMonitors)
        cm->OnConnect(this);
}
void IBConnector::positionMulti( int reqId, const std::string& account,const std::string& modelCode,
            const Contract& contract, Decimal pos, double avgCost){}
void IBConnector::positionMultiEnd( int reqId){}
void IBConnector::accountUpdateMulti( int reqId, const std::string& account, const std::string& modelCode, 
            const std::string& key, const std::string& value, const std::string& currency){}
void IBConnector::accountUpdateMultiEnd( int reqId){}
void IBConnector::securityDefinitionOptionalParameter(int reqId, const std::string& exchange, int underlyingConId, 
            const std::string& tradingClass, const std::string& multiplier, const std::set<std::string>& expirations,
            const std::set<double>& strikes) 
{
    //logger->debug("IBConnector", "securityDefinitionOptionalParameter: response for id " + std::to_string(reqId) 
    //        + " and conid " + std::to_string(underlyingConId));
    auto itr = securityDefinitionData.find(reqId);
    if (itr != securityDefinitionData.end())
    {
        auto& vec = (*itr).second;
        SecurityDefinitionOptionParameter data;
        data.reqId = reqId;
        data.exchange = exchange;
        data.underlyingConId = underlyingConId;
        data.tradingClass = tradingClass;
        data.multiplier = multiplier;
        data.expirations = expirations;
        data.strikes = strikes;
        vec.push_back(data);
    }
    else
    {
        logger->error("IBConnector", "securityDefinitionOptionalParameter: id " + std::to_string(reqId) + " not found in collection");
    }
}
void IBConnector::securityDefinitionOptionalParameterEnd(int reqId)
{
    //logger->debug("IBConnector", "securityDefinitionOptionalParameterEnd: reqId: " + std::to_string(reqId) );
    auto itr = securityDefinitionHandlers.find(reqId);
    auto dataItr = securityDefinitionData.find(reqId);
    std::vector<SecurityDefinitionOptionParameter> vec;
    if (dataItr != securityDefinitionData.end())
        vec = (*dataItr).second;
    if (itr != securityDefinitionHandlers.end())
    {
        auto& promise = (*itr).second;
        promise.set_value(vec);
        securityDefinitionHandlers.erase(itr);
        securityDefinitionData.erase(dataItr);
        //logger->debug("IBConnector", "securityDefinitionOptionalParameterEnd: id " + std::to_string(reqId) + " promise value set.");
    }
    else
    {
        logger->error("IBConnector", "securityDefinitionOptionalParameterEnd: id " + std::to_string(reqId) + " not found in collection");
    }
}
void IBConnector::softDollarTiers(int reqId, const std::vector<SoftDollarTier> &tiers){}
void IBConnector::familyCodes(const std::vector<FamilyCode> &familyCodes){}
void IBConnector::symbolSamples(int reqId, const std::vector<ContractDescription> &contractDescriptions){}
void IBConnector::mktDepthExchanges(const std::vector<DepthMktDataDescription> &depthMktDataDescriptions)
{
    std::scoped_lock lock(mktDepthExchangesPromisesMutex);
    // find the first promise
    if (mktDepthExchangesPromises.size() == 0)
        return;
    auto& promise = *mktDepthExchangesPromises.begin();
    promise.set_value(depthMktDataDescriptions);
    mktDepthExchangesPromises.erase(mktDepthExchangesPromises.begin());
}

void IBConnector::tickNews(int tickerId, time_t timeStamp, const std::string& providerCode, const std::string& articleId,
            const std::string& headline, const std::string& extraData){}
void IBConnector::smartComponents(int reqId, const SmartComponentsMap& theMap){}
void IBConnector::tickReqParams(int tickerId, double minTick, const std::string& bboExchange, int snapshotPermissions){}
void IBConnector::newsProviders(const std::vector<NewsProvider> &newsProviders){}
void IBConnector::newsArticle(int requestId, int articleType, const std::string& articleText){}
void IBConnector::historicalNews(int requestId, const std::string& time, const std::string& providerCode,
            const std::string& articleId, const std::string& headline){}
void IBConnector::historicalNewsEnd(int requestId, bool hasMore){}
void IBConnector::headTimestamp(int reqId, const std::string& headTimestamp){}
void IBConnector::histogramData(int reqId, const HistogramDataVector& data){}
void IBConnector::historicalDataUpdate(TickerId reqId, const Bar& bar)
{
    std::scoped_lock lock(historicalDataHandlersMutex);
    auto handler = historicalDataHandlers[reqId];
    if (handler != nullptr)
        handler->OnHistoricalDataUpdate(reqId, bar);
    else
    {
        std::string str;
        std::for_each(historicalDataHandlers.begin(), historicalDataHandlers.end(),
                [&str]( const const std::pair<const int, HistoricalDataHandler*>& curr)
                { if(curr.second != nullptr) str += ", " + std::to_string(curr.first); });
        /*
        logger->debug("IBConnector", "historicalDataUpdate called for id " + std::to_string(reqId) 
                + " but no subscribers. Subscriber ids: " + str);
        */
    }
}
void IBConnector::rerouteMktDataReq(int reqId, int conid, const std::string& exchange){}
void IBConnector::rerouteMktDepthReq(int reqId, int conid, const std::string& exchange){}
void IBConnector::marketRule(int marketRuleId, const std::vector<PriceIncrement> &priceIncrements){}
void IBConnector::pnl(int reqId, double dailyPnL, double unrealizedPnL, double realizedPnL){}
void IBConnector::pnlSingle(int reqId, Decimal pos, double dailyPnL, double unrealizedPnL, double realizedPnL,
            double value){}
void IBConnector::historicalTicks(int reqId, const std::vector<HistoricalTick> &ticks, bool done){}
void IBConnector::historicalTicksBidAsk(int reqId, const std::vector<HistoricalTickBidAsk> &ticks, bool done){}
void IBConnector::historicalTicksLast(int reqId, const std::vector<HistoricalTickLast> &ticks, bool done){}
void IBConnector::completedOrder(const Contract& contract, const Order& order, const OrderState& orderState){}
void IBConnector::completedOrdersEnd(){}
void IBConnector::replaceFAEnd(int reqId, const std::string& text){}
void IBConnector::wshMetaData(int reqId, const std::string& dataJson){}
void IBConnector::wshEventData(int reqId, const std::string& dataJson){}
void IBConnector::historicalSchedule(int reqId, const std::string& startDateTime, const std::string& endDateTime,
            const std::string& timeZone, const std::vector<HistoricalSession>& sessions){}
void IBConnector::userInfo(int reqId, const std::string& whiteBrandingId){}

} // namespace ib_helper
