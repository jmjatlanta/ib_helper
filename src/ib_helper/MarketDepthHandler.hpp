#pragma once
#include "Decimal.h"
#include "IbError.hpp"

namespace ib_helper {

class MarketDepthHandler {
    public:
	virtual void OnUpdateMktDepth(int tickerId, int position, int operation, int side, double price, Decimal size) = 0;
	virtual void OnUpdateMktDepthL2(int tickerId, int position, const std::string& marketMaker, int operation, 
            int side, double price, Decimal size, bool isSmartDepth) = 0;
    /***
     * TWS reported an error or warning for this subscription
     */
    virtual void OnError(int reqId, const IbError& err) {}
};

} // namespace ib_helper
