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
    ContractBuilder(IBConnector* conn, bool cacheResults = true);

    Contract Build(const std::string& secType, const std::string& ticker);
    Contract Build(SecurityType::Type secType, const std::string& ticker);

    Contract BuildStock(const std::string& ticker);
    
    Contract BuildFuture(const std::string& ticker, time_t now = time(nullptr));

    Contract BuildForex(const std::string& ticker);

    Contract BuildOption(const std::string& ticker);

    ContractDetails GetStockDetails(const std::string& in);
    std::vector<ContractDetails> GetDetails(const Contract& in);

    std::vector<ib_helper::SecurityDefinitionOptionParameter> GetOptionParameters(const Contract& opt);

    protected:
    /***
     * @param type STK, FUT, etc.
     * @param ticker the ticker symbol
     * @return the matching ContractDetails object (check contract.conId for validity)
    */
    ContractDetails checkCache(SecurityType::Type type, const std::string& ticker);
    /***
     * @brief add a ContractDetails object to the cache
     * @param type STK, FUT, etc.
     * @param contractDetails the contract to add
    */
    void addToCache(SecurityType::Type type, const ContractDetails& contractDetails);

    private:
    bool cacheEnabled = true;
    IBConnector* ib = nullptr;
    util::SysLogger* logger = nullptr;
    std::unordered_map<SecurityType::Type, std::unordered_map<std::string, ContractDetails>> contractDetailsCaches;
};

} // namespace ib_helper

