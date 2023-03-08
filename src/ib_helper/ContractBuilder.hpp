#pragma once
#include "IBConnector.hpp"
#include "../ib_api/client/Contract.h"
#include "ContractRolloverCalendar.hpp"
#include <iostream>

namespace ib_helper {

class ContractBuilder
{
    public:
    ContractBuilder(IBConnector* conn);

    Contract BuildStock(const std::string& ticker);
    
    Contract BuildFuture(const std::string& ticker, time_t now = time(nullptr));

    ContractDetails GetDetails(const Contract& in);

    private:
    IBConnector* ib = nullptr;
};

} // namespace ib_helper

