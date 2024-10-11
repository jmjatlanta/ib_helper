#include "ContractBuilder.hpp"
#include "ContractRolloverCalendar.hpp"
#include "../util/StringHelper.hpp"
#include <iomanip>
#include <iostream>
#include <algorithm>

namespace ib_helper {

static std::time_t to_time_t(const std::string& in, const std::string& format = "%Y%m%d")
{
    std::tm t = {0};
    t.tm_isdst = 0;
    std::istringstream ss(in);
    ss >> std::get_time(&t, format.c_str());
    return mktime(&t);
}

static std::string to_upper(const std::string& in)
{
    std::string retval = in;
    std::transform(retval.begin(), retval.end(), retval.begin(), ::toupper);
    return retval;
}

ContractBuilder::ContractBuilder(IBConnector* conn, bool cacheResults) 
        : ib(conn), cacheEnabled(cacheResults), logger(util::SysLogger::getInstance()) {}

Contract ContractBuilder::Build(const std::string& secType, const std::string& ticker)
{
    return Build( SecurityType::to_type(secType), ticker);
}

Contract ContractBuilder::Build(SecurityType::Type secType, const std::string& ticker)
{
    if (secType == SecurityType::Type::FUT)
        return BuildFuture(ticker).contract;
    if (secType == SecurityType::Type::STK)
        return BuildStock(ticker).contract;
    if (secType == SecurityType::Type::FOREX)
        return BuildForex(ticker).contract;
    if (secType == SecurityType::Type::OPT)
        return BuildOption(ticker);
    return Contract{};
}

ContractDetails ContractBuilder::BuildForex(const std::string& ticker)
{
    int pos = ticker.find(".");
    Contract c;
    if (pos != std::string::npos)
    {
        c.symbol = to_upper(ticker.substr(0, pos));
        c.secType = "CASH";
        c.currency = to_upper(ticker.substr(pos+1));
        c.exchange = "IDEALPRO";
    }
    auto vec = GetDetails(c);
    if (vec.size() > 0)
        return vec[0];
    ContractDetails retval;
    retval.contract = c;
    return retval;   
}

ContractDetails ContractBuilder::checkCache(SecurityType::Type type, const std::string& ticker)
{
    if (cacheEnabled)
    {
        auto& map = contractDetailsCaches[type];
        auto pos = map.find(ticker);
        if (pos != map.end())
            return (*pos).second;
    }
    return ContractDetails{};
}

void ContractBuilder::addToCache(SecurityType::Type type, const ContractDetails& contractDetails)
{
    if (cacheEnabled)
    {
        auto& map = contractDetailsCaches[type];
        auto pos = map.find(contractDetails.contract.symbol);
        if (pos == map.end())
            map[contractDetails.contract.symbol] = contractDetails;
    }
}

ContractDetails ContractBuilder::BuildStock(const std::string& ticker)
{
    ContractDetails det = checkCache(SecurityType::Type::STK, ticker);
    if (det.contract.conId == 0)
    {
        Contract contract;
        contract.symbol = to_upper(ticker);
        contract.localSymbol = to_upper(ticker);
        contract.secType = "STK";
        contract.exchange = "SMART";
        contract.currency = "USD";
        // get contract id
        auto dets = GetDetails(contract);
        if (dets.size() > 0)
        {
            det = dets[0];
            addToCache(SecurityType::Type::STK, det);
        }
        else
            det.contract = contract;
    }
    return det;
}

struct OptionDetails
{
    enum class OptionType
    {
        CALL,
        PUT,
        UNKNOWN
    };

    std::string underlying;
    std::string yyyymmdd;
    double strike = 0.0;
    OptionType type = OptionType::UNKNOWN;
};

OptionDetails getDetails(const std::string& in)
{
    OptionDetails retval;
    size_t pos = in.find(" ");
    if (pos == std::string::npos)
    {
        retval.underlying = to_upper(in);
        return retval;
    }
    retval.underlying = to_upper(in.substr(0, pos));
    std::string remainder = stringhelper::trim(in.substr(pos));
    pos = remainder.find("P");
    if (pos == std::string::npos)
    {
        pos = remainder.find("C");
        if (pos == std::string::npos)
            return retval; // Neither C nor P
        retval.type = OptionDetails::OptionType::CALL;
    }
    else
        retval.type = OptionDetails::OptionType::PUT;
    retval.yyyymmdd = "20" + remainder.substr(0, pos);
    std::string strikeStr = remainder.substr(pos+1);
    uint64_t strikeInt = strtol(strikeStr.c_str(), nullptr, 10);
    retval.strike = ((double)strikeInt) / 1000;
    return retval;
}

Contract ContractBuilder::BuildOption(const std::string &ticker)
{
    Contract retval;
    OptionDetails dets = getDetails(ticker);
    retval.symbol = dets.underlying;
    retval.secType = "OPT";
    retval.exchange = "SMART";
    retval.currency = "USD";
    if (dets.yyyymmdd.size() != 0)
        retval.lastTradeDateOrContractMonth = dets.yyyymmdd;
    retval.strike = dets.strike;
    if (dets.type != OptionDetails::OptionType::UNKNOWN)
        retval.localSymbol = ticker;
    return retval;
}

std::vector<std::string> parseCSV(const std::string &in)
{
    std::vector<std::string> values;
    std::string toProcess = in;
    int pos = toProcess.find(",");
    if (pos == std::string::npos)
        values.push_back(in);
    while (pos != std::string::npos)
    {
        values.push_back(toProcess.substr(0, pos));
        toProcess = toProcess.substr(pos + 1);
        pos = toProcess.find(",");
    }
    return values;
}

ContractDetails ContractBuilder::BuildFuture(const std::string &ticker, time_t now)
{
    Contract contract;
    contract.symbol = to_upper(ticker);
    contract.localSymbol = "";
    contract.secType = "FUT";
    contract.currency = "USD";
    if (ticker == "ES"  || ticker == "RTY")
    {
        contract.exchange = "CME";
    }
    ContractRolloverCalendar calendar;
    if (!calendar.IsValid(ticker))
    {
        ContractDetails retval;
        retval.contract = contract;
        return retval;
    }
   contract.lastTradeDateOrContractMonth = calendar.CurrentMonthYYYYMM(ticker);
    // make sure it is liquid
    auto det = GetDetails(contract);
    ContractDetails currDetails;
    if (det.size() > 0)
        currDetails = det[0];
    int month = 60 * 60 * 24 * 28;
    time_t temp_date = now;
    while (currDetails.contract.conId <= 0 && temp_date < now + ( month * 4) )
    {
        // perhaps the contract is expired
        temp_date += month;
        contract.localSymbol = "";
        contract.lastTradeDateOrContractMonth = calendar.CurrentMonthYYYYMM(ticker, temp_date);
        //logger->debug("ContractBuilder", "BuildFuture: upping the contract month for " + ticker + " to " + contract.lastTradeDateOrContractMonth);
        det = GetDetails(contract);
        if (det.size() > 0)
            currDetails = det[0];
    }
    if (currDetails.contract.conId <= 0)
    {
        logger->debug("ContractBuilder", "BuildFuture: unable to get contract for " + ticker);
        ContractDetails retval;
        retval.contract = contract;
        return retval; // we were unsuccessful
    }
    time_t expiry = to_time_t(currDetails.contract.lastTradeDateOrContractMonth);
    bool firstTry = true;
    int16_t multiplier = 1;
    while (!calendar.IsLiquid(ticker, expiry, now))
    {
        firstTry = false;
        // get next contract
        time_t old_expiry = expiry;
        expiry += month * multiplier;
        contract.lastTradeDateOrContractMonth = calendar.CurrentMonthYYYYMM(ticker, expiry);
        contract.localSymbol = "";
        currDetails = ContractDetails{};
        det = GetDetails(contract);
        if (det.size() > 0)
            currDetails = det[0];
        expiry = to_time_t(currDetails.contract.lastTradeDateOrContractMonth);
        if (expiry == old_expiry)
        {
            ++multiplier;
        }
    }
    // currDetails is now the best we have
    // fix up some details
    auto coll = parseCSV(currDetails.validExchanges);
    if (coll.size() > 0)
        currDetails.contract.exchange = coll[0];
    return currDetails;
}

std::string to_string(const Contract &in)
{
    std::stringstream ss;
    ss << "localSymbol: [" << in.localSymbol
       << "] symbol: [" << in.symbol
       << "] conId " << in.conId
       << " lastTradeDateOrContractMonth: " << in.lastTradeDateOrContractMonth
       << " currency: " << in.currency
       << " secType: " << in.secType
       << " exchange: " << in.exchange;
    return ss.str();
}

ContractDetails ContractBuilder::GetStockDetails(const std::string &ticker)
{
    Contract retval;
    if (ticker.find("@SMART") != std::string::npos)
    {
        retval.conId = strtol(ticker.c_str(), nullptr, 10);
    }
    else
    {
        retval.symbol = to_upper(ticker);
        retval.localSymbol = to_upper(ticker);
        retval.secType = "STK";
        retval.exchange = "SMART";
        retval.currency = "USD";
    }
    auto vec = GetDetails(retval);
    if (vec.size() > 0)
        return vec[0];
    return ContractDetails{};
}

std::vector<SecurityDefinitionOptionParameter> ContractBuilder::GetOptionParameters(const Contract& opt)
{
    if (ib == nullptr || !ib->IsConnected())
        return std::vector<SecurityDefinitionOptionParameter>{};

    int waitSeconds = 60;
    // we need to get the underlying
    auto underlying = BuildStock(opt.symbol);
    if (underlying.contract.conId <= 0)
        throw std::invalid_argument("option must have conId to get security definition parameters");
    auto fut = ib->GetOptionParameters(underlying.contract);
    auto result = fut.wait_for(std::chrono::seconds(waitSeconds));
    if (result == std::future_status::timeout)
    {
        logger->error("ContractBuilder", "GetDetails: timeout retrieving contract " + opt.symbol );
        return std::vector<SecurityDefinitionOptionParameter>{};
    }
    if (result == std::future_status::deferred)
    {
        logger->error("ContractBuilder", "GetDetails: future deferred retrieving contract " + opt.symbol );
    }
    return fut.get();
}

std::vector<ContractDetails> ContractBuilder::GetDetails(const Contract& in)
{
    if (ib == nullptr || !ib->IsConnected())
        return std::vector<ContractDetails>{};

    int waitSeconds = 3;
    if (in.secType == "OPT")
        waitSeconds = 60;
    auto fut = ib->GetContractDetails(in);
    try {
        //logger->debug("ContractBuilder", "GetDetails: waiting for future for " + std::to_string(waitSeconds) + " seconds.");
        auto result = fut.wait_for(std::chrono::seconds(waitSeconds));
        if (result == std::future_status::timeout)
        {
            logger->error("ContractBuilder", "GetDetails: timeout retrieving contract " + in.symbol );
            return std::vector<ContractDetails>{};
        }
        if (result == std::future_status::deferred)
        {
            logger->error("ContractBuilder", "GetDetails: future deferred retrieving contract " + in.symbol);
        }
        return fut.get();
    }
    catch (const std::exception &e)
    {
        logger->error("ContractBuilder", std::string("GetDetails: exception retrieving contract: ") + e.what());
        logger->error("ContractBuilder", "Contract: " + to_string(in));
        return std::vector<ContractDetails>{};
    }
}

} // namespace ib_helper
