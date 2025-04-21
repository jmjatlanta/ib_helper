#include <gtest/gtest.h>
#include "IBConnector.hpp"
#include "ContractBuilder.hpp"
#include "DateUtil.hpp"
struct ib_host
{
    std::string url = "localhost";
    int port = 7496;
};

static ib_host ibhost;


class MyHistoricalDataHandler : public ib_helper::HistoricalDataHandler
{
public:
    virtual void OnHistoricalData(int reqId, const Bar& bar) override 
    {
        bars.push_back(bar);
    }
    virtual void OnHistoricalDataEnd(int reqId, const std::string& start, const std::string& end) override 
    {
        endCalled = true;
    }
    virtual void OnHistoricalDataUpdate(int reqId, const Bar& bar) override {}
    std::vector<Bar> bars;
    bool endCalled = false;
};

std::string to_string(Bar in)
{
    return to_string(to_time_point(in)) + " " + std::to_string(decimalToDouble(in.volume));
}
TEST(HistoricalTest, getBars)
{
    ib_helper::IBConnector connector{ibhost.url, ibhost.port, 1};
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    // get bars
    ib_helper::ContractBuilder cb(&connector);
    auto msft = cb.BuildStock("MSFT");
    MyHistoricalDataHandler handler;
    std::string endDateTime;
    std::string timePeriod = "3 D";
    std::string barSize = "5 mins";
    bool rth = false;
    auto connId = connector.SubscribeToHistoricalData(msft.contract, &handler, endDateTime, timePeriod, barSize, rth);
    while(!handler.endCalled)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    for(auto bar : handler.bars)
    {
        std::cout << to_string(bar) << "\n";
    }
    // does it include premarket with zero volume?
    /*
    virtual uint32_t SubscribeToHistoricalData(const Contract& contract, HistoricalDataHandler* handler,
            const std::string& endDateTime, const std::string& timePeriod, const std::string& barSize,
            bool rth = true);
    */
}
