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
    ~ContractBuilder() = default;

    Contract Build(const std::string& secType, const std::string& ticker);
    Contract Build(SecurityType::Type secType, const std::string& ticker);

    virtual ContractDetails BuildStock(const std::string& ticker);
    
    ContractDetails BuildFuture(const std::string& ticker, time_t now = time(nullptr));

    ContractDetails BuildForex(const std::string& ticker);

    Contract BuildOption(const std::string& ticker);

    virtual ContractDetails GetStockDetails(const std::string& in);
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

std::string to_string(const Contract &in);

} // namespace ib_helper

