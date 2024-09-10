#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "IBManagedConnector.hpp"

class MockEClient : public EClientSocket
{
public:
    MockEClient() : EClientSocket(nullptr, nullptr) {}
    MockEClient(EWrapper* wrapper, EReaderSignal* signal) : EClientSocket(wrapper, signal) {}

    bool eConnect(const char* hostname, int port, int clientId, bool something) { return true; }
    void eDisconnect() {}

    // EClient implementation

    void reqMktData(TickerId id, const Contract& contract,
        const std::string& genericTicks, bool snapshot, bool regulatorySnaphsot, const TagValueListSPtr& mktDataOptions) {}
    void cancelMktData(TickerId id) {}
    void placeOrder(OrderId id, const Contract& contract, const Order& order) {}
    void cancelOrder(OrderId id, const std::string& manualOrderCancelTime) {}
    void reqOpenOrders() {}
    void reqAccountUpdates(bool subscribe, const std::string& acctCode) {}
    void reqExecutions(int reqId, const ExecutionFilter& filter) {}
    void reqIds(int numIds) {}
    void reqContractDetails(int reqId, const Contract& contract) {}
    void reqMktDepth(TickerId tickerId, const Contract& contract, int numRows, bool isSmartDepth, const TagValueListSPtr& mktDepthOptions) {}
    void cancelMktDepth(TickerId tickerId, bool isSmartDepth) {}
    void reqNewsBulletins(bool allMsgs) {}
    void cancelNewsBulletins() {}
    void setServerLogLevel(int level) {}
    void reqAutoOpenOrders(bool bAutoBind) {}
    void reqAllOpenOrders() {}
    void reqManagedAccts() {}
    void requestFA(faDataType pFaDataType) {}
    void replaceFA(int reqId, faDataType pFaDataType, const std::string& cxml) {}
    void reqHistoricalData(TickerId id, const Contract& contract,
        const std::string& endDateTime, const std::string& durationStr,
        const std::string&  barSizeSetting, const std::string& whatToShow,
        int useRTH, int formatDate, bool keepUpToDate, const TagValueListSPtr& chartOptions) {}
    void exerciseOptions(TickerId tickerId, const Contract& contract,
        int exerciseAction, int exerciseQuantity,
        const std::string& account, int override) {}
    void cancelHistoricalData(TickerId tickerId ) {}
    void reqRealTimeBars(TickerId id, const Contract& contract, int barSize,
        const std::string& whatToShow, bool useRTH, const TagValueListSPtr& realTimeBarsOptions) {}
    void cancelRealTimeBars(TickerId tickerId ) {}
    void cancelScannerSubscription(int tickerId) {}
    void reqScannerParameters() {}
    void reqScannerSubscription(int tickerId, const ScannerSubscription& subscription, 
                                const TagValueListSPtr& scannerSubscriptionOptions, const TagValueListSPtr& scannerSubscriptionFilterOptions) {}
    void reqCurrentTime() {}
    void reqFundamentalData(TickerId reqId, const Contract&, const std::string& reportType,
                                 //reserved for future use, must be blank
                                 const TagValueListSPtr& fundamentalDataOptions) {}
    void cancelFundamentalData(TickerId reqId) {}
    void calculateImpliedVolatility(TickerId reqId, const Contract& contract, double optionPrice, double underPrice, 
        //reserved for future use, must be blank
        const TagValueListSPtr& miscOptions) {}
    void calculateOptionPrice(TickerId reqId, const Contract& contract, double volatility, double underPrice, 
        //reserved for future use, must be blank
        const TagValueListSPtr& miscOptions) {}
    void cancelCalculateImpliedVolatility(TickerId reqId) {}
    void cancelCalculateOptionPrice(TickerId reqId) {}
    void reqGlobalCancel() {}
    void reqMarketDataType(int marketDataType) {}
    void reqPositions() {}
    void cancelPositions() {}
    void reqAccountSummary(int reqId, const std::string& groupName, const std::string& tags) {}
    void cancelAccountSummary(int reqId) {}
    void verifyRequest(const std::string& apiName, const std::string& apiVersion) {}
    void verifyMessage(const std::string& apiData) {}
    void verifyAndAuthRequest(const std::string& apiName, const std::string& apiVersion, const std::string& opaqueIsvKey) {}
    void verifyAndAuthMessage(const std::string& apiData, const std::string& xyzResponse) {}
    void queryDisplayGroups(int reqId) {}
    void subscribeToGroupEvents(int reqId, int groupId) {}
    void updateDisplayGroup(int reqId, const std::string& contractInfo) {}
    void unsubscribeFromGroupEvents(int reqId) {}
    void reqPositionsMulti(int reqId, const std::string& account, const std::string& modelCode) {}
    void cancelPositionsMulti(int reqId) {}
    void reqAccountUpdatesMulti(int reqId, const std::string& account, const std::string& modelCode, bool ledgerAndNLV) {}
    void cancelAccountUpdatesMulti(int reqId) {}
    void reqSecDefOptParams(int reqId, const std::string& underlyingSymbol, const std::string& futFopExchange, const std::string& underlyingSecType, int underlyingConId) {}
    void reqSoftDollarTiers(int reqId) {}
    void reqFamilyCodes() {}
    void reqMatchingSymbols(int reqId, const std::string& pattern) {}
    void reqMktDepthExchanges() {}
    void reqSmartComponents(int reqId, std::string bboExchange) {}
    void reqNewsProviders() {}
    void reqNewsArticle(int requestId, const std::string& providerCode, const std::string& articleId, const TagValueListSPtr& newsArticleOptions) {}
    void reqHistoricalNews(int requestId, int conId, const std::string& providerCodes, const std::string& startDateTime, const std::string& endDateTime, int totalResults, 
        const TagValueListSPtr& historicalNewsOptions) {}
    void reqHeadTimestamp(int tickerId, const Contract &contract, const std::string& whatToShow, int useRTH, int formatDate) {}
    void cancelHeadTimestamp(int tickerId) {}
    void reqHistogramData(int reqId, const Contract &contract, bool useRTH, const std::string& timePeriod) {}
    void cancelHistogramData(int reqId) {}
    void reqMarketRule(int marketRuleId) {}

    void reqPnL(int reqId, const std::string& account, const std::string& modelCode) {}
    void cancelPnL(int reqId) {}
    void reqPnLSingle(int reqId, const std::string& account, const std::string& modelCode, int conId) {}
    void cancelPnLSingle(int reqId) {}
    void reqHistoricalTicks(int reqId, const Contract &contract, const std::string& startDateTime,
            const std::string& endDateTime, int numberOfTicks, const std::string& whatToShow, int useRth, bool ignoreSize, const TagValueListSPtr& miscOptions);
    void reqTickByTickData(int reqId, const Contract &contract, const std::string& tickType, int numberOfTicks, bool ignoreSize) {}
    void cancelTickByTickData(int reqId) {}
    void reqCompletedOrders(bool apiOnly) {}
    void reqWshMetaData(int reqId) {}
    void reqWshEventData(int reqId, const WshEventData &wshEventData) {}
    void cancelWshMetaData(int reqId) {}
    void cancelWshEventData(int reqid) {}
    void reqUserInfo(int reqId) {}
};

class ReconnectableIBConnectorMock : public ib_helper::IBConnector
{
public:
    ReconnectableIBConnectorMock(ib_helper::IBConnectionMonitor* listener)// : ib_helper::IBConnector("127.0.0.1", 1234, 7, listener)
    {
        currentConnectionStatus = ib_helper::IBConnector::ConnectionStatus::FULLY_CONNECTED;
        ibClient = new MockEClient;
        connectionMonitors.push_back(listener);
    }
    ~ReconnectableIBConnectorMock()
    {
        if (messageLoopThread.joinable())
            messageLoopThread.join();
    }

    //MOCK_METHOD(bool, IsConnected, (), (const override));

    void throwError(int code, const std::string msg)
    {
        if (messageLoopThread.joinable())
            messageLoopThread.join();
        messageLoopThread = std::thread(&ReconnectableIBConnectorMock::throwError_, this, code, msg);
    }

private:
    void throwError_(int code, const std::string msg)
    {
        error(1, code, msg, "");
    }
    std::thread messageLoopThread;
};

TEST(ReconnectTest, error502)
{
    class MyStrategy : public ib_helper::IBConnectionMonitor
    {
        public:
        void OnConnect(ib_helper::IBConnector* conn) override { onConnectCount++; }
        void OnFullConnect(ib_helper::IBConnector* conn) override { onFullConnectCount++; }
        void OnDisconnect(ib_helper::IBConnector* conn) override { onDisconnectCount++; }
        void OnError(ib_helper::IBConnector* conn, int id, int errorCode, const std::string& msg, const std::string& json) override { onErrorCount++; }

        int onConnectCount = 0;
        int onFullConnectCount = 0;
        int onDisconnectCount = 0;
        int onErrorCount = 0;
    };

    MyStrategy myStrategy;
    // connection is okay
    std::shared_ptr<ReconnectableIBConnectorMock> conn = std::make_shared<ReconnectableIBConnectorMock>(&myStrategy);
    /*
    EXPECT_CALL(*conn, IsConnected())
        .Times(testing::AtLeast(2))
        .WillOnce(testing::Return(true))
        .WillOnce(testing::Return(false))
        ;
    */

    ASSERT_TRUE(conn->IsConnected()); // because we're faking it
    EXPECT_EQ( myStrategy.onConnectCount, 0);
    EXPECT_EQ( myStrategy.onFullConnectCount, 0);
    EXPECT_EQ( myStrategy.onDisconnectCount, 0);
    EXPECT_EQ( myStrategy.onErrorCount, 0);

    // subscribe to historical data

    // eventually a 502 error happens
    conn->throwError(502, "Blah!");
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    ASSERT_FALSE(conn->IsConnected());
    EXPECT_EQ( myStrategy.onConnectCount, 0);
    EXPECT_EQ( myStrategy.onFullConnectCount, 0);
    EXPECT_EQ( myStrategy.onDisconnectCount, 0); // as there is no ibClient, disconnect is not called
    EXPECT_EQ( myStrategy.onErrorCount, 1);

    // connection should be reestablished
    // everything is back to normal
}

TEST(ReconnectTest, forcedReconnect)
{
    // a class that can mess up the port number
    class MeanConnector : public ib_helper::IBConnector
    {
    public:
        MeanConnector(const std::string& host, int port, int clientId, ib_helper::IBConnectionMonitor* monitor)
                : ib_helper::IBConnector(host,port,clientId, monitor)
        {
        }
        void setPortNumber(int port) { this->port = port; }
    };
    class MyStrategy : public ib_helper::IBConnectionMonitor
    {
        public:
        void OnConnect(ib_helper::IBConnector* conn) override { onConnectCount++; }
        void OnFullConnect(ib_helper::IBConnector* conn) override { onFullConnectCount++; }
        void OnDisconnect(ib_helper::IBConnector* conn) override { onDisconnectCount++; }
        void OnError(ib_helper::IBConnector* conn, int id, int code, const std::string& msg, const std::string& json) override { onErrorCount++; }

        int onConnectCount = 0;
        int onFullConnectCount = 0;
        int onDisconnectCount = 0;
        int onErrorCount = 0;
    };

    MyStrategy myStrategy;
    // connection is okay
    std::shared_ptr<MeanConnector> conn = std::make_shared<MeanConnector>("10.8.0.1", 4002, 8, &myStrategy);
    uint32_t timeout = 5000;
    while(timeout != 0 && !conn->IsConnected())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        timeout -= 100;
    }
    EXPECT_TRUE(conn->IsConnected());

    // disconnect
    EXPECT_TRUE(conn->disconnect());
    // wait for everything to settle down
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    ASSERT_FALSE(conn->IsConnected());
    conn->setPortNumber(1234);
    // attempt retry for 1 minute
    auto endTime = std::chrono::system_clock::now() + std::chrono::minutes(1);
    uint32_t retries = 0;
    while(endTime > std::chrono::system_clock::now() && !conn->IsConnected())
    {
        conn->connect();
        retries++;
        timeout = 5000;
        while(timeout != 0 && !conn->IsConnected())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            timeout -= 100;
        }
        if (!conn->IsConnected())
            conn->disconnect();
    }
    EXPECT_FALSE(conn->IsConnected()); // we tried for a minute straight with no success
    EXPECT_GT(retries, 9);
    conn->setPortNumber(4002);
    // attempt retry for 1 minute
    endTime = std::chrono::system_clock::now() + std::chrono::minutes(1);
    retries = 0;
    while(endTime > std::chrono::system_clock::now() && !conn->IsConnected())
    {
        ASSERT_TRUE(conn->connect());
        retries++;
        timeout = 5000;
        while(timeout != 0 && !conn->IsConnected())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            timeout -= 100;
        }
        if (!conn->IsConnected())
            conn->disconnect();
    }
    EXPECT_TRUE(conn->IsConnected());
    EXPECT_LT(retries, 3);
}

/****
 * This test requires manually shutting down TWS
*/
TEST(ReconnectTest, DISABLED_managedConnection)
{
    class MyStrategy : public ib_helper::IBConnectionMonitor
    {
        public:
        void OnConnect(ib_helper::IBConnector* conn) override { onConnectCount++; }
        void OnFullConnect(ib_helper::IBConnector* conn) override { onFullConnectCount++; }
        void OnDisconnect(ib_helper::IBConnector* conn) override { onDisconnectCount++; }
        void OnError(ib_helper::IBConnector* conn, int id, int code, const std::string& msg, const std::string& json) override { onErrorCount++; }

        int onConnectCount = 0;
        int onFullConnectCount = 0;
        int onDisconnectCount = 0;
        int onErrorCount = 0;
    };

    MyStrategy myStrategy;
    std::shared_ptr<ib_helper::IBManagedConnector> conn = std::make_shared<ib_helper::IBManagedConnector>("10.8.0.1", 4002, 8, &myStrategy);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // wait for connection
    ASSERT_TRUE(conn->IsConnected());
    ASSERT_EQ( myStrategy.onConnectCount, 1);
    ASSERT_EQ( myStrategy.onFullConnectCount, 1);
    ASSERT_EQ( myStrategy.onDisconnectCount, 0);
    ASSERT_EQ( myStrategy.onErrorCount, 0);

    // eventually a 502 error happens
    std::cout << "Waiting to be disconnected\n";
    while (myStrategy.onDisconnectCount == 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "Disconnect happened, waiting for reconnect\n";
    // OK, the disconnect happens, now wait for the full reconnect
    int lastConnectCount = myStrategy.onConnectCount;
    // wait 5 minutes to attempt to get connected again
    auto endTime = std::chrono::system_clock::now() + std::chrono::minutes(5);
    while (endTime > std::chrono::system_clock::now() && lastConnectCount == myStrategy.onConnectCount)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "Reconnected\n";
    // connection should be reestablished
    // everything is back to normal
}

