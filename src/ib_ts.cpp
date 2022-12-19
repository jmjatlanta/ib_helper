#include "ib_helper/IBConnector.hpp"
#include <thread>
#include <chrono>

int main(int argc, char** argv)
{
    ib_helper::IBConnector conn("127.0.0.1", 4002, 3);
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    return 0;
}
