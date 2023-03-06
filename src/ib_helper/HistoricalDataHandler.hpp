#pragma once
#include "../ib_api/client/bar.h"
#include <string>

namespace ib_helper
{

class HistoricalDataHandler {
    public:
    virtual void OnHistoricalData(int reqId, Bar bar) = 0;
    virtual void OnHistoricalDataEnd(int reqId, const std::string& start, const std::string& end);
    virtual void OnHistoricalDataUpdate(int reqId, const Bar& bar);
};

}

