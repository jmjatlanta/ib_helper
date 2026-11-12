#include "../src/ib_helper/IBConnector.hpp"
#include <gtest/gtest.h>
#include <climits>
#include <vector>

/***
 * Tests that order ids and request ids live in separate ranges, so an id
 * reported through error() can be identified as one or the other.
 * None of these need a running TWS.
 */

namespace {

class IdTestConnector : public ib_helper::IBConnector
{
    public:
    IdTestConnector() : ib_helper::IBConnector() {}
    void SetNextRequestId(int in) { nextRequestId = in; }
    // RequestScannerSubscription needs TWS; register the id as it would
    void RegisterScanner(int reqId) { scannerRequestIds.insert(reqId); }
    // TWS callbacks are protected; expose them so the tests can play TWS
    using ib_helper::IBConnector::nextValidId;
    using ib_helper::IBConnector::error;
};

class RecordingScannerHandler : public ib_helper::ScannerHandler
{
    public:
    void OnScannerParameters(ib_helper::IBConnector*, const std::string&) override {}
    void OnScannerData(ib_helper::IBConnector*, int, int, const ContractDetails&, const std::string&,
            const std::string&, const std::string&, const std::string&) override {}
    void OnScannerDataEnd(ib_helper::IBConnector*, int) override {}
    void OnScannerSubscriptionEnd(ib_helper::IBConnector*, int reqId) override { ended.push_back(reqId); }
    std::vector<int> ended;
};

} // namespace

using ib_helper::IBConnector;

TEST(IdRangeTest, Classification)
{
    EXPECT_FALSE(IBConnector::IsOrderId(-1));
    EXPECT_FALSE(IBConnector::IsRequestId(-1));
    EXPECT_FALSE(IBConnector::IsOrderId(0));
    EXPECT_FALSE(IBConnector::IsRequestId(0));
    EXPECT_TRUE(IBConnector::IsOrderId(1));
    EXPECT_TRUE(IBConnector::IsOrderId(IBConnector::REQUEST_ID_BASE - 1));
    EXPECT_FALSE(IBConnector::IsOrderId(IBConnector::REQUEST_ID_BASE));
    EXPECT_TRUE(IBConnector::IsRequestId(IBConnector::REQUEST_ID_BASE));
    EXPECT_TRUE(IBConnector::IsRequestId(INT_MAX));
    EXPECT_FALSE(IBConnector::IsRequestId(static_cast<long>(INT_MAX) + 1));
}

TEST(IdRangeTest, RequestIdsStartAtBaseAndIncrease)
{
    IdTestConnector conn;
    int first = conn.GetNextRequestId();
    EXPECT_EQ(first, IBConnector::REQUEST_ID_BASE);
    EXPECT_EQ(conn.GetNextRequestId(), first + 1);
}

TEST(IdRangeTest, RequestIdsWrapBeforeOverflow)
{
    IdTestConnector conn;
    conn.SetNextRequestId(INT_MAX - 1);
    EXPECT_EQ(conn.GetNextRequestId(), INT_MAX);
    EXPECT_EQ(conn.GetNextRequestId(), IBConnector::REQUEST_ID_BASE);
}

TEST(IdRangeTest, OrderIdsDoNotCollideWithRequestIds)
{
    IdTestConnector conn;
    conn.nextValidId(1); // a fresh account: TWS starts order ids at 1
    int orderId = conn.GetNextOrderId();
    int requestId = conn.GetNextRequestId();
    EXPECT_TRUE(IBConnector::IsOrderId(orderId));
    EXPECT_FALSE(IBConnector::IsRequestId(orderId));
    EXPECT_TRUE(IBConnector::IsRequestId(requestId));
    EXPECT_NE(orderId, requestId);
}

TEST(IdRangeTest, ScannerEndOnlyForRequestIds)
{
    IdTestConnector conn;
    RecordingScannerHandler scanner;
    conn.AddScannerHandler(&scanner);

    // an error with an order id must not end a scanner subscription
    conn.error(5, 0, 162, "order-related 162", "");
    EXPECT_TRUE(scanner.ended.empty());

    // the same error with a request id does
    int reqId = conn.GetNextRequestId();
    conn.RegisterScanner(reqId);
    conn.error(reqId, 0, 162, "scanner cancelled", "");
    ASSERT_EQ(scanner.ended.size(), 1u);
    EXPECT_EQ(scanner.ended[0], reqId);

    conn.RemoveScannerHandler(&scanner);
}
