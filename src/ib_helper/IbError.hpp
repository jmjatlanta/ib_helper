#pragma once
#include <ctime>
#include <stdexcept>
#include <string>
#include <string_view>

namespace ib_helper
{

/***
 * How serious a message from TWS's error() callback is.
 * Many of these "errors" are warnings or confirmations.
 * The order matters: everything at RequestFailed or above is a real error.
 */
enum class IbSeverity
{
    Info,          // confirmation or status, e.g. 2104 "market data farm connection is OK", 202 order cancelled
    Warning,       // something to know about, but the request still works, e.g. 2109, 399, 10167
    Connectivity,  // the link to TWS or to IB changed state, e.g. 1100, 1102, 502, 509
    RequestFailed, // a request (market data, historical, contract details...) failed
    OrderRejected, // an order was rejected, or a modify/cancel of it failed
    System         // TWS/API level problem not tied to a request, e.g. 503 "TWS is out of date"
};

std::string_view to_string(IbSeverity in);

/***
 * Classify a TWS error code.
 * The message is needed for a few codes whose meaning depends on it (e.g. 162).
 * Codes not in the table are treated as RequestFailed.
 */
IbSeverity classify(int code, std::string_view message);

/***
 * One message from TWS's error() callback
 */
struct IbError
{
    int id = -1;
    int code = 0;
    std::string message;
    std::string advancedRejectJson;
    time_t time = 0;
    IbSeverity severity = IbSeverity::RequestFailed;

    bool isError() const { return severity >= IbSeverity::RequestFailed; }
};

/***
 * Set on a future (e.g. GetOptionParameters) when TWS reports that the request failed
 */
class IbException : public std::runtime_error
{
    public:
    explicit IbException(IbError in);
    const IbError& error() const { return err; }
    private:
    IbError err;
};

} // namespace ib_helper
