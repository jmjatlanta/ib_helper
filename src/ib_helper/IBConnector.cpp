#include "IBConnector.hpp"
#include "../util/Logger.h"
#include "ScannerSubscription.h"

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
    }
    return "UNKNOWN";
}

IBConnector::IBConnector()
{
    logger = Logger::getInstance();
}

IBConnector::IBConnector(const std::string& hostname, int port, int clientId, IBConnectionMonitor* connMonitor) 
        : hostname(hostname), port(port), clientId(clientId)
{
    logger = Logger::getInstance();
    if (connMonitor != nullptr)
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
        logger->debug("IBConnector", "connect: Attempting to connect to IB host " + hostname + ":" + std::to_string(port));
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
                    //ibClient->eDisconnect();
                    //reader->processMsgs();
                    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
                    //listenerThread->join();
                    //listenerThread = nullptr;
                    //delete reader;
                    //reader = nullptr;
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
        logger->debug("IBConnector", "connect:: eConnect returned true, creating EReader");
	    reader = new EReader(ibClient, osSignal);
        reader->start();
	    listenerThread = std::make_shared<std::thread>(&IBConnector::processMessages, this);
        return true;
    }
    return IsConnected();
}

bool IBConnector::disconnect()
{
    logger->debug("IBConnector", "disconnect: Attempting to disconnect from IB host " + hostname + ":" + std::to_string(port));
    currentConnectionStatus = ConnectionStatus::ATTEMPTING_SHUTDOWN;
    if (ibClient != nullptr)
    {
        ibClient->eDisconnect();
        // give some time to inform other threads
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    if (listenerThread != nullptr && listenerThread->joinable())
	    listenerThread->join();
    if (reader != nullptr)
    {
        // flush any last message
        //reader->processMsgs();
        delete reader;
        reader = nullptr;
    }
    if (ibClient != nullptr)
    {
        delete ibClient;
        ibClient = nullptr;
    }
    if (osSignal != nullptr)
    {
        delete osSignal;
        osSignal = nullptr;
    }
    currentConnectionStatus = ConnectionStatus::SHUTDOWN;
    return true;
}

IBConnector::~IBConnector() {
    disconnect();
}

std::future<std::vector<SecurityDefinitionOptionParameter>> IBConnector::GetOptionParameters(const Contract& contract)
{
    if (ibClient == nullptr)
    {
        std::promise<std::vector<SecurityDefinitionOptionParameter>> p;
        p.set_exception(std::exception_ptr{});
        return p.get_future();
    }
    uint32_t promiseId = GetNextRequestId();
    auto& promise = securityDefinitionHandlers[promiseId];
    auto& vec = securityDefinitionData[promiseId]; // put the vector in there
    logger->debug("IBConnector", "GetOptionParameters: about to request details for " + contract.symbol + " id: " + std::to_string(promiseId) );
    ibClient->reqSecDefOptParams(promiseId, contract.symbol, "", contract.secType, contract.conId);
    logger->debug("IBConnector", "GetContractDetails: request made for " + contract.symbol);
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
    uint32_t promiseId = GetNextRequestId();
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

uint32_t IBConnector::RequestExecutionReports(const ExecutionFilter& filter)
{
    uint32_t reqId = GetNextRequestId();
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
    std::lock_guard<std::mutex> lock(accountHandlersMutex);
    accountHandlers.push_back(in);
}

void IBConnector::RemoveAccountHandler(AccountHandler* in)
{
    std::lock_guard<std::mutex> lock(accountHandlersMutex);
    std::erase(accountHandlers, in);
}

void IBConnector::AddScannerHandler(ScannerHandler* in)
{
    std::lock_guard<std::mutex> lock(scannerHandlerMutex);
    scannerHandlers.push_back(in);
}

void IBConnector::RemoveScannerHandler(ScannerHandler* in)
{
    std::lock_guard<std::mutex> lock(scannerHandlerMutex);
    std::erase(scannerHandlers, in);
}

void IBConnector::RequestScannerParameters()
{
    ibClient->reqScannerParameters();
}

int IBConnector::RequestScannerSubscription( ScannerSubscription scannerSubscription, 
            TagValueListSPtr scannerSubscriptionOptions, TagValueListSPtr scannerSubscriptionFilterOptions)
{
    int nextReq = ++nextRequestId;
    this->ibClient->reqScannerSubscription(nextReq, scannerSubscription, scannerSubscriptionOptions, scannerSubscriptionFilterOptions);
    return nextReq;
}

void IBConnector::CancelScannerSubscription(int reqId)
{
    ibClient->cancelScannerSubscription(reqId);
}

void IBConnector::AddOrderHandler(OrderHandler* in)
{
    std::lock_guard<std::mutex> lock(orderHandlersMutex);
    for(auto* handler : orderHandlers)
        if (handler == in)
            return;
    orderHandlers.push_back(in);
}

void IBConnector::RemoveOrderHandler(OrderHandler* in)
{
    std::lock_guard<std::mutex> lock(orderHandlersMutex);
    std::erase(orderHandlers, in);
}

void IBConnector::AddExecutionHandler(ExecutionHandler* in)
{
    std::lock_guard lock(executionHandlersMutex);
    for(auto* handler : executionHandlers)
        if (handler == in)
            return;
    executionHandlers.push_back(in);
}

void IBConnector::RemoveExecutionHandler(ExecutionHandler* in)
{
    uint32_t toErase = 0;
    std::lock_guard lock(executionHandlersMutex);
    for(auto itr = executionHandlers.begin(); itr != executionHandlers.end(); ++itr)
    {
        if ((*itr) == in)
        {
            executionHandlers.erase(itr);
            break;
        }
    }
}

AccountHandler* IBConnector::GetDefaultAccountHandler() 
{ 
    std::lock_guard<std::mutex> lock(accountHandlersMutex);
    if (accountHandlers.size() == 0) 
        return nullptr; 
    return accountHandlers[0]; 
}

void IBConnector::RemoveConnectionMonitor(IBConnectionMonitor* in)
{
    std::lock_guard<std::mutex> lock(connectionMonitorsMutex);
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
    {
        std::lock_guard<std::mutex> lock(connectionMonitorsMutex);
        connectionMonitors.push_back(in);
    }
}

void IBConnector::CancelOrder(int orderId, const std::string& time)
{
    if (ibClient != nullptr)
        ibClient->cancelOrder(orderId, time);
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
        logger->debug("IBConnector", "processMessages: Loop starting");
	    while (!ConnectionShuttingDown(currentConnectionStatus)) 
        {
	        osSignal->waitForSignal();
            if (!ConnectionShuttingDown(currentConnectionStatus))
            {
	            reader->processMsgs();
            }
	    }
    } 
    catch(const std::exception& ex)
    {
        logger->error("IBConnector", "processMessages: Standard exception: " + std::string(ex.what()));
    }
    catch(...)
    {
        logger->error("IBConnector", "processMessages: Exception thrown");
    }
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
uint32_t IBConnector::SubscribeToMarketData(const Contract& contract, TickHandler* tickHandler, 
        const std::string& genericTickList, bool snapshot, bool regulatorySnapshot, 
        const TagValueListSPtr& mktDataOptions)
{
    if (ibClient == nullptr)
        return 0;

    uint32_t reqId = GetNextRequestId();
    {
        std::lock_guard<std::mutex> lock(tickHandlersMutex);
        tickHandlers[reqId] = tickHandler;
    }
    ibClient->reqMktData(reqId, contract, genericTickList, snapshot, regulatorySnapshot, mktDataOptions);
    return reqId;
};

void IBConnector::UnsubscribeFromMarketData(uint32_t reqId) 
{ 
    {
        std::lock_guard<std::mutex> lock(tickHandlersMutex);
        tickHandlers[reqId] = nullptr;
    }
    if (ibClient != nullptr)
        ibClient->cancelMktData(reqId); 
}

uint32_t IBConnector::SubscribeToTickByTick(const Contract& contract, TickHandler* handler, const std::string& tickType, 
            int numberOfTicks, bool ignoreSize)
{
    uint32_t reqId = GetNextRequestId();
    {
        std::lock_guard<std::mutex> lock(tickHandlersMutex);
        tickHandlers[reqId] = handler;
    }
    if (ibClient != nullptr)
        ibClient->reqTickByTickData(reqId, contract, tickType, numberOfTicks, ignoreSize);
    return reqId;
};

void IBConnector::UnsubscribeFromTickByTick(uint32_t reqId) 
{ 
    {
        std::lock_guard<std::mutex> lock(tickHandlersMutex);
        tickHandlers[reqId] = nullptr;
    }
    if (ibClient != nullptr)
        ibClient->cancelTickByTickData(reqId); 
}

uint32_t IBConnector::SubscribeToMarketDepth(const Contract& contract, MarketDepthHandler* depthHandler, uint32_t numLines)
{
    uint32_t reqId = GetNextRequestId();
    {
        std::lock_guard<std::mutex> lock(marketDepthHandlersMutex);
        marketDepthHandlers[reqId] = depthHandler;
    }
    if (ibClient != nullptr)
    {
        TagValueListSPtr mktDepthOptions;
        ibClient->reqMktDepth(reqId, contract, numLines, true, mktDepthOptions);
    }
    return reqId;
}

void IBConnector::UnsubscribeFromMarketDepth(uint32_t subscriptionId)
{
    {
        std::lock_guard<std::mutex> lock(marketDepthHandlersMutex);
        marketDepthHandlers[subscriptionId] = nullptr;
    }
    if (ibClient != nullptr)
        ibClient->cancelMktDepth(subscriptionId, true);
}

std::future<std::vector<DepthMktDataDescription> > IBConnector::RequestMktDepthExchanges()
{
    std::lock_guard<std::mutex> lock(mktDepthExchangesPromisesMutex);
    auto& promise = mktDepthExchangesPromises.emplace_back( );
    if (ibClient != nullptr)
        ibClient->reqMktDepthExchanges();
    return promise.get_future();
}

void IBConnector::UnsubscribeFromHistoricalData(uint32_t historicalSubscriptionId)
{
    {
        std::lock_guard<std::mutex> lock(historicalDataHandlersMutex);
        historicalDataHandlers[historicalSubscriptionId] = nullptr;
    }
    if (ibClient != nullptr)
        ibClient->cancelHistoricalData(historicalSubscriptionId);
}

uint32_t IBConnector::SubscribeToHistoricalData(const Contract& contract, HistoricalDataHandler* handler,
        const std::string& timePeriod, const std::string& barSize)
{
    if (ibClient == nullptr)
        return 0;
    std::string whatToShow = "TRADES";
    if (contract.secType == "CASH")
        whatToShow = "MIDPOINT";
    int id = GetNextRequestId();
    {
        std::lock_guard<std::mutex> lock(historicalDataHandlersMutex);
        historicalDataHandlers[id] = handler;
    }
    ibClient->reqHistoricalData(id, contract, "", timePeriod, barSize, whatToShow, 0, 2, true, nullptr);
    return id;
}

void IBConnector::tickPrice( TickerId tickerId, TickType field, double price, const TickAttrib& attrib)
{
    try 
    {
        std::lock_guard<std::mutex> lock(tickHandlersMutex);
        TickHandler* handler = tickHandlers.at(tickerId);
        if (handler != nullptr)
            handler->OnTickPrice(tickerId, field, price, attrib);
    } catch (const std::out_of_range& oor) {}
}
void IBConnector::tickSize(TickerId tickerId, TickType field, Decimal size)
{
    try 
    {
        std::lock_guard<std::mutex> lock(tickHandlersMutex);
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
        std::lock_guard<std::mutex> lock(tickHandlersMutex);
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
        std::lock_guard<std::mutex> lock(tickHandlersMutex);
        TickHandler* handler = tickHandlers.at(tickerId);
        if (handler != nullptr)
            handler->OnTickGeneric(tickerId, tickType, value);
    } catch (const std::out_of_range& oor) {}
}
void IBConnector::tickString(TickerId tickerId, TickType tickType, const std::string& value)
{
    try 
    {
        std::lock_guard<std::mutex> lock(tickHandlersMutex);
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
        std::lock_guard<std::mutex> lock(tickHandlersMutex);
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
        std::lock_guard<std::mutex> lock(tickHandlersMutex);
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
        std::lock_guard<std::mutex> lock(tickHandlersMutex);
        TickHandler* handler = tickHandlers.at(reqId);
        if (handler != nullptr)
            handler->OnTickByTickBidAsk(reqId, time, bidPrice, askPrice, bidSize, askSize, tickAttribBidAsk);
    } catch (const std::out_of_range& oor) {}
}
void IBConnector::tickByTickMidPoint(int reqId, time_t time, double midPoint)
{
    try 
    {
        std::lock_guard<std::mutex> lock(tickHandlersMutex);
        TickHandler* handler = tickHandlers.at(reqId);
        if (handler != nullptr)
            handler->OnTickByTickMidPoint(reqId, time, midPoint);
    } catch (const std::out_of_range& oor) {}
}
void IBConnector::orderBound(long long orderId, int apiClientId, int apiOrderId)
{
    std::lock_guard<std::mutex> lock(orderHandlersMutex);
    for(auto handler : orderHandlers)
    {
        if (handler != nullptr)
            handler->OnOrderBound(orderId, apiClientId, apiOrderId);
    }
}
void IBConnector::orderStatus( OrderId orderId, const std::string& status, Decimal filled,
	        Decimal remaining, double avgFillPrice, int permId, int parentId,
	        double lastFillPrice, int clientId, const std::string& whyHeld, double mktCapPrice)
{
    std::lock_guard<std::mutex> lock(orderHandlersMutex);
    for(auto handler : orderHandlers)
    {
        if (handler != nullptr)
            handler->OnOrderStatus(orderId, status, filled, remaining, avgFillPrice, permId, parentId, 
                lastFillPrice, clientId, whyHeld, mktCapPrice);
    }
}
void IBConnector::openOrder( OrderId orderId, const Contract& contract, const ::Order& order, const OrderState& orderState)
{
    std::lock_guard<std::mutex> lock(orderHandlersMutex);
    for(auto handler : orderHandlers)
    {
        if (handler != nullptr)
            handler->OnOpenOrder(orderId, contract, order, orderState);
    }
}
void IBConnector::openOrderEnd()
{
    std::lock_guard<std::mutex> lock(orderHandlersMutex);
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
    //std::lock_guard<std::mutex> lock(connectionMonitorsMutex);
    currentConnectionStatus = ConnectionStatus::SHUTDOWN;
    for(auto cm : connectionMonitors)
        cm->OnDisconnect(this);
}
void IBConnector::updateAccountValue(const std::string& key, const std::string& val,
            const std::string& currency, const std::string& accountName)
{
    std::lock_guard<std::mutex> lock(accountHandlersMutex);
    for(auto h : accountHandlers)
        h->OnAccountValueUpdate(key, val, currency, accountName);
}
void IBConnector::updatePortfolio( const Contract& contract, Decimal position, double marketPrice, double marketValue,
            double averageCost, double unrealizedPNL, double realizedPNL, const std::string& accountName)
{
    std::lock_guard<std::mutex> lock(accountHandlersMutex);
    for(auto h : accountHandlers)
        h->OnPortfolioUpdate(contract, position, marketPrice, marketValue, averageCost, unrealizedPNL, realizedPNL, accountName);
}
void IBConnector::updateAccountTime(const std::string& timeStamp)
{
    std::lock_guard<std::mutex> lock(accountHandlersMutex);
    for(auto h : accountHandlers)
        h->OnUpdateAccountTime(timeStamp);
}
void IBConnector::accountDownloadEnd(const std::string& accountName)
{
    std::lock_guard<std::mutex> lock(accountHandlersMutex);
    for(auto h : accountHandlers)
        h->OnAccountDownloadEnd(accountName);
}
void IBConnector::nextValidId( OrderId orderId)
{
    nextOrderId = orderId;
    logger->debug("IBConnector", "nextValidId: setting fullyConnected to true");
    if (currentConnectionStatus == ConnectionStatus::PARTIALLY_CONNECTED)
        currentConnectionStatus = ConnectionStatus::FULLY_CONNECTED;
    for(auto monitor : connectionMonitors)
        monitor->OnFullConnect(this);
}
void IBConnector::contractDetails( int reqId, const ContractDetails& contractDetails)
{
    logger->debug("IBConnector", "contractDetails: response for id " + std::to_string(reqId) 
            + " and symbol " + contractDetails.contract.symbol);
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
    logger->debug("IBConnector", "contractDetailsEnd: reqId: " + std::to_string(reqId) );
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
        logger->debug("IBConnector", "contractDetails: id " + std::to_string(reqId) + " promise value set.");
    }
    else
    {
        logger->error("IBConnector", "contractDetails: id " + std::to_string(reqId) + " not found in collection");
    }
}
void IBConnector::execDetails( int reqId, const Contract& contract, const Execution& execution)
{
    std::lock_guard lock(executionHandlersMutex);
    for(auto h : executionHandlers)
        h->OnExecDetails(reqId, contract, execution);
}
void IBConnector::execDetailsEnd( int reqId)
{
    std::lock_guard lock(executionHandlersMutex);
    for(auto h : executionHandlers)
        h->OnExecDetailsEnd(reqId);
}

void IBConnector::error(int id, int errorCode, const std::string& errorString, 
            const std::string& advancedOrderRejectJson)
{
    std::string msg = "Error id: " + std::to_string(id) 
        + " Code: " + std::to_string(errorCode) 
        + ": " + errorString 
        + ". JSON: " + advancedOrderRejectJson;
    // if this is the "can't connect to IB error on login, do a shutdown to get out of connection loop
    if (errorCode == 502 // couldn't connect to TWS
        || errorCode == 509 ) // error reading socket
    {
        // these could be messages stuck in the buffer( partially connected) or legit messages
        if (currentConnectionStatus == ConnectionStatus::FULLY_CONNECTED)
        {
            logger->error("IBConnector", "Received 502 while being fully connected. Attempting disconnect");
            // they're legit
            currentConnectionStatus = ConnectionStatus::ATTEMPTING_SHUTDOWN;
            disconnect();
            for(auto* monitor : connectionMonitors)
            {
                monitor->OnError(this, msg);
            }
        }
    }
    if (errorCode == 504) // not connected
    {
        // should we alert the conneciton monitors?
    }
    if (errorCode == 200) // no security definition found
    {
        // if the request id is found, set the exception in the promise/future
        auto itr = contractDetailsHandlers.find(id);
        if (itr != contractDetailsHandlers.end())
        {
            (*itr).second.set_exception(std::make_exception_ptr( std::out_of_range("Symbol not found")));
            return;
        }
    }
    if (errorCode == 162)
    {
        // we have successfully unsubscribed
        std::scoped_lock lock(scannerHandlerMutex);
        for(auto& h : scannerHandlers)
            h->OnScannerSubscriptionEnd(this, id); 
        return;
    }
    if (errorCode == 2109)
    {
        // Outside Regular Trading Hours warning
        return;
    }
    {
        std::lock_guard<std::mutex> lock(accountHandlersMutex);
        for(auto handler : accountHandlers)
        {
            if (handler != nullptr)
                handler->OnError(id, errorCode, errorString, advancedOrderRejectJson);
        }
    }
    logger->error(logCategory, msg);
}
void IBConnector::updateMktDepth(TickerId reqId, int position, int operation, int side, double price, Decimal size)
{
    try 
    {
        std::lock_guard<std::mutex> lock(marketDepthHandlersMutex);
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
        std::lock_guard<std::mutex> lock(marketDepthHandlersMutex);
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
    std::vector<std::string> accts = splitCSV(accountsList);
    if (accts.size() == 1)
        defaultAccount = accts[0];
    if (accts.size() > 1)
        defaultAccount = accts[1];
}
void IBConnector::receiveFA(faDataType pFaDataType, const std::string& cxml){}
void IBConnector::historicalData(TickerId reqId, const Bar& bar)
{
    std::lock_guard<std::mutex> lock(historicalDataHandlersMutex);
    auto handler = historicalDataHandlers[reqId];
    if (handler != nullptr)
        handler->OnHistoricalData(reqId, bar);
}
void IBConnector::historicalDataEnd(int reqId, const std::string& startDateStr, const std::string& endDateStr)
{
    std::lock_guard<std::mutex> lock(historicalDataHandlersMutex);
    auto handler = historicalDataHandlers[reqId];
    if (handler != nullptr)
        handler->OnHistoricalDataEnd(reqId, startDateStr, endDateStr);
}

void IBConnector::scannerParameters(const std::string& xml)
{
    std::lock_guard<std::mutex> lock(scannerHandlerMutex);
    for(auto handler : scannerHandlers)
        handler->OnScannerParameters(this, xml);
}

void IBConnector::scannerData(int reqId, int rank, const ContractDetails& contractDetails, const std::string& distance,
            const std::string& benchmark, const std::string& projection, const std::string& legsStr)
{
    std::lock_guard<std::mutex> lock(scannerHandlerMutex);
    for(auto handler : scannerHandlers)
        handler->OnScannerData(this, reqId, rank, contractDetails, distance, benchmark, projection, legsStr);
}
void IBConnector::scannerDataEnd(int reqId)
{
    std::lock_guard<std::mutex> lock(scannerHandlerMutex);
    for(auto handler : scannerHandlers)
        handler->OnScannerDataEnd(this, reqId);
}
void IBConnector::realtimeBar(TickerId reqId, long time, double open, double high, double low, double close,
	        Decimal volume, Decimal wap, int count){}
void IBConnector::currentTime(long time){}
void IBConnector::fundamentalData(TickerId reqId, const std::string& data){}
void IBConnector::deltaNeutralValidation(int reqId, const DeltaNeutralContract& deltaNeutralContract){}
void IBConnector::tickSnapshotEnd( int reqId){}
void IBConnector::marketDataType( TickerId reqId, int marketDataType){}
void IBConnector::commissionReport( const CommissionReport& commissionReport)
{
    std::lock_guard lock(executionHandlersMutex);
    for(auto h : executionHandlers)
    {
        h->OnCommissionReport(commissionReport);
    }
}
void IBConnector::position( const std::string& account, const Contract& contract, Decimal position, 
            double avgCost)
{
    std::lock_guard<std::mutex> lock(accountHandlersMutex);
    for(auto handler : accountHandlers)
    {
        if (handler != nullptr)
            handler->OnPosition(account, contract, position, avgCost);
    }
}
void IBConnector::positionEnd()
{
    std::lock_guard<std::mutex> lock(accountHandlersMutex);
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
void IBConnector::displayGroupUpdated( int reqId, const std::string& contractInfo){}
void IBConnector::verifyAndAuthMessageAPI( const std::string& apiData, const std::string& xyzChallange){}
void IBConnector::verifyAndAuthCompleted( bool isSuccessful, const std::string& errorText){}
void IBConnector::connectAck()
{
    std::lock_guard<std::mutex> lock(connectionMonitorsMutex);
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
    logger->debug("IBConnector", "securityDefinitionOptionalParameter: response for id " + std::to_string(reqId) 
            + " and conid " + std::to_string(underlyingConId));
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
    logger->debug("IBConnector", "securityDefinitionOptionalParameterEnd: reqId: " + std::to_string(reqId) );
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
        logger->debug("IBConnector", "securityDefinitionOptionalParameterEnd: id " + std::to_string(reqId) + " promise value set.");
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
    std::lock_guard<std::mutex> lock(mktDepthExchangesPromisesMutex);
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
    std::lock_guard<std::mutex> lock(historicalDataHandlersMutex);
    auto handler = historicalDataHandlers[reqId];
    if (handler != nullptr)
        handler->OnHistoricalDataUpdate(reqId, bar);
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
