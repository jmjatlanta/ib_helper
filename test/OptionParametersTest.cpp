#include "../src/ib_helper/IBConnector.hpp"
#include <gtest/gtest.h>
#include <chrono>
#include <deque>
#include <mutex>
#include <optional>
#include <future>
#include <thread>
#include <vector>

/***
 * Tests for the GetOptionParameters pending-request maps.
 * None of these need a running TWS: the tests play TWS by calling the callbacks.
 */

namespace {

using ib_helper::SecurityDefinitionOptionParameter;
using OptionFuture = std::future<std::vector<SecurityDefinitionOptionParameter>>;

class OptionConnector : public ib_helper::IBConnector
{
    public:
    OptionConnector() : ib_helper::IBConnector() {}
    using ib_helper::IBConnector::RegisterOptionParametersRequest;
    using ib_helper::IBConnector::securityDefinitionOptionalParameter;
    using ib_helper::IBConnector::securityDefinitionOptionalParameterEnd;
    size_t PendingRequests()
    {
        std::scoped_lock lock(securityDefinitionMutex);
        EXPECT_EQ(securityDefinitionHandlers.size(), securityDefinitionData.size());
        return securityDefinitionHandlers.size();
    }
};

void SendResult(OptionConnector& conn, int reqId, const std::string& exchange)
{
    conn.securityDefinitionOptionalParameter(reqId, exchange, 265598, "AAPL", "100",
            {"20261218", "20270115"}, {100.0, 105.0});
}

bool IsReady(OptionFuture& f) { return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; }

} // namespace

TEST(OptionParametersTest, ResultsArriveInFuture)
{
    OptionConnector conn;
    int reqId = conn.GetNextRequestId();
    auto fut = conn.RegisterOptionParametersRequest(reqId);
    EXPECT_EQ(conn.PendingRequests(), 1u);

    SendResult(conn, reqId, "SMART");
    SendResult(conn, reqId, "CBOE");
    EXPECT_FALSE(IsReady(fut));
    conn.securityDefinitionOptionalParameterEnd(reqId);

    ASSERT_TRUE(IsReady(fut));
    auto results = fut.get();
    ASSERT_EQ(results.size(), 2u);
    EXPECT_EQ(results[0].reqId, reqId);
    EXPECT_EQ(results[0].exchange, "SMART");
    EXPECT_EQ(results[1].exchange, "CBOE");
    EXPECT_EQ(results[1].expirations.size(), 2u);
    EXPECT_EQ(results[1].strikes.size(), 2u);
    EXPECT_EQ(conn.PendingRequests(), 0u);
}

TEST(OptionParametersTest, EndWithNoResultsGivesEmptyVector)
{
    OptionConnector conn;
    int reqId = conn.GetNextRequestId();
    auto fut = conn.RegisterOptionParametersRequest(reqId);
    conn.securityDefinitionOptionalParameterEnd(reqId);
    ASSERT_TRUE(IsReady(fut));
    EXPECT_TRUE(fut.get().empty());
    EXPECT_EQ(conn.PendingRequests(), 0u);
}

TEST(OptionParametersTest, UnknownIdsAreIgnored)
{
    OptionConnector conn;
    int reqId = conn.GetNextRequestId();
    auto fut = conn.RegisterOptionParametersRequest(reqId);

    int unknown = conn.GetNextRequestId();
    SendResult(conn, unknown, "SMART");
    conn.securityDefinitionOptionalParameterEnd(unknown);
    EXPECT_FALSE(IsReady(fut));
    EXPECT_EQ(conn.PendingRequests(), 1u);

    // a second End for a completed request is also ignored
    conn.securityDefinitionOptionalParameterEnd(reqId);
    conn.securityDefinitionOptionalParameterEnd(reqId);
    ASSERT_TRUE(IsReady(fut));
    EXPECT_TRUE(fut.get().empty());
}

TEST(OptionParametersTest, ConcurrentRequestsAndReplies)
{
    // Callers register on their own threads while the "reader thread" answers.
    // Run under ThreadSanitizer to check the locking; without it this checks the results.
    OptionConnector conn;
    constexpr int callers = 4;
    constexpr int perCaller = 250;
    std::vector<std::vector<std::pair<int, OptionFuture>>> futures(callers);
    // registered ids, handed to the reader
    std::mutex idsMutex;
    std::deque<int> ids;

    std::vector<std::thread> threads;
    for(int c = 0; c < callers; ++c)
    {
        threads.emplace_back([&, c]()
        {
            for(int i = 0; i < perCaller; ++i)
            {
                int reqId = conn.GetNextRequestId();
                futures[c].emplace_back(reqId, conn.RegisterOptionParametersRequest(reqId));
                std::scoped_lock lock(idsMutex);
                ids.push_back(reqId);
            }
        });
    }
    // reader: answer each request once it is registered
    std::thread reader([&]()
    {
        int answered = 0;
        while(answered < callers * perCaller)
        {
            std::optional<int> reqId;
            {
                std::scoped_lock lock(idsMutex);
                if (!ids.empty())
                {
                    reqId = ids.front();
                    ids.pop_front();
                }
            }
            if (!reqId)
            {
                std::this_thread::yield();
                continue;
            }
            SendResult(conn, *reqId, "SMART");
            conn.securityDefinitionOptionalParameterEnd(*reqId);
            ++answered;
        }
    });
    for(auto& t : threads)
        t.join();
    reader.join();

    for(auto& perThread : futures)
    {
        for(auto& [reqId, fut] : perThread)
        {
            ASSERT_EQ(fut.wait_for(std::chrono::seconds(5)), std::future_status::ready) << "id " << reqId;
            auto results = fut.get();
            ASSERT_EQ(results.size(), 1u);
            EXPECT_EQ(results[0].reqId, reqId);
        }
    }
    EXPECT_EQ(conn.PendingRequests(), 0u);
}
