#include "ContractBuilder.hpp"
#include "../util/StringHelper.hpp"
#include "BarSize.hpp"
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

typedef std::chrono::system_clock::time_point time_pnt;

time_pnt get_trading_open(const std::string& tradingHours)
{
    // can be in 2 formats
    // 20090507:0700-1830,1830-2330;20090508:CLOSED
    // 20180323:0400-20180323:2000;20180326:0400-20180326:2000
    auto pos = tradingHours.find("-");
    if (pos != std::string::npos)
    {
        std::string open = tradingHours.substr(0, pos); // up to the dash
        pos = open.find(":");
        if (pos != std::string::npos)
        {
            std::string date = open.substr(0, pos);
            std::string time = open.substr(pos + 1);
            if (date.size() == 8 && time.size() == 4)
            {
                std::string completeString = date.substr(0, 4) + "-" + date.substr(4, 2) + "-" + date.substr(6)
                        + " " + time.substr(0,2) + ":" + time.substr(2) + ":00";
                std::tm t{};
                std::istringstream ss{completeString};
                ss >> std::get_time(&t, "%Y-%m-%d %H:%M:%S");
                if (!ss.fail())
                {
                    std::time_t tt = mktime(&t);
                    if (tt != -1)
                    {
                        return std::chrono::system_clock::from_time_t(tt);
                    }
                }
            }
        }
    }
    return std::chrono::system_clock::from_time_t(0);
}
time_pnt get_trading_close(const std::string& tradingHours)
{
    // can be in 2 formats
    // 20090507:0700-1830,1830-2330;20090508:CLOSED
    // 20180323:0400-20180323:2000;20180326:0400-20180326:2000
        auto pos = tradingHours.find(":");
        if (pos != std::string::npos)
        {
            std::string date = tradingHours.substr(0, pos);
            // where to get the time depends on what comes after the dash
            pos = tradingHours.find("-");
            if (pos != std::string::npos) // we found the dash
            {
                std::string time = tradingHours.substr(pos+1, 4);
                if (tradingHours.substr(pos+5, 1) != ",") // type B
                    time = tradingHours.substr(pos + 10, 4);
                if (date.size() == 8 && time.size() == 4)
                {
                    std::string completeString = date.substr(0, 4) + "-" + date.substr(4, 2) + "-" + date.substr(6)
                            + " " + time.substr(0,2) + ":" + time.substr(2) + ":00";
                    std::tm t{};
                    std::istringstream ss{completeString};
                    ss >> std::get_time(&t, "%Y-%m-%d %H:%M:%S");
                    if (!ss.fail())
                    {
                        std::time_t tt = mktime(&t);
                        if (tt != -1)
                        {
                            return std::chrono::system_clock::from_time_t(tt);
                        }
                    }
                }
            }
        }
    return std::chrono::system_clock::from_time_t(0);
}

std::vector<std::string> tokenize(const std::string& in, const std::string& delim)
{
    std::vector<std::string> retval;
    std::string whatsLeft = in;
    auto pos = whatsLeft.find(delim);
    while (pos != std::string::npos)
    {
        retval.push_back(whatsLeft.substr(0, pos));
        if (whatsLeft.size() <= pos+delim.size())
        {
            whatsLeft = "";
            break;
        }
        whatsLeft = whatsLeft.substr(pos+delim.size());

    }
    // do we have any leftovers?
    if (!whatsLeft.empty())
        retval.push_back(whatsLeft);
    return retval;
}

/****
 * Get the number of candles per day
 * @param contractDetails the contract
 * @param barSize the bar size
 * @returns the number of candles for this contract per day (longest day)
*/
uint32_t candles_per_day(const ContractDetails& contractDetails, ib_helper::BarSize barSize)
{
    // calculate the time span
    std::string tradingHours = contractDetails.tradingHours;
    if (tradingHours.empty())
        tradingHours = "20240101:0400-20240101:2000";
    std::vector<std::string> hours = tokenize(tradingHours, ";");
    uint32_t maxBars = 0;
    for(int i = 0; i < hours.size(); ++i)
    {
        time_pnt start = get_trading_open(hours[0]);
        time_pnt end = get_trading_close(hours[0]);
        auto tradable_time = duration_cast<std::chrono::seconds>(end - start);
        // calculate how many bars fit in that time span
        auto bar_seconds = to_duration(barSize);
        auto num_bars = tradable_time / bar_seconds;
        if (tradable_time % bar_seconds != std::chrono::seconds(0))
            num_bars++;
        // add 1 if there are remainders
        if (num_bars > maxBars)
             maxBars = num_bars;
    }
    // never let it be zero
    if (maxBars == 0)
        maxBars = 1;
    return maxBars;
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
    /*
     * NOTE: this used to calculate the best future based on the rollover calendar.
     * This has been moved to a ContractCalendar library. Now this method only computes
     * a valid future, regardless of if it has rolled over
     */
    Contract contract;
    contract.symbol = to_upper(ticker);
    contract.localSymbol = "";
    contract.secType = "FUT";
    contract.currency = "USD";
    if (ticker == "ES"  || ticker == "RTY")
    {
        contract.exchange = "CME";
    }
    // make sure it is liquid
    auto vec = GetDetails(contract);
    if (vec.size() > 0)
        return vec[0];
    return ContractDetails{};
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
