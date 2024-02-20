#include "ContractBuilder.hpp"
#include "ContractRolloverCalendar.hpp"

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

ContractBuilder::ContractBuilder(IBConnector* conn) : ib(conn), logger(util::SysLogger::getInstance()) {}

Contract ContractBuilder::Build(const std::string& secType, const std::string& ticker)
{
    return Build( SecurityType::to_type(secType), ticker);
}

Contract ContractBuilder::Build(SecurityType::Type secType, const std::string& ticker)
{
    if (secType == SecurityType::Type::FUT)
        return BuildFuture(ticker);
    if (secType == SecurityType::Type::STK)
        return BuildStock(ticker);
    if (secType == SecurityType::Type::FOREX)
        return BuildForex(ticker);
    if (secType == SecurityType::Type::OPT)
        return BuildOption(ticker);
    return Contract{};
}

Contract ContractBuilder::BuildForex(const std::string& ticker)
{
    int pos = ticker.find(".");
	Contract c;
    if (pos != std::string::npos)
    {
        c.symbol = ticker.substr(0, pos);
        c.secType = "CASH";
        c.currency = ticker.substr(pos+1);
        c.exchange = "IDEALPRO";
    }
	return c;	
}

Contract ContractBuilder::BuildStock(const std::string& ticker)
{
    Contract retval;
    retval.symbol = ticker;
    retval.localSymbol = ticker;
    retval.secType = "STK";
    retval.exchange = "SMART";
    retval.currency = "USD";
    // get contract id
    auto det = GetDetails(retval);
    if (det.size() > 0)
        retval.conId = det[0].contract.conId;
    return retval;
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

inline std::string& ltrim(std::string& in)
{
    in.erase(in.begin(), std::find_if(in.begin(), in.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    return in;
}
inline std::string& rtrim(std::string& in)
{
    in.erase(std::find_if(in.rbegin(), in.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), in.end());
    return in;
}

inline std::string trim(const std::string& in)
{
    std::string retval = in;
    return rtrim(ltrim(retval));
}

OptionDetails getDetails(const std::string& in)
{
    OptionDetails retval;
    size_t pos = in.find(" ");
    if (pos == std::string::npos)
    {
        retval.underlying = in;
        return retval;
    }
    retval.underlying = in.substr(0, pos);
    std::string remainder = trim(in.substr(pos));
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

Contract ContractBuilder::BuildFuture(const std::string &ticker, time_t now)
{
    Contract retval;
    retval.symbol = ticker;
    retval.localSymbol = "";
    retval.secType = "FUT";
    retval.currency = "USD";
    if (ticker == "ES")
    {
        retval.exchange = "CME";
    }
    ContractRolloverCalendar calendar;
    if (!calendar.IsValid(ticker))
        return retval;
    retval.lastTradeDateOrContractMonth = calendar.CurrentMonthYYYYMM(ticker);
    // make sure it is liquid
    auto det = GetDetails(retval);
    ContractDetails currDetails;
    if (det.size() > 0)
        currDetails = det[0];
    int month = 60 * 60 * 24 * 28;
    time_t temp_date = now;
    while (currDetails.contract.conId <= 0 && temp_date < now + ( month * 4) )
    {
        // perhaps the contract is expired
        temp_date += month;
        retval.localSymbol = "";
        retval.lastTradeDateOrContractMonth = calendar.CurrentMonthYYYYMM(ticker, temp_date);
        //logger->debug("ContractBuilder", "BuildFuture: upping the contract month for " + ticker + " to " + retval.lastTradeDateOrContractMonth);
        det = GetDetails(retval);
        if (det.size() > 0)
            currDetails = det[0];
    }
    if (currDetails.contract.conId <= 0)
    {
        logger->debug("ContractBuilder", "BuildFuture: unable to get contract for " + ticker);
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
        retval.lastTradeDateOrContractMonth = calendar.CurrentMonthYYYYMM(ticker, expiry);
        retval.localSymbol = "";
        currDetails = ContractDetails{};
        det = GetDetails(retval);
        if (det.size() > 0)
            currDetails = det[0];
        expiry = to_time_t(currDetails.contract.lastTradeDateOrContractMonth);
        if (expiry == old_expiry)
        {
            ++multiplier;
        }
    }
    retval.conId = currDetails.contract.conId;
    auto coll = parseCSV(currDetails.validExchanges);
    if (coll.size() > 0)
        retval.exchange = coll[0];
    retval.localSymbol = currDetails.contract.localSymbol;
    retval.lastTradeDateOrContractMonth = currDetails.contract.lastTradeDateOrContractMonth;
    return retval;
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
    retval.symbol = ticker;
    retval.localSymbol = ticker;
    retval.secType = "STK";
    retval.exchange = "SMART";
    retval.currency = "USD";
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
    if (underlying.conId <= 0)
        throw std::invalid_argument("option must have conId to get security definition parameters");
    auto fut = ib->GetOptionParameters(underlying);
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
