#pragma once
#include "TickHandler.hpp"
#include "HistoricalDataHandler.hpp"
#include "MarketDepthHandler.hpp"
#include "IBConnectionMonitor.hpp"
#include "AccountHandler.hpp"
#include "OrderHandler.hpp"
#include "../ib_api/client/EReaderOSSignal.h"
#include "../ib_api/client/EClientSocket.h"
#include "../ib_api/client/EWrapper.h"
#include "../ib_api/client/EReader.h"
#include "../ib_api/client/TagValue.h"
#include <string>
#include <thread>
#include <unordered_map>
#include <atomic>
#include <memory>
#include <future>

namespace ib_helper {

class IBConnector : public EWrapper
{
    public:
    IBConnector(const std::string& hostname, int port, int clientId);
    virtual ~IBConnector();

    void AddAccountHandler(AccountHandler* in) { accountHandlers.push_back(in); }
    void AddOrderHandler(OrderHandler* in) { orderHandlers.push_back(in); }
    AccountHandler* GetDefaultAccountHandler();
    const std::string GetDefaultAccount() { return defaultAccount; }
    uint32_t GetNextOrderId() { return ++nextOrderId; }
    uint32_t GetNextRequestId() { return ++nextRequestId; }
    void SetDefaultAccount(const std::string& in) { defaultAccount = in; }
    bool IsConnected() const { return fullyConnected; }
    uint32_t SubscribeToMarketData(const Contract& contract, TickHandler* tickHandler, 
            const std::string& genericTickList, bool snapshot, bool regulatorySnapshot, 
            const TagValueListSPtr& mktDataOptions);
    void UnsubscribeFromMarketData(uint32_t reqId);
    uint32_t SubscribeToTickByTick(const Contract& contract, TickHandler* handler, const std::string& tickType, 
            int numberOfTicks, bool ignoreSize);
    void UnsubscribeFromTickByTick(uint32_t reqId);
    std::future<std::vector<DepthMktDataDescription> > RequestMktDepthExchanges();
    uint32_t SubscribeToMarketDepth(const Contract& contract, MarketDepthHandler* depthHandler, uint32_t numLines);
    void UnsubscribeFromMarketDepth(uint32_t subscriptionId);

    bool IsShuttingDown() const { return shuttingDown; }
    void PlaceOrder(int orderId, ::Order ord, const Contract& contract);
    void CancelOrder(const ::Order& ord);
    std::future<ContractDetails> GetContractDetails(const Contract& contract);
    void RequestPositions();
    void RequestAccountUpdates(bool subscribe, const std::string& account);
    void RequestOpenOrders();

    protected:
    IBConnector(); // used for testing/mocking
    /***
     * message loop
     */
    static void processMessages(IBConnector* ibConnector);
    /***
     * EWrapper virtual functions
     */

    virtual void tickPrice( TickerId tickerId, TickType field, double price, const TickAttrib& attrib) override;
    virtual void tickSize(TickerId tickerId, TickType field, Decimal size) override;
    virtual void tickOptionComputation( TickerId tickerId, TickType tickType, int tickAttrib, double impliedVol,
            double delta, double optPrice, double pvDividend, double gamma, double vega, double theta, double undPrice)
            override;
    virtual void tickGeneric(TickerId tickerId, TickType tickType, double value) override;
    virtual void tickString(TickerId tickerId, TickType tickType, const std::string& value) override;
    virtual void tickEFP(TickerId tickerId, TickType tickType, double basisPoints,
            const std::string& formattedBasisPoints, double totalDividends, int holdDays, 
            const std::string& futureLastTradeDate, double dividendImpact, double dividendsToLastTradeDate) override;
    virtual void orderStatus( OrderId orderId, const std::string& status, Decimal filled,
	        Decimal remaining, double avgFillPrice, int permId, int parentId,
	        double lastFillPrice, int clientId, const std::string& whyHeld, double mktCapPrice) override;
    virtual void openOrder( OrderId orderId, const Contract&, const Order&, const OrderState&) override;
    virtual void openOrderEnd() override;
    virtual void winError( const std::string& str, int lastError) override;
    virtual void connectionClosed() override;
    virtual void updateAccountValue(const std::string& key, const std::string& val,
            const std::string& currency, const std::string& accountName) override;
    virtual void updatePortfolio( const Contract& contract, Decimal position, double marketPrice, double marketValue,
            double averageCost, double unrealizedPNL, double realizedPNL, const std::string& accountName) override;
    virtual void updateAccountTime(const std::string& timeStamp) override;
    virtual void accountDownloadEnd(const std::string& accountName) override;
    virtual void nextValidId( OrderId orderId) override;
    virtual void contractDetails( int reqId, const ContractDetails& contractDetails) override;
    virtual void bondContractDetails( int reqId, const ContractDetails& contractDetails) override;
    virtual void contractDetailsEnd( int reqId) override;
    virtual void execDetails( int reqId, const Contract& contract, const Execution& execution) override;
    virtual void execDetailsEnd( int reqId) override;
    virtual void error(int id, int errorCode, const std::string& errorString, 
            const std::string& advancedOrderRejectJson) override;
    virtual void updateMktDepth(TickerId id, int position, int operation, int side, double price, Decimal size) override;
    virtual void updateMktDepthL2(TickerId id, int position, const std::string& marketMaker, int operation,
	        int side, double price, Decimal size, bool isSmartDepth) override;
    virtual void updateNewsBulletin(int msgId, int msgType, const std::string& newsMessage, 
            const std::string& originExch) override;
    virtual void managedAccounts( const std::string& accountsList) override;
    virtual void receiveFA(faDataType pFaDataType, const std::string& cxml) override;
    virtual void historicalData(TickerId reqId, const Bar& bar) override;
    virtual void historicalDataEnd(int reqId, const std::string& startDateStr, const std::string& endDateStr) override;
    virtual void scannerParameters(const std::string& xml) override;
    virtual void scannerData(int reqId, int rank, const ContractDetails& contractDetails, const std::string& distance,
            const std::string& benchmark, const std::string& projection, const std::string& legsStr) override;
    virtual void scannerDataEnd(int reqId) override;
    virtual void realtimeBar(TickerId reqId, long time, double open, double high, double low, double close,
	        Decimal volume, Decimal wap, int count) override;
    virtual void currentTime(long time) override;
    virtual void fundamentalData(TickerId reqId, const std::string& data) override;
    virtual void deltaNeutralValidation(int reqId, const DeltaNeutralContract& deltaNeutralContract) override;
    virtual void tickSnapshotEnd( int reqId) override;
    virtual void marketDataType( TickerId reqId, int marketDataType) override;
    virtual void commissionReport( const CommissionReport& commissionReport) override;
    virtual void position( const std::string& account, const Contract& contract, Decimal position, 
            double avgCost) override;
    virtual void positionEnd() override;
    virtual void accountSummary( int reqId, const std::string& account, const std::string& tag, 
            const std::string& value, const std::string& curency) override;
    virtual void accountSummaryEnd( int reqId) override;
    virtual void verifyMessageAPI( const std::string& apiData) override;
    virtual void verifyCompleted( bool isSuccessful, const std::string& errorText) override;
    virtual void displayGroupList( int reqId, const std::string& groups) override;
    virtual void displayGroupUpdated( int reqId, const std::string& contractInfo) override;
    virtual void verifyAndAuthMessageAPI( const std::string& apiData, const std::string& xyzChallange) override;
    virtual void verifyAndAuthCompleted( bool isSuccessful, const std::string& errorText) override;
    virtual void connectAck() override;
    virtual void positionMulti( int reqId, const std::string& account,const std::string& modelCode,
            const Contract& contract, Decimal pos, double avgCost) override;
    virtual void positionMultiEnd( int reqId) override;
    virtual void accountUpdateMulti( int reqId, const std::string& account, const std::string& modelCode, 
            const std::string& key, const std::string& value, const std::string& currency) override;
    virtual void accountUpdateMultiEnd( int reqId) override;
    virtual void securityDefinitionOptionalParameter(int reqId, const std::string& exchange, int underlyingConId, 
            const std::string& tradingClass, const std::string& multiplier, const std::set<std::string>& expirations,
            const std::set<double>& strikes) override;
    virtual void securityDefinitionOptionalParameterEnd(int reqId) override;
    virtual void softDollarTiers(int reqId, const std::vector<SoftDollarTier> &tiers) override;
    virtual void familyCodes(const std::vector<FamilyCode> &familyCodes) override;
    virtual void symbolSamples(int reqId, const std::vector<ContractDescription> &contractDescriptions) override;
    virtual void mktDepthExchanges(const std::vector<DepthMktDataDescription> &depthMktDataDescriptions) override;
    virtual void tickNews(int tickerId, time_t timeStamp, const std::string& providerCode, const std::string& articleId,
            const std::string& headline, const std::string& extraData) override;
    virtual void smartComponents(int reqId, const SmartComponentsMap& theMap) override;
    virtual void tickReqParams(int tickerId, double minTick, const std::string& bboExchange, int snapshotPermissions) override;
    virtual void newsProviders(const std::vector<NewsProvider> &newsProviders) override;
    virtual void newsArticle(int requestId, int articleType, const std::string& articleText) override;
    virtual void historicalNews(int requestId, const std::string& time, const std::string& providerCode,
            const std::string& articleId, const std::string& headline) override;
    virtual void historicalNewsEnd(int requestId, bool hasMore) override;
    virtual void headTimestamp(int reqId, const std::string& headTimestamp) override;
    virtual void histogramData(int reqId, const HistogramDataVector& data) override;
    virtual void historicalDataUpdate(TickerId reqId, const Bar& bar) override;
    virtual void rerouteMktDataReq(int reqId, int conid, const std::string& exchange) override;
    virtual void rerouteMktDepthReq(int reqId, int conid, const std::string& exchange) override;
    virtual void marketRule(int marketRuleId, const std::vector<PriceIncrement> &priceIncrements) override;
    virtual void pnl(int reqId, double dailyPnL, double unrealizedPnL, double realizedPnL) override;
    virtual void pnlSingle(int reqId, Decimal pos, double dailyPnL, double unrealizedPnL, double realizedPnL,
            double value) override;
    virtual void historicalTicks(int reqId, const std::vector<HistoricalTick> &ticks, bool done) override;
    virtual void historicalTicksBidAsk(int reqId, const std::vector<HistoricalTickBidAsk> &ticks, bool done) override;
    virtual void historicalTicksLast(int reqId, const std::vector<HistoricalTickLast> &ticks, bool done) override;
    virtual void tickByTickAllLast(int reqId, int tickType, time_t time, double price, Decimal size,
            const TickAttribLast& tickAttribLast, const std::string& exchange, 
            const std::string& specialConditions) override;
    virtual void tickByTickBidAsk(int reqId, time_t time, double bidPrice, double askPrice, Decimal bidSize,
            Decimal askSize, const TickAttribBidAsk& tickAttribBidAsk) override;
    virtual void tickByTickMidPoint(int reqId, time_t time, double midPoint) override;
    virtual void orderBound(long long orderId, int apiClientId, int apiOrderId) override;
    virtual void completedOrder(const Contract& contract, const Order& order, const OrderState& orderState) override;
    virtual void completedOrdersEnd() override;
    virtual void replaceFAEnd(int reqId, const std::string& text) override;
    virtual void wshMetaData(int reqId, const std::string& dataJson) override;
    virtual void wshEventData(int reqId, const std::string& dataJson) override;
    virtual void historicalSchedule(int reqId, const std::string& startDateTime, const std::string& endDateTime,
            const std::string& timeZone, const std::vector<HistoricalSession>& sessions) override;
    virtual void userInfo(int reqId, const std::string& whiteBrandingId) override;

    protected: // variables
    std::shared_ptr<std::thread> listenerThread = nullptr;
    volatile bool shuttingDown = false;
    bool fullyConnected = false;
    EReaderOSSignal* osSignal = nullptr;
    EClientSocket* ibClient = nullptr;
    EReader* reader = nullptr;
    std::atomic<uint32_t> nextOrderId = 0;
    std::atomic<uint32_t> nextRequestId = 0;
    std::unordered_map<uint32_t, TickHandler* > tickHandlers{};
    std::unordered_map<uint32_t, MarketDepthHandler* > marketDepthHandlers{};
    std::unordered_map<uint32_t, HistoricalDataHandler*> historicalDataHandlers{};
    std::vector<IBConnectionMonitor*> connectionMonitors{};
    std::vector<AccountHandler*> accountHandlers{};
    std::vector<OrderHandler*> orderHandlers{};
    std::unordered_map<uint32_t, std::promise<ContractDetails> > contractDetailsHandlers{};
    private:
    std::string defaultAccount;
    std::mutex mktDepthExchangesPromisesMutex;
    std::vector<std::promise<std::vector<DepthMktDataDescription> > > mktDepthExchangesPromises{};
}; 

} // namespace ib_helper

