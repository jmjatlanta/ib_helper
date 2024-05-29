#pragma once
#include <string>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>
#include "StringHelper.hpp"
#include "DateUtil.hpp"

class OptionDetails
{
    public:
    enum class Type
    {
        UNKNOWN,
        CALL,
        PUT
    };

    /***
     * Construct an object from an OCC symbol
     * root symbol (6 chars padded)
     * expiry (6 chars yymmdd)
     * type (P or C)
     * strike X 1000, 0 left filled
    */
    OptionDetails(const std::string& occSymbol)
    {
        if (occSymbol.size() > 15)
        {
            underlying = stringhelper::trim(occSymbol.substr(0, 6));
            expiry = occSymbol.substr(6, 6);
            if (occSymbol.substr(12, 1) == "P")
                type = Type::PUT;
            if (occSymbol.substr(12, 1) == "C")
                type = Type::CALL;
            strikePrice = strtoll(occSymbol.substr(13).c_str(), nullptr, 10) / (double)1000; 
        }
        else
        {
            underlying = occSymbol;
            expiry = "";
            type = Type::UNKNOWN;
            strikePrice = 0.0;
        }
    }

    OptionDetails(const std::string& und, std::string exp, Type typ, double price)
            : underlying(und), expiry(exp), type(typ), strikePrice(price)
    {
    }

    /**
     * @brief convert the expiry string (YYMMDD) into a time_t with a time of 4pm NY
     * 
     * @return the time of the expiry
     */
    std::time_t to_time_t()
    {
        // some platforms have trouble with such dates, so modify it
        std::stringstream ss;
        ss << "20" << expiry.substr(0,2) << "-" << expiry.substr(2,2) << "-" << expiry.substr(4, 2) << " 21:00:00";
        tm t;
        ss >> std::get_time(&t, "%Y-%m-%d %H:%M:%S");
#ifdef _WIN32
        return to_4pm_ny(_mkgmtime(&t));
#else
        return to_4pm_ny(timegm(&t));
#endif
    }

    std::string to_string() 
    { 
        std::time_t exp = to_time_t();
        tm t = *gmtime(&exp);

        std::stringstream ss;
        ss << std::setw(6) << std::left << underlying << std::right
            << std::setw(2) << std::setfill('0') << t.tm_year - 100
            << std::setw(2) << std::setfill('0') << t.tm_mon + 1
            << std::setw(2) << std::setfill('0') << t.tm_mday
            << (type == Type::CALL ? "C" : (type == Type::PUT ? "P" : "U"))
            << std::setw(8) << std::setfill('0') << (uint64_t)(strikePrice * 1000);
        return ss.str();
    }

    public:
    std::string underlying;
    std::string expiry;
    Type type = Type::UNKNOWN;
    double strikePrice; 
};