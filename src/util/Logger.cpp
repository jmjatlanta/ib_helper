#include "Logger.h"
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

/***
 * Include this file only once
 */

std::shared_ptr<Logger> logger_ = std::make_shared<Logger>();

Logger* Logger::getInstance() {
    boost::log::add_common_attributes();
    boost::log::core::get()->set_filter( boost::log::trivial::severity >= boost::log::trivial::debug);
    return logger_.get();
}

Logger::Logger() {}

Logger::~Logger() { }

static void add_file_log(const std::string& fullFileNamePrefix)
{
    boost::log::add_file_log(
            boost::log::keywords::file_name = fullFileNamePrefix + "_%N.log",
            boost::log::keywords::rotation_size = 10 * 1024 * 1024,
            boost::log::keywords::time_based_rotation = boost::log::sinks::file::rotation_at_time_point(0,0,0),
            boost::log::keywords::format = "[%TimeStamp%]: %Message%",
            boost::log::keywords::auto_flush = true, // write immediately
            boost::log::keywords::open_mode = std::ios_base::app,
            boost::log::keywords::max_files = 3 // no more than fullFileNamePrefix_2.log
            );
}

void Logger::log_to_file(const std::string& fileNamePrefix)
{
    add_file_log(fileNamePrefix);
}

void Logger::log_to_file(const std::filesystem::path& dir, const std::string& fileNamePrefix)
{
    std::filesystem::path fullFilename = dir;
    fullFilename /= fileNamePrefix;
    add_file_log(fullFilename.string());
}

void Logger::trace(const std::string& msg) { BOOST_LOG_TRIVIAL(trace) << msg; }

void Logger::trace(const std::string& category, const std::string& msg) 
        { BOOST_LOG_TRIVIAL(trace) << " [" << category << "] " << msg; }

void Logger::debug(const std::string& msg) { BOOST_LOG_TRIVIAL(debug) << msg; }

void Logger::debug(const std::string& category, const std::string& msg)
        { BOOST_LOG_TRIVIAL(debug) << " [" << category << "] " << msg; }

void Logger::info(const std::string& msg) { BOOST_LOG_TRIVIAL(info) << msg; }

void Logger::info(const std::string& category, const std::string& msg)
        { BOOST_LOG_TRIVIAL(info) << " [" << category << "] " << msg; }

void Logger::warning(const std::string& msg) { BOOST_LOG_TRIVIAL(warning) << msg; }

void Logger::warning(const std::string& category, const std::string& msg)
        { BOOST_LOG_TRIVIAL(warning) << " [" << category << "] " << msg; }

void Logger::error(const std::string& msg) { BOOST_LOG_TRIVIAL(error) << msg; }

void Logger::error(const std::string& category, const std::string& msg)
        { BOOST_LOG_TRIVIAL(error) << " [" << category << "] " << msg; }

void Logger::fatal(const std::string& msg) { BOOST_LOG_TRIVIAL(fatal) << msg; }

void Logger::fatal(const std::string& category, const std::string& msg)
        { BOOST_LOG_TRIVIAL(fatal) << " [" << category << "] " << msg; }

