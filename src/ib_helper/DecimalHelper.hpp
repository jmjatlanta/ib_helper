#include "Decimal.h"
#include <string>

inline bool decimalIsValid(Decimal in) {
    std::string tmpString = decimalToString(in);
    return tmpString.compare(std::string{"+NaN"}) != 0  
            && tmpString.compare(std::string{"-NaN"}) != 0
            && tmpString.compare(std::string{"-SNaN"}) != 0;
}