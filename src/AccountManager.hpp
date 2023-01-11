#pragma once
#include "ib_helper/IBConnector.hpp"
#include "ib_helper/AccountHandler.hpp"
#include "ib_helper/OrderHandler.hpp"
#include "ib_helper/Position.hpp"
#include "ib_helper/Order.hpp"
#include <string>
#include <vector>

class AccountManager : public ib_helper::AccountHandler, public ib_helper::OrderHandler
{

    public:
    AccountManager(ib_helper::IBConnector* conn, const std::string& mainAccount);
    ~AccountManager();

    /***
     * Helper methods
     */

    /****
     * @brief look for order with specific id
     * @param id the id
     * @param currentOnly TRUE to look for current orders only
     * @returns the order
     * @throws std::out_of_range if order not found
     */
    ib_helper::Order& GetOrder(int id, bool currentOnly = false);

    /****
     * Interface implementations
     */

    /****
     * AccountHandler
     */

    virtual void OnAccountValueUpdate(const std::string& key, const std::string& value, const std::string& currency,
            const std::string& accountName);
    virtual void OnPortfolioUpdate(Contract contract, Decimal position, double marketPrice, double marketValue,
            double averageCost, double unrealizedPNL, double realizedPNL, const std::string& accountName);
    virtual void OnUpdateAccountTime(const std::string& timestamp);
    virtual void OnAccountDownloadEnd(const std::string& accountName);
    /***
     * @brief IB is notifying us of a position (either we just logged in or is new)
     * @param account the account
     * @param contract the contract
     * @param pos the position size
     * @param averageCost the average cost of the position
     */
    virtual void OnPosition(const std::string& account, Contract contract, Decimal pos, double averageCost);
    virtual void OnPositionEnd();
    virtual void OnCurrentTime(long currentTime);
    virtual void OnError(int id, int errorCode, const std::string& errorMessage, 
            const std::string& advancedOrderRejectJson);

    /****
     * OrderHandler
     */

    /***
     * @brief a new order has been processed by IB
     * @param orderId the order id
     * @param contract the contract
     * @param order the order
     * @param OrderState the order state
     */
    virtual void OnOpenOrder(int orderId, Contract contract, Order order, OrderState orderState);
    virtual void OnOrderStatus(int orderId, const std::string& status, Decimal filled, Decimal remaining,
            double avgFillPrice, int permId, int parentId, double lastFillPrice, int clientId,
            const std::string& whyHeld, double mktCapPrice);
    virtual void OnOpenOrderEnd();
    virtual void OnOrderBound(long orderId, int apiClientId, int apiOrderId);

    private:
    ib_helper::IBConnector* ib = nullptr;
    std::string mainAccount = nullptr;
    std::vector<Position> positions;
    std::unordered_map<int, ib_helper::Order> orders;
    std::unordered_map<int, ib_helper::Order> past_orders;
};
