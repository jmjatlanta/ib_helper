#pragma once
#include "IBConnector.hpp"
#include "Contract.h"
#include "../util/SysLogger.h"
#include "ContractRolloverCalendar.hpp"
#include "SecurityType.hpp"
#include <iostream>

namespace ib_helper {

class ContractBuilder
{
    public:
    ContractBuilder(IBConnector* conn);

    Contract Build(const std::string& secType, const std::string& ticker);
    Contract Build(SecurityType::Type secType, const std::string& ticker);

    Contract BuildStock(const std::string& ticker);
    
    Contract BuildFuture(const std::string& ticker, time_t now = time(nullptr));

    Contract BuildForex(const std::string& ticker);

    Contract BuildOption(const std::string& ticker);

    ContractDetails GetStockDetails(const std::string& in);
    std::vector<ContractDetails> GetDetails(const Contract& in);

    std::vector<ib_helper::SecurityDefinitionOptionParameter> GetOptionParameters(const Contract& opt);

    private:
    IBConnector* ib = nullptr;
    util::SysLogger* logger = nullptr;
};

} // namespace ib_helper

