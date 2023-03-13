#include "MockIBConnector.h"
#include "HistoricalBarFeeder.h"
#include <gtest/gtest.h>

class MockHistoricalDataHandler : public ib_helper::HistoricalDataHandler
{
    public:
    virtual void OnHistoricalData(int reqId, const Bar& bar) { ++count; }
    virtual void OnHistoricalDataEnd(int reqId, const std::string& start, const std::string& end) { ++count; }
    virtual void OnHistoricalDataUpdate(int reqId, const Bar& bar) { ++count; }
    int count = 0;
};

TEST(MockIBConnectorTest, Basics)
{
    {
        HistoricalBarFileWriter writer("temp.csv");
        writer.Write("1", 11.0, 9.0, 10.0, 9.5, doubleToDecimal(0.0), doubleToDecimal(100), 1);
        writer.Write("2", 11.1, 9.1, 9.5, 10.1, doubleToDecimal(0.0), doubleToDecimal(200), 2);
    }
    MockIBConnector conn("localhost", 123, 45);
    MockHistoricalDataHandler handler;
    Contract contract;
    int subscriptionId = conn.SubscribeToHistoricalData(contract, &handler, "2 day", "2 mins");
    HistoricalBarFileReader reader("temp.csv");
    Bar bar = reader.NextBar();
    conn.SendBar(subscriptionId, bar, true);
    EXPECT_EQ(handler.count, 1);
    bar = reader.NextBar();
    conn.SendBar(subscriptionId, bar, false);
    EXPECT_EQ(handler.count, 2);
}
