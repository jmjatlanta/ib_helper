#pragma once
#include "bar.h"
#include "IbError.hpp"
#include <string>

namespace ib_helper
{

class HistoricalDataHandler {
    public:
    virtual void OnHistoricalData(int reqId, const Bar& bar) = 0;
    virtual void OnHistoricalDataEnd(int reqId, const std::string& start, const std::string& end) = 0;
    virtual void OnHistoricalDataUpdate(int reqId, const Bar& bar) = 0;
    /***
     * TWS reported an error or message for this request (e.g. 162 pacing violation or query cancelled)
     */
    virtual void OnError(int reqId, const IbError& err) {}
};

}

