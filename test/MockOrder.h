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
    public:
    Contract contract;
    std::string status;
};
