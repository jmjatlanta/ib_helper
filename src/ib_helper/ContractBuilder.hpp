#pragma once
#include "IBConnector.hpp"
#include "../ib_api/client/Contract.h"

namespace ib_helper {

class ContractBuilder
{
    public:
    ContractBuilder(IBConnector* conn) {}
    Contract BuildStock(const std::string& ticker)
    {
        Contract retval;
        retval.symbol = ticker;
        retval.localSymbol = ticker;
        retval.secType = "STK";
        retval.exchange = "SMART";
        retval.currency = "USD";
        return retval;
    }
};

} // namespace ib_helper

