#pragma once
#include "../src/ib_helper/IBConnector.hpp"
#include "MockOrder.h"
#include "../src/util/SysLogger.h"

struct MockPosition
{
    std::string account;
    Contract contract;
    Decimal position;
    double avgCost;
};

struct MockOpenOrder
{
    int orderId;
    Contract contract;
    Order order;
    OrderState orderState;
};

class MockIBConnector : public ib_helper::IBConnector
{
    public:
    MockIBConnector(const std::string& hostname, int port, int clientId);
    ~MockIBConnector();
    uint32_t GetNextRequestId() override;
    std::future<ContractDetails> GetContractDetails(const Contract& contract) override;
    bool IsConnected() const override;
    void RequestPositions() override;
    void RequestOpenOrders() override;
    void RequestAllOpenOrders() override;
    uint32_t RequestExecutionReports(const ExecutionFilter& filter) override;
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
    void SendBarWithTick(int barSubId, int tickSubId, int bidAskSubId, const Bar& bar, bool inPast = false);
    void SendTick(int subId, double lastPrice);
    void SendBidAsk(uint32_t subscriptionId, double bid, double ask);
    // orders
    void ProcessOrdersImmediately(bool yn) { processOrdersImmediately = yn; }
    void SetOrderRejectReason(uint32_t code) { orderRejectCode = code; }
    void PlaceOrder(int orderId, const Contract& contract, const ::Order& order) override;
    void CancelOrder(int orderId, const std::string& time) override;
    void SetMaxOrderFillSize(double in) { maxOrderFillSize = in; }
    /***
     * @brief prep for reqOpenOrders to later call openOrder with the passed in information
     * @param orderId the order id
     * @param contract the contract
     * @param order the order
     * @param orderState the order state
     */
    void SendOpenOrder(int orderId, const Contract& contract, const Order& order, const OrderState& orderState);
    // positions
    /***
     * @brief prep for reqPositions to later call position() with passed in information
     * @param account the account
     * @param contract the contract 
     * @param position the position size 
     * @param avgCost the average cost of the position 
     */
    void SendPosition(const std::string& account, const Contract& contract, Decimal position, double avgCost);
    virtual void orderStatus( OrderId orderId, const std::string& status, Decimal filled,
	        Decimal remaining, double avgFillPrice, int permId, int parentId,
	        double lastFillPrice, int clientId, const std::string& whyHeld, double mktCapPrice) override;
    private:
    void processOrder(MockOrder& order, double price);
    void submitOrder(MockOrder& order);
    MockOrder& findOrderById(uint32_t orderId);
    std::atomic<uint32_t> nextRequestId;
    std::atomic<uint32_t> nextOrderId;
    std::unordered_map<uint32_t, std::promise<ContractDetails>> contractDetailsHandlers;
    // orders
    bool processOrdersImmediately = true;
    uint32_t orderRejectCode = 0;
    std::vector<MockOrder> orders;
    const std::string clazz = "MockIBConnector";
    util::SysLogger* logger = nullptr;
    std::vector<MockOpenOrder> openOrders;
    std::vector<MockPosition> positions;
    double maxOrderFillSize = 100000.0; // how many shares can be bought on 1 tick
};
