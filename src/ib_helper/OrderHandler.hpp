#pragma once
#include "../ib_api/client/Contract.h"
#include "../ib_api/client/Order.h"
#include "../ib_api/client/OrderState.h"

class OrderHandler
{
    public:
    virtual void OnOpenOrder(int orderId, Contract contract, Order order, OrderState orderState) = 0;
    virtual void OnOrderStatus(int orderId, const std::string& status, Decimal filled, Decimal remaining,
            double avgFillPrice, int permId, int parentId, double lastFillPrice, int clientId,
            const std::string& whyHeld, double mktCapPrice) = 0;
    virtual void OnOpenOrderEnd() = 0;
    virtual void OnOrderBound(long orderId, int apiClientId, int apiOrderId) = 0;
};
