#pragma once
#include <string>

namespace stringhelper
{

inline std::string& ltrim(std::string& in)
{
    in.erase(in.begin(), std::find_if(in.begin(), in.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    return in;
}
inline std::string& rtrim(std::string& in)
{
    in.erase(std::find_if(in.rbegin(), in.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), in.end());
    return in;
}

inline std::string trim(const std::string& in)
{
    std::string retval = in;
    return rtrim(ltrim(retval));
}

} // namespace stringhelper
