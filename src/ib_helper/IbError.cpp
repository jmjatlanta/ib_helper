#include "IbError.hpp"

namespace ib_helper
{

IbException::IbException(IbError in)
        : std::runtime_error("TWS error " + std::to_string(in.code) + ": " + in.message), err(std::move(in))
{
}

std::string_view to_string(IbSeverity in)
{
    switch(in)
    {
        case IbSeverity::Info: return "Info";
        case IbSeverity::Warning: return "Warning";
        case IbSeverity::Connectivity: return "Connectivity";
        case IbSeverity::RequestFailed: return "RequestFailed";
        case IbSeverity::OrderRejected: return "OrderRejected";
        case IbSeverity::System: return "System";
    }
    return "Unknown";
}

namespace
{

bool contains(std::string_view haystack, std::string_view needle)
{
    return haystack.find(needle) != std::string_view::npos;
}

} // namespace

IbSeverity classify(int code, std::string_view message)
{
    switch(code)
    {
        // Connection between TWS and IB, or between the API and TWS
        case 1100: // connectivity between IB and TWS lost
        case 1101: // connectivity restored, data lost
        case 1102: // connectivity restored, data maintained
        case 1300: // socket port has been reset
        case 2110: // connectivity between TWS and server is broken
        case 502:  // couldn't connect to TWS
        case 504:  // not connected
        case 507:  // bad message length
        case 509:  // exception caught while reading socket
            return IbSeverity::Connectivity;

        // Data farm status. The "OK" and "inactive" ones are status, the "broken" ones are warnings
        case 2104: // market data farm connection is OK
        case 2106: // HMDS data farm connection is OK
        case 2107: // HMDS data farm connection is inactive
        case 2108: // market data farm connection is inactive
        case 2119: // market data farm is connecting
        case 2158: // sec-def data farm connection is OK
            return IbSeverity::Info;

        case 202: // order cancelled (confirmation of a cancel)
        case 165: // historical market data service query message
            return IbSeverity::Info;

        case 399:   // order message, e.g. "will not be placed until market opens"
        case 10167: // displaying delayed market data
        case 10168: // market data not subscribed, delayed market data not enabled
            return IbSeverity::Warning;

        case 162: // historical market data service error
            // the same code is used to confirm a cancel and to report failures (pacing, no data...)
            if (contains(message, "query cancelled") || contains(message, "query canceled"))
                return IbSeverity::Info;
            return IbSeverity::RequestFailed;

        // Order problems
        case 103:   // duplicate order id
        case 104:   // can't modify a filled order
        case 105:   // order being modified does not match original order
        case 106:   // can't transmit order id
        case 107:   // cannot transmit incomplete order
        case 109:   // price is out of the range defined by the percentage offset
        case 110:   // price does not conform to the minimum price variation
        case 111:   // TIF and order type are incompatible
        case 135:   // can't find order with id
        case 161:   // cancel attempted when order is not in a cancellable state
        case 201:   // order rejected
        case 203:   // security not available or allowed for this account
        case 10147: // order id to cancel is not found
        case 10148: // order id to cancel can not be cancelled
            return IbSeverity::OrderRejected;

        // API / TWS level problems
        case 501: // already connected
        case 503: // TWS is out of date and must be upgraded
        case 505: // fatal error: unknown message id
        case 506: // unsupported version
            return IbSeverity::System;

        default:
            break;
    }
    // remaining 21xx codes are warnings (e.g. 2103/2105/2157 farm broken, 2109 outside RTH)
    if (code >= 2100 && code <= 2199)
        return IbSeverity::Warning;
    return IbSeverity::RequestFailed;
}

} // namespace ib_helper
