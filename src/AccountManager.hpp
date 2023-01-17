#pragma once
#include "ib_helper/IBConnector.hpp"
#include "ib_helper/AccountHandler.hpp"
#include "ib_helper/OrderHandler.hpp"
#include "ib_helper/Position.hpp"
#include "ib_helper/Order.hpp"
#include "util/SysLogger.h"
#include <string>
#include <vector>

class AccountManager : public ib_helper::AccountHandler, public ib_helper::OrderHandler
{

    public:
    AccountManager(ib_helper::IBConnector* conn, const std::string& mainAccount);
    ~AccountManager();

    void AddOrderHandler(ib_helper::OrderHandler* in) { orderHandlers.push_back(in); }

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
    uint32_t PlaceOrder(ib_helper::Order ord, bool immediate);
    void CancelOrder(ib_helper::Order& ord);

    Position* GetPosition(const Contract& contract, bool createIfNecessary = false);

    /****
     * Interface implementations
     */

    /****
     * AccountHandler
     */

    virtual void OnAccountValueUpdate(const std::string& key, const std::string& value, const std::string& currency,
            const std::string& accountName);
    /***
     * @brief IB is reporting an update to portfolio values
     * @param contract the contract
     * @param position the position size
     * @param marketPrice the market price of the position
     * @param marketValue the market value of the position
     * @param averageCost the average cost of the position
     * @param unrealizedPNL the unrealized gain/loss
     * @param realizedPNL the realized gain/loss
     * @param accountName the account this portfolio belongs to
     */
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
    util::SysLogger* logger;
    ib_helper::IBConnector* ib = nullptr;
    std::string mainAccount = nullptr;
    std::unordered_map<int, Position> positions;
    std::unordered_map<int, ib_helper::Order> orders;
    std::unordered_map<int, ib_helper::Order> past_orders;
    std::vector<ib_helper::OrderHandler*> orderHandlers;
};
