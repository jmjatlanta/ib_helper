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
        det = GetDetails(retval);
    }
    // get contract id
    retval.conId = det.contract.conId;
    return retval;
}

ContractDetails ContractBuilder::GetDetails(const Contract& in)
{
    auto fut = ib->GetContractDetails(in);
    return fut.get();
}

} // namespace ib_helper


