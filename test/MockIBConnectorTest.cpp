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

class MockOrderHandler : public ib_helper::OrderHandler
{
    public:
    virtual void OnOpenOrder(int orderId, const Contract& contract, const ::Order& order, 
            const OrderState& orderState) override {}
    virtual void OnOrderStatus(int orderId, const std::string& status, Decimal filled, Decimal remaining,
            double avgFillPrice, int permId, int parentId, double lastFillPrice, int clientId,
            const std::string& whyHeld, double mktCapPrice) override 
    {
        std::cout << "Status: " << status << " Filled: " << decimalToDouble(filled) << std::endl;
        totalFilled += decimalToDouble(filled);
    }
    virtual void OnOpenOrderEnd() override {}
    virtual void OnOrderBound(long orderId, int apiClientId, int apiOrderId) override {}
    double totalFilled = 0.0;
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
    int subscriptionId = conn.SubscribeToHistoricalData(contract, &handler, "", "2 day", "2 mins");
    HistoricalBarFileReader reader("temp.csv");
    Bar bar = reader.NextBar();
    conn.SendBar(subscriptionId, bar, true);
    EXPECT_EQ(handler.count, 1);
    bar = reader.NextBar();
    conn.SendBar(subscriptionId, bar, false);
    EXPECT_EQ(handler.count, 2);
}

TEST(MockIBConnectorTest, PartialFill)
{
    {
        HistoricalBarFileWriter writer("temp.csv");
        writer.Write("1", 11.0, 9.0, 10.0, 9.5, doubleToDecimal(0.0), doubleToDecimal(100), 1);
        writer.Write("2", 11.1, 9.1, 9.5, 10.1, doubleToDecimal(0.0), doubleToDecimal(200), 2);
    }
    HistoricalBarFileReader reader("temp.csv");
    MockOrderHandler orderHandler;
    MockHistoricalDataHandler historicalHandler;
    MockIBConnector conn("localhost", 123, 45);
    conn.AddOrderHandler(&orderHandler);
    conn.SetMaxOrderFillSize(25.0);
    // contract and tick data
    Contract contract;
    int historicalSubId = conn.SubscribeToHistoricalData(contract, &historicalHandler, "", "5 mins", "2 mins");
    int tickSubId = 0;
    int bidAskSubId = 0;
    // place order
    Order order;
    order.orderId = conn.GetNextOrderId();
    order.filledQuantity = doubleToDecimal(0.0);
    order.totalQuantity = doubleToDecimal(100.0);
    order.action = "BUY";
    order.orderType = "LMT";
    order.lmtPrice = 9.0;
    conn.PlaceOrder(order.orderId, contract, order);
    Bar bar = reader.NextBar();
    conn.SendBarWithTick(historicalSubId, tickSubId, bidAskSubId, bar, false);
    // only 25 shares should have filled when the low of the bar was hit
    EXPECT_EQ(25.0, orderHandler.totalFilled);
    bar = reader.NextBar();
    conn.SendBarWithTick(historicalSubId, tickSubId, bidAskSubId, bar, false);
}


