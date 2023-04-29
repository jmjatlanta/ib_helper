#pragma once
#include "bar.h"
#include <string>
#include <fstream>

/***
 * Reads a CSV
 */
class HistoricalBarFileReader
{
    public:
    HistoricalBarFileReader(const std::string& fileName);
    Bar NextBar();
    private:
    std::ifstream file;
};


class HistoricalBarFileWriter
{
    public:
    HistoricalBarFileWriter(const std::string& fileName);
    void Write(const Bar& in);
    void Write(const std::string& time, double high, double low, double open, double close, Decimal wap, Decimal volume,
            int count);
    private:
    std::ofstream file;
};


