#pragma once
#include "ib_api/client/Decimal.h"

namespace ib_helper {

class MarketDepthHandler {
	virtual void OnUpdateMktDepth(int tickerId, int position, int operation, int side, double price, Decimal size) = 0;
	virtual void OnUpdateMktDepthL2(int tickerId, int position, String marketMaker, int operation, int side, double price,
            Decimal size, boolean isSmartDepth) = 0;
};

} // namespace ib_helper
