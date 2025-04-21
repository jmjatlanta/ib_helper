#pragma once
#include "IBConnector.hpp"
#include "Contract.h"
#include "../util/SysLogger.h"
#include "SecurityType.hpp"
#include "BarSize.hpp"

namespace ib_helper {

typedef std::chrono::system_clock::time_point time_pnt;

time_pnt get_trading_open(const std::string& tradingHours);
time_pnt get_trading_close(const std::string& tradingHours);
std::vector<std::string> tokenize(const std::string& in, const std::string& delim);

/****
 * Get the number of candles per day
 * @param contractDetails the contract
 * @param barSize the bar size
 * @returns the number of candles for this contract per day (longest day)
*/
uint32_t candles_per_day(const ContractDetails& contractDetails, ib_helper::BarSize barSize);
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

