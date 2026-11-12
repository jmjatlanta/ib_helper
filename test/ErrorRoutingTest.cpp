#include "../src/ib_helper/IBConnector.hpp"
#include <gtest/gtest.h>
#include <chrono>
#include <future>
#include <vector>

/***
 * Tests that IBConnector::error() sends each message to the owner of its id.
 * None of these need a running TWS.
 */

namespace {

using ib_helper::IbError;
using ib_helper::IbSeverity;
using ib_helper::IBConnector;

class RoutingConnector : public IBConnector
{
    public:
    RoutingConnector() : IBConnector() {}
    using IBConnector::error;
    // the Subscribe/Get calls that need TWS return early; register owners as they would
    void RegisterHistorical(int reqId, ib_helper::HistoricalDataHandler* h) { historicalDataHandlers[reqId] = h; }
    void RegisterScanner(int reqId) { scannerRequestIds.insert(reqId); }
    std::future<std::vector<ContractDetails>> RegisterContractDetails(int reqId)
    {
        contractDetailsData[reqId];
        return contractDetailsHandlers[reqId].get_future();
    }
    std::future<std::vector<ib_helper::SecurityDefinitionOptionParameter>> RegisterOptionParameters(int reqId)
    {
        return RegisterOptionParametersRequest(reqId);
    }
    bool HasContractDetailsRequest(int reqId) { return contractDetailsHandlers.contains(reqId); }
};

class RecordingOrderHandler : public ib_helper::OrderHandler
{
    public:
    void OnOpenOrder(int, const Contract&, const ::Order&, const OrderState&) override {}
    void OnOrderStatus(int, const std::string&, Decimal, Decimal, double, int, int, double, int,
            const std::string&, double) override {}
    void OnOpenOrderEnd() override {}
    void OnOrderBound(long, int, int) override {}
    void OnOrderError(int orderId, const IbError& err) override { errors.push_back(err); }
    std::vector<IbError> errors;
};

class RecordingAccountHandler : public ib_helper::AccountHandler
{
    public:
    void OnAccountValueUpdate(const std::string&, const std::string&, const std::string&, const std::string&) override {}
    void OnPortfolioUpdate(const Contract&, Decimal, double, double, double, double, double, const std::string&) override {}
    void OnUpdateAccountTime(const std::string&) override {}
    void OnAccountDownloadEnd(const std::string&) override {}
    void OnPosition(const std::string&, const Contract&, Decimal, double) override {}
    void OnPositionEnd() override {}
    void OnCurrentTime(long) override {}
    void OnError(int id, int code, const std::string&, const std::string&) override { codes.push_back(code); }
    std::vector<int> codes;
};

class RecordingErrorHandler : public ib_helper::ErrorHandler
{
    public:
    void OnError(IBConnector*, const IbError& err) override { errors.push_back(err); }
    std::vector<IbError> errors;
};

class RecordingTickHandler : public ib_helper::TickHandler
{
    public:
    void OnTickPrice(int, int, double, TickAttrib) override {}
    void OnTickSize(int, int, Decimal) override {}
    void OnTickOptionComputation(int, int, int, double, double, double, double, double, double, double, double) override {}
    void OnTickGeneric(int, int, double) override {}
    void OnTickString(int, int, const std::string&) override {}
    void OnTickEFP(int, int, double, const std::string&, double, int, const std::string&, double, double) override {}
    void OnTickByTickAllLast(int, int, time_t, double, Decimal, const TickAttribLast&, const std::string&,
            const std::string&) override {}
    void OnTickByTickBidAsk(int, time_t, double, double, Decimal, Decimal, const TickAttribBidAsk&) override {}
    void OnTickByTickMidPoint(int, time_t, double) override {}
    void OnError(int reqId, const IbError& err) override
    {
        errors.push_back(err);
        if (unsubscribeOnError != nullptr)
            unsubscribeOnError->UnsubscribeFromTickByTick(reqId);
    }
    std::vector<IbError> errors;
    IBConnector* unsubscribeOnError = nullptr;
};

class RecordingDepthHandler : public ib_helper::MarketDepthHandler
{
    public:
    void OnUpdateMktDepth(int, int, int, int, double, Decimal) override {}
    void OnUpdateMktDepthL2(int, int, const std::string&, int, int, double, Decimal, bool) override {}
    void OnError(int reqId, const IbError& err) override { errors.push_back(err); }
    std::vector<IbError> errors;
};

class RecordingHistoricalHandler : public ib_helper::HistoricalDataHandler
{
    public:
    void OnHistoricalData(int, const Bar&) override {}
    void OnHistoricalDataEnd(int reqId, const std::string&, const std::string&) override { ended.push_back(reqId); }
    void OnHistoricalDataUpdate(int, const Bar&) override {}
    void OnError(int reqId, const IbError& err) override { errors.push_back(err); }
    std::vector<IbError> errors;
    std::vector<int> ended;
};

class RecordingScannerHandler : public ib_helper::ScannerHandler
{
    public:
    void OnScannerParameters(IBConnector*, const std::string&) override {}
    void OnScannerData(IBConnector*, int, int, const ContractDetails&, const std::string&,
            const std::string&, const std::string&, const std::string&) override {}
    void OnScannerDataEnd(IBConnector*, int) override {}
    void OnScannerSubscriptionEnd(IBConnector*, int reqId) override { ended.push_back(reqId); }
    void OnError(IBConnector*, int reqId, const IbError& err) override { errors.push_back(err); }
    std::vector<int> ended;
    std::vector<IbError> errors;
};

class RecordingMonitor : public ib_helper::IBConnectionMonitor
{
    public:
    void OnConnect(IBConnector*) override {}
    void OnFullConnect(IBConnector*) override {}
    void OnDisconnect(IBConnector*) override {}
    void OnError(IBConnector*, int, int code, const std::string&, const std::string&) override { codes.push_back(code); }
    void OnIbConnectivity(IBConnector*, bool up, bool dataLost) override { states.push_back({up, dataLost}); }
    std::vector<int> codes;
    std::vector<std::pair<bool, bool>> states;
};

template<class T>
bool IsReady(std::future<T>& f) { return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; }

} // namespace

TEST(ErrorRoutingTest, OrderErrorGoesToOrderHandlers)
{
    RoutingConnector conn;
    RecordingOrderHandler orders;
    RecordingAccountHandler account;
    conn.AddOrderHandler(&orders);
    conn.AddAccountHandler(&account);

    conn.error(5, 0, 201, "Order rejected - reason:", "{\"reason\":\"x\"}");
    ASSERT_EQ(orders.errors.size(), 1u);
    EXPECT_EQ(orders.errors[0].id, 5);
    EXPECT_EQ(orders.errors[0].code, 201);
    EXPECT_EQ(orders.errors[0].advancedRejectJson, "{\"reason\":\"x\"}");
    EXPECT_EQ(orders.errors[0].severity, IbSeverity::OrderRejected);
    // routed errors no longer reach the AccountHandlers
    EXPECT_TRUE(account.codes.empty());

    conn.error(5, 0, 202, "Order Canceled - reason:", "");
    ASSERT_EQ(orders.errors.size(), 2u);
    EXPECT_FALSE(orders.errors[1].isError());
}

TEST(ErrorRoutingTest, OrderErrorWithoutOrderHandlersFallsBack)
{
    RoutingConnector conn;
    RecordingAccountHandler account;
    RecordingErrorHandler catchAll;
    conn.AddAccountHandler(&account);
    conn.AddErrorHandler(&catchAll);

    conn.error(5, 0, 201, "Order rejected - reason:", "");
    ASSERT_EQ(account.codes.size(), 1u);
    EXPECT_EQ(account.codes[0], 201);
    ASSERT_EQ(catchAll.errors.size(), 1u);
}

TEST(ErrorRoutingTest, TickAndDepthErrorsGoToTheirHandler)
{
    RoutingConnector conn;
    RecordingTickHandler ticks;
    RecordingDepthHandler depth;
    RecordingAccountHandler account;
    conn.AddAccountHandler(&account);
    int tickId = conn.SubscribeToTickByTick(Contract(), &ticks, "Last", 0, false);
    int depthId = conn.SubscribeToMarketDepth(Contract(), &depth, 5);

    conn.error(tickId, 0, 10167, "Displaying delayed market data.", "");
    conn.error(depthId, 0, 354, "Requested market data is not subscribed.", "");
    ASSERT_EQ(ticks.errors.size(), 1u);
    EXPECT_EQ(ticks.errors[0].severity, IbSeverity::Warning);
    ASSERT_EQ(depth.errors.size(), 1u);
    EXPECT_EQ(depth.errors[0].code, 354);
    EXPECT_TRUE(account.codes.empty());
}

TEST(ErrorRoutingTest, HandlerCanUnsubscribeFromOnError)
{
    RoutingConnector conn;
    RecordingTickHandler ticks;
    ticks.unsubscribeOnError = &conn;
    RecordingAccountHandler account;
    conn.AddAccountHandler(&account);
    int tickId = conn.SubscribeToTickByTick(Contract(), &ticks, "Last", 0, false);

    conn.error(tickId, 0, 354, "Requested market data is not subscribed.", ""); // must not deadlock
    ASSERT_EQ(ticks.errors.size(), 1u);
    // the subscription is gone, so a later message for that id is unmatched
    conn.error(tickId, 0, 300, "Can't find EId with tickerId", "");
    EXPECT_EQ(ticks.errors.size(), 1u);
    ASSERT_EQ(account.codes.size(), 1u);
    EXPECT_EQ(account.codes[0], 300);
}

TEST(ErrorRoutingTest, HistoricalErrors)
{
    RoutingConnector conn;
    RecordingHistoricalHandler hist;
    int reqId = conn.GetNextRequestId();
    conn.RegisterHistorical(reqId, &hist);

    conn.error(reqId, 0, 162, "Historical Market Data Service error message:Historical data request pacing violation", "");
    ASSERT_EQ(hist.errors.size(), 1u);
    EXPECT_TRUE(hist.errors[0].isError());
    EXPECT_TRUE(hist.ended.empty());

    conn.error(reqId, 0, 165, "Historical Market Data Service query message:", "");
    ASSERT_EQ(hist.errors.size(), 2u);
    ASSERT_EQ(hist.ended.size(), 1u);
    EXPECT_EQ(hist.ended[0], reqId);
}

TEST(ErrorRoutingTest, ScannerErrorsOnlyForKnownScannerIds)
{
    RoutingConnector conn;
    RecordingScannerHandler scanner;
    RecordingHistoricalHandler hist;
    conn.AddScannerHandler(&scanner);
    int histId = conn.GetNextRequestId();
    conn.RegisterHistorical(histId, &hist);
    int scanId = conn.GetNextRequestId();
    conn.RegisterScanner(scanId);

    // a 162 for a historical request does not end any scanner subscription
    conn.error(histId, 0, 162, "Historical Market Data Service error message:API historical data query cancelled", "");
    EXPECT_TRUE(scanner.ended.empty());
    EXPECT_EQ(hist.errors.size(), 1u);

    conn.error(scanId, 0, 162, "Historical Market Data Service error message:API scanner subscription cancelled", "");
    ASSERT_EQ(scanner.errors.size(), 1u);
    ASSERT_EQ(scanner.ended.size(), 1u);
    EXPECT_EQ(scanner.ended[0], scanId);

    // the subscription is over; a second 162 for it is unmatched
    conn.error(scanId, 0, 162, "again", "");
    EXPECT_EQ(scanner.ended.size(), 1u);
    conn.RemoveScannerHandler(&scanner);
}

TEST(ErrorRoutingTest, ContractDetailsFailures)
{
    RoutingConnector conn;
    RecordingAccountHandler account;
    conn.AddAccountHandler(&account);

    // 200 keeps failing with std::out_of_range
    int notFoundId = conn.GetNextRequestId();
    auto notFound = conn.RegisterContractDetails(notFoundId);
    conn.error(notFoundId, 0, 200, "No security definition has been found for the request", "");
    ASSERT_TRUE(IsReady(notFound));
    EXPECT_THROW(notFound.get(), std::out_of_range);
    EXPECT_FALSE(conn.HasContractDetailsRequest(notFoundId));

    // any other failure is an IbException carrying the error
    int failedId = conn.GetNextRequestId();
    auto failed = conn.RegisterContractDetails(failedId);
    conn.error(failedId, 0, 321, "Error validating request", "");
    ASSERT_TRUE(IsReady(failed));
    try
    {
        failed.get();
        FAIL() << "expected IbException";
    }
    catch(const ib_helper::IbException& ex)
    {
        EXPECT_EQ(ex.error().code, 321);
        EXPECT_EQ(ex.error().id, failedId);
    }

    // a warning does not fail the future
    int warnId = conn.GetNextRequestId();
    auto warned = conn.RegisterContractDetails(warnId);
    conn.error(warnId, 0, 2109, "Outside Regular Trading Hours", "");
    conn.error(warnId, 0, 2176, "Some warning", "");
    EXPECT_FALSE(IsReady(warned));
    EXPECT_TRUE(conn.HasContractDetailsRequest(warnId));
    // it is unmatched, so the AccountHandlers see it (except 2109, as before)
    ASSERT_EQ(account.codes.size(), 1u);
    EXPECT_EQ(account.codes[0], 2176);
}

TEST(ErrorRoutingTest, OptionParametersFailure)
{
    RoutingConnector conn;
    int reqId = conn.GetNextRequestId();
    auto fut = conn.RegisterOptionParameters(reqId);
    conn.error(reqId, 0, 321, "Error validating request", "");
    ASSERT_TRUE(IsReady(fut));
    EXPECT_THROW(fut.get(), ib_helper::IbException);
}

TEST(ErrorRoutingTest, UnknownIdsAreUnmatched)
{
    RoutingConnector conn;
    RecordingAccountHandler account;
    RecordingErrorHandler catchAll;
    conn.AddAccountHandler(&account);
    conn.AddErrorHandler(&catchAll);

    conn.error(-1, 0, 2104, "Market data farm connection is OK:usfarm", "");
    conn.error(conn.GetNextRequestId(), 0, 321, "Error validating request", "");
    ASSERT_EQ(catchAll.errors.size(), 2u);
    EXPECT_EQ(catchAll.errors[0].severity, IbSeverity::Info);
    EXPECT_EQ(account.codes.size(), 2u);

    // 2109 reaches the catch-all, but never reached AccountHandlers and still doesn't
    conn.error(-1, 0, 2109, "Outside Regular Trading Hours", "");
    EXPECT_EQ(catchAll.errors.size(), 3u);
    EXPECT_EQ(account.codes.size(), 2u);

    conn.RemoveErrorHandler(&catchAll);
    conn.error(-1, 0, 2104, "Market data farm connection is OK:usfarm", "");
    EXPECT_EQ(catchAll.errors.size(), 3u);
}

TEST(ErrorRoutingTest, ConnectivityGoesToMonitors)
{
    RoutingConnector conn;
    RecordingMonitor monitor;
    RecordingAccountHandler account;
    conn.AddConnectionMonitor(&monitor);
    conn.AddAccountHandler(&account);

    conn.error(-1, 0, 1100, "Connectivity between IB and TWS has been lost.", "");
    conn.error(-1, 0, 1101, "Connectivity restored - data lost.", "");
    conn.error(-1, 0, 1102, "Connectivity restored - data maintained.", "");
    conn.error(-1, 0, 1300, "TWS socket port has been reset", "");
    EXPECT_EQ(monitor.codes, (std::vector<int>{1100, 1101, 1102, 1300}));
    EXPECT_EQ(monitor.states, (std::vector<std::pair<bool, bool>>{{false, false}, {true, true}, {true, false}}));
    EXPECT_TRUE(account.codes.empty());
    conn.RemoveConnectionMonitor(&monitor);
}

TEST(ErrorRoutingTest, ConnectivityWithoutMonitorsFallsBack)
{
    RoutingConnector conn;
    RecordingAccountHandler account;
    conn.AddAccountHandler(&account);
    conn.error(-1, 0, 1100, "Connectivity between IB and TWS has been lost.", "");
    ASSERT_EQ(account.codes.size(), 1u);
    EXPECT_EQ(account.codes[0], 1100);
}

TEST(ErrorRoutingTest, FuturesFailWhenNotConnected)
{
    // the test constructor leaves ibClient null, as if connect() was never called
    RoutingConnector conn;
    auto details = conn.GetContractDetails(Contract());
    ASSERT_TRUE(IsReady(details));
    try
    {
        details.get();
        FAIL() << "expected IbException";
    }
    catch(const ib_helper::IbException& ex)
    {
        EXPECT_EQ(ex.error().code, 504);
        EXPECT_EQ(ex.error().severity, IbSeverity::Connectivity);
    }

    auto options = conn.GetOptionParameters(Contract());
    ASSERT_TRUE(IsReady(options));
    EXPECT_THROW(options.get(), ib_helper::IbException);
}
