#pragma once

class Action
{
    enum class ActionType {
        BUY,
        SELL,
        SSHORT,
        SLONG
    };

    std::string to_string(ActionType type)
    {
        switch(type)
        {
            case (BUY):
                return "BUY";
            case (SELL):
                return "SELL";
            case (SSHORT):
                return "SSHORT";
            case (SLONG):
                return "SLONG";
    }

};

