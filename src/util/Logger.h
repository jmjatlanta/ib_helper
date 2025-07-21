#pragma once
#include <string>
#include <iostream>
#include <filesystem>
#include "SysLogger.h"

/***
 * Handle logging
 */

class Logger
{
    public:
    Logger();
    ~Logger();
    static Logger* getInstance();

    void set_log_level(util::SysLogger::LogLevel in);
    /***
     * @brief send log message to a file
     * @param filenamePrefix the first part of the filename
    */
    void log_to_file(const std::string& filenamePrefix);
    /***
     * @brief send log message to a file
     * @param dir the directory where the log file will be placed
     * @param filenamePrefix the first part of the filename
    */
    void log_to_file(const std::filesystem::path& dir, const std::string& filenamePrefix);
    void log_to_console();
    std::filesystem::path get_current_file_path() const;
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

