#pragma once
#include <string>

namespace ib_helper
{

class SecurityType
{
    public:

    enum class Type
    {
        UNKNOWN,
        FUT,
        STK,
        FOREX
    };

    static std::string to_string(Type t)
    {
        switch(t)
        {
            case Type::FUT:
                return "FUT";
            case Type::STK:
                return "STK";
            case Type::FOREX:
                return "CASH";
            default:
                return "UNKNOWN";
        }
    }

    static std::string to_long_string(Type t)
    {
        switch(t)
        {
            case Type::FUT:
                return "Future";
            case Type::STK:
                return "Stock";
            case Type::FOREX:
                return "Forex";
            default:
                return "Unknown";
        }
    }

    static std::string to_long_string(const std::string& in)
    {
        return to_long_string( to_type(in) );
    }

    static std::string to_string(const std::string& in)
    {
        return to_string(to_type(in));
    }

    static Type to_type(const std::string& in)
    {
        if (in == "FUT" || in == "Future")
            return Type::FUT;
        if (in == "STK" || in == "Stock")
            return Type::STK;
        if (in == "CASH" || in == "Forex")
            return Type::FOREX;
        return Type::UNKNOWN;
    }
};

} // end namespace ib_helper

