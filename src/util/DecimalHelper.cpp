#include "DecimalHelper.hpp"

Decimal checkNaN(Decimal in)
{
    if (isNaN(in))
        return doubleToDecimal(0.0);
    return in;
}

Decimal checkInf(Decimal in)
{
    if (isInf(in))
        return doubleToDecimal(0.0);
    return in;
}

bool isNaN(Decimal in)
{
    return (in & 0x7800000000000000ull) == 0x7800000000000000ull 
            && ((in & 0x7c00000000000000ull) == 0x7c00000000000000ull);
    /* old way
    double d = decimalToDouble(in);
    return d != d; // returns true only if nan
    */
}

bool isInf(Decimal in)
{
    return (in & 0x7800000000000000ull) == 0x7800000000000000ull 
            && !((in & 0x7c00000000000000ull) == 0x7c00000000000000ull);
}

bool decimalIsValid(Decimal in)
{
    std::string tmpString = decimalToString(in);
    return tmpString.compare(std::string{"+NaN"}) != 0  
            && tmpString.compare(std::string{"-NaN"}) != 0
            && tmpString.compare(std::string{"-SNaN"}) != 0;
}
