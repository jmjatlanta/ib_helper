#pragma once
#include "../src/ib_api/client/Order.h"
#include "../src/ib_api/client/Contract.h"

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
