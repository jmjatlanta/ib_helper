#pragma once

#include <vector>
#include <string>
#include <unordered_map>

/***
 * Determine the most liquid futures contract for a specific date
 */

namespace ib_helper
{

class ContractRolloverCalendar
{
    public:
    /*****
     * Given a letter, convert it to the number of the month of the year
     * @param in the letter
     * @return the month of the year
     */
    int letterToMonth(const char in);

    /****
     * convert a month number to the future letter
     * @param in the month number (i.e. 1 for January)
     * @return the month letter (i.e. F for January)
     */
    std::string monthToLetter(int in);

    /****
     * Convert a collection of month letters to their corresponding month numbers
     * @param in the months (i.e. FJV)
     * @returns the month numbers as a collection (i.e. 1, 4, 10)
     */
    std::vector<int> lettersToMonths(const std::string& in);

    enum RolloverRule {
        DAY_AFTER,
        DAY_OF,
        ONE_DAY_PRIOR,
        TWO_DAYS_PRIOR,
        FIVE_DAYS_PRIOR,
        EIGHT_DAYS_PRIOR,
        SECOND_TO_LAST_DAY_OF_PRIOR_MONTH,
        THIRD_TO_LAST_DAY_OF_PRIOR_MONTH,
        MONDAY_OF_LAST_WEEK_OF_PRIOR_MONTH,
        SECOND_WEEK_OF_PRIOR_MONTH,
        THIRD_WEEK_OF_PRIOR_MONTH,
        FIFTEENTH_DAY_OF_PRIOR_MONTH
    };

    protected:
    std::unordered_map<std::string, std::string> validMonths;
    std::unordered_map<std::string, RolloverRule> rolloverRules;

    private:
    void InsertInHashes(const std::string& value, RolloverRule rule, std::vector<std::string> keys);

    public:
    ContractRolloverCalendar();
    
    /***
     * Given a symbol, get collection of months
     * @param symbol the symbol
     * @return vector of ints
     */
    std::vector<int> ValidMonths(const std::string& symbol);

    protected:
    /***
     * Return the last digit of the passed in year
     * @param dt the date
     * @returns the last digit of the year
     */
    int lastDigitOfYear(time_t dt);

    /***
     * Calculate the current contract month
     * @param symbol the contract
     * @param currentTime the current time
     * @param[out] closestMonth the current month
     * @param[out] addYear set to 1 if the current month is in the next year
     */
    void currentMonth(const std::string& symbol, time_t currentTime, int& closestMonth, int& addYear);

    /***
     * Calculate the next contract month
     * @param symbol the contract
     * @param currentTime the current time
     * @param[out] closestMonth the current month
     * @param[out] addYear set to 1 if the current month is in the next year
     */
    void nextMonth(const std::string& symbol, time_t currentTime, int& closestMonth, int& addYear);
    
    /***
     * Add numDays to time_t
     * @param in the time
     * @param numDays the number of days to add (negative numbers subtract)
     * @returns in + (numDays * secs per day)
     */
    time_t plusDays(time_t in, int numDays);
    time_t plusMonths(time_t in, int numMonths);
    time_t firstDayOfMonth(time_t in);
    time_t lastDayOfMonth(time_t in);
    time_t previousMonday(time_t in);
    time_t nextMonday(time_t in);

    /***
     * Calculate the date the contract is rolled over to a more liquid contract
     * @param symbol the contract
     * @param expiry the expiry of the contract
     * @return the date the contract rolls over
     */
    time_t calculateRolloverDate(const std::string& symbol, time_t expiry);

    public:

    bool IsLiquid(const std::string& symbol, time_t expiry, time_t t = time(nullptr));

    /***
     * Get current contract month and year
     * @param symbol
     * @param t the current time
     * @returns [YYYY][MM] i.e. 202302
     */
    std::string CurrentMonthYYYYMM(const std::string& symbol, time_t t = time(nullptr));

    /***
     * Convert symbol to include next contract month and year
     * @param symbol
     * @param t the current time
     * @returns [symbol][M][Y] i.e. ESH3
     */
    std::string NextMonthYYYYMM(const std::string& symbol, time_t t = time(nullptr));

    /***
     * Compute the liquid contract and return the symbol
     * @param symbol the contract
     * @param expiry the date of the contract expiry
     * @param t the current time
     * @returns the contract date ( i.e. 202303 )
     */
    std::string LiquidYearAndMonth(const std::string& symbol, time_t expiry, time_t t = time(nullptr));
    bool IsValid(const std::string& symbol);
};

} // namesapce ib_helper
