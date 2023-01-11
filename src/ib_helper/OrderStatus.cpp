#include "OrderStatus.hpp"

namespace ib_helper
{

OrderStatus to_OrderStatus(const std::string& in)
{
    if (in == "ApiPending")
        return OrderStatus::APIPENDING;
    if (in == "ApiCancelled")
        return OrderStatus::APICANCELLED;
    if (in == "PreSubmitted")
        return OrderStatus::PRESUBMITTED;
    if (in == "PendingCancel")
        return OrderStatus::PENDINGCANCEL;
    if (in == "Cancelled")
        return OrderStatus::CANCELLED;
    if (in == "Submitted")
        return OrderStatus::SUBMITTED;
    if (in == "Filled")
        return OrderStatus::FILLED;
    if (in == "Inactive")
        return OrderStatus::INACTIVE;
    if (in == "PendingSubmit")
        return OrderStatus::PENDINGSUBMIT;
    return OrderStatus::UNKNOWN;
}

std::string to_string(OrderStatus in)
{
    switch (in)
    {
        case APIPENDING:
            return "ApiPending";
        case APICANCELLED:
            return "ApiCancelled";
        case PRESUBMITTED:
            return "PreSubmitted";
        case PENDINGCANCEL:
            return "PendingCancel";
        case CANCELLED:
            return "Cancelled";
        case SUBMITTED:
            return "Submitted";
        case FILLED:
            return "Filled";
        case INACTIVE:
            return "Inactive";
        case PENDINGSUBMIT:
            return "PendingSubmit";
        default:
            return "Unknown";
    }
}

} // namespace ib_helper

