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
        FOREX,
        OPT
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
            case Type::OPT:
                return "OPT";
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
            case Type::OPT:
                return "Option";
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
        if (in == "OPT" || in == "Option")
            return Type::OPT;
        return Type::UNKNOWN;
    }
};

} // end namespace ib_helper

