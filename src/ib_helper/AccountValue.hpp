#pragma once
#include <string>

namespace ib_helper
{

class AccountValue
{
    public:
    std::string key;
    std::string value;
    std::string currency;
    std::string account;

    AccountValue() : key(""), value(""), currency(""), account("") {}

    AccountValue(const std::string& key, const std::string& value, 
            const std::string& currency, const std::string& account)
        : key(key), value(value), currency(currency), account(account) {}
};

}

