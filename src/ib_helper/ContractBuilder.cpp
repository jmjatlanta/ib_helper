#include "ContractBuilder.hpp"
#include "ContractRolloverCalendar.hpp"

#include <iomanip>
#include <iostream>

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
    auto det = GetStockDetails(ticker);
    return det.contract;
}

Contract ContractBuilder::BuildOption(const std::string &ticker)
{
    Contract retval;
    retval.symbol = ticker;
    retval.localSymbol = ticker;
    retval.secType = "OPT";
    retval.exchange = "SMART";
    retval.currency = "USD";
    // get contract id
    auto det = GetDetails(retval);
    retval.conId = det.contract.conId;
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
    int month = 60 * 60 * 24 * 28;
    time_t temp_date = now;
    while (det.contract.conId <= 0 && temp_date < now + (month * 4))
    {
        // perhaps the contract is expired
        temp_date += month;
        retval.localSymbol = "";
        retval.lastTradeDateOrContractMonth = calendar.CurrentMonthYYYYMM(ticker, temp_date);
        logger->debug("ContractBuilder", "BuildFuture: upping the contract month for " + ticker + " to " + retval.lastTradeDateOrContractMonth);
        det = GetDetails(retval);
    }
    if (det.contract.conId <= 0)
    {
        logger->debug("ContractBuilder", "BuildFuture: unable to get contract for " + ticker);
        return retval; // we were unsuccessful
    }
    time_t expiry = to_time_t(det.contract.lastTradeDateOrContractMonth);
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
        det = GetDetails(retval);
        expiry = to_time_t(det.contract.lastTradeDateOrContractMonth);
        if (expiry == old_expiry)
        {
            ++multiplier;
        }
    }
    retval.conId = det.contract.conId;
    auto coll = parseCSV(det.validExchanges);
    if (coll.size() > 0)
        retval.exchange = coll[0];
    retval.localSymbol = det.contract.localSymbol;
    retval.lastTradeDateOrContractMonth = det.contract.lastTradeDateOrContractMonth;
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
    return GetDetails(retval);
}

ContractDetails ContractBuilder::GetDetails(const Contract &in)
{
    if (ib == nullptr || !ib->IsConnected())
        return ContractDetails{};

    auto fut = ib->GetContractDetails(in);
    try
    {
        auto result = fut.wait_for(std::chrono::seconds(3));
        if (result == std::future_status::timeout)
        {
            logger->error("ContractBuilder", "GetDetails: timeout retrieving contract " + in.symbol);
            return ContractDetails{};
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
        return ContractDetails{};
    }
}

} // namespace ib_helper
