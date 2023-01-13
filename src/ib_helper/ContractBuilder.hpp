#pragma once
#include "IBConnector.hpp"
#include "../ib_api/client/Contract.h"

namespace ib_helper {

class ContractBuilder
{
    public:
    ContractBuilder(IBConnector* conn) : ib(conn) {}
    Contract BuildStock(const std::string& ticker)
    {
        Contract retval;
        retval.symbol = ticker;
        retval.localSymbol = ticker;
        retval.secType = "STK";
        retval.exchange = "SMART";
        retval.currency = "USD";
        // get contract id
        auto fut = ib->GetContractDetails(retval);
        ContractDetails det = fut.get();
        retval.conId = det.contract.conId;
        return retval;
    }
    private:
    IBConnector* ib = nullptr;
};

} // namespace ib_helper

