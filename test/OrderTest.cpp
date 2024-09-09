#include <gtest/gtest.h>
#include "../src/ib_helper/IBConnector.hpp"
#include "../src/ib_helper/ContractBuilder.hpp"
#include "../src/ib_helper/Order.hpp"
#include "test_helpers.h"
#include "../src/util/DecimalHelper.hpp"

TEST(OrderTests, decimals)
{
    Decimal initialized; 
    double initializedDouble = 0.0;
    // nan
    {
        Decimal nan = 0x7800000000000000ull | 0x7c00000000000000ull;
        double dNan = std::numeric_limits<double>::quiet_NaN();
        EXPECT_FALSE( isNaN(initialized) );
        EXPECT_FALSE( isNaN(doubleToDecimal(initializedDouble)) );
        EXPECT_TRUE( isNaN(nan));
        EXPECT_NE(decimalToDouble(nan), 0.0);
        EXPECT_EQ(decimalToDouble(checkNaN(nan)), 0.0);
        EXPECT_TRUE( isNaN( doubleToDecimal(dNan) ) );
    }
    // inf
    {
        Decimal inf = 0x7800000000000000ull;
        double dInf = std::numeric_limits<double>::infinity();
        EXPECT_TRUE( isInf(inf));
        EXPECT_FALSE( isNaN(inf) );
        EXPECT_NE(decimalToDouble(inf), 0.0);
        EXPECT_EQ(decimalToDouble(checkInf(inf)), 0.0);
        EXPECT_TRUE( isInf( doubleToDecimal(dInf) ) );
    }
    // adding
    {
        Decimal a = doubleToDecimal(2.01);
        Decimal b = doubleToDecimal(1.03);
        Decimal c = add(a, b); // a + b does not work must use a function
        EXPECT_EQ(decimalToDouble(c), 3.04);
    }
}

TEST(OrderTests, DISABLED_OnNewOrder)
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
    ContractDetails t = contractBuilder.BuildStock("T");
    ib_helper::Order o;
    o.account = connector.GetDefaultAccount();
    o.action = "BUY";
    o.totalQuantity = doubleToDecimal(100);
    o.orderType = "LMT";
    o.lmtPrice = 9.0;
    
    auto orderId = connector.GetNextOrderId();
    connector.PlaceOrder(orderId, t.contract, o);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    EXPECT_EQ(myOrderHandler.onOpenOrderCalls, 1);
    EXPECT_EQ(myOrderHandler.lastOrderState.status, "Pending");

    // cancel the order
    connector.CancelOrder(orderId, "");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}
