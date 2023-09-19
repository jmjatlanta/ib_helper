#include <gtest/gtest.h>
#include "../src/ib_helper/IBConnector.hpp"
#include "../src/ib_helper/ContractBuilder.hpp"
#include "../src/ib_helper/Order.hpp"
#include "test_helpers.h"

TEST(OrderTests, OnNewOrder)
{
    // place a new order to examine the OrderStatus
    // It seems it is coming in as PendingCancel which is strange

    class MyOrderHandler : public ib_helper::OrderHandler
    {
        public:
        virtual void OnOpenOrder(int orderId, const Contract& contract, const ::Order& order, const OrderState& orderState)
        {
            std::cout << "OnOpenOrder called with id " << orderId
                    << " and contract " << contract.localSymbol
                    << " and orderState " << orderState.status
                    << std::endl;
            onOpenOrderCalls++;
            lastOrderState = orderState;
        }
        virtual void OnOrderStatus(int orderId, const std::string& status, Decimal filled, Decimal remaining,
                double avgFillPrice, int permId, int parentId, double lastFillPrice, int clientId,
                const std::string& whyHeld, double mktCapPrice)
        {
            std::cout << "OnOrderStatus called with id " << orderId
                    << " and status " << status
                    << std::endl;
        }
        virtual void OnOpenOrderEnd()
        {
            std::cout << "OnOpenOrderEnd called" << std::endl;
        }
        virtual void OnOrderBound(long orderId, int apiClientId, int apiOrderId)
        {
            std::cout << "OnOrderBound called" << std::endl;
        }

        public:
        int onOpenOrderCalls = 0;
        OrderState lastOrderState;
    };
    ib_options ops;
    ib_helper::IBConnector connector{ops.host, ops.port, ops.connId};

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    ASSERT_TRUE(isConnected(connector));
    MyOrderHandler myOrderHandler;
    connector.AddOrderHandler(&myOrderHandler);

    // place an order
    ib_helper::ContractBuilder contractBuilder(&connector);
    Contract t = contractBuilder.BuildStock("T");
    ib_helper::Order o;
    o.account = connector.GetDefaultAccount();
    o.action = "BUY";
    o.totalQuantity = doubleToDecimal(100);
    o.orderType = "LMT";
    o.lmtPrice = 9.0;
    
    auto orderId = connector.GetNextOrderId();
    connector.PlaceOrder(orderId, t, o);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    EXPECT_EQ(myOrderHandler.onOpenOrderCalls, 1);
    EXPECT_EQ(myOrderHandler.lastOrderState.status, "Pending");

    // cancel the order
    connector.CancelOrder(orderId, "");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}
