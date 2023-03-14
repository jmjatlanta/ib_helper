#pragma once
#include "../src/ib_helper/IBConnector.hpp"

class MockIBConnector : public ib_helper::IBConnector
{
    public:
    MockIBConnector(const std::string& hostname, int port, int clientId);
    ~MockIBConnector();
    uint32_t GetNextRequestId();
    std::future<ContractDetails> GetContractDetails(const Contract& contract);
    bool IsConnected() const override;
    void RequestPositions() override;
    void RequestOpenOrders() override;
    void AddConnectionMonitor(ib_helper::IBConnectionMonitor* in) override;
    uint32_t SubscribeToHistoricalData(const Contract& contract, ib_helper::HistoricalDataHandler* handler,
            const std::string& timePeriod, const std::string& barSize) override;
    void UnsubscribeFromHistoricalData(uint32_t subId) override;
    uint32_t SubscribeToTickByTick(const Contract& contract, ib_helper::TickHandler* handler, 
        const std::string& tickType, int numberOfTicks, bool ignoreSize) override;
    void UnsubscribeFromTickByTick(uint32_t subId) override;
    void RequestAccountUpdates(bool subscribe, const std::string& account) override;
    // bars
    /***
     * Send a historical bar as if it came from IB
     */
    void SendBar(int subId, const Bar& in, bool inPast = false);
    void SendTick(int subId, double lastPrice);
    void SendBidAsk(uint32_t subscriptionId, double bid, double ask);
    // orders
    void ProcessOrdersImmediately(bool yn) { processOrdersImmediately = yn; }
    void SetOrderRejectReason(uint32_t code) { orderRejectCode = code; }
    void PlaceOrder(int orderId, const Contract& contract, const ::Order& order) override;
    void ProcessLastOrder();

    private:
    std::atomic<uint32_t> nextRequestId;
    std::atomic<uint32_t> nextOrderId;
    std::unordered_map<uint32_t, std::promise<ContractDetails>> contractDetailsHandlers;
    // orders
    bool processOrdersImmediately = true;
    uint32_t orderRejectCode = 0;
    uint32_t currentOrderId = 0;
    Contract currentContract;
    Order currentOrder;
};
