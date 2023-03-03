#pragma once
#include <string>

namespace ib_helper
{

enum Action {
    BUY,
    SELL,
    SSHORT,
    SLONG
};

std::string to_string(Action type);

Action to_action(const std::string& in);


}

