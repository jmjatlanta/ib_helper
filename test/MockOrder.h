#pragma once
#include "Order.h"
#include "Contract.h"

class MockOrder : public Order
{
    public:
    MockOrder() : Order() {}
    MockOrder(const Order& in) : Order(in)
    {
    }
    void update(const Order& in)
    {
        totalQuantity = in.totalQuantity;
        auxPrice = in.auxPrice;
        lmtPrice = in.lmtPrice;
    }
    public:
    Contract contract;
    std::string status;
    bool submitted = false;
};
