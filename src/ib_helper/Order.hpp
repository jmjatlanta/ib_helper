#pragma once
#include "../util/SysLogger.h"
#include "Contract.h"
#include "Order.h"

namespace ib_helper
{

/***
 * extends ib's order to include extra data
 */
class Order : public ::Order
{
    public:

    enum class OrderStatus {
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

    static OrderStatus to_OrderStatus(const std::string& in)
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

    static std::string to_string(OrderStatus in)
    {
        switch (in)
        {
            case OrderStatus::APIPENDING:
                return "ApiPending";
            case OrderStatus::APICANCELLED:
                return "ApiCancelled";
            case OrderStatus::PRESUBMITTED:
                return "PreSubmitted";
            case OrderStatus::PENDINGCANCEL:
                return "PendingCancel";
            case OrderStatus::CANCELLED:
                return "Cancelled";
            case OrderStatus::SUBMITTED:
                return "Submitted";
            case OrderStatus::FILLED:
                return "Filled";
            case OrderStatus::INACTIVE:
                return "Inactive";
            case OrderStatus::PENDINGSUBMIT:
                return "PendingSubmit";
            default:
                return "Unknown";
        }
    }

    static std::string to_string(const Order& in)
    {
        std::stringstream ss;
        ss << "Order: " << in.orderId
                << " Type: " << in.orderType
                << " Limit: " << in.lmtPrice
                << " Aux: " << in.auxPrice
                << " Qty: " << decimalToDouble(in.totalQuantity)
                << " Filled: " << decimalToDouble(in.filledQuantity)
                << " Status: " << to_string(in.status)
                << " Action: " << in.action;
        return ss.str();
    }

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
        this->status = to_OrderStatus(status);
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
                + "," + to_string(this->status)
                + "," + ( action == "BUY" ? "LONG" : "SHORT")
                + "," + std::to_string(lmt)
                + "," + std::to_string(aux)
                + "," + decimalStringToDisplay(totalQuantity)
                + "," + decimalStringToDisplay(filledQuantity));
    }
};

} // namespace ib_helper
