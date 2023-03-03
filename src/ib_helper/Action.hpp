#pragma once

namespace ib_helper
{

enum Action {
    BUY,
    SELL,
    SSHORT,
    SLONG
};

std::string to_string(ActionType type)
{
    switch(type)
    {
        case (ActionType::BUY):
            return "BUY";
        case (ActionType::SELL):
            return "SELL";
        case (ActionType::SSHORT):
            return "SSHORT";
        case (ActionType::SLONG):
            return "SLONG";
    }
}

}

