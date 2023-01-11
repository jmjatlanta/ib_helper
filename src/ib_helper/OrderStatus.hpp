#pragma once
#include <string>

namespace ib_helper
{

enum OrderStatus {
    APIPENDING,
    APICANCELLED,
    PRESUBMITTED,
    PENDINGCANCEL,
    CANCELLED,
    SUBMITTED,
    FILLED,
    INACTIVE,
    PENDINGSUBMIT,
    UNKNOWN
};


OrderStatus to_OrderStatus(const std::string& in);

std::string to_string(OrderStatus in);

} // namespace ib_helper

