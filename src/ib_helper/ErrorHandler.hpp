#pragma once
#include "IbError.hpp"

namespace ib_helper
{

class IBConnector;

/***
 * Receives TWS errors that IBConnector could not route to the owner of the id
 * (e.g. id -1 messages that are not about connectivity, or ids it did not issue)
 */
class ErrorHandler
{
    public:
    virtual ~ErrorHandler() = default;
    virtual void OnError(IBConnector* conn, const IbError& err) = 0;
};

} // namespace ib_helper
