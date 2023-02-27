#pragma once
#include "../util/SysLogger.h"
#include "../ib_api/client/Contract.h"
#include "../ib_api/client/Order.h"
#include "OrderStatus.hpp"

namespace ib_helper
{

/***
 * extends ib's order to include extra data
 */
class Order : public ::Order
{
    public:
    Order() { logger = util::SysLogger::getInstance(); }
    Contract contract;
    OrderStatus status = OrderStatus::UNKNOWN;

    Decimal cleanDecimal(Decimal in) { if (in == decimalInvalid) return decimalZero; return in; }

    bool update(const std::string& status, Decimal filled, Decimal remaining, double avgFillPrice,
            int permId, int parentId, double lastFillPrice, int clientId, const std::string& whyHeld, double mktCapPrice)
    {
        filled = cleanDecimal(filled);
        remaining = cleanDecimal(remaining);
        filledQuantity = cleanDecimal(filledQuantity);

        bool somethingChanged = false;
        OrderStatus oldStatus = this->status;
        this->status = ib_helper::to_OrderStatus(status);
        if (oldStatus != this->status)
            somethingChanged = true;
        Decimal oldValue = filledQuantity;
        filledQuantity = sub(totalQuantity, remaining);
        if (filledQuantity != oldValue)
            somethingChanged = true;
        log();
        return somethingChanged;
    }

    bool update(const ::Order& in)
    {
        bool somethingChanged = false;
        Decimal oldValue = cleanDecimal(filledQuantity);
        filledQuantity = cleanDecimal(in.filledQuantity);
        if (filledQuantity != oldValue)
            somethingChanged = true;
        log();
        return somethingChanged;
    }

    private:
    util::SysLogger* logger = nullptr;
    Decimal decimalZero = doubleToDecimal(0.0);
    Decimal decimalInvalid = ULLONG_MAX;

    void log()
    {
        double lmt = 0.0;
        double aux = 0.0;
        if (this->orderType == "LMT")
            lmt = lmtPrice;
        if (this->orderType == "STOP")
            aux = auxPrice;
        logger->debug("Order", std::to_string(orderId)
                + "," + (contract.localSymbol.empty() ? contract.symbol : contract.localSymbol)
                + "," + ib_helper::to_string(this->status)
                + "," + ( action == "BUY" ? "LONG" : "SHORT")
                + "," + std::to_string(lmt)
                + "," + std::to_string(aux)
                + "," + decimalStringToDisplay(totalQuantity)
                + "," + decimalStringToDisplay(filledQuantity));
    }
};

} // namespace ib_helper
