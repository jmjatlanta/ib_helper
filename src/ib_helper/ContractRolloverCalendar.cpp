#include "ContractRolloverCalendar.hpp"
#include <algorithm>

namespace ib_helper
{

int ContractRolloverCalendar::stringToMonth(const std::string& in)
{
    if (in == "F" || in == "JAN")
        return 1;
    if (in == "G" || in == "FEB")
        return 2;
    if (in == "H" || in == "MAR")
        return 3;
    if (in == "J" || in == "APR")
        return 4;
    if (in == "K" || in == "MAY")
        return 5;
    if (in == "M" || in == "JUN")
        return 6;
    if (in == "N" || in == "JUL")
        return 7;
    if (in == "O" || in == "AUG")
        return 8;
    if (in == "U" || in == "SEP")
        return 9;
    if (in == "V" || in == "OCT")
        return 10;
    if (in == "X" || in == "NOV")
        return 11;
    if (in == "Z" || in == "DEC")
        return 12;
    return 0;
}

/****
 * convert a month number to the future letter
 * @param in the month number (i.e. 1 for January)
 * @return the month letter (i.e. F for January)
 */
std::string ContractRolloverCalendar::monthToLetter(int in)
{
    const static std::string  monthOrder{"FGHJKMNQUVXZ"};
    if (in <= 0 || in > 12)
        return 0;
    return std::string(1, monthOrder[in]);
}

/****
 * Convert a collection of month letters to their corresponding month numbers
 * @param in the months (i.e. FJV)
 * @returns the month numbers as a collection (i.e. 1, 4, 10)
 */
std::vector<int> ContractRolloverCalendar::lettersToMonths(const std::string& in)
{
    std::vector<int> retVal;
    for(const auto l : in)
    {
        retVal.push_back(stringToMonth(std::string{} + l));
    }
    return retVal;
}

void ContractRolloverCalendar::InsertInHashes(const std::string& value, RolloverRule rule, std::vector<std::string> keys)
{
    for(const auto& key : keys)
    {
        validMonths[key] = value;
        rolloverRules[key] = rule;
    }
}

ContractRolloverCalendar::ContractRolloverCalendar()
{
    InsertInHashes("HMUZ", RolloverRule::DAY_OF, {"STXE", "FDX", "FSMI", "FFI", "IFS", "IFX"} );
    InsertInHashes("FGHJKMNQUVXZ", RolloverRule::DAY_OF, {"FCE", "MFXI", "OMXS30", "AEX"} );
    InsertInHashes("HMUZ", RolloverRule::FIVE_DAYS_PRIOR, {"ES", "NQ", "YM", "DM", "TFS", "RMF", "MES", "MNQ", "RTY", "M2K" } );
    InsertInHashes("FGHJKMNQUVXZ", RolloverRule::ONE_DAY_PRIOR, {"CL", "NG"});
    InsertInHashes("FGHJKMNQUVXZ", RolloverRule::EIGHT_DAYS_PRIOR, {"HO", "RB"});
    InsertInHashes("GJMQZ", RolloverRule::THIRD_TO_LAST_DAY_OF_PRIOR_MONTH, {"GC"});
    InsertInHashes("HKNUZ", RolloverRule::THIRD_TO_LAST_DAY_OF_PRIOR_MONTH, {"SI", "HG"});
    InsertInHashes("FJNV", RolloverRule::THIRD_TO_LAST_DAY_OF_PRIOR_MONTH, {"PL"});
    InsertInHashes("HMUZ", RolloverRule::SECOND_TO_LAST_DAY_OF_PRIOR_MONTH, {"PA"});
    InsertInHashes("HMUZ", RolloverRule::ONE_DAY_PRIOR, {"FGBX", "FGBL", "FGBM", "FGBS", "NK", "JNI", "JNM", "YAP", "SXF"});
    InsertInHashes("HMUZ", RolloverRule::SECOND_TO_LAST_DAY_OF_PRIOR_MONTH, {"ZB", "ZN", "ZF", "ZT"});
    InsertInHashes("HMUZ", RolloverRule::DAY_AFTER, {"FSS", "FEI", "FES", "JEY", "ED", "BAX", "YBA"});
    InsertInHashes("HMUZ", RolloverRule::THIRD_TO_LAST_DAY_OF_PRIOR_MONTH, {"FLG", "CGB"});
    InsertInHashes("HMUZ", RolloverRule::DAY_OF, {"YTC", "YTT", "JGB"});
    InsertInHashes("GJMQVZ", RolloverRule::DAY_OF, {"IND"});
    InsertInHashes("HKNUZ", RolloverRule::MONDAY_OF_LAST_WEEK_OF_PRIOR_MONTH, {"ZC/C"});
    InsertInHashes("HKNUZ", RolloverRule::MONDAY_OF_LAST_WEEK_OF_PRIOR_MONTH, {"ZW/W"});
    InsertInHashes("FHKNQUZ", RolloverRule::MONDAY_OF_LAST_WEEK_OF_PRIOR_MONTH, {"ZS/S"}); 
    InsertInHashes("FHKNQUVZ", RolloverRule::MONDAY_OF_LAST_WEEK_OF_PRIOR_MONTH, {"ZL/BO"});
    InsertInHashes("FHKNQUVZ", RolloverRule::MONDAY_OF_LAST_WEEK_OF_PRIOR_MONTH, {"ZM/SM"});
    InsertInHashes("GJMQVZ", RolloverRule::MONDAY_OF_LAST_WEEK_OF_PRIOR_MONTH, {"LC"});
    InsertInHashes("FHJKQUVX", RolloverRule::MONDAY_OF_LAST_WEEK_OF_PRIOR_MONTH, {"FC"});
    InsertInHashes("GJKMNQVZ", RolloverRule::MONDAY_OF_LAST_WEEK_OF_PRIOR_MONTH, {"LH"});
    InsertInHashes("HKNUZ", RolloverRule::SECOND_WEEK_OF_PRIOR_MONTH, {"CC"});
    InsertInHashes("HKNUZ", RolloverRule::THIRD_WEEK_OF_PRIOR_MONTH, {"KC"});
    InsertInHashes("HKNVZ", RolloverRule::FIFTEENTH_DAY_OF_PRIOR_MONTH, {"CT"});
    InsertInHashes("HKNV", RolloverRule::THIRD_WEEK_OF_PRIOR_MONTH, {"SB"});
    InsertInHashes("FGHJKMNQUVXZ", RolloverRule::TWO_DAYS_PRIOR, {"LCO/CO"});
    InsertInHashes("HMUZ", RolloverRule::ONE_DAY_PRIOR, {"URO", "BP", "JY", "RP", "SF", "CD", "AD", "KRW", "MP", "DX"});
    InsertInHashes("FHKNUX", RolloverRule::SECOND_WEEK_OF_PRIOR_MONTH, {"ZS"}); // Soybeans
    InsertInHashes("HKNZ", RolloverRule::SECOND_WEEK_OF_PRIOR_MONTH, {"ZC"}); // Corn
}

/***
 * Given a symbol, get collection of months
 * @param symbol the symbol
 * @return vector of ints
 */
std::vector<int> ContractRolloverCalendar::ValidMonths(const std::string& symbol)
{
    return lettersToMonths(validMonths[symbol]);
}

/***
 * Return the last digit of the passed in year
 * @param dt the date
 * @returns the last digit of the year
 */
int ContractRolloverCalendar::lastDigitOfYear(time_t dt)
{
    return gmtime(&dt)->tm_year % 10;
}

/***
 * Calculate the current contract month
 * @param symbol the contract
 * @param currentTime the current time
 * @param[out] closestMonth the current month
 * @param[out] addYear set to 1 if the current month is in the next year
 */
void ContractRolloverCalendar::currentMonth(const std::string& symbol, time_t currentTime, int& closestMonth, int& addYear)
{
    int currMonth = gmtime(&currentTime)->tm_mon+1;
    std::vector<int> monthNumbers = ValidMonths(symbol);
    closestMonth = 0;
    addYear = 0;
    for(int i : monthNumbers)
    {
        if (i >= currMonth)
        {
            closestMonth = i;
            break;
        }
    }
    if (closestMonth == 0 && monthNumbers.size() > 0)
    {
        closestMonth = monthNumbers[0];
        addYear = 1;
    }
}

/***
 * Calculate the next contract month
 * @param symbol the contract
 * @param currentTime the current time
 * @param[out] closestMonth the current month
 * @param[out] addYear set to 1 if the current month is in the next year
 */
void ContractRolloverCalendar::nextMonth(const std::string& symbol, time_t currentTime, int& closestMonth, int& addYear)
{
    int currMonth = gmtime(&currentTime)->tm_mon+1;
    std::vector<int> monthNumbers = ValidMonths(symbol);
    closestMonth = 0;
    addYear = 0;
    for(int i : monthNumbers)
    {
        if (i > currMonth)
        {
            closestMonth = i;
            break;
        }
    }
    if (closestMonth == 0 && monthNumbers.size() > 0)
    {
        closestMonth = monthNumbers[0];
        addYear = 1;
    }
}
    
/***
 * Determine if the month's contract is still liquid on a particular date
 * @param symbol the contract
 * @param expiry the expiry date of the contract
 * @param t the current time
 * @return true if the contract for this month is still considered liquid
 */
bool ContractRolloverCalendar::IsLiquid(const std::string& symbol, time_t expiry, time_t t)
{
    time_t rolloverDate = calculateRolloverDate(symbol, expiry);
    return rolloverDate > t;
}

/***
 * Add numDays to time_t
 * @param in the time
 * @param numDays the number of days to add (negative numbers subtract)
 * @returns in + (numDays * secs per day)
 */
time_t ContractRolloverCalendar::plusDays(time_t in, int numDays)
{
    return in + (numDays * 24 * 60 * 60);
}

time_t ContractRolloverCalendar::plusMonths(time_t in, int numMonths)
{
    tm* breakout = gmtime(&in);
    if (breakout->tm_mon > numMonths)
        breakout->tm_mon -= numMonths;
    else
    {
        breakout->tm_mon -= numMonths + 12;
        breakout->tm_year -= 1;
    }
    return mktime(breakout);
}

short max_day(short month, int year)
{
    if(month == 0 || month == 2 || month == 4 || month == 6 || month == 7 || month == 9 || month == 11)
        return 31;
    else if(month == 3 || month == 5 || month == 8 || month == 10)
        return 30;
    else {
        if(year % 4 == 0) {
            if(year % 100 == 0) {
                if(year % 400 == 0)
                    return 29;
                return 28;
            }
            return 29;
        }
        return 28;
    }
}

time_t ContractRolloverCalendar::firstDayOfMonth(time_t in)
{
    tm* breakout = gmtime(&in);
    breakout->tm_mday = 1;
    return mktime(breakout);
}

time_t ContractRolloverCalendar::previousMonday(time_t in)
{
    tm* breakout = gmtime(&in);
    breakout->tm_mday -= breakout->tm_wday > 1 ? breakout->tm_wday - 1 : 6 + breakout->tm_wday;
    breakout->tm_wday = 1;
    return mktime(breakout);
}

time_t ContractRolloverCalendar::nextMonday(time_t in)
{
    tm* breakout = gmtime(&in);
    breakout->tm_mday += breakout->tm_wday > 1 ? 7 - breakout->tm_wday : 6 - breakout->tm_wday;
    breakout->tm_wday = 1;
    return mktime(breakout);
}

time_t ContractRolloverCalendar::lastDayOfMonth(time_t in)
{
    tm* breakout = gmtime(&in);
    breakout->tm_mday = max_day(breakout->tm_mon, breakout->tm_year);
    return mktime(breakout);
}

time_t ContractRolloverCalendar::calculateRolloverDate(const std::string& symbol, time_t expiry)
{
    RolloverRule rule = rolloverRules[symbol];
    switch(rule)
    {
        case DAY_AFTER:
            return plusDays(expiry, 1);
        case DAY_OF:
            return expiry;
        case ONE_DAY_PRIOR:
            return plusDays(expiry, -1);
        case TWO_DAYS_PRIOR:
            return plusDays(expiry, -2);
        case FIVE_DAYS_PRIOR:
            return plusDays(expiry, -7); // 2 extra days for weekend
        case EIGHT_DAYS_PRIOR:
            return plusDays(expiry, -8);
        case SECOND_TO_LAST_DAY_OF_PRIOR_MONTH:
            {
                time_t t = plusMonths(expiry, -1);
                t = lastDayOfMonth(t);
                return plusDays(t, -1);
            }
        case THIRD_TO_LAST_DAY_OF_PRIOR_MONTH:
            {
                time_t t = plusMonths(expiry, -1);
                t = lastDayOfMonth(t);
                return plusDays(t, -2);
            }
        case MONDAY_OF_LAST_WEEK_OF_PRIOR_MONTH:
            {
                time_t t = plusMonths(expiry, -1);
                t = lastDayOfMonth(t);
                return previousMonday(t);
            }
        case SECOND_WEEK_OF_PRIOR_MONTH:
            {
                time_t t = plusMonths(expiry, -1);
                t = firstDayOfMonth(t);
                tm* breakout = gmtime(&t);
                if (breakout->tm_wday < 2  )
                    t = plusDays(t, 3);
                return nextMonday(t);
            }
        case THIRD_WEEK_OF_PRIOR_MONTH:
            {
                time_t t = plusMonths(expiry, -1);
                t = firstDayOfMonth(t);
                tm* breakout = gmtime(&t);
                if (breakout->tm_wday < 2  )
                    t = plusDays(t, 3);
                return nextMonday(t) + (60 * 60 * 24 * 7);
            }
        case FIFTEENTH_DAY_OF_PRIOR_MONTH:
            {
                time_t t = plusMonths(expiry, -1);
                t = firstDayOfMonth(t);
                return t + (60 * 60 * 24 * 15);
            }
    }
    return 0;
}

bool ContractRolloverCalendar::IsValid(const std::string& ticker)
{
    return !validMonths[ticker].empty();
}

/***
 * Convert "ABCH2" to 3 (H == March)
 * @param in the string to parse
 * @return the month number corresponding to the second to last letter
 */
int ContractRolloverCalendar::parseMonth(const std::string& in)
{
    return stringToMonth( in.substr(in.length()-2, in.length() - 1));
}

/***
 * Turn ESH3 to 2023
 * @param in the symbol with the year
 * @returns the year or -1 on error
 */
int ContractRolloverCalendar::parseYear(const std::string& in)
{
    if (in.empty())
        return -1;
    std::string lastChar = in.substr(in.size()-1);
    int year = strtol( lastChar.c_str(), nullptr, 10);
    if (year == 0 && lastChar != "0")
        return -1;
    time_t tt = time(nullptr);
    tm* ts = localtime(&tt);
    if(year == ts->tm_year % 10)
        return ts->tm_year;
    if (year > ts->tm_year % 10)
        return ts->tm_year / 10 * 10 + year;
    // next decade
    return (ts->tm_year / 10 + 1)  * 10 + year;
}

/****
 * If there is a year and month suffix, remove it
 * @param the symbol (i.e. "NQM3" or "YM  MAR 03")
 * @return only the symbol (i.e "NQ" or "YM")
 */
std::string ContractRolloverCalendar::parseSymbol(const std::string& in)
{
    int pos = in.find(" ");
    // spaces
    if (pos != std::string::npos)
        return in.substr(0, pos);
    // last char is a digit
    if (parseYear(in) < 0)
        return in;
    return in.substr(0, in.size()-2);
}

static inline void ltrim(std::string& s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
                return !std::isspace(ch);
            }));
}
static inline void rtrim(std::string &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
                return !std::isspace(ch);
            }).base(), s.end());
}

static inline void trim(std::string& s)
{
    rtrim(s);
    ltrim(s);
}

std::string ContractRolloverCalendar::parseYearAndMonthWithSpaces(const std::string& in)
{
    std::string newString = in.substr(in.find(" "));
    trim(newString);
    int month = stringToMonth(newString.substr(0,3));
    newString = newString.substr(newString.find(" ") + 1);
    int year = strtol(newString.c_str(), nullptr, 10) + 2000;
    if (month >= 10)
        return std::to_string(year) + std::to_string(month);
    return std::to_string(year) + "0" + std::to_string(month);
}

std::string ContractRolloverCalendar::parseYearAndMonth(const std::string& in)
{
    // spaces
    if (in.find(" ") != std::string::npos)
        return parseYearAndMonthWithSpaces(in);
    int year = parseYear(in);
    if (year < 0)
        return "";
    int month = parseMonth(in);
    if (month > 10)
        return std::to_string(year) + std::to_string(month);
    return std::to_string(year) + "0" + std::to_string(month);
}

/***
 * Get current contract month and year
 * @param symbol
 * @param t the current time
 * @returns [YYYY][MM] i.e. 202302
 */
std::string ContractRolloverCalendar::CurrentMonthYYYYMM(const std::string& symbol, time_t t)
{
    // if the symbol already includes the month and year, use that
    std::string parsed = parseYearAndMonth(symbol);
    if (!parsed.empty())
        return parsed;
    int closestMonth = 0;
    int addYear = 0;
    tm* tm_struct = gmtime(&t);
    currentMonth(symbol, t, closestMonth, addYear);
    std::string retVal = std::to_string(tm_struct->tm_year + 1900 + addYear);
    if (closestMonth < 10)
        retVal += "0" + std::to_string(closestMonth);
    else
        retVal += std::to_string(closestMonth);
    return retVal;
}

/***
 * Convert symbol to include next contract month and year
 * @param symbol
 * @param t the current time
 * @returns [symbol][M][Y] i.e. ESH3
 */
std::string ContractRolloverCalendar::NextMonthYYYYMM(const std::string& symbol, time_t t)
{
    int closestMonth = 0;
    int addYear = 0;
    tm* tm_struct = gmtime(&t);
    nextMonth(symbol, t, closestMonth, addYear);
    std::string retVal = std::to_string(tm_struct->tm_year + 1900 + addYear);
    if (closestMonth < 10)
        retVal += "0" + std::to_string(closestMonth);
    else
        retVal += std::to_string(closestMonth);
    return retVal;
}

/***
 * Compute the liquid contract and return the symbol
 * @param symbol the contract
 * @param expiry the contract expiry
 * @param t the current time
 * @returns the contract date ( i.e. 202303 )
 */
std::string ContractRolloverCalendar::LiquidYearAndMonth(const std::string& symbol, time_t expiry, time_t t)
{
    if (IsLiquid(symbol, expiry, t))
        return CurrentMonthYYYYMM(symbol, t);
    return NextMonthYYYYMM(symbol, t);
}

} // namespace ib_helper

