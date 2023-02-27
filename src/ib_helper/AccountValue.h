#pragma once

class AccountValue
{
    public:
    std::string key;
    std::string value;
    std::string currency;
    std::string account;

    AccountValue(const std::string& key, const std::string& value, 
            const std::string& currency, const std::string& account)
        : key(key), value(value), currency(currency), account(account) {}
};

