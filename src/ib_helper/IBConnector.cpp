#include "IBConnector.hpp"
#include "../util/SysLogger.h"

namespace ib_helper {

util::SysLogger* logger = nullptr;
std::string logCategory("IBConnector");

IBConnector::IBConnector()
{
    logger = util::SysLogger::getInstance();
}

IBConnector::IBConnector(const std::string& hostname, int port, int clientId)
{
    logger = util::SysLogger::getInstance();
	osSignal = new EReaderOSSignal(1000); // timeout (1000 == 1 sec)
	ibClient = new EClientSocket(this, osSignal);
	if (!ibClient->eConnect(hostname.c_str(), port, clientId, false))
    {
		return;
    }
	// message loop
	reader = new EReader(ibClient, osSignal);
	reader->start();
	listenerThread = std::make_shared<std::thread>(processMessages, this);
}

IBConnector::~IBConnector() {
	shuttingDown = true;
    if (ibClient != nullptr)
        ibClient->eDisconnect();
    if (listenerThread != nullptr)
	    listenerThread->join();
    if (reader != nullptr)
        delete reader;
    if (ibClient != nullptr)
        delete ibClient;
    if (osSignal != nullptr)
        delete osSignal;
}

std::future<ContractDetails> IBConnector::GetContractDetails(const Contract& contract)
{
    uint32_t promiseId = this->GetNextRequestId();
    auto& promise = contractDetailsHandlers[promiseId];
    ibClient->reqContractDetails(promiseId, contract);
    return promise.get_future();
}

void IBConnector::RequestPositions()
{
    this->ibClient->reqPositions();
}

void IBConnector::RequestAccountUpdates(bool subscribe, const std::string& account)
{
    this->ibClient->reqAccountUpdates(subscribe, account);
}

void IBConnector::RequestOpenOrders()
{
    this->ibClient->reqOpenOrders();
}

AccountHandler* IBConnector::GetDefaultAccountHandler() 
{ 
    if (accountHandlers.size() == 0) 
        return nullptr; 
    return accountHandlers[0]; 
}

void IBConnector::RemoveConnectionMonitor(IBConnectionMonitor* in)
{
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
    connectionMonitors.push_back(in);
    if (ibClient->isConnected())
        in->OnConnect(this);
    else
        in->OnDisconnect(this);
}

void IBConnector::CancelOrder(int orderId, const std::string& time)
{
    this->ibClient->cancelOrder(orderId, time);
}

void IBConnector::PlaceOrder(int orderId, const Contract& contract, const ::Order& ord)
{
    this->ibClient->placeOrder(orderId, contract, ord);
}

void IBConnector::IBConnector::processMessages(IBConnector* ibConnector) {
	while (!ibConnector->shuttingDown) {
		// wait for connection
		if (!ibConnector->fullyConnected) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		if (!ibConnector->shuttingDown)
        {
		    ibConnector->osSignal->waitForSignal();
            if (!ibConnector->shuttingDown)
            {
			    ibConnector->reader->processMsgs();
            }
        }
	}
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
    uint32_t reqId = GetNextRequestId();
    tickHandlers[reqId] = tickHandler;
    ibClient->reqMktData(reqId, contract, genericTickList, snapshot, regulatorySnapshot, mktDataOptions);
    return reqId;
};

void IBConnector::UnsubscribeFromMarketData(uint32_t reqId) { ibClient->cancelMktData(reqId); }

uint32_t IBConnector::SubscribeToTickByTick(const Contract& contract, TickHandler* handler, const std::string& tickType, 
            int numberOfTicks, bool ignoreSize)
{
    uint32_t reqId = GetNextRequestId();
    tickHandlers[reqId] = handler;
    ibClient->reqTickByTickData(reqId, contract, tickType, numberOfTicks, ignoreSize);
    return reqId;
};

void IBConnector::UnsubscribeFromTickByTick(uint32_t reqId) { ibClient->cancelTickByTickData(reqId); }

uint32_t IBConnector::SubscribeToMarketDepth(const Contract& contract, MarketDepthHandler* depthHandler, uint32_t numLines)
{
    uint32_t reqId = GetNextRequestId();
    marketDepthHandlers[reqId] = depthHandler;
    //ibClient->reqMktDepth(reqId, contract, int numRows, bool isSmartDepth, const int &mktDepthOptions)
    TagValueListSPtr mktDepthOptions;
    ibClient->reqMktDepth(reqId, contract, numLines, true, mktDepthOptions);
    return reqId;
}

void IBConnector::UnsubscribeFromMarketDepth(uint32_t subscriptionId)
{
    ibClient->cancelMktDepth(subscriptionId, true);
}

std::future<std::vector<DepthMktDataDescription> > IBConnector::RequestMktDepthExchanges()
{
    std::lock_guard<std::mutex> lock(mktDepthExchangesPromisesMutex);
    auto& promise = mktDepthExchangesPromises.emplace_back( );
    ibClient->reqMktDepthExchanges();
    return promise.get_future();
}

void IBConnector::UnsubscribeFromHistoricalData(uint32_t historicalSubscriptionId)
{
    ibClient->cancelHistoricalData(historicalSubscriptionId);
}

uint32_t IBConnector::SubscribeToHistoricalData(const Contract& contract, HistoricalDataHandler* handler,
        const std::string& timePeriod, const std::string& barSize)
{
    std::string whatToShow = "TRADES";
    if (contract.secType == "CASH")
        whatToShow = "MIDPOINT";
    int id = GetNextRequestId();
    historicalDataHandlers[id] = handler;
    ibClient->reqHistoricalData(id, contract, "", timePeriod, barSize, whatToShow, 0, 2, true, nullptr);
    return id;
}

void IBConnector::tickPrice( TickerId tickerId, TickType field, double price, const TickAttrib& attrib)
{
    try 
    {
        TickHandler* handler = tickHandlers.at(tickerId);
        handler->OnTickPrice(tickerId, field, price, attrib);
    } catch (const std::out_of_range& oor) {}
}
void IBConnector::tickSize(TickerId tickerId, TickType field, Decimal size)
{
    try 
    {
        TickHandler* handler = tickHandlers.at(tickerId);
        handler->OnTickSize(tickerId, field, size);
    } catch (const std::out_of_range& oor) {}
}
void IBConnector::tickOptionComputation( TickerId tickerId, TickType tickType, int tickAttrib, double impliedVol,
            double delta, double optPrice, double pvDividend, double gamma, double vega, double theta, double undPrice)
{
    try 
    {
        TickHandler* handler = tickHandlers.at(tickerId);
        handler->OnTickOptionComputation(tickerId, tickType, tickAttrib, impliedVol, delta, optPrice, pvDividend, gamma,
                vega, theta, undPrice);
    } catch (const std::out_of_range& oor) {}
}
void IBConnector::tickGeneric(TickerId tickerId, TickType tickType, double value)
{
    try 
    {
        TickHandler* handler = tickHandlers.at(tickerId);
        handler->OnTickGeneric(tickerId, tickType, value);
    } catch (const std::out_of_range& oor) {}
}
void IBConnector::tickString(TickerId tickerId, TickType tickType, const std::string& value)
{
    try 
    {
        TickHandler* handler = tickHandlers.at(tickerId);
        handler->OnTickString(tickerId, tickType, value);
    } catch (const std::out_of_range& oor) {}
}
void IBConnector::tickEFP(TickerId tickerId, TickType tickType, double basisPoints,
            const std::string& formattedBasisPoints, double totalDividends, int holdDays, 
            const std::string& futureLastTradeDate, double dividendImpact, double dividendsToLastTradeDate)
{
    try 
    {
        TickHandler* handler = tickHandlers.at(tickerId);
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
        TickHandler* handler = tickHandlers.at(reqId);
        handler->OnTickByTickAllLast(reqId, tickType, time, price, size, tickAttribLast, exchange, specialConditions);
    } catch (const std::out_of_range& oor) {}
}
void IBConnector::tickByTickBidAsk(int reqId, time_t time, double bidPrice, double askPrice, Decimal bidSize,
            Decimal askSize, const TickAttribBidAsk& tickAttribBidAsk)
{
    try 
    {
        TickHandler* handler = tickHandlers.at(reqId);
        handler->OnTickByTickBidAsk(reqId, time, bidPrice, askPrice, bidSize, askSize, tickAttribBidAsk);
    } catch (const std::out_of_range& oor) {}
}
void IBConnector::tickByTickMidPoint(int reqId, time_t time, double midPoint)
{
    try 
    {
        TickHandler* handler = tickHandlers.at(reqId);
        handler->OnTickByTickMidPoint(reqId, time, midPoint);
    } catch (const std::out_of_range& oor) {}
}
void IBConnector::orderBound(long long orderId, int apiClientId, int apiOrderId)
{
    for(auto* handler : orderHandlers)
    {
        handler->OnOrderBound(orderId, apiClientId, apiOrderId);
    }
}
void IBConnector::orderStatus( OrderId orderId, const std::string& status, Decimal filled,
	        Decimal remaining, double avgFillPrice, int permId, int parentId,
	        double lastFillPrice, int clientId, const std::string& whyHeld, double mktCapPrice)
{
    for(auto* handler : orderHandlers)
    {
        handler->OnOrderStatus(orderId, status, filled, remaining, avgFillPrice, permId, parentId, 
                lastFillPrice, clientId, whyHeld, mktCapPrice);
    }
}
void IBConnector::openOrder( OrderId orderId, const Contract& contract, const ::Order& order, const OrderState& orderState)
{
    for(auto* handler : orderHandlers)
    {
        handler->OnOpenOrder(orderId, contract, order, orderState);
    }
}
void IBConnector::openOrderEnd()
{
    for(auto* handler : orderHandlers)
    {
        handler->OnOpenOrderEnd();
    }
}
void IBConnector::winError( const std::string& str, int lastError)
{
    logger->error(logCategory, str);
}
void IBConnector::connectionClosed()
{
    for(auto cm : connectionMonitors)
        cm->OnDisconnect(this);
}
void IBConnector::updateAccountValue(const std::string& key, const std::string& val,
            const std::string& currency, const std::string& accountName){}
void IBConnector::updatePortfolio( const Contract& contract, Decimal position, double marketPrice, double marketValue,
            double averageCost, double unrealizedPNL, double realizedPNL, const std::string& accountName){}
void IBConnector::updateAccountTime(const std::string& timeStamp){}
void IBConnector::accountDownloadEnd(const std::string& accountName){}
void IBConnector::nextValidId( OrderId orderId)
{
    nextOrderId = orderId;
    fullyConnected = true;
}
void IBConnector::contractDetails( int reqId, const ContractDetails& contractDetails)
{
    auto& promise = contractDetailsHandlers[reqId];
    promise.set_value(contractDetails);
}
void IBConnector::bondContractDetails( int reqId, const ContractDetails& contractDetails){}
void IBConnector::contractDetailsEnd( int reqId){}
void IBConnector::execDetails( int reqId, const Contract& contract, const Execution& execution){}
void IBConnector::execDetailsEnd( int reqId){}
void IBConnector::error(int id, int errorCode, const std::string& errorString, 
            const std::string& advancedOrderRejectJson)
{
    // if this is the "can't connect to IB error on login, do a shutdown to get out of connection loop
    if (errorCode == 502 && !fullyConnected)
        this->shuttingDown = true;
    if (errorCode == 200) // no security definition found
    {
        // if the request id is found, set the exception in the promise/future
        auto itr = contractDetailsHandlers.find(id);
        if (itr != contractDetailsHandlers.end())
        {
            (*itr).second.set_exception(std::make_exception_ptr( std::out_of_range("Symbol not found")));
        }
    }
    for(auto handler : accountHandlers)
    {
        handler->OnError(id, errorCode, errorString, advancedOrderRejectJson);
    }
    std::string msg = "Error id: " + std::to_string(id) 
        + " Code: " + std::to_string(errorCode) 
        + ": " + errorString 
        + ". JSON: " + advancedOrderRejectJson;
    logger->error(logCategory, msg);
}
void IBConnector::updateMktDepth(TickerId reqId, int position, int operation, int side, double price, Decimal size)
{
    try 
    {
        MarketDepthHandler* handler = marketDepthHandlers.at(reqId);
        handler->OnUpdateMktDepth(reqId, position, operation, side, price, size);
    } catch (const std::out_of_range& oor) {}
}
void IBConnector::updateMktDepthL2(TickerId reqId, int position, const std::string& marketMaker, int operation,
	        int side, double price, Decimal size, bool isSmartDepth)
{
    try 
    {
        MarketDepthHandler* handler = marketDepthHandlers.at(reqId);
        handler->OnUpdateMktDepthL2(reqId, position, marketMaker, operation, side, price, size, isSmartDepth);
    } catch (const std::out_of_range& oor) {}
}
void IBConnector::updateNewsBulletin(int msgId, int msgType, const std::string& newsMessage, 
            const std::string& originExch){}
void IBConnector::managedAccounts( const std::string& accountsList){}
void IBConnector::receiveFA(faDataType pFaDataType, const std::string& cxml){}
void IBConnector::historicalData(TickerId reqId, const Bar& bar)
{
    auto* handler = historicalDataHandlers[reqId];
    if (handler != nullptr)
        handler->OnHistoricalData(reqId, bar);
}
void IBConnector::historicalDataEnd(int reqId, const std::string& startDateStr, const std::string& endDateStr)
{
    auto* handler = historicalDataHandlers[reqId];
    if (handler != nullptr)
        handler->OnHistoricalDataEnd(reqId, startDateStr, endDateStr);
}

void IBConnector::scannerParameters(const std::string& xml){}
void IBConnector::scannerData(int reqId, int rank, const ContractDetails& contractDetails, const std::string& distance,
            const std::string& benchmark, const std::string& projection, const std::string& legsStr){}
void IBConnector::scannerDataEnd(int reqId){}
void IBConnector::realtimeBar(TickerId reqId, long time, double open, double high, double low, double close,
	        Decimal volume, Decimal wap, int count){}
void IBConnector::currentTime(long time){}
void IBConnector::fundamentalData(TickerId reqId, const std::string& data){}
void IBConnector::deltaNeutralValidation(int reqId, const DeltaNeutralContract& deltaNeutralContract){}
void IBConnector::tickSnapshotEnd( int reqId){}
void IBConnector::marketDataType( TickerId reqId, int marketDataType){}
void IBConnector::commissionReport( const CommissionReport& commissionReport){}
void IBConnector::position( const std::string& account, const Contract& contract, Decimal position, 
            double avgCost)
{
    for(auto* handler : accountHandlers)
    {
        handler->OnPosition(account, contract, position, avgCost);
    }
}
void IBConnector::positionEnd()
{
    for(auto* handler : accountHandlers)
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
            const std::set<double>& strikes){}
void IBConnector::securityDefinitionOptionalParameterEnd(int reqId){}
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
    auto* handler = historicalDataHandlers[reqId];
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
