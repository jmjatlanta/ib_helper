#pragma once
#include <string>
#include <iostream>

/***
 * Handle logging
 */

//#include <boost/log/trivial.hpp>

class Logger
{
    public:
    Logger();
    ~Logger();
    static Logger* getInstance();
    void log_to_file(const std::string& filenamePrefix);
    void trace(const std::string& msg);
    void trace(const std::string& category, const std::string& msg); 
    void debug(const std::string& msg);
    void debug(const std::string& category, const std::string& msg);
    void info(const std::string& msg);
    void info(const std::string& category, const std::string& msg);
    void warning(const std::string& msg);
    void warning(const std::string& category, const std::string& msg);
    void error(const std::string& msg);
    void error(const std::string& category, const std::string& msg);
    void fatal(const std::string& msg);
    void fatal(const std::string& category, const std::string& msg);
    private:
    std::ostream* outputStream = nullptr;
};

