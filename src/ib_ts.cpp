#include "ib_helper/IBConnector.hpp"
#include "util/SysLogger.h"
#include <thread>
#include <chrono>
#include <fstream>

int main(int argc, char** argv)
{
    //ib_helper::IBConnector conn("127.0.0.1", 4002, 3);
    //std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    util::SysLogger* logger = util::SysLogger::getInstance();   
    logger->set_log_level(util::SysLogger::LogLevel::DEBUG);
    std::ofstream myFile("MyFile.out");
    logger->set_stream(myFile);
    logger->debug("main", "This is message 1");
    logger->debug("main", "This is message 2");
    return 0;
}
