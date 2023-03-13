#include "HistoricalBarFeeder.h"
#include <vector>


/****
 * Reads CSV file in the format
 * time,open,high,low,close,wap,volume,count
 */
HistoricalBarFileReader::HistoricalBarFileReader(const std::string& fileName )
{
    // open the file
    file.open(fileName);
    std::string line;
    std::getline(file, line); // header line
}

double to_double(const std::string& in)
{
    return strtod(in.c_str(), nullptr);
}

Decimal to_decimal(const std::string& in)
{
    return doubleToDecimal(to_double(in));
}

int to_int(const std::string& in)
{
    return strtol(in.c_str(), nullptr, 10);
}

std::vector<std::string> parseLine(const std::string& in)
{
    std::vector<std::string> line;
    std::string leftover = in;
    int pos = leftover.find(",");
    while (pos != std::string::npos)
    {
        line.push_back(leftover.substr(0, pos));
        leftover = leftover.substr(pos+1);
        pos = leftover.find(",");
    }
    // the last piece
    if (leftover.size() > 0)
        line.push_back(leftover);
    return line;
}

Bar HistoricalBarFileReader::NextBar()
{
    std::string line;
    std::getline(file, line);
    auto parts = parseLine(line);
    if (parts.size() != 8)
        throw 1;
    return Bar(parts[0], to_double(parts[1]), to_double(parts[2]), to_double(parts[3]), to_double(parts[4]),
            to_decimal(parts[5]), to_decimal(parts[6]), to_int(parts[7]));
}


HistoricalBarFileWriter::HistoricalBarFileWriter(const std::string& fileName)
{
    file.open(fileName);
    file << "time,high,low,open,close,wap,volume,count\n";
}


void HistoricalBarFileWriter::Write(const Bar& in)
{
    std::string line = in.time
        + "," + std::to_string(in.high)
        + "," + std::to_string(in.low)
        + "," + std::to_string(in.open)
        + "," + std::to_string(in.close)
        + "," + std::to_string(decimalToDouble(in.wap))
        + "," + std::to_string(decimalToDouble(in.volume))
        + "," + std::to_string(in.count)
        + "\n";
    file << line;
}

void HistoricalBarFileWriter::Write(const std::string& time, double high, double low, double open, double close,
        Decimal wap, Decimal volume, int count)
{
    Bar b{time, high, low, open, close, wap, volume, count};
    return Write(b);
}
