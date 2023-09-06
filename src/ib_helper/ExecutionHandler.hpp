#pragma once
#include "Contract.h"
#include "CommissionReport.h"
#include "Execution.h"

namespace ib_helper
{

class ExecutionHandler
{
    public:
    virtual void OnExecDetails(int reqId, const Contract& contract, const Execution& execution) {}
    virtual void OnExecDetailsEnd(int reqId) {}
    virtual void OnCommissionReport(const CommissionReport& commissionReport) {}
};

} // namespace ib_helper