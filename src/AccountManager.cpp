#include "AccountManager.hpp"
#include "ib_helper/OrderStatus.hpp"

Decimal decimalZero = doubleToDecimal(0.0);
Decimal decimalUnset = ULLONG_MAX;

AccountManager::AccountManager(ib_helper::IBConnector* conn, const std::string& mainAccount) 
    : ib(conn), mainAccount(mainAccount)
{
    logger = util::SysLogger::getInstance();
    conn->RequestPositions();
}
AccountManager::~AccountManager() {}

uint32_t AccountManager::PlaceOrder(ib_helper::Order ord, bool immediateOrder)
{
    ord.account = mainAccount;
    Position* p = GetPosition(ord.contract, true);
    if (immediateOrder)
    {
        if (ord.action == "SELL")
            p->expectedPos = sub(p->expectedPos, ord.totalQuantity);
        else
            p->expectedPos = add(p->expectedPos, ord.totalQuantity);
    }
    if (ord.orderId == 0)
        ord.orderId = ib->GetNextOrderId();
    orders[ord.orderId] = ord;
    ib->PlaceOrder(ord.orderId, ord, ord.contract);
    return ord.orderId;
}

Position* AccountManager::GetPosition(const Contract& contract, bool createIfNecessary)
{
    Position* retVal = nullptr;
    try {
        Position& pos = positions.at(contract.conId);
        retVal = &pos;
    } catch (const std::out_of_range& oor) {
        if(createIfNecessary)
        {
            positions[contract.conId] = Position(mainAccount, contract, 0, 0.0);
            Position& pos = positions[contract.conId];
            retVal = &pos;
        }
    }
    return retVal;
}

void AccountManager::OnAccountValueUpdate(const std::string& key, const std::string& value, const std::string& currency,
            const std::string& accountName)
{
}

void AccountManager::OnPortfolioUpdate(Contract contract, Decimal position, double marketPrice, double marketValue,
            double averageCost, double unrealizedPNL, double realizedPNL, const std::string& accountName)
{
    // ignore anything that does not belong to us
    if (accountName != mainAccount)
        return;
    // determine if we have an existing position
    Position* p = nullptr;
    try {
        p = &positions.at(contract.conId);    
    } catch (const std::out_of_range& oor)
    {
        positions[contract.conId] = Position(accountName, contract, position, averageCost);
        p = &positions[contract.conId];
    }
    // update
    logger->debug("AccountManager", "OnPortfolioUpdate updating position to " + std::to_string(decimalToDouble(position)));
    p->averageCost = averageCost;
    p->pos = position;
    p->realizedPNL = realizedPNL;
    p->unrealizedPNL = unrealizedPNL;
    // make sure we updated
}

void AccountManager::OnUpdateAccountTime(const std::string& timestamp) {}
void AccountManager::OnAccountDownloadEnd(const std::string& accountName) {}

void AccountManager::OnPosition(const std::string& account, Contract contract, Decimal pos, double averageCost)
{
    if (account != mainAccount)
        return;
    try {
        Position& p = positions.at(contract.conId);
        p.pos = pos;
        p.averageCost = averageCost;
    } catch (const std::out_of_range& oor)
    {
        positions[contract.conId] = Position(account, contract, pos, averageCost);
    }
}

void AccountManager::OnPositionEnd() {}
void AccountManager::OnCurrentTime(long currentTime) {}

void AccountManager::OnError(int id, int errorCode, const std::string& errorMessage, 
        const std::string& advancedOrderRejectJson)
{
    switch(errorCode)
	{
	    case 2109: // warning about after hours
		case 105: // order being modified does not match original order
			return;
		case 102: // duplicate ticker id
		case 103: // duplicate order id
		case 106: // can't transmit order id
		case 107: // cannot transmit incomplete order
		case 109: // price out of range
		case 110: // price does not conform (min tick)
		case 111: // TIF and order type are incompatible
		case 113: // TIF must be DAY for MOC and LOC orders
		case 116: // dead exchange
		case 117: // block order size must be at least 50
		case 118: // VWAP orders may only route to vwap exchange
		case 119: // only VWAP orders accepted on this exchange
		case 120: // too late for VWAP order
		case 131: // sweep to fill invalid for exchange
		case 132: // no clearing account
		case 133: // submit new order failed
		case 134: // modify order failed
		case 135: // order id not found (while canceling)
		case 136: // order cannot be cancelled
		case 139: // parse error
		case 140: // parse error - value type conflict
		case 141: // parse error - value type conflict
		case 143: // parse error - value type conflict
		case 144: // allocation error
		case 145: // validation error
		case 146: // invalid trigger method
		case 154: // halted
		case 161: // cannot cancel, order not active
		case 478: // parameter conflict, cancel order
		case 200: // no security found for the request
		case 201: // order rejected = margin
		case 202: // order cancelled
		case 10147: // order not found
		case 10148: // cannot be cancelled, reason passed in
		{
            try {
                ib_helper::Order& ord = GetOrder(id);
				if (ord.status == ib_helper::OrderStatus::PRESUBMITTED
						|| ord.status == ib_helper::OrderStatus::UNKNOWN) {
					// this is an order that was probably cancelled. Tell OnOrderStatus to cancel it
					OnOrderStatus(id, ib_helper::to_string(ib_helper::OrderStatus::CANCELLED), ord.filledQuantity,
                            ord.totalQuantity - ord.filledQuantity, 0.0, ord.permId, ord.parentId, 
                            0.0, 0, advancedOrderRejectJson, 0);
				}
				else
				{
					// what do we do here? The modification was not valid on what may be an existing order
				}
            } catch (const std::out_of_range& oor) {}
			return;
		}
		case 104: // cannot modify a filled order
		{
            try {
                ib_helper::Order& ord = GetOrder(id);
			    if (ord.status != ib_helper::OrderStatus::FILLED) 
                {
				    OnOrderStatus(id, to_string(ib_helper::OrderStatus::FILLED), ord.filledQuantity, 
                            ord.totalQuantity - ord.filledQuantity, 0.0, ord.permId, ord.parentId, 0.0, 0,
                            advancedOrderRejectJson, 0);
			    }
            } catch (const std::out_of_range& oor) {}
			return;
		}
	}
}

void AccountManager::OnOpenOrder(int orderId, Contract contract, Order order, OrderState orderState)
{
    ib_helper::Order& currOrder = orders[orderId];
    if (currOrder.orderId == 0)
    {
        return;
    }
    currOrder.update(order);
    for(auto* handler : orderHandlers)
    {
        handler->OnOpenOrder(orderId, contract, order, orderState);
    }
}

Decimal cleanDecimal(Decimal in) { if (in == decimalUnset) return decimalZero; return in; }

void AccountManager::OnOrderStatus(int orderId, const std::string& status, Decimal filled, Decimal remaining,
            double avgFillPrice, int permId, int parentId, double lastFillPrice, int clientId,
            const std::string& whyHeld, double mktCapPrice)
{
    ib_helper::Order& currOrder = orders[orderId];
    if (currOrder.orderId == 0)
    {
        return;
    }
    // verify decimals are sensible
    filled = cleanDecimal(filled);
    remaining = cleanDecimal(remaining);
    currOrder.filledQuantity = cleanDecimal(currOrder.filledQuantity);
    Position* currPos = GetPosition(currOrder.contract);
    Decimal oldFilled = currOrder.filledQuantity;
    if (currOrder.update(status, filled, remaining, avgFillPrice, permId, parentId, lastFillPrice, clientId, 
                whyHeld, mktCapPrice))
    {
        // something changed
        Decimal newFilled = currOrder.filledQuantity;
        Decimal filledDelta = sub(newFilled, oldFilled);
        logger->debug("AccountManager", std::string("OnOrderStatus: ") 
                    + " new filled: " + decimalStringToDisplay(newFilled)
                    + " old filled: " + decimalStringToDisplay(oldFilled)
                    + " remaining: " + decimalStringToDisplay(remaining));
        if (currPos != nullptr
                && decimalToDouble(currPos->expectedPos) > 0.0
                && currOrder.action == "SELL"
                && currOrder.status == ib_helper::OrderStatus::FILLED)
        {
            // take this off of expectedPos
            currPos->expectedPos = sub(currPos->expectedPos, filledDelta);
        }
    }
    for(auto* handler : orderHandlers)
    {
        handler->OnOrderStatus(orderId, status, filled, remaining, avgFillPrice, permId, parentId, lastFillPrice,
                clientId, whyHeld, mktCapPrice);
    }
}

void AccountManager::OnOpenOrderEnd() {}
void AccountManager::OnOrderBound(long orderId, int apiClientId, int apiOrderId) {}

ib_helper::Order& AccountManager::GetOrder(int id, bool currentOnly)
{
    if (currentOnly)
        return orders.at(id);
    else
    {
        try {
            return orders.at(id);
        } catch (const std::out_of_range& oor)
        {
            return past_orders.at(id);
        }
    }
}
