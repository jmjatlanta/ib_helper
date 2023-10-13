#include "test_helpers.h"

bool isConnected(const ib_helper::IBConnector& conn)
{
    int counter = 0;
    while (true && counter < 20)
    {
        if (conn.IsConnected())
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        counter++;
    }
    return conn.IsConnected();
}
