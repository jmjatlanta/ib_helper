#include "Action.hpp"
#include <stdexcept>

namespace ib_helper
{

std::string to_string(Action type)
{
    switch(type)
    {
        case (Action::BUY):
            return "BUY";
        case (Action::SELL):
            return "SELL";
        case (Action::SSHORT):
            return "SSHORT";
        case (Action::SLONG):
            return "SLONG";
        case(Action::UNKNOWN):
            return "UNKNOWN";
    }
    return "UNKNOWN";
}

Action to_action(const std::string& in)
{
    if (in == "BUY")
        return Action::BUY;
    if (in == "SELL")
        return Action::SELL;
    if (in == "SSHORT")
        return Action::SSHORT;
    if (in == "SLONG")
        return Action::SLONG;
    else
        throw std::invalid_argument(in);
    return Action::UNKNOWN;
}

} // namespace ib_helper

