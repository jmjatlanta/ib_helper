#pragma once
#include "../ib_api/client/Order.h"
#include "OrderStatus.hpp"

namespace ib_helper
{

/***
 * extends ib's order to include extra data
 */
class Order : public ::Order
{
    public:
    Contract contract;
    OrderStatus status = OrderStatus::UNKNOWN;
};

} // namespace ib_helper
