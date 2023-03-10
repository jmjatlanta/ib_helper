#include "ContractBuilder.hpp"
#include <iomanip>

namespace ib_helper {

static std::time_t to_time_t(const std::string& in, const std::string& format = "%Y%b%d")
{
    std::tm t = {0};
    t.tm_isdst = 0;
    std::istringstream ss(in);
    ss >> std::get_time(&t, format.c_str());
    return mktime(&t);
}

ContractBuilder::ContractBuilder(IBConnector* conn) : ib(conn) {}

Contract ContractBuilder::Build(const std::string& secType, const std::string& ticker)
{
    if (secType == "FUT")
        return BuildFuture(ticker);
    if (secType == "STK")
        return BuildStock(ticker);
    if (secType == "CASH")
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
    Contract retval;
    retval.symbol = ticker;
    retval.localSymbol = ticker;
    retval.secType = "STK";
    retval.exchange = "SMART";
    retval.currency = "USD";
    // get contract id
    auto det = GetDetails(retval);
    retval.conId = det.contract.conId;
    return retval;
}

std::vector<std::string> parseCSV(const std::string& in)
{
    std::vector<std::string> values;
    std::string toProcess = in;
    int pos = toProcess.find(",");
    if (pos == std::string::npos)
        values.push_back(in);
    while (pos != std::string::npos)
    {
        values.push_back(toProcess.substr(0, pos));
        toProcess = toProcess.substr(pos+1);
        pos = toProcess.find(",");
    }
    return values;
}

Contract ContractBuilder::BuildFuture(const std::string& ticker, time_t now)
{
    Contract retval;
    retval.symbol = ticker;
    retval.localSymbol = "";
    retval.secType = "FUT";
    retval.currency = "USD";
    ContractRolloverCalendar calendar;
    retval.lastTradeDateOrContractMonth = calendar.CurrentMonthYYYYMM(ticker);
    // make sure it is liquid
    auto det = GetDetails(retval);
    time_t expiry = to_time_t(det.contract.lastTradeDateOrContractMonth);
    while( !calendar.IsLiquid(ticker, expiry, now ) )
    {
        // get next contract
        expiry += 60 * 60 * 24 * 28; // increase by about a month
        retval.lastTradeDateOrContractMonth = calendar.CurrentMonthYYYYMM(ticker, expiry);
    }
    // get contract id
    det = GetDetails(retval);
    retval.conId = det.contract.conId;
    auto coll = parseCSV(det.validExchanges);
    retval.exchange = det.validExchanges;
    return retval;
}

ContractDetails ContractBuilder::GetDetails(const Contract& in)
{
    auto fut = ib->GetContractDetails(in);
    return fut.get();
}

} // namespace ib_helper


